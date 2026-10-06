#include "st_koks_uvc.h"
#include "koks_project_def.h"
#include "st_koks_errno.h"
#include "st_koks_isp.h"
#include "st_koks_mediamanager.h"
#include "st_koks_video.h"
#include "sys_logger.h"
#include "turbojpeg.h"
#include "uart.h"
#include "jpeg_encoder.h"
#include <mxu2.h>

#ifdef KOKS_FEATURE_VIDEO

#define MERGE_SOURCE(a, b)      ((a) | ((b) << 8))
#define MAIN_SOURCE(n)          ((n) & 0xFF)
#define SUB_SOURCE(n)           (((n) >> 8) & 0xFF)
#define IS_DUMMY_SOURCE(b)      ((b) == DUMMY_FRAME_SOURCE)

using namespace st::sense_koks;

v16u8 nv12_to_yuy2_u = { 0, 4, 8, 12, 16, 20, 24, 28, 1, 5, 9, 13, 17, 21, 25, 29 };
v16u8 nv12_to_yuy2_v = { 2, 6, 10, 14, 18, 22, 26, 30, 3, 7, 11, 15, 19, 23, 27, 31 };

#ifdef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
static int nv12_crop_width_simd(uint8_t* src, int src_width, int src_height, int crop_left, int crop_right, uint8_t* dst, int dst_width, int dst_height)
{
    if (!src || !dst) { 
        LOG_ERROR("src or dst is null\n");
        return -1;
    }

    if ((crop_left & 1) || (crop_right & 1)) { 
        LOG_ERROR("crop_left or crop_right is not even\n");
        return -1;
    }

    int dst_w = src_width - crop_left - crop_right;
    if (dst_w <= 0 || (dst_w & 1)) {
        LOG_ERROR("invalid output width: %d (must be positive and even)\n", dst_w);
        return -1;
    }

    if (dst_height != src_height) {
        LOG_ERROR("output height (%d) does not match input height (%d)\n", dst_height, src_height);
        return -1;
    }

    const uint8_t* y_src_base = src + crop_left;
    const uint8_t* uv_src_base = src + src_width * src_height + crop_left;

    uint8_t* y_dst = dst;
    uint8_t* uv_dst = dst + dst_width * dst_height;

    for (int j = 0; j < dst_height; ++j) {
        const uint8_t* y_row_src = y_src_base + j * src_width;
        uint8_t* y_row_dst = y_dst + j * dst_width;

        int i = 0;
        for (; i <= dst_width - 32; i += 32) {
            v16u8 v0 = _mx128_lu1q((void*)&y_row_src[i], 0);
            v16u8 v1 = _mx128_lu1q((void*)&y_row_src[i+16], 0);
            _mx128_su1q(v0, &y_row_dst[i], 0);
            _mx128_su1q(v1, &y_row_dst[i+16], 0);
        }
        for (; i <= dst_width - 16; i += 16) {
            v16u8 v = _mx128_lu1q((void*)&y_row_src[i], 0);
            _mx128_su1q(v, &y_row_dst[i], 0);
        }
        for (; i < dst_width; ++i) {
            y_row_dst[i] = y_row_src[i];
        }
    }

    int uv_height = dst_height / 2;
    for (int j = 0; j < uv_height; ++j) {
        const uint8_t* uv_row_src = uv_src_base + j * src_width;
        uint8_t* uv_row_dst = uv_dst + j * dst_width;

        int i = 0;
        for (; i <= dst_width - 32; i += 32) {
            v16u8 v0 = _mx128_lu1q((void*)&uv_row_src[i], 0);
            v16u8 v1 = _mx128_lu1q((void*)&uv_row_src[i+16], 0);
            _mx128_su1q(v0, &uv_row_dst[i], 0);
            _mx128_su1q(v1, &uv_row_dst[i+16], 0);
        }
        for (; i <= dst_width - 16; i += 16) {
            v16u8 v = _mx128_lu1q((void*)&uv_row_src[i], 0);
            _mx128_su1q(v, &uv_row_dst[i], 0);
        }
        for (; i < dst_width; ++i) {
            uv_row_dst[i] = uv_row_src[i];
        }
    }

    return 0;
}
#endif

static void nv12toyuv422p_simd(uint8_t* image_in, uint8_t* image_out, int width, int height)
{
    int j;

    uint8_t* y_src = image_in;
    uint8_t* uv_src = image_in + width * height;

    uint8_t* y_dst = image_out;
    int uv_stride = width / 2; // TurboJPEG TJSAMP_422 必须严格是 width / 2
    uint8_t* u_dst = y_dst + width * height;
    uint8_t* v_dst = u_dst + uv_stride * height;
    uint8_t tmp[16];

    for (j = 0; j < height; j++) {
        uint8_t* y_row = y_src + j * width;
        uint8_t* uv_row = uv_src + (j / 2) * width;

        uint8_t* y_out = y_dst + j * width;
        uint8_t* u_out = u_dst + j * uv_stride;
        uint8_t* v_out = v_dst + j * uv_stride;
        int i = 0;
        int index = 0;
        for (; i <= (width - 32); i += 32, index += 16) {
            v16u8 vr_y0 = _mx128_lu1q(&y_row[i], 0);
            v16u8 vr_y1 = _mx128_lu1q(&y_row[i + 16], 0);
            v16u8 vr_uv0 = _mx128_lu1q(&uv_row[i], 0);
            v16u8 vr_uv1 = _mx128_lu1q(&uv_row[i + 16], 0);

            v16u8 vr_u = _mx128_shufv(vr_uv1, vr_uv0, nv12_to_yuy2_u);
            v16u8 vr_v = _mx128_shufv(vr_uv1, vr_uv0, nv12_to_yuy2_v);

            _mx128_su1q(vr_y0, &y_out[i], 0);
            _mx128_su1q(vr_y1, &y_out[i + 16], 0);
            _mx128_su1q(vr_u, &u_out[index], 0);
            _mx128_su1q(vr_v, &v_out[index], 0);
        }
        for (; i <= (width - 16); i += 16, index += 8) {
            v16u8 vr_y0 = _mx128_lu1q(&y_row[i], 0);
            v16u8 vr_uv0 = _mx128_lu1q(&uv_row[i], 0);
            v16u8 vr_u = _mx128_shufv(vr_uv0, vr_uv0, nv12_to_yuy2_u);
            v16u8 vr_v = _mx128_shufv(vr_uv0, vr_uv0, nv12_to_yuy2_v);
            _mx128_su1q(vr_y0, &y_out[i], 0);
            _mx128_su1q(vr_u, tmp, 0);
            memcpy(&u_out[index], tmp, 8);
            _mx128_su1q(vr_v, tmp, 0);
            memcpy(&v_out[index], tmp, 8);
        }
        for (; i < width; i += 2) {
            y_out[i] = y_row[i];
            y_out[i + 1] = y_row[i + 1];

            u_out[i / 2] = uv_row[i];     // U
            v_out[i / 2] = uv_row[i + 1]; // V
        }
    }
}

