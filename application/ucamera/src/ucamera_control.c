#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uvc_control.h"
#include "uac_control.h"

#include "ucamera_control.h"
#include "inlock_param_init.h"
#include "koks_project_def.h"

#define TAG "module_ucamera"

/*=============================
 * ucamera system init
 *============================*/
int ucamera_system_init(config_func_param_t *cfg_param)
{
	int ret;

	/* Step first */
	ret = IF_Ucam_Init();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Init failed!\n", TAG);
		return -1;
	}

	/* Step second */
	ret = IF_Ucam_Device_Config(cfg_param->dev_info);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Device_Config failed!\n", TAG);
		return -1;
	}

#if 0
	/* Set uvc fps default */
	ret = IF_Ucam_Video_Fps_Default(20, 0);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Video_Fps_Default failed!\n", TAG);
		return -1;
	}
#endif

#ifndef KOKS_FEATURE_USB_ISO
	/* Set uvc bulk transfer */
	ret = IF_Ucam_Bulk_Mode();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Bulk_Mode failed!\n", TAG);
		return -1;
	}
#endif

#if 0
	/* Set uvc interrupt reduction level */
	ret = IF_Ucam_UVC_Interrupt_level(4);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_UVC_Interrupt_level failed!\n", TAG);
		return -1;
	}

	/* Set full speed support */
	ret = IF_Ucam_FS_Enable();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_FS_Enable failed!\n", TAG);
		return -1;
	}
#endif
	/* Steps ... */
	ret = IF_Ucam_Video_Config(0, cfg_param->video_info[0]);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Video_Config(0) failed!\n", TAG);
		return -1;
	}
#if (VIDEO_NUM == 2)
	ret = IF_Ucam_Video_Config(1, cfg_param->video_info[1]);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Video_Config(1) failed!\n", TAG);
		return -1;
	}
#endif

#ifdef MODULE_UAC
	ret = IF_Ucam_Audio_Config(cfg_param->audio_info);
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Audio_Config failed!\n", TAG);
		return -1;
	}
#endif

#ifdef MMSE_ENABLE
	IF_Ucam_Audio_Set_Channel(1);
#endif

	/* Set v4l2 info buff size */
	v4l2_info_t v4l2_info_test;
#ifndef UVC_BUF_SIZE
	v4l2_info_test.compress_factor = 0.3;
#else
	// v4l2_info_test.buff_size = UVC_BUF_SIZE;
	v4l2_info_test.buff_size = cfg_param->buff_size;
#endif

#if defined(KOKS_PROJECT_M53B)
	v4l2_info_test.max_width = 1440;
	v4l2_info_test.max_height = 2560;
#elif defined(KOKS_PROJECT_M55)
	v4l2_info_test.max_width = 2560;
	v4l2_info_test.max_height = 1440;
#elif defined(KOKS_PROJECT_M53) || defined(KOKS_PROJECT_M53A)
	v4l2_info_test.max_width = 1200;
	v4l2_info_test.max_height = 1600;
#else
	v4l2_info_test.max_width = 1600;
	v4l2_info_test.max_height = 1200;
#endif
	ret = IF_Ucam_V4l2_Set_Config(&v4l2_info_test);
	if (ret < 0) {
		printf("ERROR(%s): IF_UCAM_V4l2_Set_Config failed!\n", TAG);
		return -1;
	}

	/* Step last */
	ret = IF_Ucam_Enable();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Start failed!\n", TAG);
		return -1;
	}

	return 0;
}

int ucamera_system_deinit()
{
	int ret;

	ret = IF_Ucam_Disable();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_Stop failed!\n", TAG);
		return -1;
	}

	IF_Ucam_Audio_Stop();

	ret = IF_Ucam_DeInit();
	if (ret < 0) {
		printf("ERROR(%s): IF_Ucam_DeInit failed!\n", TAG);
		return -1;
	}

	return 0;

}

/*=============================
 * ucamera control init
 *============================*/
int ucamera_control_init(void* param)
{
	int ret;

	ret = uvc_control_init(param);
	if (ret < 0) {
		printf("ERROR(%s): uvc_control_init failed!\n", TAG);
		return -1;
	}

#ifdef MODULE_UAC
	ret = uac_control_init(param);
	if (ret < 0) {
		printf("ERROR(%s): uac_control_init failed!\n", TAG);
		return -1;
	}
#endif

	return 0;

}

int ucamera_control_deinit()
{
	uvc_control_deinit();

#ifdef MODULE_UAC
	uac_control_deinit();
#endif
	return 0;

}

/*=============================
 * module ucamera get stream status : ON or OFF
 *============================*/
int inlock_ucamera_stream_on(int stream_id)
{
	return uvc_control_get_stream_status(stream_id);
}

void inlock_ucamera_get_frame(uint32_t *frame_width, uint32_t *frame_height)
{
	uvc_control_get_frame(frame_width, frame_height);
}

/*=============================
 * module ucamera init
 * ucamera system and control init
 *============================*/
int inlock_ucamera_init(void *param)
{
	int ret = -1;
	config_func_param_t *cfg_param = (config_func_param_t *)param;

	ret = ucamera_system_init(cfg_param);
	if (ret < 0) {
		printf("ERROR(%s): ucamera_system_init failed!\n", TAG);
		return -1;
	}

	usleep(50 * 1000);

	ret = ucamera_control_init(param);
	if (ret < 0) {
		printf("ERROR(%s): ucamera_control_init failed!\n", TAG);
		return -1;
	}

	return 0;
}

void inlock_ucamera_deinit()
{
	ucamera_control_deinit();

	ucamera_system_deinit();
	printf("INFO(%s): inlock_ucamera_deinit ...ok \n", TAG);
}

pthread_t tid_emu;
const char *ucamera_dev = "/dev/ucamera";

static void *emu_pthread(void *arg)
{
	int ret;
	/* 判断ucamera节点是否存在 */
	while (access(ucamera_dev, F_OK))
	{
		printf(" ERROR[%s]: %s is not exist!!!\n", TAG, ucamera_dev);
		usleep(10 * 1000);
	}
	printf(" INFO[%s]: %s is exist\n", TAG, ucamera_dev);


	ret = inlock_ucamera_init(arg);
	if ( ret < 0 ) {
		printf( "ERROR(%s): module_ucamera_int failed!\n", TAG );
		return NULL;
	}
	printf(" INFO[%s]: module ucamera init ok!\n", TAG );
	return NULL;
}

void gf_ucamera_init(void *param)
{
	pthread_attr_t emu_attr;/* enumeration 枚举 */
	pthread_attr_init( &emu_attr );
	pthread_attr_setdetachstate( &emu_attr, PTHREAD_CREATE_DETACHED );
	pthread_attr_setschedpolicy( &emu_attr, SCHED_OTHER );
	pthread_create(&tid_emu, &emu_attr, &emu_pthread, param);
}

int uvc_camera_init_with_config_file(const char *file, unsigned int buff_size)
{
	int ret = inlock_param_init( (char*)file, &g_inlock_param_info);
	if ( ret < 0 ) {
		printf( "ERROR(%s): inlock_param_init failed!\n", TAG );
		return -1;
	}

	g_inlock_param_info.buff_size = buff_size;
	gf_ucamera_init(&g_inlock_param_info);

	return 0;
}

int uvc_camera_init(void)
{
	return uvc_camera_init_with_config_file(INLOCK_CONFIG_FILE_PATH, UVC_BUF_SIZE);
}

void uvc_camera_deinit()
{
	inlock_ucamera_deinit();
}