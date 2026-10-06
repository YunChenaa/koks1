/*
 * H264VideoServerMediaSubsession.cpp
 *
 * Implementation: wires up H264FramedSource ->
 * H264VideoStreamDiscreteFramer / H265VideoStreamDiscreteFramer ->
 * H264VideoRTPSink / H265VideoRTPSink for each RTSP client session.
 */

#include "RTSPH264VideoServerMediaSubsession.h"
#include "RTSPH264FramedSource.h"
#include <H264VideoStreamDiscreteFramer.hh>
#include <H264VideoRTPSink.hh>
#include <H265VideoStreamDiscreteFramer.hh>   // 新增
#include <H265VideoRTPSink.hh>                // 新增
#include <cstddef>

// ============================================================
// 文件模式：工厂 + 构造函数
// ============================================================

RTSPH264VideoServerMediaSubsession*
RTSPH264VideoServerMediaSubsession::createNew(UsageEnvironment& env,
                                          char const* h264FileName,
                                          unsigned fps,
                                          unsigned estimatedBitrate,
                                          Boolean reuseFirstSource,
                                          VideoCodecType codecType)
{
    return new RTSPH264VideoServerMediaSubsession(env, h264FileName, fps,
                                              estimatedBitrate,
                                              reuseFirstSource, codecType);
}

RTSPH264VideoServerMediaSubsession::RTSPH264VideoServerMediaSubsession(
        UsageEnvironment& env,
        char const* h264FileName,
        unsigned fps,
        unsigned estimatedBitrate,
        Boolean reuseFirstSource,
        VideoCodecType codecType)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      fH264FileName(h264FileName),
      fFps(fps),
      fEstimatedBitrate(estimatedBitrate),
      fCodecType(codecType),
      fCallback(NULL),
      fCallbackData(NULL),
      fUseCallback(false)
{
}

// ============================================================
// 回调模式：工厂 + 构造函数
// ============================================================

RTSPH264VideoServerMediaSubsession*
RTSPH264VideoServerMediaSubsession::createNew(UsageEnvironment& env,
                                          FrameAcquireCallback callback,
                                          void* userData,
                                          unsigned fps,
                                          unsigned estimatedBitrate,
                                          Boolean reuseFirstSource,
                                          VideoCodecType codecType)
{
    return new RTSPH264VideoServerMediaSubsession(env, callback, userData,
                                              fps, estimatedBitrate,
                                              reuseFirstSource, codecType);
}

RTSPH264VideoServerMediaSubsession::RTSPH264VideoServerMediaSubsession(
        UsageEnvironment& env,
        FrameAcquireCallback callback,
        void* userData,
        unsigned fps,
        unsigned estimatedBitrate,
        Boolean reuseFirstSource,
        VideoCodecType codecType)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      fH264FileName(NULL),
      fFps(fps),
      fEstimatedBitrate(estimatedBitrate),
      fCodecType(codecType),
      fCallback(callback),
      fCallbackData(userData),
      fUseCallback(true)
{
}

// ============================================================
// 析构
// ============================================================

RTSPH264VideoServerMediaSubsession::~RTSPH264VideoServerMediaSubsession()
{
}

// ============================================================
// live555 回调：创建流源
// ============================================================

FramedSource* RTSPH264VideoServerMediaSubsession::createNewStreamSource(
    unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    estBitrate = fEstimatedBitrate;

    RTSPH264FramedSource* rawSource;
    if (fUseCallback) {
        rawSource = RTSPH264FramedSource::createNew(envir(), fCallback,
                                                fCallbackData, fFps);
    } else {
        rawSource = RTSPH264FramedSource::createNewFromFile(envir(),
                                                        fH264FileName, fFps);
    }

    if (rawSource == NULL) {
        envir() << "H264VideoServerMediaSubsession: "
                << "failed to create H264FramedSource\n";
        return NULL;
    }

    // 根据编码类型选择不同的 DiscreteFramer
    if (fCodecType == VIDEO_CODEC_H265) {
        return H265VideoStreamDiscreteFramer::createNew(envir(), rawSource);
    } else {
        return H264VideoStreamDiscreteFramer::createNew(envir(), rawSource);
    }
}

// ============================================================
// live555 回调：创建 RTP Sink
// ============================================================

RTPSink* RTSPH264VideoServerMediaSubsession::createNewRTPSink(
        Groupsock* rtpGroupsock,
        unsigned char rtpPayloadTypeIfDynamic,
        FramedSource* /*inputSource*/)
{
    // 根据编码类型选择不同的 RTPSink
    if (fCodecType == VIDEO_CODEC_H265) {
        return H265VideoRTPSink::createNew(envir(), rtpGroupsock,
                                           rtpPayloadTypeIfDynamic);
    } else {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock,
                                           rtpPayloadTypeIfDynamic);
    }
}