#ifdef KOKS_FEATURE_UVC_CENTER_MJPEG
static void nv12center(const uint8_t* image_in, uint8_t* image_out, int out_width, int out_height, int in_width, bool fillblack)
{
    const int out_y_size = out_width * out_height;
    const int out_uv_size = out_width * out_height / 2;
    const int total_out_size = out_y_size + out_uv_size;
    const int offset_x = (out_width - in_width) / 2;

    // 填充黑色
    if (fillblack) {
        std::memset(image_out, 0x00, out_y_size);
        std::memset(image_out + out_y_size, 0x80, out_uv_size);
    }

    // 拷贝 Y 平面
    const int in_y_size = in_width * out_height;
    for (int y = 0; y < out_height; ++y) {
        std::memcpy(image_out + y * out_width + offset_x, image_in + y * in_width, in_width);
    }

    // 拷贝 UV 平面
    const int uv_height = out_height / 2;
    const int in_uv_width = in_width; // NV12 UV stride = width
    const int out_uv_width = out_width;

    const uint8_t* in_uv = image_in + in_y_size;
    uint8_t* out_uv = image_out + out_y_size;

    for (int y = 0; y < uv_height; ++y) {
        std::memcpy(out_uv + y * out_uv_width + offset_x, in_uv + y * in_uv_width, in_uv_width);
    }
}
#endif

static uint8_t GetMJPEGVencCh(uint8_t source)
{
    switch (source) {
    case RGB_MJPEG_FRAME_SOURCE:
        return DEV_VENC_0;
    case IR_MJPEG_FRAME_SOURCE:
        return DEV_VENC_2;
    case DOWN_MJPEG_FRAME_SOURCE:
        return DEV_VENC_4;
    default:
        LOG_ERROR("venc not found for mjpeg %u\n", source);
        break;
    }
    return DEV_VENC_0;
}

uint8_t GetH264VencCh(int source)
{
    switch (source) {
    case RGB_H264_FRAME_SOURCE:
        return DEV_VENC_1;
    case IR_H264_FRAME_SOURCE:
        return DEV_VENC_3;
    case DOWN_H264_FRAME_SOURCE:
        return DEV_VENC_5;
    default:
        LOG_ERROR("venc not found for h264 %u\n", source);
        break;
    }
    return DEV_VENC_1;
}

static void RestartParamEnableI2d(DeviceFrameSource* dev_fs, const RestartAttr* fs_attr)
{
    if (dev_fs && fs_attr) {
        int angle = 0;
        switch (fs_attr->i2d) {
        case 1:
            angle = 90;
            break;
        case 2:
            angle = 270;
            break;
        case 3:
            angle = 180;
            break;
        default:
            break;
        }
        if (angle != 0) {
            dev_fs->EnableI2d(angle);
        }
    }
}

static void GetRestartParam(const std::vector<ReStartParam>& restart_params, uint32_t width, uint32_t height, DeviceFrameSource* dev_fs, DeviceFrameSource* dev_fs2,
                            const RestartAttr*& fs_attr, const RestartAttr*& fs2_attr, RestartAttr& venc_attr, ReStartLayout& layout)
{
    fs_attr = nullptr;
    fs2_attr = nullptr;
    memset(&layout, 0, sizeof(layout));
    for (size_t i = 0; i < restart_params.size(); i++) {
        auto& param = restart_params[i];
        if (param.out_width == width && param.out_height == height) {
            if (param.v == 0) {
                fs_attr = &param.fs;
                venc_attr.width = fs_attr->crop_width;
                venc_attr.height = fs_attr->crop_height;
                venc_attr.crop_enable = false;
                venc_attr.crop_left = 0;
                venc_attr.crop_top = 0;
                venc_attr.crop_width = fs_attr->crop_width;
                venc_attr.crop_height =fs_attr->crop_height;
                break;
            }
            if (param.ch == dev_fs->GetSourceID()) {
                if ((!dev_fs2 && param.v == 1) || (dev_fs2 && param.v == 2 && param.ch2 == dev_fs2->GetSourceID())) {
                    fs_attr = &param.fs;
                    fs2_attr = &param.fs2;
                    venc_attr = param.venc;
                    layout = param.layout;

                    // 适配M5-3 M5-3B等项目的下摄旋转需求
                    RestartParamEnableI2d(dev_fs, fs_attr);
                    RestartParamEnableI2d(dev_fs2, fs2_attr);
                    break;
                }
            }
        }
    }
}

MediaUvc& MediaUvc::GetInstance()
{
    static MediaUvc instance;
    return instance;
}

MediaUvc::MediaUvc()
{
#if defined(KOKS_PROJECT_M53) || defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M53A)
    mjpeg_cam_type_new_.store(UVC_CAM_MERGE);
    h264_cam_type_new_.store(UVC_CAM_MERGE);
    mjpeg_cam_type_ = UVC_CAM_MERGE;
    h264_cam_type_ = UVC_CAM_MERGE;
#endif
    SetMJPEGSource();
    SetH264Source();
}

