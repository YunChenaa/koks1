#include "st_koks_uvc.h"
#include "uvc_camera.h"
#include "st_koks_led.h"
#include "st_koks_isp.h"
#include "st_koks_video.h"
#include "st_koks_errno.h"
#include "st_koks_mediamanager.h"

using namespace st::sense_koks;

static int factory_mjpeg_push_callback(int encoder_index, char* stream_buffer)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
    uint16_t dev_venc_chn = encoder_index == 1 ? 1 : 0;
#else
    uint16_t dev_venc_chn = encoder_index == 1 ? 2 : 0;
#endif
    uint32_t copy_length = 0;

    IMPEncoderStream frame;
    memset(&frame, 0, sizeof(frame));
    auto& media_manager = MediaManager::GetInstance();
    ret = media_manager.GetDeviceVenc(dev_venc_chn)->GetFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("GetFrame failed\n");
        return -1;
    }
    for (int i = 0; i < frame.packCount; i++) {
        memcpy(stream_buffer + copy_length, (void*)frame.pack[i].virAddr, frame.pack[i].length);
        copy_length += frame.pack[i].length;
    }
    ret = media_manager.GetDeviceVenc(dev_venc_chn)->ReleaseFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("ReleaseFrame failed\n");
        return -1;
    }
    return copy_length;
}

const static ReStartParam main_mj_enc_size_param[] = {
    { .v = 0, 1600, 1200, { false, 1600, 1200, false, 0, 0, 1600, 1200 } },
    { .v = 0, 800, 608, { true, 800, 608, false, 0, 0, 800, 608 } },
    { .v = 0, 608, 800, { true, 800, 608, false, 0, 0, 800, 608 } },
#if defined(KOKS_PROJECT_M53B)
    { .v = 0, 480, 640, { true, 836, 627, true, 75, 85, 640, 480 } },
#elif defined(KOKS_PROJECT_M55)
    { .v = 0, 480, 640, { true, 656, 492, true, 8, 6, 640, 480 } },
#else
    { .v = 0, 480, 640, { true, 640, 480, false, 0, 0, 640, 480 } }
#endif
};

const static ReStartParam sub_mj_enc_size_param[] = {
    { .v = 0, 1600, 1200, { false, 1600, 1200, false, 0, 0, 1600, 1200 } },
    { .v = 0, 800, 608, { true, 800, 608, false, 0, 0, 800, 608 } },
    { .v = 0, 608, 800, { true, 800, 608, false, 0, 0, 800, 608 } },
#if defined(KOKS_PROJECT_M21A) || defined(KOKS_PROJECT_M21B) || defined(KOKS_PROJECT_M25I) || defined(KOKS_PROJECT_M25P)
    { .v = 0, 480, 640, { true, 800, 608, true, 80, 64, 640, 480 } },
#elif defined(KOKS_PROJECT_M24B1) || defined(KOKS_PROJECT_M24B3)
    { .v = 0, 480, 640, { true, 768, 576, true, 64, 48, 640, 480 } },
#elif defined(KOKS_PROJECT_M53B)
    { .v = 0, 480, 640, { true, 864, 486, true, 112, 3, 640, 480 } },
#elif defined(KOKS_PROJECT_M55)
    { .v = 0, 480, 640, { true, 944, 531, true, 162, 10, 640, 480 } },
#else
    { .v = 0, 480, 640, { true, 640, 480, true, 0, 0, 640, 480 } }
#endif
};

