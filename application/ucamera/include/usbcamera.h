#ifndef __USBCAMERA_H
#define __USBCAMERA_H

#include <stdint.h>

#ifdef DUAL_VIDEO
#define VIDEO_NUM			2
#define VIDEO_DEVICE_NAME		"/dev/video0"
#define VIDEO2_DEVICE_NAME		"/dev/video1"
#define VIDEO_MULT			1
#define VIDEO2_MULT			0
#else
#define VIDEO_NUM			1
#define VIDEO_DEVICE_NAME		"/dev/video0"
#define VIDEO_MULT			2
#define VIDEO2_MULT			0	/* useless,only for compiling */
#endif

#define UVC_BUF_NUM			2
#define UVC_BUF_SIZE			(512*1024)

#define LABEL_LEN               64

#define UCAMERA_EVENT_STREAMON			1
#define UCAMERA_EVENT_STREAMOFF			2

/*uvc process unit cs*/
#define UVC_BACKLIGHT_COMPENSATION_CONTROL			0x01
#define UVC_BRIGHTNESS_CONTROL					0x02
#define UVC_CONTRAST_CONTROL					0x03
#define UVC_GAIN_CONTROL					0x04
#define UVC_POWER_LINE_FREQUENCY_CONTROL			0x05
#define UVC_HUE_CONTROL						0x06
#define UVC_SATURATION_CONTROL					0x07
#define UVC_SHARPNESS_CONTROL					0x08
#define UVC_GAMMA_CONTROL					0x09
#define UVC_WHITE_BALANCE_TEMPERATURE_CONTROL			0x0a
#define UVC_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL		0x0b
#define UVC_DIGITAL_MULTIPLIER_CONTROL          		0x0e

/*uvc extension unit cs*/
#define UVC_EU_CMD_USR1			0x01
#define UVC_EU_CMD_USR2			0x02
#define UVC_EU_CMD_USR3			0x03
#define UVC_EU_CMD_USR4			0x04
#define UVC_EU_CMD_USR5			0x05
#define UVC_EU_CMD_USR6			0x06
#define UVC_EU_CMD_USR7			0x07
#define UVC_EU_CMD_USR8			0x08
#define UVC_EU_CMD_USR9			0x09
#define UVC_EU_CMD_USR10		0x0A
#define UVC_EU_CMD_USR11		0x0B
#define UVC_EU_CMD_USR12		0x0C
#define UVC_EU_CMD_USR13		0x0D
#define UVC_EU_CMD_USR14		0x0E
#define UVC_EU_CMD_USR15		0x0F
#define UVC_EU_CMD_USR16		0x10
#define UVC_EU_CMD_USR17		0x11
#define UVC_EU_CMD_USR18		0x12
#define UVC_EU_CMD_USR19		0x13
#define UVC_EU_CMD_USR20		0x14
#define UVC_EU_CMD_USR21		0x15
#define UVC_EU_CMD_USR22		0x16

/* CT unit cs */
#define UVC_AUTO_EXPOSURE_MODE_CONTROL  0x02
#define UVC_EXPOSURE_TIME_CONTROL       0x04
#define UVC_FOCUS_CONTROL               0x06
#define UVC_FOCUS_AUTO_CONTROL          0x08
#define UVC_ZOOM_ABSOLUTE_CONTROL	0x0b
#define UVC_PANTILT_ABSOLUTE_CONTROL	0x0d
#define UVC_ROLL_ABSOLUTE_CONTROL	0x0f
#define UVC_PRIVACY_CONTROL		0x11
#define UVC_REGION_OF_INTEREST_CONTROL  0x14

/* cmd data index */
#define UVC_CUR					0
#define UVC_MIN					1
#define UVC_MAX					2
#define UVC_RES					3
#define UVC_LEN					4
#define UVC_INFO				5
#define UVC_DEF					6

#ifdef FLASH_4M
#define OTA_NORFLASH_OFFSET (4 * 1024 * 1024 - 32 * 1024)
#elif defined(FLASH_8M)
#define OTA_NORFLASH_OFFSET (8 * 1024 * 1024 - 32 * 1024)
#elif defined(FLASH_16M)
#define OTA_NORFLASH_OFFSET (16 * 1024 * 1024 - 32 * 1024)
#else
#define OTA_NORFLASH_OFFSET (8 * 1024 * 1024 - 32 * 1024)
#endif

/*V4L2 INFO */
typedef struct _v4l2_info {
#ifndef UVC_BUF_SIZE
	float compress_factor;		/* Video compression factor */
#else
	unsigned int buff_size;          /* Fixed buff size */
#endif
	uint16_t max_width;	        /* The maximum width of the image */
	uint16_t max_height;	        /* The maximum height of the image */
} v4l2_info_t;

/*UVC Frame */
typedef struct _uvc_frame {
	uint16_t width;			/* frame width */
	uint16_t height;		/* frame height */
	uint8_t fps_num;		/* supported fps num */
	uint32_t *fps_list;		/* fps list */
	uint32_t *payload_size_list;		/* mult alt setting */
} uvc_frame_t;

/* UVC Format */
typedef struct _uvc_format {
	uint32_t fcc;			/* format type */
	uint8_t frames_num;             /* frame numbers */
	uvc_frame_t *uvc_frame;		/* frame list */
} uvc_format_t;

/* UVC CT */
typedef struct _uvc_ct{
	uint8_t controls[3];		/* CT unit controls support select */
}uvc_ct_t;

/* UVC PU*/
typedef struct _uvc_pu{
#ifdef CONFIG_UVC_1_5
	uint8_t controls[3];		/* PU unit controls support select */
#else
	uint8_t controls[2];
#endif
}uvc_pu_t;

/* UVC EU*/
typedef struct _uvc_eu{
	uint8_t en;			/* EU switch */
	uint8_t guid[16];               /* EU supplier extension unit code */
	uint8_t num_controls;           /* EU unit controls num */
	uint8_t controls[3];	        /* EU unit controls support select */
}uvc_eu_t;

/* LED Attr */
typedef struct _led_ctl {
	uint8_t led_en;			/* uvc frame led control  */
	uint8_t led_status;		/* led status */
	uint8_t led_level;              /* volume level */
	int16_t led_gpio;               /* GPIO pin */
} led_ctl_t;

/*Video Info */
typedef struct _video_info {
	uint8_t uvc_format_num;			/* format type nums */
	uint8_t stillcap_en;
	uint8_t h264_en;                      	/* h264 enable */
	uint8_t h265_en;
	uint8_t yuv_en;
	uint8_t mjpeg_en;
	uint8_t yuyv_en;
	uint8_t winhello_en;                  	/* winhello en */

	uint32_t mult;                  	/* bandwidth=mps*(mult+1) */
	uint32_t mps;                  		/* limit the negotiated maximum bandwidth */
	uint32_t mps_num;
	uint32_t *mps_list;

	uint8_t video_name[LABEL_LEN];		/* video name */

	led_ctl_t led_ctl;			/* led control */
	uvc_format_t **uvc_format;		/* format list */

	uvc_ct_t uvc_ct;			/* camera terminal controls */
	uvc_pu_t uvc_pu;			/* processing unit controls */
	uvc_eu_t uvc_eu;			/* extension unit controls */

} video_info_t;

/**
  * USB Device Info
  */
typedef struct _device_info {
	uint8_t adb_en;
	uint32_t vendor_id;                    /* VID */
	uint32_t product_id;                   /* PID */
	uint32_t dev_bcd;                      /* usb version */
	uint8_t serial_number[LABEL_LEN];      /* SN */
	uint8_t product_label[LABEL_LEN];      /* product name */
	uint8_t manufact_label[LABEL_LEN];     /* manufact name */
} device_info_t;

/**
  * video control structure
  */
struct Ucamera_Video_PU_Control {
	int type;
	int cmd;
	int data[8];				/* data buf  */
	int (*set)(int video_index, int value);
	int (*get)(int video_index);
};
/**
  * video control structure
  */
struct Ucamera_Video_CT_Control {
	int type;
	int cmd;
	uint8_t data[8][12];				/* data buf  */
	int (*set)(int video_index, char *data, int len);
	void* (*get)(int video_index);
};

/**
  * video extention unit structure
  */