MediaUvc::~MediaUvc()
{
}

void MediaUvc::SetMJPEGParam(struct uvc_video_params* video_param)
{
    if (video_param) {
        memcpy(&mjpeg_param_, video_param, sizeof(uvc_video_params));
    }
}

void MediaUvc::SetH264Param(struct uvc_video_params* video_param)
{
    if (video_param) {
        memcpy(&h264_param_, video_param, sizeof(uvc_video_params));
    }
}

int MediaUvc::TurnOffMJPEG()
{
    auto sources = mjpeg_source_.load();
    if (sources == DUMMY_SOURCES) {
        return ST_ERRNO_SUCCESS;
    }

    mjpeg_source_.store(DUMMY_SOURCES);
    auto& media_manager = MediaManager::GetInstance();

    uint16_t source = MAIN_SOURCE(sources);
    uint16_t source2 = SUB_SOURCE(sources);
    uint16_t dev_venc_chn = GetMJPEGVencCh(source);
    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
    DeviceFrameSource* dev_fs2 = IS_DUMMY_SOURCE(source2) ? nullptr : media_manager.GetDeviceFrameSource(source2);

    do {
        ST_ERR ret = ST_ERRNO_SUCCESS;

#ifdef KOKS_FEATURE_UVC_HW_MJPEG
        ret = dev_venc->Stop();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Stop failed\n");
            break;
        }
#endif // KOKS_FEATURE_UVC_HW_MJPEG

        ret = dev_fs->Stop();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Stop failed\n");
            break;
        }

        if (dev_fs2) {
            ret = dev_fs2->Stop();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Stop failed\n");
                break;
            }
        }

#ifdef KOKS_FEATURE_UVC_HW_MJPEG
        ret = dev_fs->Unbind(dev_fs->GetIMPCell(), dev_venc->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Unbind venc group %d failed with %d\n", dev_fs->GetSourceID(), dev_venc->GetGroup(), ret);

        ret = dev_venc->DeInit();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("DeInit failed\n");
            break;
        }
#endif // KOKS_FEATURE_UVC_HW_MJPEG

        if (dev_fs2) {
            ret = dev_fs->DeJoin();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("DeJoin failed\n");
                break;
            }
        }

        ret = dev_fs->DeInit();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("DeInit failed\n");
            break;
        }
        if (dev_fs2) {
            ret = dev_fs2->DeInit();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("DeInit failed\n");
                break;
            }
        }

        LOG_INFO("framesource %d, enc group %d DeInit success!\n", dev_fs->GetSourceID(), dev_venc->GetGroup());

    } while (0);

    return ST_ERRNO_SUCCESS;
}

int MediaUvc::TurnOnMJPEG()
{
    auto sources = mjpeg_source_new_.load();
    if (sources == DUMMY_SOURCES || sources == mjpeg_source_) {
        return ST_ERRNO_SUCCESS;
    }

    mjpeg_source_.store(sources);

    auto& media_manager = MediaManager::GetInstance();

    uint16_t source = MAIN_SOURCE(sources);
    uint16_t source2 = SUB_SOURCE(sources);
    auto dev_venc_chn = GetMJPEGVencCh(source);
    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
    DeviceFrameSource* dev_fs2 = IS_DUMMY_SOURCE(source2) ? nullptr : media_manager.GetDeviceFrameSource(source2);

    auto video_param = &mjpeg_param_;
    const RestartAttr* fs_attr;
    const RestartAttr* fs2_attr;
    RestartAttr venc_attr;
    GetRestartParam(koks_config.mj_restart_params, video_param->width, video_param->height, dev_fs, dev_fs2, fs_attr, fs2_attr, venc_attr, mjpeg_layout_);
    if (!fs_attr) {
        LOG_ERROR("mjpeg param not found\n");
        return -1;
    }

    do {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        if (video_param->fcc == V4L2_PIX_FMT_MJPEG)
            LOG_INFO("mjpeg reset to %dx%d\n", video_param->width, video_param->height);
        else if (video_param->fcc == V4L2_PIX_FMT_YUYV)
            LOG_INFO("yuyv reset to %dx%d\n", video_param->width, video_param->height);

        if (dev_fs2 && fs2_attr) {
            ret = dev_fs2->ReInit(*fs2_attr);
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("ReInit failed\n");
                break;
            }
        }

        ret = dev_fs->ReInit(*fs_attr);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("ReInit failed\n");
            break;
        }

        if (dev_fs2) {
            ret = dev_fs->Join(dev_fs2->GetSourceID(), mjpeg_layout_.joint);
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Join failed\n");
                break;
            }
        }

#ifdef KOKS_FEATURE_UVC_HW_MJPEG
        ret = dev_venc->ReInit(venc_attr);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("ReInit failed\n");
            break;
        }
        ret = dev_fs->Bind(dev_fs->GetIMPCell(), dev_venc->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind venc group %d failed with %d\n", dev_fs->GetSourceID(), dev_venc->GetGroup(), ret);
#endif // KOKS_FEATURE_UVC_HW_MJPEG

        if (dev_fs2) {
            ret = dev_fs2->Start();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Start failed\n");
                break;
            }
        }

        ret = dev_fs->Start();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Start failed\n");
            break;
        }

#ifdef KOKS_FEATURE_UVC_HW_MJPEG
        ret = dev_venc->Start();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Start failed\n");
            break;
        }
#endif // KOKS_FEATURE_UVC_HW_MJPEG

        LOG_INFO("framesource %d, enc group %d Re Init success!\n", dev_fs->GetSourceID(), dev_venc->GetGroup());
    } while (0);

    return ST_ERRNO_SUCCESS;
}

