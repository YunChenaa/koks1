/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 19:47:02
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-25 11:54:41
 * @FilePath: /sense-koks/application/media_core/include/st_koks_video.h
 */
#pragma once
#include "imp_common.h"
#include "imp_encoder.h"
#include "imp_framesource.h"
#include "imp_osd.h"
#include "imp_system.h"
#include "json11.hpp"
#include "koks1_config.h"
#include "st_koks_errno.h"
#include "sys_logger.h"
#include "fast_frame_info.h"
#include "koks_project_def.h"
#include <memory>
#include <stdio.h>
#include <thread>
#include <utility>
namespace st
{
namespace sense_koks
{

// Sensor WIDTH HEIGHT 定义
#ifdef KOKS_FEATURE_60FPS
constexpr int32_t SENSOR0_WIDTH = 800;  // Main channel
constexpr int32_t SENSOR0_HEIGHT = 600; // Main channel
constexpr int32_t SENSOR1_WIDTH = 800;  // Main channel
constexpr int32_t SENSOR1_HEIGHT = 600; // Main channel
#else
constexpr int32_t SENSOR0_WIDTH = 1600;  // Main channel
constexpr int32_t SENSOR0_HEIGHT = 1200; // Main channel

#if defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M55)
constexpr int32_t SENSOR1_WIDTH = 2560;  // Main channel
constexpr int32_t SENSOR1_HEIGHT = 1440; // Main channel
#else
constexpr int32_t SENSOR1_WIDTH = 1600;  // Main channel
constexpr int32_t SENSOR1_HEIGHT = 1200; // Main channel
#endif

#endif // Sensor WIDTH HEIGHT 定义

typedef std::shared_ptr<IMPEncoderStream> SpEncodeVideoFrameInfo;

using VnecFrameProcCallback = std::function<ST_ERR(uint32_t venc_chan, SpEncodeVideoFrameInfo& sp_video_frame_info)>;


#ifdef  KOKS_FEATURE_60FPS
#define SENSOR_FRAME_RATE_NUM 50
#elif defined(KOKS_FEATURE_30FPS)
#define SENSOR_FRAME_RATE_NUM 30
#else
#define SENSOR_FRAME_RATE_NUM 15
#endif

#define SENSOR_UVC_FRAME_RATE_NUM 15
#define SENSOR_ENC_FRAME_RATE_NUM 15
#define SENSOR_FRAME_RATE_DEN 1
#define MAX_ENC_CHN (1)

#ifdef KOKS_PROJECT_M53D
#define BITRATE_720P_Kbs 800
#else
#define BITRATE_720P_Kbs 1000
#endif

typedef struct FrameSourceChnConfig {
    IMPFSChnAttr fs_chn_attr;
    IMPCell SelfCell;
    IMPCell imp_encoder;
} FrameSourceChnConfig;

typedef struct VencChnConfig {
    unsigned int chn_id; /* 0 for main channel, 1 for second channel */
    bool is_enable;
    IMPPayloadType type;
    IMPEncoderCHNAttr chn_attr;
    std::pair<bool, std::shared_ptr<std::thread>> sp_video_thread{ false, nullptr };
    VnecFrameProcCallback frame_proc_cb{ nullptr };
} VencChnConfig;

class RecvPicGuard
{
    public:
    RecvPicGuard(int32_t _chn) : chn(_chn)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_Encoder_StartRecvPic(chn);
        if (ret < 0) {
            LOG_ERROR("IMP_Encoder_StartRecvPic chn %d failed with 0x%#x!\n", chn, ret);
        } else
            is_valid = true;
    }
    ~RecvPicGuard()
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_Encoder_StopRecvPic(chn);
        if (ret < 0) {
            LOG_ERROR("IMP_Encoder_StopRecvPic chn %d failed with 0x%#x!\n", chn, ret);
        }
    }
    bool IsSuccess(void) const
    {
        return is_valid;
    }

    private:
    bool is_valid{ false };
    int32_t chn;
};

// template <typename... Args>
class Device
{
    public:
    Device(int32_t dev_id) : dev_id_(dev_id) {};
    virtual ST_ERR Init(void) = 0;
    virtual ST_ERR Start(void) = 0;
    virtual ST_ERR Stop(void) = 0;
    virtual ST_ERR DeInit(void) = 0;
    virtual ST_ERR Bind(IMPCell* srcCell, IMPCell* dstCell)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_System_Bind(srcCell, dstCell);
        MISC_CHK_STATUS_REPORT(ret, ret, "src[%d,%d,%d] IMP_System_Bind dst[%d,%d,%d] failed with %#x\n", srcCell->deviceID, srcCell->groupID, srcCell->outputID, dstCell->deviceID,
                               dstCell->groupID, dstCell->outputID, ret);
        LOG_DEBUG("src[%d,%d,%d] Bind dst[%d,%d,%d] success!\n", srcCell->deviceID, srcCell->groupID, srcCell->outputID, dstCell->deviceID, dstCell->groupID, dstCell->outputID);
        return ST_ERRNO_SUCCESS;
    }
    virtual ST_ERR Unbind(IMPCell* srcCell, IMPCell* dstCell)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_System_UnBind(srcCell, dstCell);
        MISC_CHK_STATUS_REPORT(ret, ret, "src[%d,%d,%d] Unbind dst[%d,%d,%d] failed with %#x\n", srcCell->deviceID, srcCell->groupID, srcCell->outputID, dstCell->deviceID,
                               dstCell->groupID, dstCell->outputID, ret);
        LOG_DEBUG("src[%d,%d,%d] Unbind dst[%d,%d,%d] success!\n", srcCell->deviceID, srcCell->groupID, srcCell->outputID, dstCell->deviceID, dstCell->groupID, dstCell->outputID);
        return ST_ERRNO_SUCCESS;
    }

    private:
    int32_t dev_id_;
};

