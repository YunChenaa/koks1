#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>
#include <linux/videodev2.h>
#include "global_config.h"
// #include "imp_video.h"
#include "koks_project_def.h"

#include "inlock_param_init.h"

#define INLOCK_PARAM_TAG     "INLOCK_PARAM_TAG"

#define MAX_CONFIG_FILE_LINE_LENGTH       128

config_func_param_t g_inlock_param_info;

typedef struct _config_context {
	FILE *fp;// 配置文件的文件指针
	int fd;// 文件描述符
	char *fbuf; // 行缓冲区，用于存放读取的一行数据,行行增加
	pthread_mutex_t mutex;// 互斥锁
	fpos_t line_pos;// 文件中的当前位置（比如某一行的位置）
} config_context_t;

static config_context_t cfg_context;//全局静态实例

static inline void strim(char *str)//遍历字符串，把遇到的 \t \n \v \f \r（空白控制符）替换为 \0，相当于去掉字符串末尾的换行符/制表符。
{
	char *p = str;

	while (*p != '\0') {
		if ((*p == '\t') || (*p == '\n') || (*p == '\v') ||
		    (*p == '\f') || (*p == '\r')) {
			*p = '\0';
			break;
		}
		p++;
	}
}

/**
 * file operation
 **/
static inline void set_pos_to_begin(void)//把文件指针移动到文件开头
{
	fseek(cfg_context.fp, 0L, SEEK_SET);
}

static inline void set_pos_to(fpos_t pos)//把文件指针移动到指定位置
{
	/*fseek(cfg_context.fp, pos, SEEK_SET);*/
	fsetpos(cfg_context.fp, &pos);
}

static inline void set_pos_to_end(void)//把文件指针移动到文件末尾
{
	fseek(cfg_context.fp, 0L, SEEK_END);
}

/*
参数:
key：要查找的关键字（字符串）。
line：输出缓冲区，用来存放找到的整行内容。
返回值:
0：成功找到，内容放在 line 中。=
<0：出错（例如 fgetpos 失败）。
CONFIG_RECORD_NOT_EXSIT：没找到。
*/
static int get_line_by_key(const char *key, char *line)//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line 中
{
	int ret = 0;
	char *line_ptr;
	int key_len = strlen(key);

	pthread_mutex_lock(&cfg_context.mutex);//防止同时有多个线程读写配置文件

	set_pos_to_begin();//从文件开头开始读
	do {
		line_ptr = fgets(cfg_context.fbuf, MAX_CONFIG_FILE_LINE_LENGTH, cfg_context.fp);//读取文件的每一行

		if (line_ptr && (!strncmp(line_ptr, key, key_len))) {//如果该行返回正常,检查这一行的前缀是否与 key 相同
			/*if (line_ptr[key_len] == ':') {*/
			if (strchr(line_ptr, ':')) {//检查该行是否包含 : 确认它是 key:value 这种格式
				strcpy(line, line_ptr);//把这一行内容复制到line
				ret = fgetpos(cfg_context.fp, &cfg_context.line_pos);//记录当前行的位置
				if(ret < 0) {
					printf("ERROR[%s]: fgetpos failed!\n", INLOCK_PARAM_TAG);
					return ret;
				}
				break;
			}
		}

	} while (line_ptr);//fgets返回NULL表示读到文件末尾或者出错

	if (!line_ptr) {//如果读到文件末尾还没找到
		ret = CONFIG_RECORD_NOT_EXSIT;
	}

	pthread_mutex_unlock(&cfg_context.mutex);//解锁

	return ret;
}

static int add_line_to_tail(const char *line)//把 line 的内容添加到配置文件的末尾
{
	int ret = 0;

	pthread_mutex_lock(&cfg_context.mutex);//上锁

	set_pos_to_end();//把文件指针移动到文件末尾
	ret = fputs(line, cfg_context.fp);//把 line 的内容写入文件
	if (ret < 0) {
		printf("ERROR(%s): update config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
		ret = CONFIG_OPS_FAIL;
	} else {
		fflush(cfg_context.fp);//把 FILE* 的缓冲区刷到内核缓冲区
		fsync(cfg_context.fd);//把内核缓冲区的数据同步到磁盘，保证即使断电数据也已经写到文件
		ret = 0;
	}

	pthread_mutex_unlock(&cfg_context.mutex);//开锁

	return ret;
}

static int delete_line_by_key(const char *key)//按 key 删除配置文件中的某一行
{
	int ret = 0;
	char *line_ptr;//指向读向的一行
	int key_len = strlen(key);//key 长
	int pos = 0, rest_len, wlen;

	pthread_mutex_lock(&cfg_context.mutex);//上锁

	set_pos_to_begin();//seek到文件开头
	do {//循环读每一行,存到fbuf中,最终pos指向要删除的行的起始位置
		line_ptr = fgets(cfg_context.fbuf + pos, MAX_CONFIG_FILE_LINE_LENGTH, cfg_context.fp);
//当遇到目标行时，pos 指向的是 目标行开头的偏移量
		if (line_ptr) {
			if ((!strncmp(line_ptr, key, key_len)) && (line_ptr[key_len] == ':')) {//找到要删除的行
				break;
			}
			pos += strlen(line_ptr);//pos 指向下一行的起始位置
		}
	} while (line_ptr);

	if (line_ptr) {//找到了要删除的行
//当遇到目标行时,把剩余部分拼接到缓冲区
		rest_len = fread(cfg_context.fbuf + pos, 1, MAX_CONFIG_FILE_SIZE, cfg_context.fp);//把要删除行之后的内容读到 fbuf 的 pos 位置
		if ((rest_len < 0) || (rest_len > MAX_CONFIG_FILE_SIZE)) {
			printf("ERROR(%s): read config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		ret = ftruncate(cfg_context.fd, 0);//把文件截断为0长度，相当于清空文件
		if (ret) {
			printf("ERROR(%s): cleanup config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		set_pos_to_begin();//把文件指针重新 seek 到文件开头
		wlen = fwrite(cfg_context.fbuf, 1, pos + rest_len, cfg_context.fp);//把 fbuf 中的数据写回文件，相当于覆盖掉了要删除的行
		if (wlen != (pos + rest_len)) {
			printf("ERROR(%s): write config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		fflush(cfg_context.fp);//把 FILE* 的缓冲区刷到内核缓冲区
		fsync(cfg_context.fd);//把内核缓冲区的数据同步到磁盘，保证即使断电数据也已经写到文件
	} else {
		ret = CONFIG_RECORD_NOT_EXSIT;
	}

err:
	pthread_mutex_unlock(&cfg_context.mutex);//开锁

	return ret;
}

static int replace_line_by_key(const char *key, const char *line)//按 key 替换配置文件中的某一行内容为 line
{
	int ret = 0;
	char *line_ptr;
	int key_len = strlen(key);
	int pos = 0, rest_len, wlen;

	pthread_mutex_lock(&cfg_context.mutex);//上锁

	set_pos_to_begin();//seek到文件开头
	do {//循环读每一行,存到fbuf中,最终pos指向要替换的行的起始位置
		line_ptr = fgets(cfg_context.fbuf + pos, MAX_CONFIG_FILE_LINE_LENGTH, cfg_context.fp);//读取文件的每一行
		if (line_ptr) {
			if ((!strncmp(line_ptr, key, key_len)) && (line_ptr[key_len] == ':')) {
				break;
			}
			pos += strlen(line_ptr);//pos 指向下一行的起始位置
		}
	} while (line_ptr);

	if (line_ptr) {//找到了要替换的行
		memcpy(cfg_context.fbuf + pos, line, strlen(line));//把要替换的行内容复制到 fbuf 的 pos 位置
		pos += strlen(line);

		rest_len = fread(cfg_context.fbuf + pos, 1, MAX_CONFIG_FILE_SIZE, cfg_context.fp);//把要替换行之后的内容读到 fbuf 的 pos 位置
		if ((rest_len < 0) || (rest_len > MAX_CONFIG_FILE_SIZE)) {
			printf("ERROR(%s): read config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		ret = ftruncate(cfg_context.fd, 0);//把文件截断为0长度，相当于清空文件
		if (ret) {
			printf("ERROR(%s): cleanup config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		set_pos_to_begin();//把文件指针重新 seek 到文件开头
		wlen = fwrite(cfg_context.fbuf, 1, pos + rest_len, cfg_context.fp);//把 fbuf 中的数据写回文件，相当于覆盖掉了要替换的行
		if (wlen != (pos + rest_len)) {
			printf("ERROR(%s): write config file failed! %s\n", INLOCK_PARAM_TAG, strerror(errno));
			ret = CONFIG_OPS_FAIL;
			goto err;
		}

		fflush(cfg_context.fp);//把 FILE* 的缓冲区刷到内核缓冲区
		fsync(cfg_context.fd);//把内核缓冲区的数据同步到磁盘，保证即使断电数据也已经写到文件
	} else {
		ret = CONFIG_RECORD_NOT_EXSIT;
	}

err:
	pthread_mutex_unlock(&cfg_context.mutex);//开锁

	return ret;
}

int inlock_param_add_string(const char *key, char *value)//把 key:value 形式的字符串添加到配置文件末尾
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	if (value)
		strim(value);
	sprintf(line_buf, "%s:%s\n", key, value);
	ret = add_line_to_tail(line_buf);//把 line_buf 的内容添加到配置文件的末尾

	return ret;
}

int inlock_param_add_int(const char *key, int value)//把 key:value 形式的整型添加到配置文件末尾
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:%d\n", key, value);
	ret = add_line_to_tail(line_buf);//把 line_buf 的内容添加到配置文件的末尾

	return ret;
}

int inlock_param_add_hex(const char *key, int value)//把 key:value 形式的十六进制整型添加到配置文件末尾
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:0x%x\n", key, value);
	ret = add_line_to_tail(line_buf);//把 line_buf 的内容添加到配置文件的末尾

	return ret;
}

int inlock_param_add_float(const char *key, float value)//把 key:value 形式的浮点型添加到配置文件末尾
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:%f\n", key, value);
	ret = add_line_to_tail(line_buf);//把 line_buf 的内容添加到配置文件的末尾

	return ret;
}

int inlock_param_update_string(const char *key, char *value, int create)//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	if (value)
		strim(value);
	sprintf(line_buf, "%s:%s\n", key, value);
	ret = replace_line_by_key(key, line_buf);//按 key 替换配置文件中的某一行内容为 line
	if ((ret == CONFIG_RECORD_NOT_EXSIT) && (create == 1)) {
		ret = inlock_param_add_string(key, value);//把 key:value 形式的字符串添加到配置文件末尾
	}

	return ret;
}

int inlock_param_update_int(const char *key, int value, int create)//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:%d\n", key, value);
	ret = replace_line_by_key(key, line_buf);//按 key 替换配置文件中的某一行内容为 line
	if ((ret == CONFIG_RECORD_NOT_EXSIT) && (create == 1)) {
		ret = inlock_param_add_int(key, value);//把 key:value 形式的整型添加到配置文件末尾
	}

	return ret;
}

int inlock_param_update_hex(const char *key, int value, int create)//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:0x%x\n", key, value);
	ret = replace_line_by_key(key, line_buf);//按 key 替换配置文件中的某一行内容为 line
	if ((ret == CONFIG_RECORD_NOT_EXSIT) && (create == 1)) {
		ret = inlock_param_add_hex(key, value);//把 key:value 形式的十六进制整型添加到配置文件末尾
	}

	return ret;
}

int inlock_param_update_float(const char *key, float value, int create)//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
{
	int ret = 0;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	sprintf(line_buf, "%s:%f\n", key, value);
	ret = replace_line_by_key(key, line_buf);//按 key 替换配置文件中的某一行内容为 line
	if ((ret == CONFIG_RECORD_NOT_EXSIT) && (create == 1)) {
		ret = inlock_param_add_float(key, value);//把 key:value 形式的浮点型添加到配置文件末尾
	}

	return ret;
}
int inlock_param_get_string(const char *key, char *value)//按 key 获取配置文件中的某一行内容，并把该行的 value 部分复制到 value 中
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	ret = get_line_by_key(key, line_buf);//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line_buf 中
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);//去掉字符串末尾的换行符/制表符
			strcpy(value, ptr);//把 ptr 部分复制到 value 中
		} else
			return CONFIG_OPS_FAIL;//没找到 value 部分
	}

	return ret;
}

int inlock_param_get_int(const char *key, int *value)//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为整数后存到 value 中
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	ret = get_line_by_key(key, line_buf);//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line_buf 中
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);//去掉字符串末尾的换行符/制表符
			*value = atoi(ptr);//把 ptr 部分转换为整数后存到 value 中
		} else
			return CONFIG_OPS_FAIL;
	}

	return ret;
}

int inlock_param_get_uhex(const char *key, uint8_t *value,int num)//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为十六进制整数后存到 value 数组中，num 指定了 value 数组的大小
{
	int ret = 0;
	int i = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	ret = get_line_by_key(key, line_buf);//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line_buf 中
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
			ptr = strtok(ptr,",");
			memset(value,0,num);
		}

		if (ptr) {
			while((ptr != NULL) && (i != num))
			{
				strim(ptr);
				//printf("INFO[%s]ptr:%s num:%d \n",__func__,ptr,num);
				value[i++] = (uint8_t)strtol(ptr, NULL, 16); // 0x32 -> 50
				ptr = strtok(NULL,",");
				//printf("INFO[%s]:%hhu %d\n",__func__,value[i-1],i);
			}

		} else
			return CONFIG_OPS_FAIL;
	}

	return ret;
}

int inlock_param_get_hex(const char *key, int *value)//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为十六进制整数后存到 value 中
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	ret = get_line_by_key(key, line_buf);//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line_buf 中
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);//去掉字符串末尾的换行符/制表符
			*value = (int)strtol(ptr, NULL, 16); // 0x32 -> 50
		} else
			return CONFIG_OPS_FAIL;
	}

	return ret;
}

int inlock_param_get_float(const char *key, float *value)//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为浮点数后存到 value 中
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];

	ret = get_line_by_key(key, line_buf);//根据 key 查找配置文件中的某一行内容，并将该行内容复制到 line_buf 中
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);
			*value = atof(ptr);
		} else
			return CONFIG_OPS_FAIL;
	}

	return ret;
}

int inlock_param_delete(const char *key)//按 key 删除配置文件中的某一行
{
	int ret = 0;

	ret = delete_line_by_key(key);//按 key 删除配置文件中的某一行

	return ret;
}

int inlock_param_get_mult_maxpacksize(char *key, unsigned int* mps_list)
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];
	int mps_num = 0;

	if (!mps_list){
		return -1;
	}

	ret = get_line_by_key(key, line_buf);
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);
			ptr = strtok(ptr, "/");
			while(ptr != NULL) {
				mps_list[mps_num] = atoi(ptr);
				ptr = strtok(NULL, "/");
				mps_num++;
			}
		} else
			return CONFIG_OPS_FAIL;
	} else
		return -1;


	return 0;
}

int inlock_param_get_mult_fps(char *key, uint32_t * fps_list)
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];
	int fps_num = 0;

	if (!fps_list){
		return -1;
	}

	ret = get_line_by_key(key, line_buf);
	if (!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}

		if (ptr) {
			strim(ptr);
			ptr = strtok(ptr, "/");
			while(ptr != NULL) {
				if ( atoi(ptr) < 5 ) {
					fps_list[fps_num] = (10 * 1000 * 1000) / 5;
					printf("warn: The minimum frame rate supported by the sensor is 5, fps[%d] \n", atoi(ptr));
				} else
					fps_list[fps_num] = (10 * 1000 * 1000) / atoi(ptr);
				ptr = strtok(NULL, "/");
				fps_num++;
			}
		} else
			return CONFIG_OPS_FAIL;
	}

	return 0;
}

int inlock_param_get_mult_frames(int * frames_num, unsigned int* frame_list)
{
	int i = 0;
	int frames_num_t = 0;
	unsigned int width, height;

	if (!frame_list){
		return -1;
	}

	frames_num_t = *frames_num;

	pthread_mutex_lock(&cfg_context.mutex);
	set_pos_to(cfg_context.line_pos);
	while ((i++ < *frames_num)) {
		if (fscanf(cfg_context.fp, "{%d, %d}", &width, &height) < 0)
			break;
		if (width > 0 && height > 0 && (width % 16 == 0)) {
			/* 不能使用以下方式赋值，程序会出现do page fault 错误
			 *frame_list[i++] = width;
			 *frame_list[i++] = height;
			 */
			*frame_list++ = width;
			*frame_list++ = height;
			/*printf("width %d, height %d\n", width, height);*/
		} else if (width > 0 && height > 0 && (height % 16 == 0)) {
				*frame_list++ = width;
				*frame_list++ = height;
		} else {
			printf("WARNING(%s): Width and height configuration do not meet 16 alignment requirements.(%d,%d)\n",
					INLOCK_PARAM_TAG, width, height);
			frames_num_t--;
		}

		fseek(cfg_context.fp, 1, SEEK_CUR);
	}
	*frames_num = frames_num_t;

	pthread_mutex_unlock(&cfg_context.mutex);
	return 0;
}