struct Ucamera_Video_EU_Control {
	int type;
	int len;
	uint8_t data[8][60];				/* data buf  */
	int (*set)(int video_index, int cs, void *data, int len);
	//int (*get)(int cs, char *data);
	int (*get)(int video_index, int cs, int req, void *data);
};

/**
  * video get frame structure
  */
struct Ucamera_Video_CB_Func {
	int (*get_YuvFrame)(int video_index, char *frame);
	int (*get_JpegFrame)(int video_index, char *frame);
	int (*get_H264Frame)(int video_index, char *frame);
};

/*
 * UVC Frame Temp
 */
struct Ucamera_Video_Frame {
	uint32_t fcc;
	uint32_t width;
	uint32_t height;
	uint32_t intervals;
};

/* audio device info */
#define AUDIO_DEVICE_NAME		"/dev/gaudio"

/* audio param info */
typedef struct _audio_param_info {
	uint8_t audio_name[LABEL_LEN];  /* mic audio name */
	uint32_t samplerate_num;	/* samplerate */
	uint8_t bitwidth;		/* bitwidth */
	uint8_t soundmode;		/* soundmode */
	int8_t volume;			/* volume */
	uint32_t *samplerate_list;
	uint8_t prev[32];               /* reserved */
					/* prev[0] ==> channel num         [1 Byte] */
					/* prev[1] ==> current samplerate  [4 Byte] */
					/* prev[5] ==> frame time (unit:ms)[2 Byte]*/
} audio_param_info_t;

/*Audio Info */
typedef struct _audio_info {
	uint8_t amic_en;		/* amic enable */
	uint8_t dmic_en;		/* dmic enable */
	uint8_t spk_en;			/* spk enable */
	audio_param_info_t amic_info;		/* mic config */
	audio_param_info_t dmic_info;		/* mic config */
	audio_param_info_t spk_info;		/* spk config */

	uint8_t aec_en;			/* audio echo cancellation enable */
	uint8_t audio_ns;               /* audio noise level */
} audio_info_t;

struct Ucamera_Audio_Frame {
	unsigned int bitwidth;				/**< 音频采样精度 */
	unsigned int soundmode;				/**< 音频声道模式 */
	char *data;
	int len;							/**< 音频帧长度 */
};
/**
 * 音频功能回调函数结构体
 */
struct Ucamera_Audio_CB_Func {
    int (*get_AudioPcm)(short *pcm);              /*!< 获取音频 PCM 数据 */
    int (*set_Spk_Volume)(int16_t vol);           /*!< 设置扬声器音量 */
    int (*set_Mic_Volume)(int16_t vol);           /*!< 设置麦克风音量 */
    int (*set_Spk_Mute)(int mute);                /*!< 设置扬声器静音状态 */
    int (*set_Mic_Mute)(int mute);                /*!< 设置麦克风静音状态 */
    int (*set_Mic_Samplerate)(uint32_t samplerate); /*!< 设置麦克风采样率 */
    int (*get_record_off)();                      /*!< 停止录音 */
    int (*get_record_on)();                       /*!< 开始录音 */
    int (*get_speak_off)();                       /*!< 停止播放 */
    int (*get_speak_on)();                        /*!< 开始播放 */
};

typedef struct __hwvreg_write_data {
	int offset;
	int len;
	void *data;
} hwvreg_write_data;

struct ucam_version {
	char cpu[32];            /* cpu platform */
	char version[64];        /* usbcamera version */
};

struct Ucamera_Version {
	struct ucam_version ucam;
	struct ucam_version ucam_drv;
};

/**
 * @fn int IF_Ucam_Init(void)
 *
 * open ucamera device
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: This function must be called firstly.
 */
int IF_Ucam_Init(void);

/**
 * @fn int IF_Ucam_DeInit(void)
 *
 * close ucamera device
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: This function must be called lastly.
 */
int IF_Ucam_DeInit(void);

/**
 * @fn int IF_Ucam_Enable(void)
 *
 * enable ucamera
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Audio_Config use
 */
int IF_Ucam_Enable(void);

/**
 * @fn int IF_Ucam_Disable(void)
 *
 * disable ucamera
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Disable(void);

/**
 * @fn int IF_Ucam_Device_Config(device_info_t device_info)
 *
 * config device descriptor
 *
 * @param: device_info: vid, pid etc.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Init use
 */
