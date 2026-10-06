#pragma once

#include "st_koks_const.h"
#include "koks1_config_types.h"
#include "koks_project_def.h"
#include "uvc_camera.h"
#include <atomic>

void media_uvc_init();
void media_uvc_factory_init();

typedef enum { UVC_CAM_MERGE = 0, UVC_CAM_UP = 1, UVC_CAM_DOWN = 2, UVC_CAM_INVALID = 0xFF } UvcCamType;
const uint16_t DUMMY_SOURCES = 0xFFFF;

// 协议相关的定义
enum VideoFrameMode {
    VIDEO_FRAME_MODE0 = 0, //主摄像头
    VIDEO_FRAME_MODE1 = 1, //副摄像头
    VIDEO_FRAME_MODE2 = 2, //主副摄像头合并（副摄像头在小框）
    VIDEO_FRAME_MODE3 = 3, //主副摄像头合并（主摄像头在小框）
    VIDEO_FRAME_MODE_INVALID = 0xFF
};

// 协议相关的定义
enum UVCStreamType {
    UVC_STREAM_MAIN = 0, //主码流
    UVC_STREAM_SUB = 1,  //子码流
};

// 协议相关的定义
enum EncodeVideoMode {
    ENCODE_VIDEO_MODE_H264 = 0,
    ENCODE_VIDEO_MODE_H265 = 1,
    ENCODE_VIDEO_MODE_MJPEG = 2,
};

// KOKS定义的Video stream类型
enum VideoStreamType {
    VIDEO_STREAM_MJPEG = 0,
    VIDEO_STREAM_NV12 = 1,
    VIDEO_STREAM_H264 = 2,
    VIDEO_STREAM_H265 = 3,
};

class MediaUvc
{
    public:
    static MediaUvc& GetInstance();

    // --------------------------------------------------------------------------------------------------------------------
    // 工作在 KOKS_FEATURE_UVC_MEDIA_STREAM_MODE 模式下。
#ifdef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE

    // 打开或者关闭 video stream
    static int MediaStreamConfigVideo(VideoStreamType stream_type, bool on, uint32_t width, uint32_t height);

    // 获取 video stream 数据
    static int MediaStreamGetVideo(VideoStreamType stream_type, uint8_t* data, bool& is_key_frame);

    static int updateStreamCodecStandard(int codec_type);

    //裁剪NV12数据宽度
    static int MediaCorpNV12Width(uint8_t* src, int src_width, int src_height, int crop_left, int crop_right, uint8_t* dst, int dst_width, int dst_height);
#endif
    // --------------------------------------------------------------------------------------------------------------------

    MediaUvc();
    ~MediaUvc();

    void SetMJPEGParam(struct uvc_video_params* video_param);
    void SetH264Param(struct uvc_video_params* video_param);

    int TurnOffMJPEG();
    int TurnOnMJPEG();

    int TurnOffH264();
    int TurnOnH264();

    void UpdateSources();
    void CheckUvcSources();
    
    uint16_t CheckMJPEGSource();
    uint16_t CheckH264Source();
    void SetNightMode(bool night);
    void SetCamType(uint8_t encode_mode, uint8_t video_mode);
    void SetCamType2(uint8_t cam_type);
    uint8_t GetVideoMode(uint8_t encodeMode);
    int SetFrameIDR();

    bool IsVideoActive() const;

    void SetMJPEGSource();
    void SetH264Source();
#ifdef KOKS_FEATURE_UVC_CENTER_MJPEG
    bool NeedAlignMjpegToCenter(uint32_t& width, uint32_t& height) const;
#endif
#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    bool NeedDrawMjpegLineV(int& x) const;
#endif
    private:
    std::atomic<uint16_t> mjpeg_source_{ DUMMY_SOURCES };
    std::atomic<uint16_t> h264_source_{ DUMMY_SOURCES };
    std::atomic<uint16_t> mjpeg_source_new_{ DUMMY_SOURCES };
    std::atomic<uint16_t> h264_source_new_{ DUMMY_SOURCES };
    std::atomic<uint8_t> night_mode_new_{ 0 };
    std::atomic<uint8_t> mjpeg_cam_type_new_{ UVC_CAM_UP };
    std::atomic<uint8_t> h264_cam_type_new_{ UVC_CAM_UP };
    std::atomic<uint8_t> mjpeg_cam_type_{ UVC_CAM_UP };
    std::atomic<uint8_t> h264_cam_type_{ UVC_CAM_UP };
    uint8_t night_mode_{ 0 };

    uvc_video_params mjpeg_param_{ 0 };
    uvc_video_params h264_param_{ 0 };
    ReStartLayout mjpeg_layout_;
    ReStartLayout h264_layout_;
};