int MediaUvc::TurnOffH264()
{
    auto sources = h264_source_.load();
    if (sources == DUMMY_SOURCES) {
        return ST_ERRNO_SUCCESS;
    }

    h264_source_.store(DUMMY_SOURCES);
    auto& media_manager = MediaManager::GetInstance();

    uint16_t source = MAIN_SOURCE(sources);
    uint16_t source2 = SUB_SOURCE(sources);
    uint16_t dev_venc_chn = GetH264VencCh(source);
    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
    auto dev_osd = media_manager.GetDeviceOSD(DEV_OSD_0);
    DeviceFrameSource* dev_fs2 = IS_DUMMY_SOURCE(source2) ? nullptr : media_manager.GetDeviceFrameSource(source2);

    do {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = dev_venc->Stop();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Stop failed\n");
            break;
        }
        ret = dev_osd->Stop();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Stop failed\n");
            break;
        }
        ret = dev_fs->Stop();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Stop failed\n");
            break;
        }
        if (dev_fs2) {
            ret = dev_fs2->Stop();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Stop failed\n");
                break;
            }
        }
        ret = dev_fs->Unbind(dev_fs->GetIMPCell(), dev_osd->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Unbind osd group %d failed with %d\n", dev_fs->GetSourceID(), dev_osd->GetOsdID(), ret);

        ret = dev_osd->Unbind(dev_osd->GetIMPCell(), dev_venc->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "osd %d Unbind enc group %d failed with %d\n", dev_osd->GetOsdID(), dev_venc->GetGroup(), ret);

        ret = dev_venc->DeInit();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("DeInit failed\n");
            break;
        }
        ret = dev_osd->DeInit();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("DeInit failed\n");
            break;
        }
        if (dev_fs2) {
            ret = dev_fs->DeJoin();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("DeJoin failed\n");
                break;
            }
        }
        ret = dev_fs->DeInit();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("DeInit failed\n");
            break;
        }
        if (dev_fs2) {
            ret = dev_fs2->DeInit();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("DeInit failed\n");
                break;
            }
        }

        LOG_INFO("framesource %d, enc group %d DeInit success!\n", dev_fs->GetSourceID(), dev_venc->GetGroup());
    } while (0);

    return ST_ERRNO_SUCCESS;
}

int MediaUvc::TurnOnH264()
{
    auto sources = h264_source_new_.load();
    if (sources == DUMMY_SOURCES || sources == h264_source_) {
        return ST_ERRNO_SUCCESS;
    }

    h264_source_.store(sources);
    auto& media_manager = MediaManager::GetInstance();

    uint16_t source = MAIN_SOURCE(sources);
    uint16_t source2 = SUB_SOURCE(sources);
    auto dev_venc_chn = GetH264VencCh(source);
    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
    auto dev_osd = media_manager.GetDeviceOSD(DEV_OSD_0);
    DeviceFrameSource* dev_fs2 = IS_DUMMY_SOURCE(source2) ? nullptr : media_manager.GetDeviceFrameSource(source2);

    auto video_param = &h264_param_;
    const RestartAttr* fs_attr;
    const RestartAttr* fs2_attr;
    RestartAttr venc_attr;
    GetRestartParam(koks_config.h264_restart_params, video_param->width, video_param->height, dev_fs, dev_fs2, fs_attr, fs2_attr, venc_attr, h264_layout_);
    if (!fs_attr) {
        LOG_ERROR("h264 param not found\n");
        return -1;
    }

    do {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        LOG_INFO("h264 reset to %dx%d\n", video_param->width, video_param->height);

        if (dev_fs2 && fs2_attr) {
            ret = dev_fs2->ReInit(*fs2_attr);
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("ReInit failed\n");
                break;
            }
        }

        ret = dev_fs->ReInit(*fs_attr);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("ReInit failed\n");
            break;
        }

        if (dev_fs2) {
            ret = dev_fs->Join(dev_fs2->GetSourceID(), h264_layout_.joint);
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Join failed\n");
                break;
            }
        }

        ret = dev_venc->ReInit(venc_attr);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("ReInit failed\n");
            break;
        }
        ret = dev_osd->Init();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Init failed\n");
            break;
        }
        ret = dev_fs->Bind(dev_fs->GetIMPCell(), dev_osd->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "framesource %d Bind osd group %d failed with %d\n", dev_fs->GetSourceID(), dev_osd->GetOsdID(), ret);
        ret = dev_osd->Bind(dev_osd->GetIMPCell(), dev_venc->GetIMPCell());
        MISC_CHK_STATUS_REPORT(ret, ret, "osd %d Bind enc group %d failed with %d\n", dev_osd->GetOsdID(), dev_venc->GetGroup(), ret);

        if (dev_fs2) {
            ret = dev_fs2->Start();
            if (ret != ST_ERRNO_SUCCESS) {
                LOG_ERROR("Start failed\n");
                break;
            }
        }
        ret = dev_fs->Start();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Start failed\n");
            break;
        }
        ret = dev_osd->Start(venc_attr.width, venc_attr.height, h264_layout_.line_x, h264_layout_.line_y);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Start failed\n");
            break;
        }
        ret = dev_venc->Start();
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("Start failed\n");
            break;
        }
        LOG_INFO("framesource %d, enc group %d Re Init success!\n", dev_fs->GetSourceID(), dev_venc->GetGroup());
    } while (0);

    return ST_ERRNO_SUCCESS;
}

void MediaUvc::SetMJPEGSource()
{
    if (mjpeg_cam_type_ == UVC_CAM_UP) {
        uint16_t source = (night_mode_ ? IR_MJPEG_FRAME_SOURCE : RGB_MJPEG_FRAME_SOURCE);
        mjpeg_source_new_.store(MERGE_SOURCE(source, DUMMY_FRAME_SOURCE));
    } else if (mjpeg_cam_type_ == UVC_CAM_DOWN) {
        mjpeg_source_new_.store(MERGE_SOURCE(DOWN_MJPEG_FRAME_SOURCE, DUMMY_FRAME_SOURCE));
    } else if (mjpeg_cam_type_ == UVC_CAM_MERGE) {
        uint16_t source = (night_mode_ ? IR_MJPEG_FRAME_SOURCE : RGB_MJPEG_FRAME_SOURCE);
        mjpeg_source_new_.store(MERGE_SOURCE(source, DOWN_MJPEG_FRAME_SOURCE));
    }
}

void MediaUvc::SetH264Source()
{
    if (h264_cam_type_ == UVC_CAM_UP) {
        uint16_t source = (night_mode_ ? IR_H264_FRAME_SOURCE : RGB_H264_FRAME_SOURCE);
        h264_source_new_.store(MERGE_SOURCE(source, DUMMY_FRAME_SOURCE));
    } else if (h264_cam_type_ == UVC_CAM_DOWN) {
        h264_source_new_.store(MERGE_SOURCE(DOWN_H264_FRAME_SOURCE, DUMMY_FRAME_SOURCE));
    } else if (h264_cam_type_ == UVC_CAM_MERGE) {
        uint16_t source = (night_mode_ ? IR_H264_FRAME_SOURCE : RGB_H264_FRAME_SOURCE);
        h264_source_new_.store(MERGE_SOURCE(source, DOWN_H264_FRAME_SOURCE));
    }
}

void MediaUvc::UpdateSources()
{
    auto new_mjpeg_cam_type = mjpeg_cam_type_new_.load(std::memory_order_relaxed);
    auto new_h264_cam_type = h264_cam_type_new_.load(std::memory_order_relaxed);
    auto new_night_mode = night_mode_new_.load(std::memory_order_relaxed);
    if (mjpeg_cam_type_ != new_mjpeg_cam_type || h264_cam_type_ != new_h264_cam_type || night_mode_ != new_night_mode) {
        mjpeg_cam_type_ = new_mjpeg_cam_type;
        h264_cam_type_ = new_h264_cam_type;
        night_mode_ = new_night_mode;

        SetMJPEGSource();
        SetH264Source();
    }
}
#if defined(KOKS_PROJECT_M53D)
static std::mutex CheckUvcSources_mutex;
#endif

void MediaUvc::CheckUvcSources()
{
    // LOG_INFO("CheckUvcSources: mjpeg_source_=%04x, mjpeg_source_new_=%04x, h264_source_=%04x, h264_source_new_=%04x\n", mjpeg_source_.load(), mjpeg_source_new_.load(), h264_source_.load(), h264_source_new_.load());
    //加锁，防止在更新源的过程中，源被其他线程修改
#if defined(KOKS_PROJECT_M53D)
    std::lock_guard<std::mutex> lock(CheckUvcSources_mutex);
#endif

    UpdateSources();

    if ((mjpeg_source_ != DUMMY_SOURCES && mjpeg_source_ != mjpeg_source_new_) || (h264_source_ != DUMMY_SOURCES && h264_source_ != h264_source_new_)) {
        auto cur_mjpeg = mjpeg_source_.load(std::memory_order_relaxed);
        auto cur_h264 = h264_source_.load(std::memory_order_relaxed);
        LOG_INFO("MJPEG source: %04x -> %04x, H264 source: %04x -> %04x\n", cur_mjpeg, mjpeg_source_new_.load(), cur_h264, h264_source_new_.load());

        if (cur_mjpeg != DUMMY_SOURCES && cur_h264 != DUMMY_SOURCES) {
            TurnOffMJPEG();
            TurnOffH264();
            TurnOnMJPEG();
            TurnOnH264();
        }
        else if (cur_mjpeg != DUMMY_SOURCES) {
            TurnOffMJPEG();
            TurnOnMJPEG();
        }
        else if (cur_h264 != DUMMY_SOURCES) {
            TurnOffH264();
            TurnOnH264();
        }
    }
}

uint16_t MediaUvc::CheckMJPEGSource()
{
    CheckUvcSources();
    return mjpeg_source_;
}

uint16_t MediaUvc::CheckH264Source()
{
    CheckUvcSources();
    return h264_source_;
}

void MediaUvc::SetNightMode(bool night)
{
#if !defined(KOKS_FEATURE_UVC_NIGHT_RGB)
    // 夜视使用IR，更新 night_mode_new_
    night_mode_new_.store(night, std::memory_order_relaxed);
#endif
}

void MediaUvc::SetCamType(uint8_t encode_mode, uint8_t video_mode)
{
#if defined(KOKS_PROJECT_M53) || defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M53A) || defined(KOKS_PROJECT_M55)
    uint8_t cam_type = UVC_CAM_INVALID;
    switch (video_mode) {
    case VIDEO_FRAME_MODE0:
        cam_type = UVC_CAM_UP;
        break;
    case VIDEO_FRAME_MODE1:
        cam_type = UVC_CAM_DOWN;
        break;
    case VIDEO_FRAME_MODE2:
        cam_type = UVC_CAM_MERGE;
        break;
    default:
        break;
    }

    if (cam_type != UVC_CAM_INVALID) {
        if (encode_mode == ENCODE_VIDEO_MODE_MJPEG) {
            mjpeg_cam_type_new_.store(cam_type, std::memory_order_relaxed);
        } else if (encode_mode == ENCODE_VIDEO_MODE_H264 || encode_mode == ENCODE_VIDEO_MODE_H265) {
            h264_cam_type_new_.store(cam_type, std::memory_order_relaxed);
        }
    }
#endif
}

void MediaUvc::SetCamType2(uint8_t cam_type)
{
#if defined(KOKS_PROJECT_M53) || defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M53A) || defined(KOKS_PROJECT_M55)
    mjpeg_cam_type_new_.store(cam_type, std::memory_order_relaxed);
    h264_cam_type_new_.store(cam_type, std::memory_order_relaxed);
#endif
}

uint8_t MediaUvc::GetVideoMode(uint8_t encode_mode)
{
    uint8_t cam_type = UVC_CAM_INVALID;

    if (encode_mode == ENCODE_VIDEO_MODE_MJPEG) {
        cam_type = mjpeg_cam_type_.load();
    } else if (encode_mode == ENCODE_VIDEO_MODE_H264 || encode_mode == ENCODE_VIDEO_MODE_H265) {
        cam_type = h264_cam_type_.load();
    }

    uint8_t video_mode = VIDEO_FRAME_MODE_INVALID;
    switch (cam_type) {
    case UVC_CAM_UP:
        video_mode = VIDEO_FRAME_MODE0;
        break;
    case UVC_CAM_DOWN:
        video_mode = VIDEO_FRAME_MODE1;
        break;
    case UVC_CAM_MERGE:
        video_mode = VIDEO_FRAME_MODE2;
        break;
    default:
        break;
    }
    return video_mode;
}

int MediaUvc::SetFrameIDR()
{
    auto sources = h264_source_.load();
    if (sources != DUMMY_SOURCES) {
        uint16_t source = MAIN_SOURCE(sources);
        uint16_t dev_venc_chn = GetH264VencCh(source);
        auto& media_manager = MediaManager::GetInstance();
        auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
        return dev_venc->SetFrameIDR();
    }
    return 0;
}

bool MediaUvc::IsVideoActive() const
{
    return mjpeg_source_ != DUMMY_SOURCES || h264_source_ != DUMMY_SOURCES;
}

#ifdef KOKS_FEATURE_UVC_CENTER_MJPEG
bool MediaUvc::NeedAlignMjpegToCenter(uint32_t& width, uint32_t& height) const
{
    bool need = false;
    if (mjpeg_layout_.center == 1) { // 水平居中
        need = (mjpeg_param_.height == height && mjpeg_param_.width > width);
    } else if (mjpeg_layout_.center == 2) { // 垂直居中
        need = (mjpeg_param_.width == width && mjpeg_param_.height > height);
    }

    if (mjpeg_layout_.center != 0 && !need) {
        LOG_ERROR("Cannot align mjpeg to center, center %u, out %u x %u, frame %u x %u\n", mjpeg_layout_.center, mjpeg_param_.width, mjpeg_param_.height, width, height);
    }

    if (need) {
        width = mjpeg_param_.width;
        height = mjpeg_param_.height;
    }
    return need;
}
#endif

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
bool MediaUvc::NeedDrawMjpegLineV(int& x) const
{
    if (mjpeg_layout_.line_x > 0) {
        x = mjpeg_layout_.line_x;
        return true;
    }
    return false;
}
#endif

static int uvc_config_callback(struct uvc_video_params* video_param)
{
    LOG_INFO("video_onoff %d video_index %d fcc %x width %d height %d\n", video_param->video_onoff, video_param->video_index, video_param->fcc, video_param->width,
             video_param->height);

    if (video_param->video_onoff == 1) {
        Isp::GetInstance().SetSensorFrameRate(SENSOR_UVC_FRAME_RATE_NUM);
    }

    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto& uvc = MediaUvc::GetInstance();

    if (video_param->video_index == 0) { // MJPEG
        if (video_param->video_onoff == 0) {
            ret = uvc.TurnOffMJPEG();
        } else if (video_param->video_onoff == 1) {
            uvc.TurnOffMJPEG();
            uvc.CheckUvcSources();
            uvc.SetMJPEGParam(video_param);
            ret = uvc.TurnOnMJPEG();
        }
    } else if (video_param->video_index == 1) { // h264
        if (video_param->video_onoff == 0) {
            ret = uvc.TurnOffH264();
        } else if (video_param->video_onoff == 1) {
            uvc.TurnOffH264();
            uvc.CheckUvcSources();
            uvc.SetH264Param(video_param);
            ret = uvc.TurnOnH264();
        }
    }

    MediaManager::GetInstance().GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->SetSpkEnable(uvc.IsVideoActive());
    Isp::GetInstance().setVideoStatus(uvc.IsVideoActive());
    return ret;
}

