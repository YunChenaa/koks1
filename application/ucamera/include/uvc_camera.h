#ifndef _UVC_CAMERA_H__
#define _UVC_CAMERA_H__
#include <stdint.h>
#if defined(__cplusplus)
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

// 视频参数结构体
struct uvc_video_params {
    int video_onoff;             // 视频开关
    int video_index;                // 视频索引
    uint32_t fcc;;                  // 像素格式
    uint32_t width;                 // 宽度
    uint32_t height;                // 高度
};

extern int uvc_camera_init();
extern int uvc_camera_init_with_config_file(const char *file, unsigned int buf_size);
extern void uvc_camera_deinit();

extern int uac_audio_spk_get_frame(struct Ucamera_Audio_Frame *frame);
extern int uac_audio_spk_release_frame(struct Ucamera_Audio_Frame *frame);

extern void register_Mjpegvframe_get_func(int (*func)(int video_index, char *stream));
extern void register_H264vframe_get_func(int (*func)(int video_index, char *stream));
extern void register_NV12vframe_get_func(int (*func)(int video_index, char *stream));
extern void register_UvcVideoParam_get_func(int (*func)(struct uvc_video_params *video_param));
extern void register_UvcUsbConnectStatus(int (*func)(int *status));
// audio_index: 0: record; 1: speak.
// status: 0: off, 1: on
extern void register_UacStatus(int (*func)(int audio_index, int status));

extern void register_amic_pcm_get_func(int (*func)(short *pcm));

#if defined(__cplusplus)
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif
