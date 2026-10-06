#include "OVD_define.h"
#include "OVD_OpenAPI.h"
#include "sys_logger.h"
#include "ovd.h"
#ifdef CMCC_VOIP
#include "cmcc_voip_fun.h"
#endif
#include "st_koks_uvc.h"
#include "cmcc_thread_pool.h"

#include <chrono>
#include <unistd.h>

#define VIDEO_BUFFER_SIZE (300 * 1024)
#define AUDIO_BUFFER_SIZE 2048

static uint8_t videoBuffer[VIDEO_BUFFER_SIZE];
static uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

static OVDVideoDataFormat videoFormat;
static OVDAudioDataFormat audioFormat;

static ManagedThread g_copyVideoDataThread(nullptr);
static ManagedThread g_audioThread(nullptr);
static ManagedThread g_ovdPushVideoThread(nullptr);
std::unique_ptr<EventThread> g_monitorThread;
static bool OVD_AVParamModifyflag = true;
static OVDAVCodec g_currentVideoCodec = source_STREAM_CODEC_H265;

#ifdef CMCC_VOIP
static void put_audio_data(const std::vector<uint8_t>& data) {
    audio_queue.push(data);
}

static void put_video_data(std::vector<uint8_t>& data) {
    video_queue.push(data);
}
#endif

static BlockingQueue<VideoFrameData> ovd_video_queue(QUEUE_MAX);
static void OVD_put_video_data(std::vector<uint8_t>& data, bool is_key_frame, size_t length) {
    VideoFrameData frame;
    frame.data = std::move(data);
    frame.is_key_frame = is_key_frame;
    frame.length = length;
    frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    ovd_video_queue.push(std::move(frame));
}


static void monitor_voip_status_task() {
#ifdef CMCC_VOIP
    std::thread monitorThread([]() {
        extern int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);
        while (1) {
            if(VoipManager::getInstance().get_voip_flag_info().g_calling == 1)
            {
                // If a call is in progress, prevent the device from entering hibernation
                extern void set_host_keepalive();
                set_host_keepalive();
                OVD_schedule_hibernate(0, 1, 3, OVD_HIBERNATE_P2P_OPEN);
            }
            sleep(1);
        }
    });
    monitorThread.detach();
#endif
}

static void copyDataVideoTask(std::atomic<bool>& stop, OVDCallbackList* callbackList) {
    LOG_INFO("DEV/OVD %s(%d)\n", __FUNCTION__, __LINE__);
    callbackList->requestIDR();
    while (!stop) {
        bool is_key_frame = false;
        auto length = callbackList->getVideoStream(videoFormat.codec, videoBuffer, is_key_frame);
        //LOG_INFO("DEV/OVD %s(%d): length: %d\n", __FUNCTION__, __LINE__, length);
        if (length > 0) {
#ifdef CMCC_VOIP
            auto isCallIn = VoipManager::getInstance().get_voip_flag_info().g_enable_video == 1;
            if (isCallIn) {
                std::vector<uint8_t> video_data((uint8_t*)videoBuffer, (uint8_t*)videoBuffer + length);
                put_video_data(video_data);
            }
#endif
            if (OVD_AVParamModifyflag) {
                std::vector<uint8_t> ovd_video_data((uint8_t*)videoBuffer, (uint8_t*)videoBuffer + length);
                OVD_put_video_data(ovd_video_data, is_key_frame, length);
            }
        }else{
            usleep(60 * 1000);
        }
    }
}


static void audioTask(std::atomic<bool>& stop, OVDCallbackList* callbackList) {
    LOG_INFO("DEV/OVD %s(%d)\n", __FUNCTION__, __LINE__);
    while (!stop) {
        IMPAudioFrame frame;
        auto length = callbackList->getAudioStream(audioFormat.codec, audioBuffer, AUDIO_BUFFER_SIZE, &frame);
#ifdef CMCC_VOIP
        if (VoipManager::getInstance().get_voip_flag_info().g_enable_audio == 1) {
            std::vector<uint8_t> audio_data((uint8_t*)frame.virAddr, (uint8_t*)frame.virAddr + frame.len);
            put_audio_data(audio_data);
        }
        free(frame.virAddr);
        frame.virAddr = nullptr;
#endif
        if (length > 0) {
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            OVD_AVPushData(0, OVD_HIGH_STREAM, OVD_Audio, OVD_FALSE, (OVD_uchar*)audioBuffer, length, timestamp);
        } else if (length != 0) {
            LOG_ERROR("DEV/OVD getAudioStream failed\n");
        }
    }
}


static void ovdPushVideoTask(std::atomic<bool>& stop) {
    LOG_INFO("DEV/OVD %s(%d)\n", __FUNCTION__, __LINE__);
    while (!stop) {
        if(ovd_video_queue.empty()){
            usleep(1000 * 60);
            continue;
        }

        VideoFrameData frame;
        frame = ovd_video_queue.pop();
        OVD_AVPushData(0, OVD_HIGH_STREAM, OVD_Video, 
                       frame.is_key_frame ? OVD_TRUE : OVD_FALSE,
                       (OVD_uchar*)frame.data.data(), frame.length, frame.timestamp);
    }
}