static int factory_uvc_config_callback(struct uvc_video_params* video_param)
{
    auto& media_manager = MediaManager::GetInstance();
    ST_ERR ret = ST_ERRNO_SUCCESS;
    LOG_INFO("video_onoff %d video_index %d fcc %x width %d height %d\n", video_param->video_onoff, video_param->video_index, video_param->fcc, video_param->width,
             video_param->height);

    if (video_param->video_onoff == 1) {
        Isp::GetInstance().SetSensorFrameRate(SENSOR_UVC_FRAME_RATE_NUM);
    }

    if (video_param->video_index == 0) // main camera
    {
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
        const int UVC_SOURCE = 2;
#else
        const int UVC_SOURCE = 4;
#endif
        const uint16_t dev_venc_chn = 0;
        if (video_param->video_onoff == 0) {
            if (video_param->fcc == V4L2_PIX_FMT_MJPEG) {
                ret = media_manager.GetDeviceVenc(dev_venc_chn)->Stop();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Stop failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Stop();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Stop failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)
                        ->Unbind(media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetIMPCell(), media_manager.GetDeviceVenc(dev_venc_chn)->GetIMPCell());
                MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                                    media_manager.GetDeviceVenc(0)->GetGroup(), ret);

                ret = media_manager.GetDeviceVenc(dev_venc_chn)->DeInit();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("DeInit failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->DeInit();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("DeInit failed\n");
                    return -1;
                }
            }
#ifdef PALM_IMAGE_PICK
            else if (video_param->fcc == V4L2_PIX_FMT_NV12) {
                st::sense_koks::LedProxy::SetAiMode(false);
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Stop();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Stop failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->DeInit();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("DeInit failed\n");
                    return -1;
                }
            }
#endif
        } else if (video_param->video_onoff == 1) {
            if (video_param->fcc == V4L2_PIX_FMT_NV12) {
#ifdef PALM_IMAGE_PICK
                param.out_width = video_param->height;
                param.out_height = video_param->width;
                param.scaler_width = video_param->height;
                param.scaler_height = video_param->width;
                param.crop_left = 0;
                param.crop_top = 0;
                param.crop_width = video_param->height;
                param.crop_height = video_param->width;

                st::sense_koks::LedProxy::SetAiMode(true);
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->ReInit(param);
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("ReInit failed\n");
                    return -1;
                }

                LOG_INFO("nv12 reinit OK\n");
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Start();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Start failed\n");
                    return -1;
                }
                return 0;