int inlock_param_get_mult_samplerate(char *key, unsigned int *samplerate_list)
{
	int ret = 0;
	char *ptr = NULL;
	char line_buf[MAX_CONFIG_FILE_LINE_LENGTH];
	int num = 0;

	if(!samplerate_list)
		return -1;

	ret = get_line_by_key(key, line_buf);
	if(!ret) {
		if (strtok(line_buf, ":")) {
			ptr = strtok(NULL, ":");
		}
		if(ptr) {
			strim(ptr);
			ptr = strtok(ptr, "/");
			while(ptr != NULL){
				samplerate_list[num] = atoi(ptr);
				ptr = strtok(NULL, "/");
				num++;
			}
		}else
			return CONFIG_OPS_FAIL;
    } else {
        return -1;
    }

    return 0;
}

/* start of device_info_init */
int device_param_init(device_info_t* dev_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_hex("vendor_id", &temp_int);
	if (ret < 0) {
		dev_info->vendor_id = VENDOR_ID;
	} else {
		dev_info->vendor_id = (uint32_t)temp_int;
		printf("INFO[%s]: vendor id 0x%x\n", INLOCK_PARAM_TAG, dev_info->vendor_id);
	}

	ret = inlock_param_get_hex("device_bcd", &temp_int);
	if (ret < 0) {
		dev_info->dev_bcd = DEVICE_BCD;
	} else {
		dev_info->dev_bcd = (uint32_t)temp_int;
	}

	ret = inlock_param_get_hex("product_id", &temp_int);
	if (ret < 0) {
		dev_info->product_id = PRODUCT_ID;
	} else {
		dev_info->product_id = (uint32_t)temp_int;
	}

	ret = inlock_param_get_string("serial_lab", (char*)dev_info->serial_number);
	if (ret < 0) {
		strcpy((char*)dev_info->serial_number, SERIAL_LABEL);
	} else {
		/*printf("INFO[%s]: serial_label %s\n", INLOCK_PARAM_TAG, dev_info->serial_number);*/
	}

	ret = inlock_param_get_string("product_lab", (char*)dev_info->product_label);
	if (ret < 0) {
		strcpy((char*)dev_info->product_label, PRODUCT_LABEL);
	}
	/*printf("INFO[%s]: product_label %s\n", INLOCK_PARAM_TAG, dev_info->product_label);*/

	ret = inlock_param_get_string("manufact_lab", (char*)dev_info->manufact_label);
	if (ret < 0) {
		strcpy((char*)dev_info->manufact_label, MANUFACT_LABEL);
	}

	ret = inlock_param_get_int("adb_en", &temp_int);
	if (ret < 0) {
		dev_info->adb_en = !ADB_ENABLE;
	} else {
		dev_info->adb_en = (uint8_t)temp_int;
	}
	/*printf("INFO[%s]: adb en %d\n", INLOCK_PARAM_TAG, dev_info->adb_en);*/

	return 0;
}
/* end of device_info_init */

static unsigned int get_suitable_payloadsize(int video_num,unsigned int payload_size)
{
	int i = 0;
	unsigned int payload_size_temp = 0;

	video_info_t *video_info_p = &g_inlock_param_info.video_info[video_num];
	if (video_info_p->mps_num == 0 || video_info_p->mps_list == NULL) {
		printf("ERROR[%s]: get suitable payload_size err\n", INLOCK_PARAM_TAG);
		return 0;
	}

	for (i = 0; i < video_info_p->mps_num; i++) {
		if (payload_size <= video_info_p->mps_list[i] || i == (video_info_p->mps_num - 1)) {
			payload_size_temp = video_info_p->mps_list[i];
			break;
		}
	}

	return payload_size_temp;
}

/* start of video_info_init */
int config_format(uint32_t fcc, uvc_format_t **format, int frames_num, uint32_t *frame_list, uint8_t fps_num, uint32_t* fps_list)
{
	int i = 0, j = 0, k = 0;
	int ret = 0, temp_int = 0, mult = 0;
	uvc_frame_t *frame = NULL;

	int fps = 0;
	unsigned int image_size = 0;
	unsigned int payload_size = 0;
	int image_compress_ratio = 0;
	int ImageSizeBase = 0;

	if (!*format) {
		/* config format */
		*format = (uvc_format_t*)malloc(sizeof(uvc_format_t));
		if (!*format) {
			printf("ERROR(%s): format malloc failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
	}

	ret = inlock_param_get_int("mc", &temp_int);
	if (ret < 0) {
		mult = 2;
	} else {
		mult = mult != 0 ? mult : temp_int;
	}

	ret = inlock_param_get_int("image_size_base", &temp_int);
	if (ret < 0) {
		ImageSizeBase = 1450000;
	} else {
		if (temp_int == 0) {
			ImageSizeBase = 1450000;
		} else {
			ImageSizeBase = temp_int;
		}
	}

	/*mjpeg h264的带宽分配都是根据1920*1080分辨率图像大小为1450k的数据量作为一个base值*/
	image_compress_ratio = ImageSizeBase * 100 / (1920 * 1080);

	if(fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12) {
		int pos = 0;
		int frames_num_tmp = 0;

		frames_num_tmp = frames_num;
		for(i = 0; i < frames_num; i++) {
			if(frame_list[j] <= 1920) {
				//Will change the content of "frame_list"
				frame_list[pos] = frame_list[j];
				frame_list[pos+1] = frame_list[j+1];
				pos+=2;
			} else {
				frames_num_tmp--;
				printf("WARNING(%s): YUYV/NV12 the restricted formats(%d,%d) \n", INLOCK_PARAM_TAG, frame_list[j], frame_list[j+1]);
			}
			j+=2;
		}

		if(pos == 0) {
			printf("ERROR(%s): Yuyv format, does not support 1920 resolution separately!\n", INLOCK_PARAM_TAG);
		}

		j=0;
		frames_num = frames_num_tmp;
	}

	/* config frame init */
	frame = (uvc_frame_t*)malloc(frames_num * sizeof(uvc_frame_t));
	if (!frame) {
		printf("ERROR(%s): frame malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}
/* mult alt setting start */
	for(i = 0; i < frames_num; i++) {
		frame[i].width = (uint16_t)frame_list[j++];
		frame[i].height = (uint16_t)frame_list[j++];
		frame[i].fps_num = fps_num;
		frame[i].fps_list = fps_list;

		frame[i].payload_size_list = (uint32_t *)malloc(fps_num * sizeof(uint32_t));
		if (!frame[i].payload_size_list) {
			 printf("ERROR(%s): payload_size_list malloc failed!\n", INLOCK_PARAM_TAG);
			 return -1;
		}

		if (fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12) {
				for (k = 0; k < fps_num; k++) {
					fps = 10000000 / frame[i].fps_list[k];
					image_size = frame[i].width * frame[i].height * fps * 2;
					payload_size = image_size / 8000;
					frame[i].payload_size_list[k] = get_suitable_payloadsize(0,payload_size);
				}
		} else {
				for (k = 0; k < fps_num; k++) {
					fps = 10000000 / frame[i].fps_list[k];
					image_size = frame[i].width * frame[i].height / 100  * fps * image_compress_ratio;
					payload_size = image_size / 8000;
					frame[i].payload_size_list[k] = get_suitable_payloadsize(0,payload_size);
				}
		}
	}
/* mult alt setting end */

	(*format)->fcc = fcc;
	(*format)->frames_num = frames_num;
	(*format)->uvc_frame = frame;

	return 0;
}

#ifdef DUAL_VIDEO
int config2_format(uint32_t fcc, uvc_format_t **format, int frames_num, uint32_t *frame_list, uint8_t fps_num, uint32_t* fps_list)
{
	int i = 0, j = 0, k = 0;
	int ret = 0, temp_int = 0, mult = 0;
	uvc_frame_t *frame = NULL;

	int fps = 0;
	unsigned int image_size = 0;
	unsigned int payload_size = 0;
	int image_compress_ratio = 0;
	int ImageSizeBase = 0;

	if (!*format) {
		/* config format */
		*format = (uvc_format_t*)malloc(sizeof(uvc_format_t));
		if (!*format) {
			printf("ERROR(%s): format malloc failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
	}

	ret = inlock_param_get_int("video2_mc", &temp_int);
	if (ret < 0) {
		mult = 2;
	} else {
		mult = mult != 0 ? mult : temp_int;
	}

	ret = inlock_param_get_int("video2_image_size_base", &temp_int);
	if (ret < 0) {
		ImageSizeBase = 1450000;
	} else {
		if (temp_int == 0) {
			ImageSizeBase = 1450000;
		} else {
			ImageSizeBase = temp_int;
		}
	}
	/*mjpeg h264的带宽分配都是根据1920*1080分辨率图像大小为1450k的数据量作为一个base值*/
	image_compress_ratio = ImageSizeBase * 100 / (1920 * 1080);

	if(fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12) {
		int pos = 0;
		int frames_num_tmp = 0;

		frames_num_tmp = frames_num;
		for(i = 0; i < frames_num; i++) {
			if(frame_list[j] <= 1920) {
				//Will change the content of "frame_list"
				frame_list[pos] = frame_list[j];
				frame_list[pos+1] = frame_list[j+1];
				pos+=2;
			} else {
				frames_num_tmp--;
				printf("WARNING(%s): YUYV/NV12 the restricted formats(%d,%d) \n", INLOCK_PARAM_TAG, frame_list[j], frame_list[j+1]);
			}
			j+=2;
		}

		if(pos == 0) {
			printf("ERROR(%s): Yuyv format, does not support 1920 resolution separately!\n", INLOCK_PARAM_TAG);
		}

		j=0;
		frames_num = frames_num_tmp;
	}

	/* config frame init */
	frame = (uvc_frame_t*)malloc(frames_num * sizeof(uvc_frame_t));
	if (!frame) {
		printf("ERROR(%s): frame malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}
	for(i = 0; i < frames_num; i++) {
		frame[i].width = (uint16_t)frame_list[j++];
		frame[i].height = (uint16_t)frame_list[j++];
		frame[i].fps_num = fps_num;
		frame[i].fps_list = fps_list;

		frame[i].payload_size_list = (uint32_t *)malloc(fps_num * sizeof(uint32_t));
		if (!frame[i].payload_size_list) {
			 printf("ERROR(%s): payload_size_list malloc failed!\n", INLOCK_PARAM_TAG);
			 return -1;
		}

		if (fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12) {
				for (k = 0; k < fps_num; k++) {
					fps = 10000000 / frame[i].fps_list[k];
					image_size = frame[i].width * frame[i].height * fps * 2;
					payload_size = image_size / 8000;
					frame[i].payload_size_list[k] = get_suitable_payloadsize(1,payload_size);
				}
		} else {
				for (k = 0; k < fps_num; k++) {
					fps = 10000000 / frame[i].fps_list[k];
					image_size = frame[i].width * frame[i].height / 100  * fps * image_compress_ratio;
					payload_size = image_size / 8000;
					frame[i].payload_size_list[k] = get_suitable_payloadsize(1,payload_size);
				}
		}
	}

	(*format)->fcc = fcc;
	(*format)->frames_num = frames_num;
	(*format)->uvc_frame = frame;

	return 0;
}
#endif

int uvc_format_init(video_info_t* video_info)
{
	int ret;
	uint8_t format_num = 0;
	uvc_format_t *yuv_format = NULL;
	uvc_format_t *yuyv_format = NULL;
	uvc_format_t *mjpeg_format = NULL;
	uvc_format_t *h264_format = NULL;
	uvc_format_t *h265_format = NULL;
	uvc_format_t *uvc_format_tmp[UVC_FORMAT_MAX]; /* adjust the order of uvc_format */

	uint32_t frames_num = 0;
	uint32_t *frame_list = NULL;
	uint32_t fps_num = 0;
	uint32_t *fps_list = NULL;
	int i = 0;

	/* get the number of supported fps */
	ret = inlock_param_get_int("fps_num", (int*)&fps_num);
	if (ret < 0) {
		fps_num = 1;
	}

	/* get fps_list ,such as 60,40, 30, 25 etc. */
	fps_list = (uint32_t*)malloc(fps_num * sizeof(uint32_t));
	if (!fps_list) {
		printf("ERROR(%s): fps_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_fps("fps_list", fps_list);
	if (ret < 0) {
		printf("ERROR(%s): fps_list get failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	/* get the number of supported frames */
	ret = inlock_param_get_int("nframes", (int*)&frames_num);
	if (ret < 0) {
		frames_num = 1;
	}
	printf("frame num %d\n", frames_num);

	/* get frame list : frames_num * (width + height)*/
	frame_list = (uint32_t *)malloc(frames_num * 2 * sizeof(uint32_t));
	if (!frame_list) {
		printf("ERROR(%s): frame list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_frames((int*)&frames_num, frame_list);
	if (ret < 0) {
		printf("ERROR(%s): frame_list get failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	printf("---- %d, %d, %d, %d, %d, %d -----\n",frame_list[0], frame_list[1], frame_list[2],frame_list[3],frame_list[4],frame_list[5]);
	if (video_info->mjpeg_en) {
		ret = config_format(V4L2_PIX_FMT_MJPEG, &mjpeg_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->mjpeg_en - 1] = mjpeg_format;
		format_num ++;
	}

	if (video_info->h264_en) {
		ret = config_format(V4L2_PIX_FMT_H264, &h264_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->h264_en - 1] = h264_format;
		format_num ++;
	}

	if (video_info->h265_en) {
		ret = config_format(V4L2_PIX_FMT_H265, &h265_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->h265_en - 1] = h265_format;
		format_num ++;
	}

	/*
		Changing the format needs to be placed at the end,
		as it will change the "frame_list" and affect subsequent formatting settings
	*/
	if (video_info->yuv_en) {
		ret = config_format(V4L2_PIX_FMT_NV12, &yuv_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->yuv_en - 1] = yuv_format;
		format_num ++;
	}

	if (video_info->yuyv_en) {
		ret = config_format(V4L2_PIX_FMT_YUYV, &yuyv_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->yuyv_en - 1] = yuyv_format;
		format_num ++;
	}

	/* set uvc format list */
	video_info->uvc_format_num = format_num;
	video_info->uvc_format = (uvc_format_t**)malloc(video_info->uvc_format_num * sizeof(uvc_format_t*));
	if (!video_info->uvc_format) {
		printf("ERROR(%s): uvc_format malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	for (i = 0; i < format_num; i++) {
		video_info->uvc_format[i] = uvc_format_tmp[i];
	}

	free(frame_list);

	return 0;
}

int uvc_vc_config(video_info_t* video_info)
{
	int ret;
	uint8_t *temp_int = NULL;
	temp_int = (uint8_t *)malloc(16*sizeof(uint8_t));
	int temp; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_uhex("ct_controls", temp_int,sizeof(video_info->uvc_ct.controls));
	if (ret < 0) {
		video_info->uvc_ct.controls[0] = (uint8_t)0xa;
		video_info->uvc_ct.controls[1] = (uint8_t)0x2;
		video_info->uvc_ct.controls[2] = (uint8_t)0x0;
		/*printf("INFO[CT]sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_ct.controls,temp_int,sizeof(video_info->uvc_ct.controls));
		/*printf("uvc CT config: %hhu %hhu %hhu\n",video_info->uvc_ct.controls[0]\*/
				/*,video_info->uvc_ct.controls[1],\*/
				/*video_info->uvc_ct.controls[2]);*/
	}

	ret = inlock_param_get_uhex("pu_controls", temp_int,sizeof(video_info->uvc_pu.controls));
	if (ret < 0) {
		video_info->uvc_pu.controls[0] = (uint8_t)0xff;
		video_info->uvc_pu.controls[1] = (uint8_t)0x15;
#ifdef CONFIG_UVC_1_5
		video_info->uvc_pu.controls[2] = (uint8_t)0x77;
#endif
		/*printf("INFO[PU]sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_pu.controls,temp_int,sizeof(video_info->uvc_pu.controls));
		/*printf("uvc PU config: %hhu %hhu %hhu\n",video_info->uvc_pu.controls[0]\*/
				/*,video_info->uvc_pu.controls[1],\*/
				/*temp_int[2]);*/
	}

	ret = inlock_param_get_int("eu_en", &temp);
	if (ret < 0) {
		video_info->uvc_eu.en = (uint8_t)0x00;
		/*printf("INFO[EU]en sucess !!!\n");*/
	}else{
		video_info->uvc_eu.en = temp;
		/*printf("INFO[EU]en:%hhu \n",temp_int[0]);*/
	}

	ret = inlock_param_get_int("eu_num", &temp);
	if (ret < 0) {
		video_info->uvc_eu.num_controls = (uint8_t)0xF;
		/*printf("INFO[EU]num sucess !!!\n");*/
	}else{
		video_info->uvc_eu.num_controls = temp;
		/*printf("INFO[EU]num:%hhu \n",temp_int[0]);*/
	}

	uint8_t def_guid[16] = {0xa2, 0x9e, 0x76, 0x41, 0xde, 0x04, 0x47, 0xe3,0x8b, 0x2b, 0xf4, 0x34, 0x1a, 0xff, 0x00, 0x3b};
	ret = inlock_param_get_uhex("eu_guid", temp_int,sizeof(video_info->uvc_eu.guid));
	if (ret < 0) {
		memcpy(video_info->uvc_eu.guid,def_guid,sizeof(video_info->uvc_eu.guid));
		/*printf("INFO[EU]guid sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_eu.guid,temp_int,sizeof(video_info->uvc_eu.guid));
		/*printf("INFO[EU]guid 16:%hhu \n",temp_int[15]);*/
	}

	ret = inlock_param_get_uhex("eu_controls",temp_int,sizeof(video_info->uvc_eu.controls));
	if (ret < 0) {
		video_info->uvc_eu.controls[0] = (uint8_t)0x00;
		video_info->uvc_eu.controls[1] = (uint8_t)0xf8;
		video_info->uvc_eu.controls[2] = (uint8_t)0x03;
		/*printf("INFO[EU]controls sucess !!!\n");*/
	}else{

		memcpy(video_info->uvc_eu.controls,temp_int,sizeof(video_info->uvc_eu.controls));
		/*printf("uvc EU config: %hhu %hhu %hhu\n",video_info->uvc_eu.controls[0]\*/
				/*,video_info->uvc_eu.controls[1],\*/
				/*video_info->uvc_eu.controls[2]);*/
	}

	printf("INFO[%s] --> vc config success <--\n", INLOCK_PARAM_TAG);
	free(temp_int);

	return 0;
};

int video_param_init(video_info_t* video_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_string("video_name", (char*)video_info->video_name);
	if (ret < 0) {
		strcpy((char*)video_info->video_name, VIDEO_DEVICE_NAME);
	}

	ret = inlock_param_get_int("mc", &temp_int);
	if (ret < 0) {
		video_info->mult = 2;
	} else {
		video_info->mult = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("mps", &temp_int);
	if (ret < 0) {
		video_info->mps = 1024;
	} else {
		video_info->mps = temp_int;
	}

	/* get the number of supported maxpacketsize */
	ret = inlock_param_get_int("mps_num", &temp_int);
	if (ret < 0) {
		video_info->mps_num = 11;
	} else {
		if (temp_int > 11) {
			printf("ERROR:mps_num > 11,please check uvc.config");
			return 0;
		}
		video_info->mps_num = temp_int;
	}

	/* get mps_list ,such as  3072, 2048, 1024 etc. */
	video_info->mps_list = (uint32_t*)malloc(video_info->mps_num * sizeof(uint32_t));
	if (!video_info->mps_list) {
		printf("ERROR(%s): mps_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_maxpacksize("mps_list", video_info->mps_list);
	if (ret < 0) {
		printf("Warning(%s): mps_list get failed, use default settings!\n", INLOCK_PARAM_TAG);
		video_info->mps_num = 11;
		video_info->mps_list[0] = 192;
		video_info->mps_list[1] = 384;
		video_info->mps_list[2] = 512;
		video_info->mps_list[3] = 640;
		video_info->mps_list[4] = 800;
		video_info->mps_list[5] = 944;
		video_info->mps_list[6] = 1280;
		video_info->mps_list[7] = 1600;
		video_info->mps_list[8] = 1984;
		video_info->mps_list[9] = 2688;
		video_info->mps_list[10] = 3072;
	}

	/*根据mc mps算出最大带宽，如果带宽列表中的值大于最大带宽，修改带宽列表的值为最大带宽*/
	for (int i = 0; i < video_info->mps_num; i++) {
		if((video_info->mps * (video_info->mult + 1)) <= video_info->mps_list[i]) {
			video_info->mps_list[i] = video_info->mps * (video_info->mult + 1);

			if (i != video_info->mps_num -1) {
				int AbnormalPayloadSizeNum = video_info->mps_num - (i + 1);
				video_info->mps_num -= AbnormalPayloadSizeNum;
				break;
			}
		}

	}

	ret = inlock_param_get_int("stillcap_en", &temp_int);
	if (ret < 0) {
		video_info->stillcap_en = 1;
	} else {
		video_info->stillcap_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("h264_en", &temp_int);
	if (ret < 0) {
		video_info->h264_en = 0;
	} else {
		video_info->h264_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("h265_en", &temp_int);
	if (ret < 0) {
		video_info->h265_en = 0;
	} else {
		video_info->h265_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("nv12_en", &temp_int);
	if (ret < 0) {
		video_info->yuv_en = 0;
	} else {
		video_info->yuv_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("yuyv_en", &temp_int);
	if (ret < 0) {
		video_info->yuyv_en = 1;
	} else {
		video_info->yuyv_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("mjpeg_en", &temp_int);
	if (ret < 0) {
		video_info->mjpeg_en = 1;
	} else {
		video_info->mjpeg_en = (uint8_t)temp_int;
	}

	/* led_ctr */
	ret = inlock_param_get_int("led_en", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_en = 0;
	} else {
		video_info->led_ctl.led_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("led_gpio", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_gpio = -1;
	} else {
		video_info->led_ctl.led_gpio = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("led_level", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_level = 0;
	} else {
		video_info->led_ctl.led_level = (uint8_t)temp_int;
	}

	/* init uvc_format such as yuv ,mjpeg, h264 etc. */
	ret = uvc_format_init(video_info);
	if (ret < 0) {
		printf("ERROR(%s): uvc_format_init failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}
	/* uvc vc init*/
	ret = uvc_vc_config(video_info);
	if (ret < 0) {
		printf("ERROR(%s): uvc_vc_config failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	return 0;
}
#if (VIDEO_NUM == 2)
/* uvc format init */
/* init supported format */
/* init supported frames num and frames list */
/* init supported fps num and fps list */
int uvc2_format_init(video_info_t* video_info)
{
	int ret;
	uint8_t format_num = 0;
	uvc_format_t *yuv_format = NULL;
	uvc_format_t *yuyv_format = NULL;
	uvc_format_t *mjpeg_format = NULL;
	uvc_format_t *h264_format = NULL;
	uvc_format_t *h265_format = NULL;
	uvc_format_t *uvc_format_tmp[UVC_FORMAT_MAX]; /* adjust the order of uvc_format */

	uint32_t frames_num = 0;
	uint32_t *frame_list = NULL;
	uint32_t fps_num = 0;
	uint32_t *fps_list = NULL;
	int i = 0;

	/* get the number of supported fps */
	ret = inlock_param_get_int("video2_fps_num", (int*)&fps_num);
	if (ret < 0) {
		fps_num = 1;
	}

	/* get fps_list ,such as 60,40, 30, 25 etc. */
	fps_list = (uint32_t*)malloc(fps_num * sizeof(uint32_t));
	if (!fps_list) {
		printf("ERROR(%s): fps_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_fps("video2_fps_list", fps_list);
	if (ret < 0) {
		printf("ERROR(%s): video2_fps_list get failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	/* get the number of supported frames */
	ret = inlock_param_get_int("video2_nframes", (int*)&frames_num);
	if (ret < 0) {
		frames_num = 1;
	}
	printf("video2 frame num %d\n", frames_num);

	/* get frame list : frames_num * (width + height)*/
	frame_list = (uint32_t *)malloc(frames_num * 2 * sizeof(uint32_t));
	if (!frame_list) {
		printf("ERROR(%s): frame list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_frames((int*)&frames_num, frame_list);
	if (ret < 0) {
		printf("ERROR(%s): frame_list get failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	printf("----video2: %d, %d, %d, %d, %d, %d -----\n",frame_list[0], frame_list[1], frame_list[2],frame_list[3],frame_list[4],frame_list[5]);
	if (video_info->mjpeg_en) {
		ret = config2_format(V4L2_PIX_FMT_MJPEG, &mjpeg_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->mjpeg_en - 1] = mjpeg_format;
		format_num ++;
	}

	if (video_info->h264_en) {
		ret = config2_format(V4L2_PIX_FMT_H264, &h264_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->h264_en - 1] = h264_format;
		format_num ++;
	}

	if (video_info->h265_en) {
		ret = config2_format(V4L2_PIX_FMT_H265, &h265_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->h265_en - 1] = h265_format;
		format_num ++;
	}

	if (video_info->yuv_en) {
		ret = config2_format(V4L2_PIX_FMT_NV12, &yuv_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->yuv_en - 1] = yuv_format;
		format_num ++;
	}

	/*
		Changing the format needs to be placed at the end,
		as it will change the "frame_list" and affect subsequent formatting settings
	*/
	if (video_info->yuyv_en) {

#ifdef WINHELLO_ENABLE
		if(video_info->yuv_en || video_info->mjpeg_en || video_info->h264_en || video_info->h265_en) {
			printf("ERROR(%s): fatal error, winhello only supported yuv(y) format\n", INLOCK_PARAM_TAG);
			return -1;
		}

		ret = config2_format(V4L2_PIX_FMT_GREY, &yuyv_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->yuyv_en - 1] = yuyv_format;
#else
		ret = config2_format(V4L2_PIX_FMT_YUYV, &yuyv_format, frames_num, frame_list, fps_num, fps_list);
		if (ret < 0) {
			printf("ERROR(%s): config format failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
		uvc_format_tmp[video_info->yuyv_en - 1] = yuyv_format;
#endif
		format_num ++;
	}

	/* set uvc format list */
	video_info->uvc_format_num = format_num;
	video_info->uvc_format = (uvc_format_t**)malloc(video_info->uvc_format_num * sizeof(uvc_format_t*));
	if (!video_info->uvc_format) {
		printf("ERROR(%s): uvc_format malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	for (i = 0; i < format_num; i++) {
		video_info->uvc_format[i] = uvc_format_tmp[i];
	}

	free(frame_list);

	return 0;
}

int uvc2_vc_config(video_info_t* video_info)
{
	int ret;
	uint8_t *temp_int = NULL;
	temp_int = (uint8_t *)malloc(16*sizeof(uint8_t));
	int temp; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_uhex("ct2_controls", temp_int,sizeof(video_info->uvc_ct.controls));
	if (ret < 0) {
#ifdef KOKS_FEATURE_KDS
		video_info->uvc_ct.controls[0] = (uint8_t)0x0;
		video_info->uvc_ct.controls[1] = (uint8_t)0x0;
#else
		video_info->uvc_ct.controls[0] = (uint8_t)0xa;
		video_info->uvc_ct.controls[1] = (uint8_t)0x2;
#endif
		video_info->uvc_ct.controls[2] = (uint8_t)0x0;
		/*printf("INFO[CT2]sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_ct.controls,temp_int,sizeof(video_info->uvc_ct.controls));
		/*printf("uvc2 CT config: %hhu %hhu %hhu\n",video_info->uvc_ct.controls[0]\*/
				/*,video_info->uvc_ct.controls[1],\*/
				/*video_info->uvc_ct.controls[2]);*/
	}

	ret = inlock_param_get_uhex("pu2_controls", temp_int,sizeof(video_info->uvc_pu.controls));
	if (ret < 0) {
#ifdef KOKS_FEATURE_KDS
		video_info->uvc_pu.controls[0] = (uint8_t)0x00;
		video_info->uvc_pu.controls[1] = (uint8_t)0x00;
#else
		video_info->uvc_pu.controls[0] = (uint8_t)0xff;
		video_info->uvc_pu.controls[1] = (uint8_t)0x15;
#endif

#ifdef CONFIG_UVC_1_5
		video_info->uvc_pu.controls[2] = (uint8_t)0x77;
#endif
		/*printf("INFO[PU2]sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_pu.controls,temp_int,sizeof(video_info->uvc_pu.controls));
		/*printf("uvc2 PU config: %hhu %hhu %hhu\n",video_info->uvc_pu.controls[0]\*/
				/*,video_info->uvc_pu.controls[1],\*/
				/*temp_int[2]);*/
	}

	ret = inlock_param_get_int("eu2_en", &temp);
	if (ret < 0) {
		video_info->uvc_eu.en = (uint8_t)0x00;
		/*printf("INFO[EU2]en sucess !!!\n");*/
	}else{
		video_info->uvc_eu.en = temp;
		/*printf("INFO[EU2]en:%hhu \n",temp_int[0]);*/
	}

	ret = inlock_param_get_int("eu2_num", &temp);
	if (ret < 0) {
		video_info->uvc_eu.num_controls = (uint8_t)0xF;
		/*printf("INFO[EU2]num sucess !!!\n");*/
	}else{
		video_info->uvc_eu.num_controls = temp;
		/*printf("INFO[EU2]num:%hhu \n",temp_int[0]);*/
	}

	uint8_t def_guid[16] = {0xa2, 0x9e, 0x76, 0x41, 0xde, 0x04, 0x47, 0xe3,0x8b, 0x2b, 0xf4, 0x34, 0x1a, 0xff, 0x00, 0x3b};
	ret = inlock_param_get_uhex("eu2_guid", temp_int,sizeof(video_info->uvc_eu.guid));
	if (ret < 0) {
		memcpy(video_info->uvc_eu.guid,def_guid,sizeof(video_info->uvc_eu.guid));
		/*printf("INFO[EU2]guid sucess !!!\n");*/
	}else{
		memcpy(video_info->uvc_eu.guid,temp_int,sizeof(video_info->uvc_eu.guid));
		/*printf("INFO[EU2]guid 16:%hhu \n",temp_int[15]);*/
	}

	ret = inlock_param_get_uhex("eu2_controls",temp_int,sizeof(video_info->uvc_eu.controls));
	if (ret < 0) {
		video_info->uvc_eu.controls[0] = (uint8_t)0x00;
		video_info->uvc_eu.controls[1] = (uint8_t)0xf8;
		video_info->uvc_eu.controls[2] = (uint8_t)0x03;
		/*printf("INFO[EU2]controls sucess !!!\n");*/
	}else{

		memcpy(video_info->uvc_eu.controls,temp_int,sizeof(video_info->uvc_eu.controls));
		/*printf("uvc2 EU config: %hhu %hhu %hhu\n",video_info->uvc_eu.controls[0]\*/
				/*,video_info->uvc_eu.controls[1],\*/
				/*video_info->uvc_eu.controls[2]);*/
	}

	printf("INFO[VC2 CONIFG] success !\n");
	free(temp_int);

	return 0;
};
int video2_param_init(video_info_t* video_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_string("video2_name", (char*)video_info->video_name);
	if (ret < 0) {
		strcpy((char*)video_info->video_name, VIDEO_DEVICE_NAME);
	}

	ret = inlock_param_get_int("video2_mc", &temp_int);
	if (ret < 0) {
		video_info->mult = 1;
	} else {
		video_info->mult = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_mps", &temp_int);
	if (ret < 0) {
		video_info->mps = 1024;
	} else {
		video_info->mps = temp_int;
	}

	/* get the number of supported maxpacketsize */
	ret = inlock_param_get_int("video2_mps_num", &temp_int);
	if (ret < 0) {
		video_info->mps_num = 11;
	} else {
		if (temp_int > 11) {
			printf("ERROR:mps_num > 11,please check uvc.config");
			return 0;
		}
		video_info->mps_num = temp_int;
	}

	/* get mps_list ,such as  3072, 2048, 1024 etc. */
	video_info->mps_list = (uint32_t*)malloc(video_info->mps_num * sizeof(uint32_t));
	if (!video_info->mps_list) {
		printf("ERROR(%s): mps_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_maxpacksize("video2_mps_list", video_info->mps_list);
	if (ret < 0) {
		printf("Warning(%s): video2_mps_list get failed, use default settings!\n", INLOCK_PARAM_TAG);
		video_info->mps_num = 11;
		video_info->mps_list[0] = 192;
		video_info->mps_list[1] = 384;
		video_info->mps_list[2] = 512;
		video_info->mps_list[3] = 640;
		video_info->mps_list[4] = 800;
		video_info->mps_list[5] = 944;
		video_info->mps_list[6] = 1280;
		video_info->mps_list[7] = 1600;
		video_info->mps_list[8] = 1984;
		video_info->mps_list[9] = 2688;
		video_info->mps_list[10] = 3072;
	}

	/*根据mc mps算出最大带宽，如果带宽列表中的值大于最大带宽，修改带宽列表的值为最大带宽*/
	for (int i = 0; i < video_info->mps_num; i++) {
		if(video_info->mps * (video_info->mult + 1) <= video_info->mps_list[i]) {
			video_info->mps_list[i] = video_info->mps * (video_info->mult + 1);

			if (i != video_info->mps_num -1) {
				printf("warning:uvc2:payload_size value is larger than MaxPayloadSize\n");
				int AbnormalPayloadSizeNum = video_info->mps_num - (i + 1);
				video_info->mps_num -= AbnormalPayloadSizeNum;
				break;
			}
		}
	}

	ret = inlock_param_get_int("video2_stillcap_en", &temp_int);
	if (ret < 0) {
		video_info->stillcap_en = 1;
	} else {
		video_info->stillcap_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_h264_en", &temp_int);
	if (ret < 0) {
		video_info->h264_en = 0;
	} else {
		video_info->h264_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_h265_en", &temp_int);
	if (ret < 0) {
		video_info->h265_en = 0;
	} else {
		video_info->h265_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_nv12_en", &temp_int);
	if (ret < 0) {
		video_info->yuv_en = 1;
	} else {
		video_info->yuv_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_yuyv_en", &temp_int);
	if (ret < 0) {
		video_info->yuyv_en = 1;
	} else {
		video_info->yuyv_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("video2_mjpeg_en", &temp_int);
	if (ret < 0) {
		video_info->mjpeg_en = 1;
	} else {
		video_info->mjpeg_en = (uint8_t)temp_int;
	}

	/* led_ctr */
	ret = inlock_param_get_int("led2_en", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_en = 0;
	} else {
		video_info->led_ctl.led_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("led2_gpio", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_gpio = -1;
	} else {
		video_info->led_ctl.led_gpio = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("led2_level", &temp_int);
	if (ret < 0) {
		video_info->led_ctl.led_level = 0;
	} else {
		video_info->led_ctl.led_level = (uint8_t)temp_int;
	}

	/*winhello_en*/
	ret = inlock_param_get_int("winhello_en", &temp_int);
	if (ret < 0) {
		video_info->winhello_en = 0;
	} else {
		video_info->winhello_en = (uint8_t)temp_int;
	}

	/* init uvc_format such as yuv ,mjpeg, h264 etc. */
	ret = uvc2_format_init(video_info);
	if (ret < 0) {
		printf("ERROR(%s): uvc2_format_init failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}
	/* uvc2 vc init*/
	ret = uvc2_vc_config(video_info);
	if (ret < 0) {
		printf("ERROR(%s): uvc2_vc_config failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	return 0;
}
#endif
/* end of video_info_init */

/* start of audio_info_init */
int amic_info_init(audio_param_info_t *mic_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错
	uint32_t *samplerate_list = NULL;

	ret = inlock_param_get_string("audio_name", (char*)mic_info->audio_name);
	if (ret < 0) {
		strcpy((char*)mic_info->audio_name, AUDIO_DEVICE_NAME);;
	}

	ret = inlock_param_get_int("amic_samplerate_num", &temp_int);
	if (ret < 0) {
		mic_info->samplerate_num = SAMPLERATE_NUM_DEF;
	} else {
		mic_info->samplerate_num = (uint32_t)temp_int;
	}
	printf("INFO[%s]: amic samplerate_num %d\n", INLOCK_PARAM_TAG, mic_info->samplerate_num);

	samplerate_list = (uint32_t*)malloc(mic_info->samplerate_num * sizeof(uint32_t));
	if(!samplerate_list){
		printf("ERROR(%s):samplerate_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_samplerate("amic_samplerate_list", samplerate_list);
	if(ret < 0){
		samplerate_list[mic_info->samplerate_num - 1] = SAMPLERATE_DEF;
	}

	mic_info->samplerate_list = samplerate_list;

	ret = inlock_param_get_int("amic_bitwidth", &temp_int);
	if (ret < 0) {
		mic_info->bitwidth = BITWIDTH_DEF;
	} else {
		mic_info->bitwidth = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("amic_soundmode", &temp_int);
	if (ret < 0) {
		mic_info->soundmode = SOUNDMODE_DEF;
	} else {
		mic_info->soundmode = (uint8_t)temp_int;
	}
	printf("INFO[%s]: amic soundmode %d\n", INLOCK_PARAM_TAG, mic_info->soundmode);

	ret = inlock_param_get_int("amic_volume", &temp_int);
	if (ret < 0) {
		mic_info->volume = MIC_VOL;
	} else {
		mic_info->volume = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("amic_chncnt", &temp_int);
	if (ret < 0) {
		mic_info->prev[0] = MIC_CHNCNT;
	} else{
		mic_info->prev[0] = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("amic_frame_time", &temp_int);
	if (ret < 0) {
		temp_int = MIC_FRAME_TIME;
		memcpy(&mic_info->prev[5], &temp_int, sizeof(uint16_t));
	} else{
		memcpy(&mic_info->prev[5], &temp_int, sizeof(uint16_t));
	}
	return 0;
}

int dmic_info_init(audio_param_info_t *mic_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错
	uint32_t *samplerate_list = NULL;

	ret = inlock_param_get_string("audio_name", (char*)mic_info->audio_name);
	if (ret < 0) {
		strcpy((char*)mic_info->audio_name, AUDIO_DEVICE_NAME);;
	}

	ret = inlock_param_get_int("dmic_samplerate_num", &temp_int);
	if (ret < 0) {
		mic_info->samplerate_num = SAMPLERATE_NUM_DEF;
	} else {
		mic_info->samplerate_num = (uint32_t)temp_int;
	}
	printf("INFO[%s]: dmic samplerate_num %d\n", INLOCK_PARAM_TAG, mic_info->samplerate_num);

	samplerate_list = (uint32_t*)malloc(mic_info->samplerate_num * sizeof(uint32_t));
	if(!samplerate_list){
		printf("ERROR(%s):samplerate_list malloc failed!\n", INLOCK_PARAM_TAG);
		return -1;
	}

	ret = inlock_param_get_mult_samplerate("dmic_samplerate_list", samplerate_list);
	if(ret < 0){
		samplerate_list[mic_info->samplerate_num - 1] = SAMPLERATE_DEF;
	}
	mic_info->samplerate_list = samplerate_list;

	ret = inlock_param_get_int("dmic_bitwidth", &temp_int);
	if (ret < 0) {
		mic_info->bitwidth = BITWIDTH_DEF;
	} else {
		mic_info->bitwidth = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("dmic_soundmode", &temp_int);
	if (ret < 0) {
		mic_info->soundmode = SOUNDMODE_DEF;
	} else {
		mic_info->soundmode = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("dmic_volume", &temp_int);
	if (ret < 0) {
		mic_info->volume = MIC_VOL;
	} else {
		mic_info->volume = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("dmic_chncnt", &temp_int);
	if (ret < 0) {
		mic_info->prev[0] = MIC_CHNCNT;
	} else {
		mic_info->prev[0] = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("dmic_frame_time", &temp_int);
	if (ret < 0) {
		temp_int = MIC_FRAME_TIME;
		memcpy(&mic_info->prev[5], &temp_int, sizeof(uint16_t));
	} else{
		memcpy(&mic_info->prev[5], &temp_int, sizeof(uint16_t));
	}
	return 0;
}
int spk_info_init(audio_param_info_t *spk_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错
	uint32_t *samplerate = NULL;

	ret = inlock_param_get_string("audio_name", (char*)spk_info->audio_name);
	if (ret < 0) {
		strcpy((char*)spk_info->audio_name, AUDIO_DEVICE_NAME);;
	}

	spk_info->samplerate_num = 1;

	samplerate = (uint32_t*)malloc(sizeof(uint32_t));
	spk_info->samplerate_list = samplerate;
	ret = inlock_param_get_int("spk_samplerate", &temp_int);
	if (ret < 0) {
		spk_info->samplerate_list[0] = SAMPLERATE_DEF;
	} else {
		spk_info->samplerate_list[0] = (uint32_t)temp_int;
	}

	ret = inlock_param_get_int("spk_bitwidth", &temp_int);
	if (ret < 0) {
		spk_info->bitwidth = BITWIDTH_DEF;
	} else {
		spk_info->bitwidth = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("spk_soundmode", &temp_int);
	if (ret < 0) {
		spk_info->soundmode = SOUNDMODE_DEF;
	} else {
		spk_info->soundmode = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("spk_volume", &temp_int);
	if (ret < 0) {
		spk_info->volume = MIC_VOL;
	} else {
		spk_info->volume = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("spk_chncnt", &temp_int);
	if (ret < 0) {
		spk_info->prev[0] = SPK_CHNCNT;
	} else {
		spk_info->prev[0] = (int8_t)temp_int;
	}

	ret = inlock_param_get_int("spk_frame_time", &temp_int);
	if (ret < 0) {
		temp_int = SPK_FRAME_TIME;
		memcpy(&spk_info->prev[5], &temp_int, sizeof(uint16_t));
	} else{
		memcpy(&spk_info->prev[5], &temp_int, sizeof(uint16_t));
	}
	return 0;
}

int audio_param_init(audio_info_t* audio_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_int("amic_en", &temp_int);
	if (ret < 0) {
		audio_info->amic_en = 1;
	} else {
		audio_info->amic_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("dmic_en", &temp_int);
	if (ret < 0) {
		audio_info->dmic_en = 0;
	} else {
		audio_info->dmic_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("spk_en", &temp_int);
	if (ret < 0) {
		audio_info->spk_en = 1;
	} else {
		audio_info->spk_en = (uint8_t)temp_int;
	}

	if (audio_info->amic_en) {
		ret = amic_info_init(&audio_info->amic_info);
		if (ret < 0) {
			printf("ERROR(%s): amic info init failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
	}

	if (audio_info->dmic_en) {
		ret = dmic_info_init(&audio_info->dmic_info);
		if (ret < 0) {
			printf("ERROR(%s): dmic info init failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
	}

	if (audio_info->spk_en) {
		ret = spk_info_init(&audio_info->spk_info);
		if (ret < 0) {
			printf("ERROR(%s): spk info init failed!\n", INLOCK_PARAM_TAG);
			return -1;
		}
	}

	ret = inlock_param_get_int("aec_en", &temp_int);
	if (ret < 0) {
		audio_info->aec_en = 0;
	} else {
		audio_info->aec_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("audio_ns", &temp_int);
	if (ret < 0) {
		audio_info->audio_ns = 0;
	} else {
		audio_info->audio_ns = (uint8_t)temp_int;
	}

	return 0;
}
/* end of audio_info_init */

/* start of imp_info_init */
int sensor_info_init(sensor_info_t *sensor_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_string("sensor_name", (char*)sensor_info->sensor_name);
	if (ret < 0) {
		strcpy((char*)sensor_info->sensor_name, FIRST_SENSOR_NAME);
	}

	ret = inlock_param_get_hex("i2c_addr", &temp_int);
	if (ret < 0) {
		sensor_info->i2c_addr = FIRST_I2C_ADDR;
	} else {
		sensor_info->i2c_addr = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("default_boot", &temp_int);
	if (ret < 0) {
		sensor_info->default_boot = FIRST_DEFAULT_BOOT;
	} else {
		sensor_info->default_boot = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("sensor_width", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_width = FIRST_SENSOR_WIDTH;
	} else {
		sensor_info->sensor_width = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("sensor_height", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_height = FIRST_SENSOR_HEIGHT;
	} else {
		sensor_info->sensor_height = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("sensor_fps", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_fps = 30;
	} else {
		sensor_info->sensor_fps = (uint8_t)temp_int;
	}

	return 0;
}

#if (SENSOR_NUM == 2)
int sensor2_info_init(sensor_info_t *sensor_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_string("sensor2_name", (char*)sensor_info->sensor_name);
	if (ret < 0) {
		strcpy((char*)sensor_info->sensor_name, FIRST_SENSOR_NAME);
	}

	ret = inlock_param_get_hex("sensor2_i2c_addr", &temp_int);
	if (ret < 0) {
		sensor_info->i2c_addr = FIRST_I2C_ADDR;
	} else {
		sensor_info->i2c_addr = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("sensor2_default_boot", &temp_int);
	if (ret < 0) {
		sensor_info->default_boot = FIRST_DEFAULT_BOOT;
	} else {
		sensor_info->default_boot = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("sensor2_width", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_width = FIRST_SENSOR_WIDTH;
	} else {
		sensor_info->sensor_width = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("sensor2_height", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_height = FIRST_SENSOR_HEIGHT;
	} else {
		sensor_info->sensor_height = (uint16_t)temp_int;
	}

	ret = inlock_param_get_int("sensor2_fps", &temp_int);
	if (ret < 0) {
		sensor_info->sensor_fps = 30;
	} else {
		sensor_info->sensor_fps = (uint8_t)temp_int;
	}

	return 0;
}
#endif

int encoder_info_init(encoder_info_t *encoder_info)
{
	int ret;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	ret = inlock_param_get_int("rcmode", &temp_int);
	if (ret < 0) {
		encoder_info->rcmode = 1 ;	/* IMP_ENC_RC_MODE_CBR */
	} else {
		encoder_info->rcmode = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("gop", &temp_int);
	if (ret < 0) {
		encoder_info->gop = 25;
	} else {
		encoder_info->gop = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("bitrate", &temp_int);
	if (ret < 0) {
		encoder_info->bitrate = 4000;
	} else {
		encoder_info->bitrate = (uint32_t)temp_int;
	}

	ret = inlock_param_get_int("qp_value", &temp_int);
	if (ret < 0) {
		encoder_info->qp_value = 35;
	} else {
		encoder_info->qp_value = (uint8_t)temp_int;
	}

	return 0;
}
int imp_param_init(imp_attr_t* imp_attr)
{
	int ret;
	bcsh_info_t *bcsh_info = &imp_attr->bcsh_info;
	int temp_int; // 使用这个变量去获取int型的数据，防止非int型变量引入地址时发生强转int型导致内存数据出错

	/* sensor_info[0]: MAIN SENSOR */
	sensor_info_init(&imp_attr->sensor_info[0]);
#if (SENSOR_NUM == 2)
	sensor2_info_init(&imp_attr->sensor_info[1]);
#elif (VIDEO_NUM == 2)
	sensor_info_init(&imp_attr->sensor_info[1]);
#endif
	encoder_info_init(&imp_attr->encoder_info);

	ret = inlock_param_get_int("hvflip", &temp_int);
	if (ret < 0) {
		imp_attr->hvflip = 3;
	} else {
		imp_attr->hvflip = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("bcsh_en", &temp_int);
	if (ret < 0) {
		bcsh_info->bcsh_en = 0;
	} else {
		bcsh_info->bcsh_en = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("bcsh_low", &temp_int);
	if (ret < 0) {
		bcsh_info->bcsh_low = 15;
	} else {
		bcsh_info->bcsh_low = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("bcsh_high", &temp_int);
	if (ret < 0) {
		bcsh_info->bcsh_high = 235;
	} else {
		bcsh_info->bcsh_high = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("ivdc_mode", &temp_int);
	if (ret < 0) {
		imp_attr->ivdc_info.ivdc_mode = 0;
	} else {
		imp_attr->ivdc_info.ivdc_mode = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("ivdc_jpegv", &temp_int);
	if (ret < 0) {
		imp_attr->ivdc_info.ivdc_jpegv = 0;
	} else {
		imp_attr->ivdc_info.ivdc_jpegv = (uint8_t)temp_int;
	}

#ifdef OSD_ENABLE
	ret = inlock_param_get_int("osd_mode", &temp_int);
	if (ret < 0) {
		imp_attr->osd_mode = ISP_OSD;
	} else {
		imp_attr->osd_mode = (uint8_t)temp_int;
	}

	ret = inlock_param_get_int("osd_mem", &temp_int);
	if (ret < 0) {
		imp_attr->osd_mem = OSD_MEM;
	} else {
		imp_attr->osd_mem = (uint8_t)temp_int;
	}
#endif
	ret = inlock_param_get_int("wdr_en", &temp_int);
	if (ret < 0) {
		imp_attr->wdr_en = 0;
	} else {
		imp_attr->wdr_en = (uint8_t)temp_int;
	}

	return 0;
}
/* end of imp_info_init */

int inlock_config_init(char *cfg_file_path)//初始化配置模块，打开或创建配置文件，分配缓冲区，初始化互斥锁
{
	/* if file not exsit, create one */
	if (access(cfg_file_path, F_OK)) {//检查文件是否存在
		cfg_context.fp = fopen(cfg_file_path, "w+");//不存在则以读写方式创建新文件
	} else {
		cfg_context.fp = fopen(cfg_file_path, "r+");//存在则以读写方式打开文件
	}
	if (!cfg_context.fp) {//打开文件失败
		printf("ERROR(%s): inlock_param_init open file [%s] failed! %s\n",
		       INLOCK_PARAM_TAG, cfg_file_path, strerror(errno));
		return -1;
	}

	cfg_context.fd = fileno(cfg_context.fp);//获取文件描述符

	cfg_context.fbuf = (char *)malloc(MAX_CONFIG_FILE_SIZE);//分配缓冲区
	if (!cfg_context.fbuf) {
		printf("ERROR(%s): inlock_param_init alloc memory failed! %s\n",
		       INLOCK_PARAM_TAG, strerror(errno));
		fclose(cfg_context.fp);
		return -1;
	}

	pthread_mutex_init(&cfg_context.mutex, NULL);//初始化互斥锁

	return 0;
}

int inlock_config_load(void *param)
{
	config_func_param_t *cfg_param = (config_func_param_t*)param;

	device_param_init(&cfg_param->dev_info);/* 初始化设备参数, usb 设备信息*/
	video_param_init(&cfg_param->video_info[0]);/* 初始化 uvc 配置 */
#if (VIDEO_NUM == 2)
	video2_param_init(&cfg_param->video_info[1]);
#endif
	audio_param_init(&cfg_param->audio_info);/* 初始化 uac 配置 */
	imp_param_init(&cfg_param->imp_attr);/* 初始化 imp 配置 */

	return 0;
}


int inlock_param_init(char *cfg_file_path, config_func_param_t *param)
{
	int ret = 0;

	ret = inlock_config_init(cfg_file_path);
	if (ret < 0) {
		printf("ERROR(%s): inlock_config_init failed!\n", __func__);
		return -1;
	}

	ret = inlock_config_load(param);
	if (ret < 0) {
		printf("ERROR(%s): inlock_config_load failed!\n", __func__);
		return -1;
	}

	return 0;
}


int inlock_param_deinit(void)//释放配置模块资源，关闭配置文件，释放缓冲区，销毁互斥锁
{
	if (cfg_context.fbuf) {//缓冲区是否存在
		free(cfg_context.fbuf);//释放缓冲区
		cfg_context.fbuf = NULL;
	}

	if (cfg_context.fp) {//文件指针是否存在
		fclose(cfg_context.fp);//关闭文件
		cfg_context.fp = NULL;
		cfg_context.fd = 0;
	}
	pthread_mutex_destroy(&cfg_context.mutex);//销毁互斥锁
	printf("%s...OK\n",__func__);
	return 0;
}
