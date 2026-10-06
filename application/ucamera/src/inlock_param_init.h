#ifndef __INLOCK_PARAM_INIT_H__
#define __INLOCK_PARAM_INIT_H__

#include<stdint.h>
#include <stdint.h>
#include "usbcamera.h"
#include "global_config.h"
#define LABEL_LEN               64

#define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5')

#define FIRST_SENSOR_NAME 			"ov02f"			//sensor name (match with snesor driver name)
#define	FIRST_I2C_ADDR				0x37				//sensor i2c address
#define	FIRST_I2C_ADAPTER_ID		        0				//sensor controller number used (0/1/2/3)
#define FIRST_SENSOR_WIDTH			1920				//sensor width
#define FIRST_SENSOR_HEIGHT			1080				//sensor height
#define	FIRST_RST_GPIO				GPIO_PA(20)			//sensor reset gpio
#define	FIRST_PWDN_GPIO				-1				//sensor pwdn gpio
#define	FIRST_POWER_GPIO			-1				//sensor power gpio
#define	FIRST_SENSOR_ID				0				//sensor index
#define	FIRST_VIDEO_INTERFACE   IMPISP_SENSOR_VI_MIPI_CSI0 	//sensor interface type (dvp/csi0/csi1)
#define	FIRST_MCLK				IMPISP_SENSOR_MCLK0		//sensor clk source (mclk0/mclk1/mclk2)
#define	FIRST_DEFAULT_BOOT			2				//sensor default mode(0/1/2/3/4)

typedef struct _inlock_param_info {
	uint8_t uvc_format_num;			/* format type nums */
	uint8_t stillcap_en;
	uint8_t h264_en;                      	/* h264 enable */
	uint8_t h265_en;
	uint8_t yuv_en;
	uint8_t mjpeg_en;
	uint8_t yuyv_en;
	uint8_t winhello_en;                  	/* winhello en */
	uint8_t adb_en;
	uint32_t vendor_id;                    /* VID */
	uint32_t product_id;                   /* PID */
	uint32_t dev_bcd;                      /* usb version */
	uint8_t serial_number[LABEL_LEN];      /* SN */
	uint8_t product_label[LABEL_LEN];      /* product name */
	uint8_t manufact_label[LABEL_LEN];     /* manufact name */

	uint32_t mult;                  	/* bandwidth=mps*(mult+1) */
	uint32_t mps;                  		/* limit the negotiated maximum bandwidth */
	uint32_t mps_num;
	uint32_t *mps_list;
    uint8_t video_name[LABEL_LEN];		/* video name */
    uint8_t audio_name[LABEL_LEN];		/* audio name */
    uint8_t mic_name[LABEL_LEN];		/* mic name */
    uint8_t spk_name[LABEL_LEN];		/* spk name */
    uint8_t camera_name[LABEL_LEN];		/* camera name */
    uint8_t camera_devnode[LABEL_LEN];	/* camera devnode */
    uint8_t mic_devnode[LABEL_LEN];		/* mic devnode */
    uint8_t spk_devnode[LABEL_LEN];		/* spk devnode */

} inlock_param_t;


#define MAX_CONFIG_FILE_SIZE              4096  
#define MAX_KEY_LENGTH                    16
#define MAX_VALUE_LENGTH                  96

#define CONFIG_OPS_SUCCESS                0
#define CONFIG_OPS_FAIL                   -1
#define CONFIG_RECORD_NOT_EXSIT           -2

int inlock_param_init(char *cfg_file_path, config_func_param_t *inlock_param_info);//初始化配置模块，打开或创建配置文件，分配缓冲区，初始化互斥锁
int inlock_param_deinit(void);//释放配置模块资源，关闭配置文件，释放缓冲区，销毁互斥锁

/* add new record */
int inlock_param_add_string(const char *key, char *value);//把 key:value 形式的字符串添加到配置文件末尾
int inlock_param_add_int(const char *key, int value);//把 key:value 形式的整型添加到配置文件末尾
int inlock_param_add_float(const char *key, float value);//把 key:value 形式的浮点型添加到配置文件末尾
int inlock_param_add_hex(const char *key, int value);//把 key:value 形式的十六进制整型添加到配置文件末尾

/**
 * update record
 * if record not exist and create == 1,
 * it will add the new record to config file.
 **/
int inlock_param_update_string(const char *key, char *value, int create);//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
int inlock_param_update_int(const char *key, int value, int create);//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
int inlock_param_update_float(const char *key, float value, int create);//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行
int inlock_param_update_hex(const char *key, int value, int create);//按 key 更新配置文件中的某一行内容为 line，如果该行不存在且 create 为 1，则添加该行

/* get existed record */
int inlock_param_get_string(const char *key, char *value);//按 key 获取配置文件中的某一行内容，并把该行的 value 部分复制到 value 中
int inlock_param_get_int(const char *key, int *value);//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为整数后存到 value 中
int inlock_param_get_float(const char *key, float *value);//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为浮点数后存到 value 中
int inlock_param_get_hex(const char *key, int *value);//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为十六进制整数后存到 value 中
int inlock_param_get_uhex(const char *key, uint8_t *value,int num);//按 key 获取配置文件中的某一行内容，并把该行的 value 部分转换为十六进制整数后存到 value 数组中，num 指定了 value 数组的大小

/* delete record */
int inlock_param_delete(const char *key);//按 key 删除配置文件中的某一行

#endif /* __INLOCK_PARAM_INIT_H__ */
