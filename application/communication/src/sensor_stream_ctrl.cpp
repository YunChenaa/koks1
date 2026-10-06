#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;

//apb总线时钟和i2c工作时钟
#define DEV_CLK 90000000
#define I2C_CLK 1000000

#define I2C_M_RD 1

#define I2C_BASE_PHYS_SMB0	0x10050000   //i2c index 0

#define I2C_TAR(base)       (*(volatile u32*)((base)+0x004))
#define I2C_DC(base)        (*(volatile u32*)((base)+0x010))
#define I2C_RXTL(base)      (*(volatile u32*)((base)+0x038))
#define I2C_TXTL(base)      (*(volatile u32*)((base)+0x03C))
#define I2C_CTXOF(base)     (*(volatile u32*)((base)+0x04C))
#define I2C_CTXABRT(base)   (*(volatile u32*)((base)+0x054))
#define I2C_CSTP(base)      (*(volatile u32*)((base)+0x060))
#define I2C_STA(base)       (*(volatile u32*)((base)+0x070))
#define I2C_DMACR(base)     (*(volatile u32*)((base)+0x088))

/* I2C 消息结构 */
struct i2c_msg {
	unsigned int device_addr;
	unsigned char flag;
	unsigned char *buf;
	int len;
};

u32 i2c_base = 0;

