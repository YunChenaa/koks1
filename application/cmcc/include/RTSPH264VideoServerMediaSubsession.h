/*
 * H264VideoServerMediaSubsession.hh
 *
 * A custom OnDemandServerMediaSubsession that supports both
 * H.264 and H.265 via H264FramedSource / H265FramedSource,
 * H264/H265VideoStreamDiscreteFramer and H264/H265VideoRTPSink.
 */

#ifndef _H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH

#include <OnDemandServerMediaSubsession.hh>
#include "RTSPH264FramedSource.h"

// ============================================================
// 视频编码类型
// ============================================================
enum VideoCodecType {
    VIDEO_CODEC_H264 = 0,
    VIDEO_CODEC_H265 = 1
};

class RTSPH264VideoServerMediaSubsession
    : public OnDemandServerMediaSubsession
{
public:
    // ---- 文件模式 (H.264 默认，保持向后兼容) ----
    static RTSPH264VideoServerMediaSubsession* createNew(
        UsageEnvironment& env,
        char const* h264FileName,
        unsigned fps = 15,
        unsigned estimatedBitrate = 300 * 1024,
        Boolean reuseFirstSource = False,
        VideoCodecType codecType = VIDEO_CODEC_H264);

    // ---- 回调模式 (H.264 默认，保持向后兼容) ----
    static RTSPH264VideoServerMediaSubsession* createNew(
        UsageEnvironment& env,
        FrameAcquireCallback callback,
        void* userData,
        unsigned fps = 15,
        unsigned estimatedBitrate = 300 * 1024,
        Boolean reuseFirstSource = False,
        VideoCodecType codecType = VIDEO_CODEC_H264);

protected:
    // 文件模式构造函数
    RTSPH264VideoServerMediaSubsession(UsageEnvironment& env,
                                   char const* h264FileName,
                                   unsigned fps,
                                   unsigned estimatedBitrate,
                                   Boolean reuseFirstSource,
                                   VideoCodecType codecType);

    // 回调模式构造函数
    RTSPH264VideoServerMediaSubsession(UsageEnvironment& env,
                                   FrameAcquireCallback callback,
                                   void* userData,
                                   unsigned fps,
                                   unsigned estimatedBitrate,
                                   Boolean reuseFirstSource,
                                   VideoCodecType codecType);

    virtual ~RTSPH264VideoServerMediaSubsession();

    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                                                unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource* inputSource);

private:
    char const*           fH264FileName;
    unsigned              fFps;
    unsigned              fEstimatedBitrate;
    VideoCodecType        fCodecType;       // 新增：编码类型

    // 回调模式专用
    FrameAcquireCallback  fCallback;
    void*                 fCallbackData;
    bool                  fUseCallback;
};

#endif // _H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH