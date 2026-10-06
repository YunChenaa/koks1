#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/prctl.h>

// #include <imp/imp_audio.h>

#include "uac_control.h"
// #include <imp-audio.h>
// #include "audio_ctl.h"

#include "global_config.h"

#define CHECK_ERR(expr, fmt, ...)                                                  \
    {                                                                              \
        int __ret = (expr);                                                        \
        if (__ret != 0)                                                            \
        {                                                                          \
            printf("[CHECK_ERR]-[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
            return -1;                                                             \
        }                                                                          \
    }


struct Ucamera_Audio_CB_Func a_func;

static audio_info_t uac_ctx;

static int (*GF_UvcUsbConnectStatus)(int *status) = NULL;
static int (*GF_UacStatus)(int audio_index, int status) = NULL;
static bool record_status = false;
static bool speak_status = false;
static bool last_combined_status = false;  // 记录上次组合状态

void register_UvcUsbConnectStatus(int (*func)(int *status))
{
	GF_UvcUsbConnectStatus = func;
}

void register_UacStatus(int (*func)(int audio_index, int status))
{
	GF_UacStatus = func;
}

// 当record和speak状态同时变化时调用回调
static void check_and_notify_status_change()
{
    bool current_combined_status = record_status && speak_status;
    // 仅在两者同时为true或同时为false时触发回调
    if (current_combined_status != last_combined_status) {
        last_combined_status = current_combined_status;
        if (GF_UvcUsbConnectStatus != NULL) {
            int status = current_combined_status ? 1 : 0;
            GF_UvcUsbConnectStatus(&status);
        }
    }
}

static int inlock_set_mic_volume(int16_t db)
{
	return 0;
}

static int inlock_set_mic_mute(int mute)
{
	return 0;
}

static int inlock_set_mic_samplerate(uint32_t samplerate)
{
	return 0;
}

static int inlock_set_spk_volume(int16_t db)
{
	return 0;
}

static int inlock_set_spk_mute(int mute)
{
	return 0;
}

static int inlock_get_record_off()
{
	if(record_status) {
		record_status = false;
		printf("INFO(%s): ------RECORD  OFF------\n", MODULE_TAG);
		if (GF_UacStatus) {
			GF_UacStatus(0, 0);
		}
		check_and_notify_status_change();
	}
	return 0;
}

static int inlock_get_record_on()
{
	if(!record_status) {
		record_status = true;
		printf("INFO(%s): ------RECORD  ON------\n", MODULE_TAG);
		if (GF_UacStatus) {
			GF_UacStatus(0, 1);
		}
		check_and_notify_status_change();
	}
	return 0;
}

static int inlock_get_speak_off()
{
	if(speak_status) {
		speak_status = false;
		printf("INFO(%s): ------SPEAK  OFF------\n", MODULE_TAG);
		if (GF_UacStatus) {
			GF_UacStatus(1, 0);
		}
		check_and_notify_status_change();
	}
	return 0;
}

static int inlock_get_speak_on()
{
	if(!speak_status) {
		speak_status = true;
		printf("INFO(%s): ------SPEAK  ON------\n", MODULE_TAG);
		if (GF_UacStatus) {
			GF_UacStatus(1, 1);
		}
		check_and_notify_status_change();
	}
	return 0;
}

static pthread_t play_thread_id;
static int ao_play_stop = 0;

/* 
 * @brief 音频播放线程函数
 * 
 * @param argv 线程参数指针
 * @return void* 线程返回值指针
 */
// static void *_ao_test_play_thread(void *argv)
// {
// 	int ret = -1;
// 	int devID = 0;
// 	int chnID = 0;

// 	prctl(PR_SET_NAME, "audio_speak");

// 	struct Ucamera_Audio_Frame u_frame;

// 	while (!ao_play_stop) {
// 		ret = IF_Ucam_Audio_Get_Frame(&u_frame);
// 		if (ret || u_frame.len <= 0)
// 		{
// 			usleep(10000);
// 			continue;
// 		}

// 		/* 发送音频数据到音频播放设备 */
// 		IMPAudioFrame frm;
// 		frm.virAddr = (uint32_t *)u_frame.data;
// 		frm.len = u_frame.len;
// 		ret = IMP_AO_SendFrame(devID, chnID, &frm, BLOCK);
// 		if (ret != 0) {
// 			IMP_LOG_ERR(MODULE_TAG, "send Frame Data error\n");
// 			usleep(10000);
// 			continue;
// 		}

// 		IF_Ucam_Audio_Release_Frame(&u_frame);

// 	}

// 	return NULL;
// }

/* 
 * @brief 启动音频播放线程
 * 
 * @return int 0:成功 -1:失败
 */
// static int inlock_audio_play_start(void)
// {
// 	int ret = -1;

// 	ret = pthread_create(&play_thread_id, NULL, _ao_test_play_thread, NULL);
// 	CHECK_ERR(ret, "pthread_create Audio Record failed:%d\n", ret);
// 	return ret;
// }

/* 
 * @brief 停止音频播放线程
 * 
 * @return int 0:成功 -1:失败
 */
// static int inlock_audio_play_stop(void)
// {
// 	int ret = -1;
// 	ao_play_stop = 1;

// 	ret = pthread_join(play_thread_id, NULL);
// 	CHECK_ERR(ret, "pthread_join failed:%d\n", ret);

// 	return ret;
// }

/* 
 * @brief 初始化音频
 * 
 * @return int 0:成功 -1:失败
 */
// static int imp_audio_init(void)
// {
// 	int ret = -1;
// 	/*printf("INFO[%s]:  amic_en %d, spk_en %d!\n", MODULE_TAG, uac_ctx.amic_en, uac_ctx.spk_en);*/
// 	if(uac_ctx.amic_en && (uac_ctx.amic_info.samplerate_num == 1)) {
// 		ret = ai_init(uac_ctx.amic_info.samplerate_list[0]);
// 		CHECK_ERR(ret, "ai_init failed:%d\n", ret);
// 	}

// 	if(uac_ctx.spk_en) {
// 		ret = ao_init(uac_ctx.spk_info.samplerate_list[0]);
// 		CHECK_ERR(ret, "ao_init failed:%d\n", ret);
// 	}
// 	return ret;
// }

/* 
 * @brief 反初始化音频
 * 
 * @return int 0:成功 -1:失败
 */
// static int imp_audio_deinit()
// {
// 	int ret = -1;
// 	if (uac_ctx.amic_en) {
// 		ret = ai_deinit();
// 		CHECK_ERR(ret, "ai_deinit failed:%d\n", ret);
// 	}
	
// 	if(uac_ctx.spk_en) {
// 		ret = ao_deinit();
// 		CHECK_ERR(ret, "ao_deinit failed:%d\n", ret);
// 	}
// 	return ret;
// }

static int (*amic_pcm_get_func)(short *pcm) = NULL;
void register_amic_pcm_get_func(int (*func)(short *pcm))
{
	printf("INFO(%s): register_amic_pcm_get_func\n", MODULE_TAG);
	if (func == NULL) {
		printf("ERROR(%s): func is null pointer!\n", MODULE_TAG);
		return;
	}
	if (amic_pcm_get_func != NULL) {
		printf("ERROR(%s): func is already registered!\n", MODULE_TAG);
		return;
	}
	printf("INFO(%s): register_amic_pcm_get_func %p\n", MODULE_TAG, func);
	amic_pcm_get_func = func;
}

int inlock_audio_amic_pcm_get(short *pcm)
{
	if (amic_pcm_get_func == NULL) {
		printf("ERROR(%s): amic_pcm_get_func is null pointer!\n", MODULE_TAG);
		return -1;
	}
	return amic_pcm_get_func(pcm);
}

/* 
 * @brief 注册音频回调函数
 * 
 * @return int 0:成功 -1:失败
 */
static void register_audio_func(void)
{
	/* 注册音频取流函数 */
	a_func.get_AudioPcm = inlock_audio_amic_pcm_get;
	/* 注册音频音量控制函数 */
	a_func.set_Mic_Volume = inlock_set_mic_volume;
	a_func.set_Spk_Volume = inlock_set_spk_volume;
	/* 注册音频静音控制函数 */
	a_func.set_Mic_Mute = inlock_set_mic_mute;
	a_func.set_Spk_Mute = inlock_set_spk_mute;
	/* 注册音频采样率控制函数 */
	a_func.set_Mic_Samplerate = inlock_set_mic_samplerate;
	/* 注册音频采集控制函数 */
	a_func.get_record_on = inlock_get_record_on;
	a_func.get_record_off = inlock_get_record_off;
	/* 注册音频播放控制函数 */
	a_func.get_speak_on = inlock_get_speak_on;
	a_func.get_speak_off = inlock_get_speak_off;

	IF_Ucam_Audio_Func(&a_func);
}

/* 
 * @brief 初始化uac控制模块
 * 
 * @param param 配置参数指针
 * @return int 0:成功 -1:失败
 */
int uac_control_init(void *param)
{
	int ret = 0;
	config_func_param_t *cfg_param = (config_func_param_t*)param;

	/* 初始化音频上下文 */
	memcpy(&uac_ctx, &cfg_param->audio_info, sizeof(audio_info_t));
	if(uac_ctx.amic_en || uac_ctx.spk_en){
		/* 初始化音频相关的imp */
		// imp_audio_init();
		/* 打开音频设备 */
		ret = IF_Ucam_Audio_Init(AUDIO_DEVICE_NAME, &cfg_param->audio_info);
		CHECK_ERR(ret, "IF_UCAM_AUDIO_Init failed:%d\n", ret);
		/* 注册uac回调函数 */
		register_audio_func();
		/* 创建uac线程，启动音频设备 */
		ret = IF_Ucam_Audio_Start(&cfg_param->audio_info);
		CHECK_ERR(ret, "IF_UCAM_AUDIO_Start failed:%d\n", ret);
	}

	/* 初始化音频播放线程 */
	// if(uac_ctx.spk_en) {
	// 	inlock_audio_play_start();
	// 	printf("INFO[%s]: audio play start...!\n", MODULE_TAG);
	// }

	return 0;
}

/* 
 * @brief 反初始化uac控制模块
 * 
 */
void uac_control_deinit()
{
	if(!(uac_ctx.amic_en | uac_ctx.spk_en))
		return;
	// if(uac_ctx.spk_en) {
	// 	inlock_audio_play_stop();
	// }

	IF_Ucam_Audio_DeInit();

	// imp_audio_deinit();
}