static int h264_push_callback_ex(int encoder_index, char* stream_data, bool& is_key_frame)
{
    auto source = MediaUvc::GetInstance().CheckH264Source();
    if (source == DUMMY_SOURCES) {
        LOG_ERROR("CheckH264Source failed\n");
        return -1;
    }
    source = MAIN_SOURCE(source);

    auto& media_manager = MediaManager::GetInstance();

    auto dev_venc_chn = GetH264VencCh(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
    int copy_length = dev_venc->GetAndCopyFrame(stream_data, is_key_frame);

#ifdef KOKS_FEATURE_BITRATE_FPS
    //统计每秒的帧率和码率
    static auto last_time = std::chrono::high_resolution_clock::now();
    static int frame_count = 0;
    static int total_size = 0;
    frame_count++;
    total_size += copy_length;
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0;
    // LOG_INFO("H264 encoder index %d, copy_length: %d, duration: %.2f seconds\n", encoder_index, copy_length, duration);
    if (duration >= 1) {
        LOG_INFO("H264 encoder index %d, fps: %.2f, bitrate: %.2f Mbps,total_size %d\n", encoder_index, (double)frame_count / duration, total_size * 8.0 / duration / 1024.0 / 1024.0,total_size);
        last_time = now;
        frame_count = 0;
        total_size = 0;
    }
#endif
    return copy_length;
}

static int h264_push_callback(int encoder_index, char* stream_data)
{
    bool is_key_frame;
    return h264_push_callback_ex(encoder_index, stream_data, is_key_frame);
}

static int yuv422_to_mjpeg_buffer(uint8_t* image_in, uint8_t* image_out, int width, int height)
{
    static std::unique_ptr<tjhandle, decltype(&tjDestroy)> handle(nullptr, &tjDestroy);
    int ret = -1;
    if (handle == nullptr) {
        handle.reset(reinterpret_cast<tjhandle*>(tjInitCompress()));
    }

    if (width % 2 != 0) {
        LOG_ERROR("width is not a multiple of 2: %d\n", width);
        return -1;
    }

    const int y_size = width * height;
    const int uv_size = width * height / 2;
    const unsigned char* planes[] = { image_in, image_in + y_size, image_in + y_size + uv_size };

    const int strides[] = { width, width / 2, width / 2 };

    unsigned long jpeg_bufsize = tjBufSize(width, height, TJSAMP_422);
    unsigned char* jpeg_buf = image_out;

    ret = tjCompressFromYUVPlanes(handle.get(), planes, width, strides, height, TJSAMP_422, &jpeg_buf, &jpeg_bufsize, 80, TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE | TJFLAG_NOREALLOC);
    return (ret == 0) ? (int)jpeg_bufsize : -1;
}

// 绘制黑色竖线
static void draw_line_v(uint8_t* yuv422, int width, int height, int x, int thickness)
{
    // 确保参数有效
    if (x < 0 || thickness <= 0 || !yuv422)
        return;

    // 计算实际绘制的起始点和结束点
    int start_x = x;
    int end_x = x + thickness;

    // 限制范围
    if (start_x >= width)
        return;
    if (end_x > width)
        end_x = width;

    // 计算每个平面的大小
    int y_plane_size = width * height;
    int uv_plane_size = (width / 2) * height;

    // 获取各平面指针
    uint8_t* y_plane = yuv422;
    uint8_t* u_plane = yuv422 + y_plane_size;
    uint8_t* v_plane = u_plane + uv_plane_size;

    // 绘制黑色竖线
    for (int h = 0; h < height; h++) {
        for (int w = start_x; w < end_x; w++) {
            y_plane[h * width + w] = 0;
        }

        int u_start_idx = h * (width / 2) + start_x / 2;
        int u_end_idx = h * (width / 2) + (end_x - 1) / 2;

        for (int idx = u_start_idx; idx <= u_end_idx; idx++) {
            u_plane[idx] = 128;
            v_plane[idx] = 128;
        }
    }
}

static int mjpeg_push_callback(int encoder_index, char* stream_data)
{
    auto source = MediaUvc::GetInstance().CheckMJPEGSource();
    if (source == DUMMY_SOURCES) {
        LOG_ERROR("CheckMJPEGSource failed\n");
        return -1;
    }
    source = MAIN_SOURCE(source);

    auto& media_manager = MediaManager::GetInstance();

#ifdef KOKS_FEATURE_UVC_HW_MJPEG

    auto dev_venc_chn = GetMJPEGVencCh(source);
    auto dev_venc = media_manager.GetDeviceVenc(dev_venc_chn);
#ifdef KOKS_PROJECT_M53B
    int copy_length = dev_venc->GetAndCopyFrameWithoutChromaDQT(stream_data);
#else
    bool is_key_frame;
    int copy_length = dev_venc->GetAndCopyFrame(stream_data, is_key_frame);
#endif // KOKS_PROJECT_M53B
    return copy_length;

#else // KOKS_FEATURE_UVC_HW_MJPEG
    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    IMPFrameInfo* frame = nullptr;
    auto ret = dev_fs->GetFrame(&frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("GetFrame failed\n");
        return -1;
    }

    auto width = frame->width;
    auto height = frame->height;

    uint8_t* p = (uint8_t*)frame->virAddr;

#ifdef KOKS_FEATURE_UVC_CENTER_MJPEG
    static std::vector<uint8_t> tmp_buf;
    static bool has_tmp_buf = false;
    auto& uvc = MediaUvc::GetInstance();
    if (uvc.NeedAlignMjpegToCenter(width, height)) {
        if (!has_tmp_buf) {
            tmp_buf.resize(width * height * 3 / 2);
        }
        nv12center((uint8_t*)frame->virAddr, tmp_buf.data(), width, height, frame->width, !has_tmp_buf);
        p = tmp_buf.data();
        has_tmp_buf = true;
    }
    else {
        if (has_tmp_buf) {
            std::vector<uint8_t>().swap(tmp_buf);
            has_tmp_buf = false;
        }
    }
#endif
    size_t jpeg_size = 0;
#ifdef KOKS_FEATURE_UVC_ENCODE_YUV420P
    int encode_ret  = nv12_to_jpeg420(p, (uint8_t*)stream_data, width, height,
                                        (size_t)width * height * 2, &jpeg_size);
#else
    int encode_ret  = nv12_to_jpeg422(p, (uint8_t*)stream_data, width, height,
                                        (size_t)width * height * 2, &jpeg_size);
#endif

    dev_fs->ReleaseFrame(frame);
    if (encode_ret  < 0) {
        LOG_ERROR("nv12_to_jpeg failed: %d\n", encode_ret);
        return -1;
    }
    return (int)jpeg_size;

#endif // KOKS_FEATURE_UVC_HW_MJPEG
}

static int nv12_push_callback(int encoder_index, char* stream_data)
{
    auto source = MediaUvc::GetInstance().CheckMJPEGSource();
    if (source == DUMMY_SOURCES) {
        LOG_ERROR("CheckMJPEGSource failed\n");
        return -1;
    }
    source = MAIN_SOURCE(source);

    auto& media_manager = MediaManager::GetInstance();

#if defined(KOKS_PROJECT_M53D)
    std::lock_guard<std::mutex> lock(CheckUvcSources_mutex);
#endif

    auto dev_fs = media_manager.GetDeviceFrameSource(source);
    IMPFrameInfo* frame = nullptr;
    auto ret = dev_fs->GetFrame(&frame);
    if (ret != ST_ERRNO_SUCCESS || !frame) {
        LOG_ERROR("GetFrame failed %d frame %p\n", ret, frame);
        return -1;
    }

    int copy_length = 0;
    if (frame->virAddr) {
        memcpy(stream_data, (void*)frame->virAddr, frame->size);
        copy_length = frame->size;
    }
    dev_fs->ReleaseFrame(frame);
    return copy_length;
}

static int pcm_push_callback(short* pcm)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPAudioFrame frame;
    auto& media_manager = MediaManager::GetInstance();
    auto dev_audio_in = media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
    // if (dev_audio_in->GetAeC() == false) {
    //     dev_audio_in->AudioSetAec(true);
    // }
    ret = dev_audio_in->GetFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("GetFrame failed\n");
        return -1;
    }
    memcpy(pcm, (void*)frame.virAddr, frame.len);
    ret = dev_audio_in->ReleaseFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("ReleaseFrame failed\n");
        return -1;
    }
    return frame.len;
}

