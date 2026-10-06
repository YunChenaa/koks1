#ifndef __GLOBAL_CONFIG__
#define __GLOBAL_CONFIG__

#include <stdint.h>

#include <usbcamera.h>
// #include "inlock_log.h"

#define UCAMERA_DEBUG           0

#if UCAMERA_DEBUG
#define Ucamera_DEBUG(format, arg...)                                            \
	printf("%s: " format "\n" , "[Ucamera]", ## arg)
#else
#define Ucamera_DEBUG(format, arg...)
#endif

#ifndef INLOCK_VERSION
#define INLOCK_VERSION                            "V0.0.0"
#endif

#define INLOCK_CONFIG_FILE_PATH			"/config_bak/inlock.config"
#define INLOCK_ATTR_FILE_PATH			"/config_bak/inlock.attr"
#define INLOCK2_ATTR_FILE_PATH			"/config_bak/inlock2.attr"
#define MERT_MODEL_FILE_PATH			"/system/etc/model/AEC_T41_16K_SS_UC.mgk"

#define VENDOR_ID           0xa108	// ingenic VID
#define PRODUCT_ID          0x2240	// 2022.T40
#define DEVICE_BCD          0x0105
#define MANUFACT_LABEL      "Ingenic Semiconductor Co.,Ltd"
#define PRODUCT_LABEL       "Ingenic HD Web Camera"
#define SERIAL_LABEL        "Ucamera001"

#define H264_MAX_WIDTH      3840
#define H264_MAX_HEIGHT     2160
#define H264_MAX_FPS        25

#define UVC_FORMAT_MAX		5

#define AMIC_ENABLE             	   1
#define DMIC_ENABLE     	           1
#define SPK_ENABLE	                   1
#define AEC_ENABLE			   1
#define MIC_VOL                            0
#define SPK_VOL                            0
#define MIC_CHNCNT                         1
#define SPK_CHNCNT                         1
#define MIC_FRAME_TIME                     40
#define SPK_FRAME_TIME                     40
#define SAMPLERATE_NUM_DEF		1
#define SAMPLERATE_DEF			48000
#define BITWIDTH_DEF			16
#define SOUNDMODE_DEF			1

#define ADB_ENABLE                         1
#define STILLCAP_ENABLE                    1
#define H264_ENABLE                        1
#define H265_ENABLE                        1

#ifdef OSD_ENABLE
#define DISABLE_OSD				0
#define IPU_OSD					1
#define ISP_OSD					2
#define IPU_ISP_OSD				3
#define OSD_MEM                                 1
#define OSD_EN                                  1
#endif

typedef struct _sensor_info {
	uint8_t default_boot;
	uint8_t sensor_fps;
	uint16_t i2c_addr;                     /* i2c_addr */
	uint16_t sensor_width;                 /* sensor width */
	uint16_t sensor_height;                /* sensor height */
	uint8_t sensor_name[LABEL_LEN];        /* sesnor_name */
} sensor_info_t;

typedef struct _encoder_info {
	uint8_t rcmode;                        /* 编码模式 */
	uint8_t gop;
	uint8_t qp_value;                      /* qp value */
	uint32_t bitrate;                      /* 码率 */
} encoder_info_t;

typedef struct {
	unsigned char bcsh_en;
	unsigned char bcsh_low;
	unsigned char bcsh_high;
} bcsh_info_t;

typedef struct _ivdc_info{
	uint8_t ivdc_mode;
	uint8_t ivdc_jpegv;
} ivdc_info_t;

/* IMP ISP Attr */
typedef struct _imp_attr {
	uint8_t hvflip;
	uint8_t dynamic_fps;
#ifdef OSD_ENABLE
	uint8_t osd_mode;                    /* 0: disable osd 1: ipu osd, 2: isp osd, 3: ipu osd and isp osd */
	uint8_t osd_mem;                     /* 1：512K  2：512*2K */
#endif
	uint8_t wdr_en;

	ivdc_info_t ivdc_info;
	bcsh_info_t bcsh_info;
	sensor_info_t sensor_info[VIDEO_NUM];
	encoder_info_t encoder_info;
} imp_attr_t;

typedef struct _config_func_param {
	device_info_t dev_info;			/* usb 设备信息 */
	audio_info_t audio_info;		/* uac 配置 */
	video_info_t video_info[VIDEO_NUM];	/* uvc 配置 */

	imp_attr_t imp_attr;			/* imp 可配参数配置 */
	unsigned int buff_size;

} config_func_param_t;

extern config_func_param_t g_inlock_param_info;

#endif
