#pragma once

#include <OnDemandServerMediaSubsession.hh>
#include <SimpleRTPSink.hh>

#include <cstdint>
#include <cstddef>
#include <sys/time.h>   // 添加头文件

#define AUDIO_SAMPLE_RATE   8000
#define AUDIO_CHANNELS      1


void get_audio_data_thread_start(void);
int AudioFrameAcquireCallback(uint8_t* buffer, size_t* size);
void get_audio_data_thread_stop(void);

class PCMASource : public FramedSource {
public:
    typedef int (*FrameAcquireCallback)(uint8_t* buffer, size_t* size);
    static PCMASource* createNew(UsageEnvironment& env, FrameAcquireCallback callback, int sampleRate) {
        return new PCMASource(env, callback, sampleRate);
    }
    
    virtual ~PCMASource() {
        if (fTaskToken) {
            envir().taskScheduler().unscheduleDelayedTask(fTaskToken);
        }
    }

protected:
    PCMASource(UsageEnvironment& env, FrameAcquireCallback callback, int sampleRate)
        : FramedSource(env), fCallback(callback), fSampleRate(sampleRate) {}

    virtual void doGetNextFrame() override {
        size_t dataSize = fMaxSize;
        int ret = fCallback((uint8_t*)fTo, &dataSize);
        if (ret == 0 && dataSize > 0) {
            fFrameSize = dataSize;
            fNumTruncatedBytes = 0;
            struct timeval now;
            gettimeofday(&now, NULL);
            fPresentationTime = now;

            afterGetting(this);
        } else {
            // 无数据，延迟重试
            fTaskToken = envir().taskScheduler().scheduleDelayedTask(10, (TaskFunc*)deliverFrameStub, this);
        }
    }

private:
    static void deliverFrameStub(void* clientData) {
        ((PCMASource*)clientData)->doGetNextFrame();
    }
    FrameAcquireCallback fCallback;
    int fSampleRate;
    TaskToken fTaskToken = NULL;
    // 移除 fTimestampUsec（不再使用）
};

// ==================== 音频子会话（PCMA）====================
class RTSPPCMAAudioServerMediaSubsession : public OnDemandServerMediaSubsession {
public:
    static RTSPPCMAAudioServerMediaSubsession* createNew(
        UsageEnvironment& env,
        PCMASource::FrameAcquireCallback callback,
        int sampleRate = 8000,
        int channels = 1) {
        return new RTSPPCMAAudioServerMediaSubsession(env, callback, sampleRate, channels);
    }

protected:
    RTSPPCMAAudioServerMediaSubsession(UsageEnvironment& env,
                                       PCMASource::FrameAcquireCallback callback,
                                       int sampleRate,
                                       int channels)
        : OnDemandServerMediaSubsession(env, False),
          fCallback(callback),
          fSampleRate(sampleRate),
          fChannels(channels) {}

    virtual FramedSource* createNewStreamSource(unsigned /*clientSessionId*/,
                                                unsigned& estBitrate) override {
        estBitrate = (fSampleRate * fChannels * 8) / 1000; // 64 kbps
        return PCMASource::createNew(envir(), fCallback, fSampleRate);
    }

    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char /*rtpPayloadTypeIfDynamic*/,
                                      FramedSource* /*inputSource*/) override {
        return SimpleRTPSink::createNew(envir(), rtpGroupsock,
                                        8,           // payload type
                                        fSampleRate,
                                        "audio",
                                        "PCMA",
                                        fChannels);
    }

private:
    PCMASource::FrameAcquireCallback fCallback;
    int fSampleRate;
    int fChannels;
};