static int usb_disconnect_callback(int* status)
{
#ifndef KOKS_FEATURE_KDS
    auto& media_manager = MediaManager::GetInstance();
    auto& serial = Uart::GetInstance();
    auto& uvc = MediaUvc::GetInstance();
    // 重新建立连接
    if (*status == 1) {
        if (!uvc.IsVideoActive()) {
            LOG_INFO("usb connect success!\n");
            serial.cdc_Init("/dev/ttyGS0");
            // media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->ReInit();
            // media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Start();

            // media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->ReInit();
            // media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Start();
        }
    }
    // 重新连接失败
    else if (*status == 0) {
        if (uvc.IsVideoActive()) {
            LOG_ERROR("usb disconnect failed!\n");
            serial.cdc_DeInit();
            // media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Stop();
            // media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->DeInit();

            // media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Stop();
            // media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->DeInit();
        }
    }
#endif
    return 0;
}

static int uac_status_callback(int audio_index, int status)
{
#ifdef KOKS_FEATURE_AUDIO
    auto& media_manager = MediaManager::GetInstance();
    if (audio_index == 0) { // record
        if (status == 0) { // off
            media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Stop();
        }
        else if (status == 1) { // on
            media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Start();
        }
    }
    else if (audio_index == 1) { // speak
        if (status == 0) { // off
            media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Stop();
        }
        else if (status == 1) { // on
            media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Start();
        }
    }
#endif
    return 0;
}

static int register_uvc_callback(void)
{
    register_Mjpegvframe_get_func(mjpeg_push_callback);
    register_H264vframe_get_func(h264_push_callback);
    register_amic_pcm_get_func(pcm_push_callback);
    register_UvcVideoParam_get_func(uvc_config_callback);
    register_UvcUsbConnectStatus(usb_disconnect_callback);
    register_UacStatus(uac_status_callback);
    return 0;
}

void media_uvc_init()
{
    Isp::GetInstance().setAEMode(0);
    Isp::GetInstance().setAEMode(1);

    // UVC相关初始化。在 KOKS_FEATURE_UVC_MEDIA_STREAM_MODE 模式下不调用。
#ifndef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
    register_uvc_callback();
#ifdef KOKS_FEATURE_T33_ZN
    uvc_camera_init_with_config_file("/config_bak/inlock.config", 1024 * 1024);
#else
    uvc_camera_init_with_config_file("/config_bak/inlock.config", 300 * 1024);
#endif
#endif // KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
}

#ifdef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE
int MediaUvc::MediaStreamConfigVideo(VideoStreamType stream_type, bool on, uint32_t width, uint32_t height)
{
    // 复用uvc的实现
    uvc_video_params params;
    params.video_index = (stream_type == VIDEO_STREAM_MJPEG || stream_type == VIDEO_STREAM_NV12) ? 0 : 1;
    params.video_onoff = on ? 1 : 0;
    params.fcc = 0; // not in use
    params.width = width;
    params.height = height;
    return uvc_config_callback(&params);
}

int MediaUvc::updateStreamCodecStandard(int codec_type)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto sources = MediaUvc::GetInstance().CheckH264Source();
    if (sources == DUMMY_SOURCES) {
        LOG_ERROR("CheckH264Source failed\n");
        return ST_ERRNO_FAIL;
    }

    uint16_t source = MAIN_SOURCE(sources);
    uint16_t dev_venc_chn = GetH264VencCh(source);
    auto dev_venc = MediaManager::GetInstance().GetDeviceVenc(dev_venc_chn);

    MediaUvc::GetInstance().MediaStreamConfigVideo((codec_type == 1) ? VIDEO_STREAM_H264 : VIDEO_STREAM_H265, false, 1080, 1920);
    ret = dev_venc->CodecTypeReInit(codec_type);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("ReInit failed\n");
        return ST_ERRNO_FAIL;
    }
    MediaUvc::GetInstance().MediaStreamConfigVideo((codec_type == 1) ? VIDEO_STREAM_H264 : VIDEO_STREAM_H265, true, 1080, 1920);
    return ST_ERRNO_SUCCESS;
}

int MediaUvc::MediaStreamGetVideo(VideoStreamType stream_type, uint8_t* stream_data, bool& is_key_frame)
{
    if (stream_type == VIDEO_STREAM_NV12) {
        is_key_frame = true;
        return nv12_push_callback(0, (char*)stream_data);
    } else if (stream_type == VIDEO_STREAM_H264 || stream_type == VIDEO_STREAM_H265) {
        return h264_push_callback_ex(1, (char*)stream_data, is_key_frame);
    } else {
        LOG_ERROR("Not support stream type %d\n", stream_type);
    }
    return -1;
}

int MediaUvc::MediaCorpNV12Width(uint8_t* src, int src_width, int src_height, int crop_left, int crop_right, uint8_t* dst, int dst_width, int dst_height)
{
    if(nv12_crop_width_simd(src, src_width, src_height, crop_left, crop_right, dst, dst_width, dst_height) != 0) {
        LOG_ERROR("nv12 crop failed\n");
        return -1;
    }   
    return 0;
}

#endif // KOKS_FEATURE_UVC_MEDIA_STREAM_MODE

#endif //KOKS_FEATURE_VIDEO