static void monitorTask(std::atomic<bool>& stop, OVDCallbackList* callbackList) {
    LOG_INFO("DEV/OVD %s(%d) invoked\n", __FUNCTION__, __LINE__);
    if (stop) return;

    OVDAVCodec target_codec = videoFormat.codec;
    if (VoipManager::getInstance().get_voip_flag_info().g_codec_type == 1) {
        target_codec = source_STREAM_CODEC_H264;
    } else if (VoipManager::getInstance().get_voip_flag_info().g_codec_type == 2) {
        target_codec = source_STREAM_CODEC_H265;
    }

    if (g_currentVideoCodec == target_codec) {
        return;
    }

    LOG_INFO("DEV/OVD Detected codec switch: curr=%d, target=%d",
            g_currentVideoCodec, target_codec);

    g_copyVideoDataThread.stop();
    g_ovdPushVideoThread.stop();
    OVD_AVParamModifyflag = false;

    if (MediaUvc::GetInstance().updateStreamCodecStandard(target_codec) != 0) {
        LOG_ERROR("DEV/OVD updateStreamCodecStandard failed for %s\n",
                target_codec == source_STREAM_CODEC_H265 ? "H265" : "H264");
        OVD_AVParamModifyflag = true;
        g_copyVideoDataThread.reset();
        g_copyVideoDataThread.start();

        g_ovdPushVideoThread.reset();
        g_ovdPushVideoThread.start();
        return;
    }

#ifdef CMCC_VOIP
    video_queue.clear();
#endif

    g_copyVideoDataThread.reset();
    g_copyVideoDataThread.start();
    videoFormat.codec = target_codec;

    if (OVD_AVParamModify(0, OVD_HIGH_STREAM, &videoFormat, &audioFormat) != 0) {
        LOG_ERROR("DEV/OVD OVD_AVParamModify failed\n");
        return;
    } else {
        g_currentVideoCodec = target_codec;
    }

    callbackList->requestIDR();
    ovd_video_queue.clear();
    OVD_AVParamModifyflag = true;
    g_ovdPushVideoThread.reset();
    g_ovdPushVideoThread.start();
}

static void initThreads(OVDCallbackList* callbackList) {
    g_copyVideoDataThread = ManagedThread([callbackList](std::atomic<bool>& stop) {
        copyDataVideoTask(stop, callbackList);
    });

    g_audioThread = ManagedThread([callbackList](std::atomic<bool>& stop) {
        audioTask(stop, callbackList);
    });

    g_ovdPushVideoThread = ManagedThread([](std::atomic<bool>& stop) {
        ovdPushVideoTask(stop);
    });

    g_monitorThread = std::make_unique<EventThread>(
        [callbackList](std::atomic<bool>& stop) {
            monitorTask(stop, callbackList);
        }
    );
}

void startAllThreads() {
    g_copyVideoDataThread.reset();
    g_copyVideoDataThread.start();
    
    g_ovdPushVideoThread.reset();
    g_ovdPushVideoThread.start();

    g_audioThread.reset();
    g_audioThread.startDetached();

    if (g_monitorThread) {
        g_monitorThread->reset();
        g_monitorThread->startDetached();
    }
}

int ovd_run(OVDCallbackList *callbackList)
{
    memset(&videoFormat, 0, sizeof(videoFormat));
    memset(&audioFormat, 0, sizeof(audioFormat));

    videoFormat.codec = source_STREAM_CODEC_H265;
    videoFormat.framerate = 15;
    videoFormat.frameInterval = 30;
    videoFormat.width = 720;
    videoFormat.height = 1280;

    audioFormat.codec = source_STREAM_CODEC_AAC_WITH_ADTS;
    audioFormat.samplesRate = 8000;
    audioFormat.channelNumber = 1;

    // OVD_setloglevel(OVD_LOGLEVEL_DEBUG);
    if(OVD_AVPushStart(0, OVD_HIGH_STREAM, &videoFormat, &audioFormat) != 0) {
        LOG_ERROR("DEV/OVD OVD_AVPushStart failed\n");
        return -1;
    }
    extern OVDCallbackList g_callbackList;
    memcpy(&g_callbackList, callbackList, sizeof(OVDCallbackList));
    
    g_currentVideoCodec = videoFormat.codec;
    initThreads(&g_callbackList);
    startAllThreads();

    start_alarm_monitor();
    monitor_voip_status_task();
    if(OVD_ServiceStart() != 0) {
        LOG_ERROR("DEV/OVD OVD_ServiceStart failed\n");
        return -1;
    }

    return 0;
}

int ovd_stop(void)
{
    g_copyVideoDataThread.stop();
    g_ovdPushVideoThread.stop();
    g_audioThread.stop();
    if(g_monitorThread)
        g_monitorThread->stop();
}
