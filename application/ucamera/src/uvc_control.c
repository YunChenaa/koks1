#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/usb/video.h>

#include "uvc_control.h"
#include <usbcamera.h>

#include "xu_control/uvc_eu_control.h"
#include "xu_control/uvc_xu_control.h"

#include "uvc_camera.h"

#define GET_CUR			0x81

#define SAVE_FOR_POWERDOWN             0
#define UVC_PAN_ATTR_CONTROL           0x1
#define UVC_TILT_ATTR_CONTROL          0x2
#define EU_FOV_ATTR_CONTROL            0x3
#define EU_AUDIO_ANGLE_ATTR_CONTROL    0x4
#define EU_PANTILT_MODE_CONTROL        0x5
#define EU_HDR_MODE_CONTROL	       0x6
#define UVC_ZOOM_ATTR_CONTROL          0xB

static struct Ucamera_Video_CB_Func v_func;
// extern struct chn_conf chn[];
// extern IMPSensorInfo sensor_info[];
uint32_t v4l2_buff_size[VIDEO_NUM];              /* Actual v4l2 buff size used */
/*static char serial_label[LABEL_LEN] = "1234567890123456";*/


typedef struct _uvc_control {
	uint8_t hvflip;
	uint8_t stream_on;
	uint8_t imp_inited;
	uint8_t dynamic_fps;
	uint32_t sensor_fps;
	uint32_t sensor_width;
	uint32_t sensor_height;
	uint32_t uvc_width;
	uint32_t uvc_height;
	pthread_mutex_t uvc_attr_mutex;
	FILE *uvc_attr_fd;
	/*sem_t ucam_fs_sem;    */
} uvc_control_t;

static uvc_control_t uvc_ctx[VIDEO_NUM];

/* ct */
static int sample_video_ae_mode_set(int video_index, char* data, int length);
static void* sample_video_ae_mode_get(int video_index);

static int sample_video_exposure_time_abs_set(int video_index, char* data, int length);
static void* sample_video_exposure_time_abs_get(int video_index);

static int sample_video_focus_abs_set(int video_index, char *data, int length);
static void* sample_video_focus_abs_get(int video_index);

static int sample_video_focus_auto_set(int video_index, char *data, int length);
static void* sample_video_focus_auto_get(int video_index);

static int sample_video_zoom_abs_set(int video_index, char *data, int length);
static void* sample_video_zoom_get(int video_index);

static int sample_video_roi_set(int video_index, char *data, int length);
static void* sample_video_roi_get(int video_index);

/*pu*/
static int sample_video_backlight_compens_set(int video_index, int value);
static int sample_video_backlight_compens_get(int video_index);

static int sample_video_brightness_set(int video_index, int value);
static int sample_video_brightness_get(int video_index);

static int sample_video_gain_set(int video_index, int value);
static int sample_video_gain_get(int video_index);

static int sample_video_contrast_set(int video_index, int value);
static int sample_video_contrast_get(int video_index);

static int sample_video_powerlinefreq_set(int video_index, int value);
static int sample_video_powerlinefreq_get(int video_index);

static int sample_video_hue_set(int video_index, int value);
static int sample_video_hue_get(int video_index);

static int sample_video_saturation_set(int video_index, int value);
static int sample_video_saturation_get(int video_index);

static int sample_video_sharpness_set(int video_index, int value);
static int sample_video_sharpness_get(int video_index);

static int sample_video_gamma_set(int video_index, int value);
static int sample_video_gamma_get(int video_index);

static int sample_video_whitebalance_set(int video_index, int value);
static int sample_video_whitebalance_get(int video_index);

static int sample_video_whitebalance_set_auto(int video_index,int value);
static int sample_video_whitebalance_get_auto(int video_index);
/* eu */
static int sample_video_eu_set(int video_index, int cmd, void *data, int len);
static int sample_video_eu_get(int video_index, int cmd, int req, void *data);

static int sample_video_pantilt_abs_set(int video_index, char* data, int length);
static void* sample_video_pantilt_abs_get(int video_index);

static int sample_video_roll_abs_set(int video_index, char* data, int length);
static void* sample_video_roll_abs_get(int video_index);

int ucamera_uvc_ct_attr_save(int video_index, int type, int value);
/*
 * this is pu_string
 */
struct uvc_pu_string {
	char id;
	const char *s;
};

struct uvc_ct_string {
	char id;
	const char *s;
};

struct uvc_pu_string pu_string[] = {
	{0, "null"},
	{UVC_BACKLIGHT_COMPENSATION_CONTROL, "backlight"},
	{UVC_BRIGHTNESS_CONTROL, "brightness"},
	{UVC_CONTRAST_CONTROL, "contrast"},
	{UVC_GAIN_CONTROL, "gain"},
	{UVC_POWER_LINE_FREQUENCY_CONTROL, "powerline_freq"},
	{UVC_HUE_CONTROL, "hue"},
	{UVC_SATURATION_CONTROL, "saturation"},
	{UVC_SHARPNESS_CONTROL, "sharpness"},
	{UVC_GAMMA_CONTROL, "gamma"},
	{UVC_WHITE_BALANCE_TEMPERATURE_CONTROL, "white_balance"},
	{UVC_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, "white_balance_auto"}
};

struct uvc_ct_string ct_string[] = {
	{0, "null"},
	{UVC_PAN_ATTR_CONTROL, "pan"},
	{UVC_TILT_ATTR_CONTROL, "tilt"},
	{EU_FOV_ATTR_CONTROL, "fov"},
	{EU_AUDIO_ANGLE_ATTR_CONTROL, "audio_angle"},
	{EU_PANTILT_MODE_CONTROL, "pantilt_mode"},
	{EU_HDR_MODE_CONTROL, "hdr_mode"},
	{7, "null"},
	{8, "null"},
	{9, "null"},
	{10, "null"},
	{UVC_ZOOM_ATTR_CONTROL, "zoom"},
	{12, "null"},
};

/*
 * this is uvc process uinit control
 **/
static struct Ucamera_Video_PU_Control backlight_compens = {
	.type = UVC_BACKLIGHT_COMPENSATION_CONTROL,
	.data[UVC_MIN] = 0,
	.data[UVC_MAX] = 10,
	.data[UVC_DEF] = 0,
	.data[UVC_CUR] = 0,
	.set = sample_video_backlight_compens_set,
	.get = sample_video_backlight_compens_get,
};

static struct Ucamera_Video_PU_Control brightness = {
	.type = UVC_BRIGHTNESS_CONTROL,
	.data[UVC_MIN] = 1,
	.data[UVC_MAX] = 255,
	.data[UVC_DEF] = 128,
	.data[UVC_CUR] = 128,
	.set = sample_video_brightness_set,
	.get = sample_video_brightness_get,
};

static struct Ucamera_Video_PU_Control gain = {
	.type = UVC_GAIN_CONTROL,
	.data[UVC_MIN] = 1,
	.data[UVC_MAX] = 10,
	.data[UVC_DEF] = 1,
	.data[UVC_CUR] = 1,
	.set = sample_video_gain_set,
	.get = sample_video_gain_get,
};

static struct Ucamera_Video_PU_Control contrast = {
	.type = UVC_CONTRAST_CONTROL,
	.data[UVC_MIN] = 1,
	.data[UVC_MAX] = 200,
	.data[UVC_DEF] = 128,
	.data[UVC_CUR] = 128,
	.set = sample_video_contrast_set,
	.get = sample_video_contrast_get,
};

static struct Ucamera_Video_PU_Control saturation = {
	.type = UVC_SATURATION_CONTROL,
	.data[UVC_MIN] = 1,
	.data[UVC_MAX] = 255,
	.data[UVC_DEF] = 128,
	.data[UVC_CUR] = 128,
	.set = sample_video_saturation_set,
	.get = sample_video_saturation_get,
};

static struct Ucamera_Video_PU_Control sharpness = {
	.type = UVC_SHARPNESS_CONTROL,
	.data[UVC_MIN] = 1,
	.data[UVC_MAX] = 200,
	.data[UVC_DEF] = 128,
	.data[UVC_CUR] = 128,
	.set = sample_video_sharpness_set,
	.get = sample_video_sharpness_get,
};

static struct Ucamera_Video_PU_Control hue = {
	.type = UVC_HUE_CONTROL,
	.data[UVC_MIN] = 0,
	.data[UVC_MAX] = 255,
	.data[UVC_DEF] = 128,
	.data[UVC_CUR] = 128,
	.set = sample_video_hue_set,
	.get = sample_video_hue_get,
};

static struct Ucamera_Video_PU_Control gamma_pu = {
	.type = UVC_GAMMA_CONTROL,
	.data[UVC_MIN] = 0,
	.data[UVC_MAX] = 200,
	.data[UVC_DEF] = 100,
	.data[UVC_CUR] = 100,
	.set = sample_video_gamma_set,
	.get = sample_video_gamma_get,
};

static struct Ucamera_Video_PU_Control whitebalance = {
	.type = UVC_WHITE_BALANCE_TEMPERATURE_CONTROL,
	.data[UVC_MIN] = 2300,
	.data[UVC_MAX] = 6500,
	.data[UVC_DEF] = 4500,
	.data[UVC_CUR] = 4500,
	.set = sample_video_whitebalance_set,
	.get = sample_video_whitebalance_get,
};

static struct Ucamera_Video_PU_Control whitebalance_auto = {
	.type = UVC_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
	.data[UVC_MIN] = 0,
	.data[UVC_MAX] = 1,
	.data[UVC_DEF] = 1,
	.data[UVC_CUR] = 1,
	.set = sample_video_whitebalance_set_auto,
	.get = sample_video_whitebalance_get_auto,
};

static struct Ucamera_Video_PU_Control powerlinefreq = {
	.type = UVC_POWER_LINE_FREQUENCY_CONTROL,
	.data[UVC_MIN] = 0,
	.data[UVC_MAX] = 2,
	.data[UVC_DEF] = 0,
	.data[UVC_CUR] = 0,
	.set = sample_video_powerlinefreq_set,
	.get = sample_video_powerlinefreq_get,
};

static struct Ucamera_Video_PU_Control *Pu_ctrl[] = {
	&backlight_compens,
	&brightness,
	&contrast,
	&gain,
	&hue,
	&saturation,
	&sharpness,
	&gamma_pu,
	&whitebalance,
	&whitebalance_auto,
	&powerlinefreq,
	NULL,
};

/*
 * this is Camera Terminal
 */
static struct Ucamera_Video_CT_Control exposure_time = {
	.type = UVC_EXPOSURE_TIME_CONTROL,
	.data[UVC_MIN] = { 0x01 },
	.data[UVC_MAX] = { 0xE8, 0x03}, /* 1000 */
	.data[UVC_DEF] = { 0x27 },
	.data[UVC_CUR] = { 0x27 },
	.set = sample_video_exposure_time_abs_set,
	.get = sample_video_exposure_time_abs_get,
};

static struct Ucamera_Video_CT_Control auto_exposure_mode = {
	.type = UVC_AUTO_EXPOSURE_MODE_CONTROL,
	.data[UVC_MIN] = { 0x1 },
	.data[UVC_MAX] = { 0xf },
	.data[UVC_DEF] = { 0x8 },
	.data[UVC_CUR] = { 0x8 },
	.set = sample_video_ae_mode_set,
	.get = sample_video_ae_mode_get,
};

static struct Ucamera_Video_CT_Control focus = {
	.type = UVC_FOCUS_CONTROL,
	.data[UVC_MIN] = { 0x5E, 0x01},		/* 350 */
	.data[UVC_MAX] = { 0xBC, 0x02},		/* 700 */
	.data[UVC_DEF] = { 0xF4, 0x01},
	.data[UVC_CUR] = { 0xF4, 0x01},		/* 500 */
	.set = sample_video_focus_abs_set,
	.get = sample_video_focus_abs_get,
};

static struct Ucamera_Video_CT_Control focus_auto = {
	.type = UVC_FOCUS_AUTO_CONTROL,
	.data[UVC_MAX] = { 0x1 },
	.data[UVC_MIN] = { 0x0 },
	.data[UVC_DEF] = { 0x1 },
	.data[UVC_CUR] = { 0x1 },
	.set = sample_video_focus_auto_set,
	.get = sample_video_focus_auto_get,
};

static struct Ucamera_Video_CT_Control zoom = {
	.type = UVC_ZOOM_ABSOLUTE_CONTROL,
	.data[UVC_MAX] = { 0x82 },
	.data[UVC_MIN] = { 0x64 },
	.data[UVC_DEF] = { 0x64 },
	.data[UVC_CUR] = { 0x64 },
	.set = sample_video_zoom_abs_set,
	.get = sample_video_zoom_get,
};

int htoi(char s[])
{
	int n = 0;
	int i = 0;
	while (s[i] != '\0' && s[i] != '\n') {
		if (s[i] == '0') {
			if (s[i+1] == 'x' || s[i+1] == 'X')
				i+=2;
		}
		if (s[i] >= '0' && s[i] <= '9') {
			n = n * 16 + (s[i] - '0');
		} else if (s[i] >= 'a' && s[i] <= 'f') {
			n = n * 16 + (s[i] - 'a') + 10;
		} else if (s[i] >= 'A' && s[i] <= 'F') {
			n = n * 16 + (s[i] - 'A') + 10;
		} else
			return -1;
		++i;
	}

	return n;
}

/*
 * TOP : 00 00
 * LEFT: 00 00
 * BOTTOM: 0x67 0x01  (0x167: 360 -1) 437
 * RIGHT:  0x7f 0x0x2 (0x27f: 640 -1) 77f
 * bm: 0x01, 0x00 (auto_exposure)
 */

static struct Ucamera_Video_CT_Control face_roi = {
	.type = UVC_REGION_OF_INTEREST_CONTROL,
	.data[UVC_MAX] = {0x00, 0x00, 0x00, 0x00, 0x67, 0x01, 0x7F, 0x02, 0x01, 0x00},
	.data[UVC_DEF] = {0x00, 0x00, 0x00, 0x00, 0x67, 0x01, 0x7F, 0x02, 0x01, 0x00},
	.data[UVC_CUR] = {0x00, 0x00, 0x00, 0x00, 0x67, 0x01, 0x7F, 0x02, 0x01, 0x00},
	.set = sample_video_roi_set,
	.get = sample_video_roi_get,
};

static struct Ucamera_Video_CT_Control pantilt = {
	.type = UVC_PANTILT_ABSOLUTE_CONTROL,
	.data[UVC_MAX] = {0xa0, 0x8c, 0x00, 0x00, 0xa0, 0x8c, 0x00, 0x00},
	.data[UVC_MIN] = {0x60, 0x73, 0xff, 0xff, 0x60, 0x73, 0xff, 0xff},
	.data[UVC_RES] = {0x10, 0x0e, 0x00, 0x00, 0x10, 0x0e, 0x00, 0x00},
	.data[UVC_DEF] = {0},
	.data[UVC_CUR] = {0},
	.set = sample_video_pantilt_abs_set,
	.get = sample_video_pantilt_abs_get,
};

static struct Ucamera_Video_CT_Control roll = {
	.type = UVC_ROLL_ABSOLUTE_CONTROL,
	.data[UVC_MAX] = {0x03},
	.data[UVC_MIN] = {0x00},
	.data[UVC_RES] = {0x01},
	.data[UVC_DEF] = {0},
	.data[UVC_CUR] = {0},
	.set = sample_video_roll_abs_set,
	.get = sample_video_roll_abs_get,
};