#endif
            } else {
                for (int i = 0; i < sizeof(main_mj_enc_size_param) / sizeof(ReStartParam); i++) {
                    if (video_param->width == main_mj_enc_size_param[i].out_width && video_param->height == main_mj_enc_size_param[i].out_height) {
                        auto& param = main_mj_enc_size_param[i].fs;
                        LOG_INFO("mjpeg(%d) reset to %dx%d\n", UVC_SOURCE, main_mj_enc_size_param[i].out_width, main_mj_enc_size_param[i].out_height);
                        bool i2d_enable = (video_param->height  > video_param->width);
                        media_manager.GetDeviceFrameSource(UVC_SOURCE)->EnableI2d(i2d_enable ? 90 : 0);

                        ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->ReInit(param);
                        if (ret != ST_ERRNO_SUCCESS) {
                            LOG_ERROR("ReInit failed\n");
                            return -1;
                        }
                        auto venc_param = param;
                        venc_param.width = i2d_enable ? param.crop_height : param.crop_width;
                        venc_param.height = i2d_enable ? param.crop_width : param.crop_height;
                        venc_param.crop_enable = false;
                        venc_param.crop_left = 0;
                        venc_param.crop_top = 0;
                        venc_param.crop_width = i2d_enable ? param.crop_height : param.crop_width;
                        venc_param.crop_height = i2d_enable ? param.crop_width : param.crop_height;
                        ret = media_manager.GetDeviceVenc(dev_venc_chn)->ReInit(venc_param);
                        if (ret != ST_ERRNO_SUCCESS) {
                            LOG_ERROR("Venc ReInit failed\n");
                            return -1;
                        }
                        ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)
                                ->Bind(media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetIMPCell(), media_manager.GetDeviceVenc(dev_venc_chn)->GetIMPCell());
                        MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                                            media_manager.GetDeviceVenc(dev_venc_chn)->GetGroup(), ret);
                        ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Start();
                        if (ret != ST_ERRNO_SUCCESS) {
                            LOG_ERROR("Start failed\n");
                            return -1;
                        }
                        ret = media_manager.GetDeviceVenc(dev_venc_chn)->Start();
                        if (ret != ST_ERRNO_SUCCESS) {
                            LOG_ERROR("Start failed\n");
                            return -1;
                        }
                        LOG_INFO("framesource %d, enc group %d Re Init success!\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                                media_manager.GetDeviceVenc(dev_venc_chn)->GetGroup());
                    }
                }
            }
        }
    }
    else if (video_param->video_index == 1) // camera 2
    {
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
        const int UVC_SOURCE = 3;
        const uint16_t dev_venc_chn = 1;
#else
        const int UVC_SOURCE = 3;
        const uint16_t dev_venc_chn = 2;
#endif
        if (video_param->video_onoff == 0) {
            if (video_param->fcc == V4L2_PIX_FMT_MJPEG) {
                ret = media_manager.GetDeviceVenc(dev_venc_chn)->Stop();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Stop failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Stop();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("Stop failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)
                        ->Unbind(media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetIMPCell(), media_manager.GetDeviceVenc(dev_venc_chn)->GetIMPCell());
                MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                                    media_manager.GetDeviceVenc(dev_venc_chn)->GetGroup(), ret);

                ret = media_manager.GetDeviceVenc(dev_venc_chn)->DeInit();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("DeInit failed\n");
                    return -1;
                }
                ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->DeInit();
                if (ret != ST_ERRNO_SUCCESS) {
                    LOG_ERROR("DeInit failed\n");
                    return -1;
                }
            }
        } else if (video_param->video_onoff == 1) { //sub camera
            for (int i = 0; i < sizeof(sub_mj_enc_size_param) / sizeof(ReStartParam); i++) {
                if (video_param->width == sub_mj_enc_size_param[i].out_width && video_param->height == sub_mj_enc_size_param[i].out_height) {
                    auto& param = sub_mj_enc_size_param[i].fs;
                    LOG_INFO("mjpeg reset to %dx%d\n", sub_mj_enc_size_param[i].out_width, sub_mj_enc_size_param[i].out_height);
                    bool i2d_enable = (video_param->height  > video_param->width);
                    media_manager.GetDeviceFrameSource(UVC_SOURCE)->EnableI2d(i2d_enable ? 90 : 0);

                    ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->ReInit(param);
                    if (ret != ST_ERRNO_SUCCESS) {
                        LOG_ERROR("ReInit failed\n");
                        return -1;
                    }
                    auto venc_param = param;
                    venc_param.width = i2d_enable ? param.crop_height : param.crop_width;
                    venc_param.height = i2d_enable ? param.crop_width : param.crop_height;
                    venc_param.crop_enable = false;
                    venc_param.crop_left = 0;
                    venc_param.crop_top = 0;
                    venc_param.crop_width = i2d_enable ? param.crop_height : param.crop_width;
                    venc_param.crop_height = i2d_enable ? param.crop_width : param.crop_height;
                    ret = media_manager.GetDeviceVenc(dev_venc_chn)->ReInit(venc_param);
                    if (ret != ST_ERRNO_SUCCESS) {
                        LOG_ERROR("Venc ReInit failed\n");
                        return -1;
                    }
                    ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)
                              ->Bind(media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetIMPCell(), media_manager.GetDeviceVenc(dev_venc_chn)->GetIMPCell());
                    MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind enc group %d failed with %d\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                                           media_manager.GetDeviceVenc(dev_venc_chn)->GetGroup(), ret);
                    ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->Start();
                    if (ret != ST_ERRNO_SUCCESS) {
                        LOG_ERROR("Start failed\n");
                        return -1;
                    }
                    ret = media_manager.GetDeviceVenc(dev_venc_chn)->Start();
                    if (ret != ST_ERRNO_SUCCESS) {
                        LOG_ERROR("Start failed\n");
                        return -1;
                    }
                    LOG_INFO("framesource %d, enc group %d Re Init success!\n", media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetSourceID(),
                             media_manager.GetDeviceVenc(dev_venc_chn)->GetGroup());
                }
            }
        }
    }
    return 0;
}

void media_uvc_factory_init()
{
    Isp::GetInstance().setAEMode(0);
    Isp::GetInstance().setAEMode(1);
    register_Mjpegvframe_get_func(factory_mjpeg_push_callback);
#ifdef PALM_IMAGE_PICK
    extern int yuv_push_callback(int encoder_index, char* buf);
    register_NV12vframe_get_func(yuv_push_callback);
#endif
    register_UvcVideoParam_get_func(factory_uvc_config_callback);
    uvc_camera_init_with_config_file("/config_bak/factory_uvc.config", 512 * 1024);
}