int IF_Ucam_Device_Config(device_info_t device_info);

/**
 * @fn int IF_Ucam_Video_Config(uint8_t video_index, video_info_t video_info)
 *
 * config video descriptor
 *
 * @param: video_index
 * @param: video_info: frames, fps, and other parameters.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Device_Config
 */
int IF_Ucam_Video_Config(uint8_t video_index, video_info_t video_info);

/**
 * @fn int IF_Ucam_Video_Init(char* video_name)
 *
 * open uvc device
 *
 * @param: video name. eg: /dev/video0
 *
 * @retval: video index
 * @retval: -1 failure.
 *
 * @attention: after IF_Ucam_Enable use
 */
int IF_Ucam_Video_Init(char *video_name);

/**
 * @fn int IF_Ucam_Video_DeInit(uint8_t video_index)
 *
 * close uvc device
 *
 * @param: none.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Video_DeInit(uint8_t video_index);

/**
 * @fn int IF_Ucam_Video_Start(void)
 *
 * video start streaming
 *
 * @param: none.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_Start(void);

/**
 * @fn int IF_Ucam_Video_Stop(void)
 *
 * video stop streaming
 *
 * @param: none.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Video_Stop(void);

/**
 * @fn int IF_Ucam_Video_VfCtl(struct Ucamera_Video_CB_Func *v_func, uint8_t video_index)
 *
 * register get frame function
 *
 * @param: function list
 * @param: video index
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_VfCtl(struct Ucamera_Video_CB_Func *v_func, uint8_t video_index);

/**
 * @fn int IF_Ucam_Video_Puctl(void)
 *
 * register process unit control
 *
 * @param: none.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_Puctl(struct Ucamera_Video_PU_Control *puctl[], uint8_t video_index);

/**
 * @fn int IF_Ucam_Video_Ctctl(void)
 *
 * register camera terminal control
 *
 * @param: none.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_Ctctl(struct Ucamera_Video_CT_Control *ctctl[], uint8_t video_index);

/**
 * @fn int IF_Ucam_Video_Euctl(struct Ucamera_Video_EU_Control *euctl, uint8_t video_index)
 *
 * register extention unit control
 *
 * @param: extention unit control
 * @param: video_index
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_Euctl(struct Ucamera_Video_EU_Control *euctl, uint8_t video_index);

/**
 * @fn int IF_Ucam_Video_EventProcess(int (*event_process)(int, int , void *), uint8_t video_index)
 *
 * register event process
 *
 * @param: event process
 * @param: video index.
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Video_EventProcess(int (*event_process)(int, int , void *), uint8_t video_index);

/**
 * @fn int IF_Ucam_Audio_Config(audio_info_t audio_info)
 *
 * config audio descriptor
 *
 * @param: audio_info: mic, spk parameters
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Start, after IF_Ucam_Video_Init use
 */
int IF_Ucam_Audio_Config(audio_info_t audio_info);

/**
 * @fn int IF_Ucam_Audio_Init(char *audio_name, audio_info_t *audio_info)
 *
 * open audio device
 *
 * @param: audio_name: AUDIO_DEVICE_NAME
 * @param: audio info: mic info and spk info
 *
 * @retval: 0 audio device fd
 * @retval: -1 failure.
 *
 * @attention: before IF_Ucam_Enable, after IF_Ucam_Device_Config use
 */
int IF_Ucam_Audio_Init(char *audio_name, audio_info_t *audio_info);

/**
 * @fn int IF_Ucam_Audio_DeInit(void)
 *
 * close audio device
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Audio_DeInit(void);

/**
 * @fn int IF_Ucam_Audio_Start(void)
 *
 * create audio thread
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Audio_Config use
 */
int IF_Ucam_Audio_Start(audio_info_t *audio_info);

/**
 * @fn int IF_Ucam_Audio_Stop(void)
 *
 * stop audio thread
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Audio_Stop(void);

/**
 * @fn int IF_Ucam_Audio_Func(Ucamera_Audio_CB_Func)
 *
 * register audio functions
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Audio_Start, after IF_Ucam_Audio_Init use
 */
int IF_Ucam_Audio_Func(struct Ucamera_Audio_CB_Func *a_func);