class DeviceFrameSource final : public Device
{
    public:
    DeviceFrameSource(int32_t dev_id = 0);
    ~DeviceFrameSource();
    ST_ERR Init(void)
    {
    }
    ST_ERR Init(const struct FrameSourceConfig& config);
    ST_ERR Start(void);
    ST_ERR ReInit(const RestartAttr& param);
    void EnableI2d(int rotate_angle);
    ST_ERR Join(uint32_t chn, uint16_t join_type);
    ST_ERR DeJoin();
    ST_ERR Stop(void);
    ST_ERR DeInit(void);
    ST_ERR GetFrame(IMPFrameInfo** frame);
    ST_ERR ReleaseFrame(IMPFrameInfo* frame);
    ST_ERR SnapFrame(void* framedata, IMPFrameInfo* frame);
    static ST_ERR SaveFrame(const std::string& path, IMPFrameInfo& frame);
    void SetIMPCell(void);
    IMPCell* GetIMPCell(void);
    int32_t GetSourceID(void) const
    {
        return source_id_;
    }
    FrameSourceChnConfig* GetChnConfig(void)
    {
        return &ChnConfig;
    }

    private:
    FrameSourceChnConfig ChnConfig;
    IMPFSJointAttr joint_attr_;
    int32_t source_id_;
    bool is_enable{ false };
    int rotate_angle_{ 0 };
};

class DeviceVenc final : public Device
{
    public:
    DeviceVenc(int32_t dev_id = 0);
    ~DeviceVenc();
    ST_ERR Init(void)
    {
    }
    ST_ERR Init(const struct VencConfig& config);
    ST_ERR Start(void);
    ST_ERR ReInit(const RestartAttr& param);
    ST_ERR CodecTypeReInit(const int &codec_type);
    ST_ERR Stop(void);
    ST_ERR DeInit(void);
    void SetIMPCell(void);
    IMPCell* GetIMPCell(void);
    ST_ERR GetFrame(IMPEncoderStream& frame);
    ST_ERR ReleaseFrame(IMPEncoderStream& frame);
    int GetAndCopyFrame(char *stream_data, bool& is_key_frame);
    int GetAndCopyFrameWithoutChromaDQT(char *stream_data);
    int RemoveChromaDQTfromMjpeg(char* stream_date, IMPEncoderStream* stream); 
    ST_ERR SaveJpeg(const std::string& path);
    ST_ERR SetFrameProcCb(int32_t chn, VnecFrameProcCallback Cb);
    ST_ERR SetFrameIDR(void);
    IMPCell SelfCell;
    VencChnConfig* GetChnConfig(int chn_)
    {
        if (chn_ < MAX_ENC_CHN)
            return &_VencChnConfig[chn_];
        else
            return nullptr;
    }

    static IMPPayloadType GetEncType(const std::string& type)
    {
        if (type == "jpeg")
            return PT_JPEG;
        else if (type == "h264")
            return PT_H264;
        else if (type == "h265")
            return PT_H265;
    }
    int32_t GetJpegChn()
    {
        for (int i = 0; i < MAX_ENC_CHN; i++) {
            if (_VencChnConfig[i].type == PT_JPEG)
                return _VencChnConfig[i].chn_id;
        }
        return -1;
    }

    static int SaveStream(int fd, IMPEncoderStream* stream)
    {
        int i = 0;
        int ret = 0;
        int nr_pack = stream->packCount;
        for (i = 0; i < nr_pack; i++) {
            ret = write(fd, (void*)stream->pack[i].virAddr, stream->pack[i].length);
            if (ret != stream->pack[i].length) {
                LOG_ERROR("stream write failed\n");
                return -1;
            }
        }
        return 0;
    }
    int32_t GetGroup(void) const
    {
        return group_id;
    }

    private:
    int32_t group_id;
    bool is_enable{ false };
    VencChnConfig _VencChnConfig[MAX_ENC_CHN];
};

class DeviceOSD final : public Device
{
    constexpr static int32_t OSD_LETTER_NUM = 19;

    public:
    enum class os_show {
        hide = 0,
        show = 1,
    };
    DeviceOSD(int32_t dev_id = 0);
    ~DeviceOSD();
    ST_ERR Init(void);
    ST_ERR Start(void)
    {
    }
    ST_ERR Start(uint32_t width, uint32_t height, uint16_t line_x, uint16_t line_y);
    ST_ERR Stop(void);
    ST_ERR DeInit(void);
    ST_ERR SetOsdText(const std::string& text);
    ST_ERR ClearOsdText(void);
    void SetIMPCell(void)
    {
        bind_handle.deviceID = DEV_ID_OSD;
        bind_handle.groupID = group;
        bind_handle.outputID = 0;
    }
    IMPCell* GetIMPCell(void)
    {
        return &bind_handle;
    }
    int32_t GetOsdID(void) const
    {
        return group;
    }

    private:
    IMPRgnHandle handle_{ INVHANDLE };
    IMPRgnHandle handle_cover_{ INVHANDLE };
    int group{ 0 };
    IMPCell bind_handle;
    IMPOSDRgnAttr rAttrFont;
    uint16_t line_x_{ 0 };
    void* OSDProcessStream(void);
    std::pair<bool, std::shared_ptr<std::thread>> sp_osd_thread{ false, nullptr };
};

ST_ERR GetFastFrame(void *firstImage, void *secondImage, int16_t width, int16_t height, struct fast_frame_ev_info *info);

} // namespace sense_koks

} // namespace st