/* I2C mmap初始化函数 */
static int i2c_mmap_init()
{
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("Error opening /dev/mem");
		return -1;
	}

	i2c_base = (u32)mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, I2C_BASE_PHYS_SMB0);
	if (i2c_base == (u32)MAP_FAILED) {
		perror("Error mmap I2C");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

/* I2C mmap清理函数 */
static void i2c_mmap_exit() {
	if (i2c_base) {
		munmap((void*)i2c_base, 0x1000);
	}

	i2c_base = 0;
}

/* I2C 初始化 */
static int i2c_init(int speed) {
	if (i2c_mmap_init() < 0) {
		return -1;
	}

	return 0;
}

/* CPU 模式写操作 */
static int i2c_write_cpu(struct i2c_msg *i2c) {
	unsigned int i2c_dc = 0;
	int i2c_tar = 0;
	unsigned int i = 0;

	I2C_DMACR(i2c_base) = 0;
	/* 设置设备地址 */
	i2c_tar = I2C_TAR(i2c_base);
	i2c_tar &= ~(1 << 12);
	i2c_tar &= ~(0x3ff);
	i2c_tar |= i2c->device_addr;
	I2C_TAR(i2c_base) = i2c_tar;

	I2C_CSTP(i2c_base);
	I2C_CTXOF(i2c_base);
	I2C_CTXABRT(i2c_base);

	I2C_TXTL(i2c_base) = i2c->len;
	/* 写数据 */
	for (i = 0; i < i2c->len; i++) {
		i2c_dc = 0;
		i2c_dc &= ~(1 << 8);

		/* 最后一个字节发送 STOP */
		if (i == (i2c->len - 1)) {
			i2c_dc |= (1 << 9); // stop
		}
		/* 第一个字节发送 RESTART */
		if (i == 0) {
			i2c_dc |= (1 << 10); // restart
		}

		i2c_dc |= i2c->buf[i];
		I2C_DC(i2c_base) = i2c_dc;
	}

	/* 等待传输完成 */
	while (!(I2C_STA(i2c_base) & (1 << 2)))
		;

	return 0;
}

/* CPU 模式读操作 */
static void i2c_read_cpu(struct i2c_msg *i2c) {
	unsigned int i2c_dc;
	int i2c_tar = 0;
	unsigned int i = 0;

	/* 设置设备地址 */
	i2c_tar = I2C_TAR(i2c_base);
	i2c_tar &= ~(1 << 12);
	i2c_tar &= ~(0x3ff);
	i2c_tar |= i2c->device_addr;
	I2C_TAR(i2c_base) = i2c_tar;

	I2C_CSTP(i2c_base);
	I2C_CTXOF(i2c_base);
	I2C_CTXABRT(i2c_base);

	I2C_RXTL(i2c_base) = i2c->len - 1;

	/* 读数据 */
	for (i = 0; i < i2c->len; i++) {
		i2c_dc = 0;
		i2c_dc |= (1 << 8); // 读标志

		if (i == 0 && i2c->len > 1) {
			i2c_dc |= (1 << 10); // restart
		} else if (i == i2c->len - 1) {
			i2c_dc |= (1 << 9); // stop
		}

		I2C_DC(i2c_base) = i2c_dc;

		/* 等待数据接收 */
		while (!(I2C_STA(i2c_base) & (1 << 3)))
			;

		i2c->buf[i] = (I2C_DC(i2c_base) & 0Xff);
	}
}


/* I2C 传输函数 */
static void i2c_xfer(struct i2c_msg *i2c) {
	if (i2c->flag != I2C_M_RD) {
		i2c_write_cpu(i2c);
	} else {
		i2c_read_cpu(i2c);
	}
}


#define SENSOR0_I2C_ADDR 0x3c
#define SENSOR1_I2C_ADDR 0x3d

#define SENSOR_REG_END    0xff

struct regval_list {
    u8 reg_num;
    u8 value;
};

static struct regval_list ov02f_stream_on_mipi[] = {
	{0xfd,0x03},
	{0xc2,0x01},
	{0xfd,0x01},
	{ SENSOR_REG_END, 0x00 }, /* END MARKER */
};

static struct regval_list ov02f_stream_off_mipi[] = {
	{0xfd,0x03},
	{0xc2,0x00},
	{0xfd,0x01},
	{ SENSOR_REG_END, 0x00 }, /* END MARKER */
};

int sensor_stream_on(int sernsor_id)
{
    // 初始化 I2C
	if (i2c_init(I2C_CLK) != 0) {
		printf("Error: Failed to initialize I2C\n");
		// 添加错误处理时的资源清理
		i2c_mmap_exit();
		return -1;
	}

    struct i2c_msg msg;
    int i = 0;
    msg.device_addr = (sernsor_id == 0) ? SENSOR0_I2C_ADDR : SENSOR1_I2C_ADDR;
    msg.flag = 0; // 写操作
    while(ov02f_stream_on_mipi[i].reg_num != SENSOR_REG_END) {
        unsigned char buf[2];
        buf[0] = ov02f_stream_on_mipi[i].reg_num;
        buf[1] = ov02f_stream_on_mipi[i].value;
        msg.buf = buf;
        msg.len = 2;
        i2c_xfer(&msg);
        i++;
    }
    return 0;
}

int sensor_stream_off(int sernsor_id)
{
    // 初始化 I2C
	if (i2c_init(I2C_CLK) != 0) {
		printf("Error: Failed to initialize I2C\n");
		// 添加错误处理时的资源清理
		i2c_mmap_exit();
		return -1;
	}

    struct i2c_msg msg;
    int i = 0;
    msg.device_addr = (sernsor_id == 0) ? SENSOR0_I2C_ADDR : SENSOR1_I2C_ADDR;
    msg.flag = 0; // 写操作
    while(ov02f_stream_off_mipi[i].reg_num != SENSOR_REG_END) {
        unsigned char buf[2];
        buf[0] = ov02f_stream_off_mipi[i].reg_num;
        buf[1] = ov02f_stream_off_mipi[i].value;
        msg.buf = buf;
        msg.len = 2;
        i2c_xfer(&msg);
        i++;
    }
    return 0;
}

int sensor_load_ev(int sernsor_id, int ev)
{
	// 初始化 I2C
	if (i2c_init(I2C_CLK) != 0) {
		printf("Error: Failed to initialize I2C\n");
		// 添加错误处理时的资源清理
		i2c_mmap_exit();
		return -1;
	}

	struct regval_list ov02f_ev[] = {
		{0xfd,0x01},
		{0x0e,(unsigned char)((ev >> 8) & 0xff)},
		{0x0f,(unsigned char)(ev & 0xff)},
		{0x22,(unsigned char)(((ev & 0xffff0000) >> 16) & 0xff)},
		{0xfe,0x02},
		{ SENSOR_REG_END, 0x00 }, /* END MARKER */
	};

	struct i2c_msg msg;
    int i = 0;
	msg.device_addr = (sernsor_id == 0) ? SENSOR0_I2C_ADDR : SENSOR1_I2C_ADDR;
	msg.flag = 0; // 写操作

	while(ov02f_ev[i].reg_num != SENSOR_REG_END) {
        unsigned char buf[2];
        buf[0] = ov02f_ev[i].reg_num;
        buf[1] = ov02f_ev[i].value;
        msg.buf = buf;
        msg.len = 2;
        i2c_xfer(&msg);
        i++;
    }

	return 0;
}