/**
 * @fn int IF_Ucam_Audio_Get_Frame(struct Ucamera_Audio_Frame *frame)
 *
 * get uac speak frame to play
 *
 * @param: frame data and length
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: after IF_Ucam_Audio_Start use
 */
int IF_Ucam_Audio_Get_Frame(struct Ucamera_Audio_Frame *frame);

/**
 * @fn int IF_Ucam_Audio_Release_Frame(struct Ucamera_Audio_Frame *frame)
 *
 * release uac frame
 *
 * @param: frame addr
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Audio_Release_Frame(struct Ucamera_Audio_Frame *frame);

/**
 * @fn int IF_Ucam_Audio_Set_Channel(int value)
 *
 * set uac channel
 *
 * @param: value is channel num
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention:
 */
int IF_Ucam_Audio_Set_Channel(int value);

/**
 * @fn int IF_Ucam_GetVersion()
 *
 * get ucam version info
 *
 * @param: version
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: This function must be call after IF_Ucam_Init.
 */
int IF_Ucam_GetVersion(struct Ucamera_Version* version);

/**
 * @fn int IF_Ucam_Write_NorFlash()
 *
 * write norflash
 *
 * @param: hwvreg_write_data
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: This function takes a struct as an entry.
 */
int IF_Ucam_Write_NorFlash(hwvreg_write_data hwvreg_data);

/**
 * @fn int IF_Ucam_Read_Norflash()
 *
 * read norflash
 *
 * @param: hwvreg_read_data
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: This function takes a struct as an entry.
 */
int IF_Ucam_Read_Norflash(hwvreg_write_data hwvreg_data);

/**
 * @fn int IF_Ucam_Set_RecoveryMode()
 *
 * Reset device to recovery mode
 *
 * @param: value
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: 	support mode
 *          CDC : value = CDC_MODE_SIGNATURE
 *          HID : value = HID_MODE_SIGNATURE
 */
int IF_Ucam_Set_RecoveryMode(int value);

/**
 * @fn int IF_Ucam_Bulk_Mode(void)
 *
 * set uvc bulk transmission
 *
 * @param: not have
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Config,after IF_Ucam_Device_Config use
 */
int IF_Ucam_Bulk_Mode(void);

/**
 * @fn int IF_Ucam_UVC_Interrupt_level(int value)
 *
 * set uvc intervals level
 *
 * @param: value The recommended range is 1-4
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Config,after IF_Ucam_Device_Config use
 */
int IF_Ucam_UVC_Interrupt_level(int value);

/**
 * @fn int IF_Ucam_Video_Fps_Default()
 *
 * set default fps
 *
 * @param: default_fps: fps_default
 * @param: primary and secondary stream(fps_mode): 0 mean primary stream,
 * 1 mean secondary stream, 2 mean primary and secondary stream
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Config,after IF_Ucam_Device_Config use
 */
int IF_Ucam_Video_Fps_Default(uint32_t fps_default, int fps_mode);

/**
 * @fn int IF_Ucam_FS_Enable(void)
 *
 * ucamera full speed enable
 *
 * @param: none
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Config,after IF_Ucam_Device_Config use
 */
int IF_Ucam_FS_Enable(void);

/**
 * @fn int IF_Ucam_V4l2_Set_Config(v4l2_info_t * info)
 *
 * set config v4l2 info
 *
 * @param[in]: info.compress_factor :  compression factor (v4l2_buffersize = w * h * 1.5 * compress_factor)
 * @param[in]: info.max_width :  max width frame (Default value 3840)
 * @param[in]: info.max_height :  max height frame (Default value 2160)
 *
 * @retval: 0 success
 * @retval: other values means failure.
 *
 * @attention: before IF_Ucam_Video_Init,after IF_Ucam_Video_Config use
 */
int IF_Ucam_V4l2_Set_Config(v4l2_info_t * info);

/**
 * @fn uint32_t IF_Ucam_V4l2_Get_BuffSize(uint8_t video_index)
 *
 * get v4l2 buff size
 *
 * @param:  video index
 *
 * @retval: v4l2 buff size
 *
 * @attention: afterwards, imp_sdk_init will use
 */
uint32_t IF_Ucam_V4l2_Get_BuffSize(uint8_t video_index);

#endif