static struct Ucamera_Video_CT_Control *Ct_ctrl[] = {
	&exposure_time,
	&auto_exposure_mode,
	&focus,
	&focus_auto,
	&zoom,
	&face_roi,
	&pantilt,
	&roll,
	NULL,
};

void *module_ucamera_getCtHandler(void)
{
	return (void*)Ct_ctrl;
}
void *module_ucamera_getPuHandler(void)
{
	return (void*)(Pu_ctrl);
}

void module_ucamera_setCtHandler(void *ct)
{
	memcpy(Ct_ctrl, ct, 9 * sizeof(struct Ucamera_Video_CT_Control*));
}
void module_ucamera_setPuHandler(void *pu)
{
	memcpy(Pu_ctrl, pu, 9 * sizeof(struct Ucamera_Video_PU_Control*));
	/*printf("Pu_ctrl[1] = %p \n", Pu_ctrl[1]);*/
	/*printf("Pu_ctrl[1].data[UVC_CUR] = %d\n",Pu_ctrl[1]->data[UVC_CUR] );*/
}

static struct Ucamera_Video_EU_Control euctl;

int ucamera_uvc_pu_attr_save(int video_index, int type, int value);
/*
 * this is proces unit interface
 **/
static int sample_video_backlight_compens_set(int video_index, int value)
{
	return 0;
}

static int sample_video_backlight_compens_get(int video_index)
{
	//printf("backlight_compens.data[UVC_CUR] = %d \n", backlight_compens.data[UVC_CUR]);
	return backlight_compens.data[UVC_CUR];
}

static int sample_video_brightness_set(int video_index, int value)
{
	return 0;
}

static int sample_video_brightness_get(int video_index)
{
	return brightness.data[UVC_CUR];
}

static int sample_video_gain_set(int video_index, int value)
{
	return 0;
}

static int sample_video_gain_get(int video_index)
{
	//printf("gain.data[UVC_CUR] = %d \n", gain.data[UVC_CUR]);
	return gain.data[UVC_CUR];
}

static int sample_video_contrast_set(int video_index, int value)
{
	return 0;
}

static int sample_video_contrast_get(int video_index)
{
	return contrast.data[UVC_CUR];
}

static int sample_video_powerlinefreq_set(int video_index, int value)
{
	return 0;
}

static int sample_video_powerlinefreq_get(int video_index)
{
	return 0;
}

static int sample_video_hue_set(int video_index, int value)
{
	return 0;
}

static int sample_video_hue_get(int video_index)
{
	return hue.data[UVC_CUR];
}

static int sample_video_saturation_set(int video_index, int value)
{
	return 0;
}

static int sample_video_saturation_get(int video_index)
{
	return saturation.data[UVC_CUR];
}

static int sample_video_sharpness_set(int video_index, int value)
{
	return 0;
}

static int sample_video_sharpness_get(int video_index)
{
	return sharpness.data[UVC_CUR];
}

static int sample_video_gamma_set(int video_index, int value)
{
	return 0;
}

static int sample_video_gamma_get(int video_index)
{
	//printf("gamma_pu.data[UVC_CUR] = %d \n", gamma_pu.data[UVC_CUR]);
	return gamma_pu.data[UVC_CUR];
}

/*
 * value高16位代表模式（0:自动/ 1:手动
 * value 底16位为白平衡实际值
 */
static int sample_video_whitebalance_set(int video_index, int value)
{
	return 0;
}

static int sample_video_whitebalance_get(int video_index)
{
	return whitebalance.data[UVC_CUR];
}

static int sample_video_whitebalance_set_auto(int video_index, int value)
{
	return 0;
}


static int sample_video_whitebalance_get_auto(int video_index)
{
	return whitebalance_auto.data[UVC_CUR];
}
/**
 * this is Camera Terminal interface
 */

static int sample_video_focus_abs_set(int video_index, char* data, int len)
{
	return 0;
}

static void* sample_video_focus_abs_get(int video_index)
{
	printf("foucs.data[UVC_CUR] = %d \n", focus.data[UVC_CUR][0]);
	return focus.data[UVC_CUR];
}

static int sample_video_focus_auto_set(int video_index, char* data, int len)
{
	return 0;
}

static void* sample_video_focus_auto_get(int video_index)
{
	printf("foucs_auto.data[UVC_CUR] = %d \n", focus_auto.data[UVC_CUR][0]);
	return focus_auto.data[UVC_CUR];
}

static int sample_video_zoom_abs_set(int video_index, char* data, int length)
{
	return 0;
}

static void* sample_video_zoom_get(int video_index)
{
	printf("zoom.data[UVC_CUR] = %d \n", zoom.data[UVC_CUR][0]);
	return zoom.data[UVC_CUR];
}

static int sample_video_pantilt_abs_set(int video_index, char* data, int length)
{
	return 0;
}

static void* sample_video_pantilt_abs_get(int video_index)
{
	return pantilt.data[UVC_CUR];
}

static int sample_video_roll_abs_set(int video_index, char* data, int length)
{
	return 0;
}

static void* sample_video_roll_abs_get(int video_index)
{
	printf("roll.data[UVC_CUR] = %d \n", roll.data[UVC_CUR][0]);
	return roll.data[UVC_CUR];
}

static int sample_video_roi_set(int video_index, char* data, int length)
{
	return 0;
}

static void* sample_video_roi_get(int video_index)
{
	return face_roi.data[UVC_CUR];
}

static int sample_video_exposure_time_abs_set(int video_index, char* data, int len)
{
	return 0;
}

static void* sample_video_exposure_time_abs_get(int video_index)
{

	printf("%s: exposure_time.data[UVC_CUR] %d \n", __func__,exposure_time.data[UVC_CUR][0]);
	return exposure_time.data[UVC_CUR];
}

static int sample_video_ae_mode_set(int video_index, char* data, int length)
{
	return 0;
}

static void* sample_video_ae_mode_get(int video_index)
{
	printf("%s: auto_exposure_mode.data[UVC_CUR] = %d \n", __func__, auto_exposure_mode.data[UVC_CUR][0]);
	return auto_exposure_mode.data[UVC_CUR];
}

/*
 * 扩展单元严格依赖于 客户协议，故从corelib 提出来
 * 根据客户协议填充(MAX/MIN/LEN/CUR)，并对特定通道解析
 * 两个摄像头扩展单元单独剥离开(video 1、 viedo2)
 * 默认每个摄像头支持两个扩展单元，每个扩展单元32通道(winhello 24通道)
 * 扩展单元解析从uvc_control 拆除
 *
 */
static int sample_video_eu_set(int video_index, int cmd, void *data, int len)
{
	/* register callback */
	if (data == NULL) {
		printf("ERROR(%s,%s): Pointer is null pointer!\n", MODULE_TAG, __func__);
		return -1;
	}

	switch(video_index) {
	case 0:
		return sample_video1_eu_set(cmd, data, len);
	case 1:
		return sample_video2_eu_set(cmd, data, len);
	default:
		break;
	}

	return 0;

}

static int sample_video_eu_get(int video_index, int cmd, int req, void *data)
{
	/* register callback */
	if (data == NULL) {
		printf("ERROR(%s,%s): Pointer is null pointer!\n", MODULE_TAG, __func__);
		return -1;
	}

	switch(video_index) {
	case 0:
		return sample_video1_eu_get(cmd, req, data);
	case 1:
		return sample_video2_eu_get(cmd, req, data);
	default:
		break;
	}

	return 0;
}

static int (*GF_GetMjpegStream)(int video_index, char *stream_data) = NULL;

void register_Mjpegvframe_get_func(int (*func)(int video_index, char *stream_data))
{
	GF_GetMjpegStream = func;
}

static int (*GF_GetH264Stream)(int video_index, char *stream_data) = NULL;
void register_H264vframe_get_func(int (*func)(int video_index, char *stream_data))
{
	GF_GetH264Stream = func;
}

static int (*GF_GetNV12Stream)(int video_index, char *stream_data) = NULL;
void register_NV12vframe_get_func(int (*func)(int video_index, char *stream_data))
{
	GF_GetNV12Stream = func;
}

static int (*GF_UvcVideoParamGet)(struct uvc_video_params *video_params) = NULL;
void register_UvcVideoParam_get_func(int (*func)(struct uvc_video_params *video_params))
{
	GF_UvcVideoParamGet = func;
}

static int get_jpeg_snap(int video_index, char *img_data)
{
	return  GF_GetMjpegStream(video_index, img_data);
}

static int get_yuv_snap(int video_index, char *img_data)
{
	return GF_GetNV12Stream(video_index, img_data);
}

static int get_h264_snap(int video_index, char *img_data)
{
	return GF_GetH264Stream(video_index, img_data);
}

const char *uvc_pu_type_to_string(int type)
{
	return pu_string[type].s;
}

int uvc_pu_string_to_type(char *string)
{
	int i, index;
	index = sizeof(pu_string)/sizeof(struct uvc_pu_string);

	if (string == NULL)
		return -1;

	for (i = 0; i < index; i++) {
		if (strcmp(string, pu_string[i].s) == 0)
			return pu_string[i].id;
	}
	return 0;
}

const char *uvc_ct_type_to_string(int type)
{
	return ct_string[type].s;
}

int uvc_ct_string_to_type(char *string)
{
	int i, index;
	index = sizeof(ct_string)/sizeof(struct uvc_ct_string);

	if (string == NULL)
		return -1;

	for (i = 0; i < index; i++) {
		if (strcmp(string, ct_string[i].s) == 0)
			return ct_string[i].id;
	}
	return 0;
}

int uvc_pu_attr_setcur(int type, int value)
{
	int ret = 0;
	struct Ucamera_Video_PU_Control *pu_attr = NULL;

	switch (type) {
	case UVC_BACKLIGHT_COMPENSATION_CONTROL:
		pu_attr = &backlight_compens;
		break;
	case UVC_BRIGHTNESS_CONTROL:
		pu_attr = &brightness;
		break;
	case UVC_CONTRAST_CONTROL:
		pu_attr = &contrast;
		break;
	case UVC_GAIN_CONTROL:
		pu_attr = &gain;
		break;
	case UVC_POWER_LINE_FREQUENCY_CONTROL:
		pu_attr = &powerlinefreq;
		break;
	case UVC_HUE_CONTROL:
		pu_attr = &hue;
		break;
	case UVC_SATURATION_CONTROL:
		pu_attr = &saturation;
		break;
	case UVC_SHARPNESS_CONTROL:
		pu_attr = &sharpness;
		break;
	case UVC_GAMMA_CONTROL:
		pu_attr = &gamma_pu;
		break;
	case UVC_WHITE_BALANCE_TEMPERATURE_CONTROL:
		pu_attr = &whitebalance;
		break;
	case UVC_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
		break;
	default:
		printf("Unkown uvc pu type:%d\n", type);
		ret = -1;
		break;
	}
	if (pu_attr)
		pu_attr->data[UVC_CUR] = value;
	return ret;
}

int uvc_ct_attr_setcur(int type, int value)
{
	int ret = 0;
	struct Ucamera_Video_CT_Control *ct_attr = NULL;

	switch (type) {
	case UVC_ZOOM_ATTR_CONTROL:
		ct_attr = &zoom;
		if (ct_attr) {
			memcpy(&ct_attr->data[UVC_CUR][0], &value, 4);
			/*printf("### zomm = %d\n", ct_attr->data[UVC_CUR][0]);*/
		}
		break;
	case UVC_PAN_ATTR_CONTROL:
		ct_attr = &pantilt;
		if (ct_attr) {
			memcpy(&ct_attr->data[UVC_CUR][0], &value, 4);
		}
		break;
	case UVC_TILT_ATTR_CONTROL:
		ct_attr = &pantilt;
		if (ct_attr) {
			memcpy(&ct_attr->data[UVC_CUR][4], &value, 4);
		}
		break;
	default:
		Ucamera_DEBUG("Unkown uvc ct type:%d\n", type);
		ret = -1;
		break;
	}

	return ret;
}

int ucamera_uvc_pu_attr_save(int video_index, int type, int value)
{
	char key[64] = {0};
	char data[16] = {0};
	char line_str[128] = {0};
	const char *attr_string = NULL;

	if (uvc_ctx[video_index].uvc_attr_fd == NULL) {
		printf("%s can not open uvc config file!\n", __func__);
		return -1;
	}
	attr_string = uvc_pu_type_to_string(type);
	if (attr_string == NULL)
		return -1;

	pthread_mutex_lock(&uvc_ctx[video_index].uvc_attr_mutex);
	fseek(uvc_ctx[video_index].uvc_attr_fd, 0L, SEEK_SET);

	while (!feof(uvc_ctx[video_index].uvc_attr_fd)) {
		if (fscanf(uvc_ctx[video_index].uvc_attr_fd, "%[^\n]", line_str) < 0)
			break;

		if (sscanf(line_str, "%[^:]:%[^\n]", key, data) != 2) {
			printf("warning: Invalid param:%s\n", line_str);
			fseek(uvc_ctx[video_index].uvc_attr_fd , 1L, SEEK_CUR);
			continue;
		}
		if (strcmp(key, attr_string) == 0) {
			fseek(uvc_ctx[video_index].uvc_attr_fd, -strlen(line_str), SEEK_CUR);
			fprintf(uvc_ctx[video_index].uvc_attr_fd, "%s:%04d", key, value);
		}
		fseek(uvc_ctx[video_index].uvc_attr_fd , 1L, SEEK_CUR);
	}

	pthread_mutex_unlock(&uvc_ctx[video_index].uvc_attr_mutex);
	return 0;
}

static int ucamera_uvc_attr_load(int video_index)
{
	char key[64] = {0};
	char value[16] = {0};
	char line_str[128] = {0};
	int type;
	char *uvc_attr_file = ((video_index < 1) ? INLOCK_ATTR_FILE_PATH : INLOCK2_ATTR_FILE_PATH);

	if ((uvc_ctx[video_index].uvc_attr_fd = fopen(uvc_attr_file, "r+")) == NULL) {
		printf("%s open config file failed!\n", __func__);
		return -1;
	}

	if (pthread_mutex_init(&uvc_ctx[video_index].uvc_attr_mutex, NULL) != 0){
		printf("%s init mutex failed\n", __func__);
		return -1;
	}

	while (!feof(uvc_ctx[video_index].uvc_attr_fd)) {
		if (fscanf(uvc_ctx[video_index].uvc_attr_fd, "%[^\n]", line_str) < 0)
			break;

		if (sscanf(line_str, "%[^:]:%[^\n]", key, value) != 2) {
			printf("warning: skip config %s\n", line_str);
			fseek(uvc_ctx[video_index].uvc_attr_fd , 1L, SEEK_CUR);
			continue;
		}
		/*printf("%s : %d\n", key, atoi(value)); */
		if ((type = uvc_pu_string_to_type(key))) {
			uvc_pu_attr_setcur(type, atoi(value));
		}
		if ((type = uvc_ct_string_to_type(key))) {
			uvc_ct_attr_setcur(type, htoi(value));
		}
		fseek(uvc_ctx[video_index].uvc_attr_fd , 1L, SEEK_CUR);
	}

	return 0;
}

int ucamera_uvc_ct_attr_save(int video_index, int type, int value)
{
	char key[64] = {0};
	char data[16] = {0};
	char line_str[128] = {0};
	const char *attr_string = NULL;

	FILE *uvc_attr_fd = uvc_ctx[video_index].uvc_attr_fd;
	if (uvc_attr_fd == NULL) {
		Ucamera_DEBUG("%s can not open uvc config file!\n", __func__);
		return -1;
	}
	attr_string = uvc_ct_type_to_string(type);
	if (attr_string == NULL)
		return -1;

	pthread_mutex_lock(&uvc_ctx[video_index].uvc_attr_mutex);
	fseek(uvc_attr_fd, 0L, SEEK_SET);

	while (!feof(uvc_attr_fd)) {
		if (fscanf(uvc_attr_fd, "%[^\n]", line_str) < 0)
			break;

		if (sscanf(line_str, "%[^:]:%[^\n]", key, data) != 2) {
			Ucamera_DEBUG("warning: Invalid param:%s\n", line_str);
			fseek(uvc_attr_fd , 1L, SEEK_CUR);
			continue;
		}
		if (strcmp(key, attr_string) == 0) {
			fseek(uvc_attr_fd, -strlen(line_str), SEEK_CUR);
			fprintf(uvc_attr_fd, "%s:%08x", key, value);
		}
		fseek(uvc_attr_fd , 1L, SEEK_CUR);
	}

	pthread_mutex_unlock(&uvc_ctx[video_index].uvc_attr_mutex);
	return 0;
}

static int uvc_event_process(int video_index, int event_id, void *data)
{
	uint32_t intervals = 0;
	int ret = -1;
	struct uvc_video_params video_params;
	if (data == NULL) {
		printf("ERROR(%s,%s): Pointer is null pointer!\n", MODULE_TAG, __func__);
		return -1;
	}
	/*IMPVI_NUM sensor = ((!DUAL_SENSOR || (video_index < 1)) ? IMPVI_MAIN : IMPVI_SEC);*/
	switch (event_id) {
	case UCAMERA_EVENT_STREAMON: {
					struct Ucamera_Video_Frame *frame = (struct Ucamera_Video_Frame *) data;
					intervals = frame->intervals;
					uvc_ctx[video_index].sensor_fps = 10000000 / intervals;
					uvc_ctx[video_index].uvc_width = frame->width;
					uvc_ctx[video_index].uvc_height = frame->height;
					uvc_ctx[video_index].stream_on = 1;
#if SAVE_FOR_POWERDOWN
					/*sample_video_backlight_compens_set(video_index, backlight_compens.data[UVC_CUR]);*/
					/*sample_video_powerlinefreq_set(video_index, powerlinefreq.data[UVC_CUR]);*/
					/*sample_video_hue_set(video_index, hue.data[UVC_CUR]);*/
					/*sample_video_gamma_set(video_index, gamma_pu.data[UVC_CUR]);*/
					/*sample_video_whitebalance_set(video_index, whitebalance.data[UVC_CUR]);*/

					sample_video_brightness_set(video_index, brightness.data[UVC_CUR]);
					sample_video_contrast_set(video_index, contrast.data[UVC_CUR]);
					sample_video_saturation_set(video_index, saturation.data[UVC_CUR]);
					sample_video_sharpness_set(video_index, sharpness.data[UVC_CUR]);
#endif
					// if((DUAL_SENSOR == 0 && video_index == 0) || DUAL_SENSOR == 1) {
					// 	ret = sample_sensor_setfps(video_index, uvc_ctx[video_index].sensor_fps, 1);
					// 	if( ret < 0 ) {
					// 		return ret;
					// 	}
					// }

					// ret = imp_sdk_init(video_index, frame->fcc, frame->width, frame->height);
					if(!ret){
#if 0
						sample_video_zoom_abs_set(video_index, (char*)zoom.data[UVC_CUR], 1);
						sample_video_pantilt_abs_set(video_index, (char*)pantilt.data[UVC_CUR], 1);
#endif
						uvc_ctx[video_index].stream_on = 1;
					}

					/* get actual v4l2 buff size used */
					v4l2_buff_size[video_index] = IF_Ucam_V4l2_Get_BuffSize(video_index);
					// 通知上层应用
					if (GF_UvcVideoParamGet != NULL) {
						video_params.video_onoff = uvc_ctx[video_index].stream_on;
						video_params.video_index = video_index;
						video_params.fcc = frame->fcc;
						video_params.width = frame->width;
						video_params.height = frame->height;
						ret = GF_UvcVideoParamGet(&video_params);
						if (ret < 0) {
							printf("ERROR(%s,%s): GF_UvcVideoParamGet failed!\n", MODULE_TAG, __func__);
							return ret;
						}
					}
					printf("v4l2_buff_size[%d] is %d\n", video_index, v4l2_buff_size[video_index]);
					return ret;
				}
	case UCAMERA_EVENT_STREAMOFF: {
					struct Ucamera_Video_Frame *frame = (struct Ucamera_Video_Frame *) data;
					uvc_ctx[video_index].stream_on = 0;
					if (GF_UvcVideoParamGet != NULL) {
						video_params.video_onoff = uvc_ctx[video_index].stream_on;
						video_params.video_index = video_index;
						video_params.fcc = frame->fcc;
						video_params.width = frame->width;
						video_params.height = frame->height;
						ret = GF_UvcVideoParamGet(&video_params);
						if (ret < 0) {
							printf("ERROR(%s,%s): GF_UvcVideoParamGet failed!\n", MODULE_TAG, __func__);
							return ret;
						}
					}

					// return imp_sdk_deinit(video_index, frame->fcc);
					return ret;
				}

	default:
				printf("%s(ERROR): unknown message ->[%d]!\n", MODULE_TAG, event_id);
				return -1;
	};

	return 0;
}

int sample_get_ucamera_streamonoff(int video_index)
{
	return uvc_ctx[video_index].stream_on;
}

int sample_get_ucamera_af_onoff()
{
	return focus_auto.data[UVC_CUR][0];
}

int sample_get_ucamera_af_cur()
{
	/* focus电机步长介于100-500之间，需要占用两个字节空间 */
	int value = focus.data[UVC_CUR][0] + (focus.data[UVC_CUR][1] << 8);

	return value;
}

void sample_set_ucamera_af_cur(int value)
{
	focus.data[UVC_CUR][0] = value & 0x00FF;
	focus.data[UVC_CUR][1] = value >> 8;
}

void register_video_callback(void)
{
	v_func.get_YuvFrame = get_yuv_snap;
	v_func.get_JpegFrame = get_jpeg_snap;
	v_func.get_H264Frame = get_h264_snap;
	IF_Ucam_Video_VfCtl(&v_func, 0);

#if (VIDEO_NUM == 2)
	IF_Ucam_Video_VfCtl(&v_func, 1);
#endif
}

void register_pu_callback(void)
{
	ucamera_uvc_attr_load(0);

	IF_Ucam_Video_Puctl(Pu_ctrl, 0);

#if (VIDEO_NUM == 2)
	ucamera_uvc_attr_load(1);
	IF_Ucam_Video_Puctl(Pu_ctrl, 1);
#endif
}

void register_ct_callback(void)
{
	IF_Ucam_Video_Ctctl(Ct_ctrl, 0);

#if (VIDEO_NUM == 2)
	IF_Ucam_Video_Ctctl(Ct_ctrl, 1);
#endif
}

void register_eu_callback(void)
{
	euctl.set = sample_video_eu_set;
	euctl.get = sample_video_eu_get;
	IF_Ucam_Video_Euctl(&euctl, 0);

#if (VIDEO_NUM == 2)
	IF_Ucam_Video_Euctl(&euctl, 1);
#endif
}
void register_eventprocess_callback(void)
{
	IF_Ucam_Video_EventProcess(uvc_event_process, 0);

#if (VIDEO_NUM == 2)
	IF_Ucam_Video_EventProcess(uvc_event_process, 1);
#endif
}

void uvc_control_enable_impinited(int video_index)
{
	uvc_ctx[video_index].imp_inited = 1;
}

int uvc_control_get_stream_status(int video_index)
{
	return uvc_ctx[video_index].stream_on;
}

void uvc_control_get_frame(uint32_t *frame_width,uint32_t *frame_height)
{
	if (frame_width == NULL || frame_height == NULL) {
		printf("ERROR(%s,%s): Pointer is null pointer!\n", MODULE_TAG, __func__);
		return;
	}
	*frame_width = uvc_ctx[0].uvc_width;
	*frame_height = uvc_ctx[0].uvc_height;
}

int uvc_control_init(void *param)
{
	int ret;
	if (param == NULL) {
		printf("ERROR(%s,%s): Pointer is null pointer!\n", MODULE_TAG, __func__);
		return -1;
	}
	config_func_param_t *cfg_param = (config_func_param_t*)param;
	imp_attr_t *imp_attr = &cfg_param->imp_attr;

	/* uvc context initialize */
	uvc_ctx[0].stream_on = 0;
	uvc_ctx[0].hvflip = imp_attr->hvflip;
	uvc_ctx[0].sensor_fps = imp_attr->sensor_info[0].sensor_fps;
	uvc_ctx[0].sensor_width = imp_attr->sensor_info[0].sensor_width;
	uvc_ctx[0].sensor_height = imp_attr->sensor_info[0].sensor_height;

	/* uvc init */
	/* Step first */
	ret = IF_Ucam_Video_Init(VIDEO_DEVICE_NAME);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Video_Init failed!\n", MODULE_TAG);
		return -1;
	}

#if (VIDEO_NUM == 2)
#if (DUAL_SENSOR == 1)
	uvc_ctx[1].stream_on = 0;
	uvc_ctx[1].hvflip = imp_attr->hvflip;
	uvc_ctx[1].sensor_fps = imp_attr->sensor_info[1].sensor_fps;
	uvc_ctx[1].sensor_width = imp_attr->sensor_info[1].sensor_width;
	uvc_ctx[1].sensor_height = imp_attr->sensor_info[1].sensor_height;
#endif
	uvc_ctx[1].hvflip = imp_attr->hvflip;
	ret = IF_Ucam_Video_Init(VIDEO2_DEVICE_NAME);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Video_Init failed!\n", MODULE_TAG);
		return -1;
	}
	printf("-------- video 2 init ok!\n");
#endif

	register_video_callback();
	register_ct_callback();
	register_pu_callback();
	register_eu_callback();
	register_eventprocess_callback();

	/* Step last */
	ret = IF_Ucam_Video_Start();
	if (ret < 0)
		printf("ERROR(%s): IF_UCAM_Video_Start failed!\n", MODULE_TAG);
	return 0;
}

void uvc_control_deinit()
{
	IF_Ucam_Video_Stop();
	IF_Ucam_Video_DeInit(0);
#if (VIDEO_NUM == 2)
	IF_Ucam_Video_DeInit(1);
#endif
}