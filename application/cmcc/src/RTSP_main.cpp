// ============================================================
// 主程序入口
// ============================================================
#include "RTSPH264VideoServerMediaSubsession.h"
#include "RTSPAudioServerMediaSubession.h"
#include "sys_logger.h"
#include "st_koks_uvc.h"
#include "OVD_define.h"
#include "st_koks_isp.h"
#include <BasicUsageEnvironment.hh>
#include <RTSPServer.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----- Configuration -----
#define RTSP_DEFAULT_PORT   554
#define STREAM_NAME         "channel=1/subtype=1"
#define STREAM_INFO         "H.264/H.265 RTSP Stream"
#define STREAM_DESC         "Live555 H.264/H.265 RTSP"
#define ESTIMATED_BITRATE   300 * 1024   // 300 kbps
#define DEFAULT_FPS         15

// ============================================================
// 选择编码类型：VIDEO_CODEC_H264 或 VIDEO_CODEC_H265
// ============================================================
#define RTSP_VIDEO_CODEC    VIDEO_CODEC_H264

bool VideoFrameAcquireCallback(void* userData,
                            unsigned char* buffer,
                            unsigned maxSize,
                            unsigned& frameSize,
                            struct timeval& presentationTime)
{
    std::vector<unsigned char>* Databuf = (std::vector<unsigned char>*)userData;
    bool is_key_frame = false;

    int lenth = MediaUvc::GetInstance().MediaStreamGetVideo(
                    VIDEO_STREAM_H265, Databuf->data(), is_key_frame);

    if (lenth <= 0) {
        LOG_ERROR("Failed to get video frame, ret: %d\n", lenth);
        return false;
    }

    // 跳过 start code（0x00000001 或 0x000001）
    unsigned char* frameData = Databuf->data();
    if (lenth >= 4 && memcmp(frameData, "\x00\x00\x00\x01", 4) == 0) {
        frameData += 4;
        lenth -= 4;
    } else if (lenth >= 3 && memcmp(frameData, "\x00\x00\x01", 3) == 0) {
        frameData += 3;
        lenth -= 3;
    }

    frameSize = (unsigned)lenth;
    if (frameSize > maxSize) frameSize = maxSize;
    memcpy(buffer, frameData, frameSize);
    struct timeval now;
    gettimeofday(&now, NULL);
    presentationTime = now;
    LOG_INFO("Video frame acquired: size=%u, key_frame=%d\n", frameSize, is_key_frame);
    return true;
}

// ============================================================
// 自定义 RTSP Server — 记录客户端连接/断开
// ============================================================
class LoggingRTSPServer : public RTSPServer {
public:
    static LoggingRTSPServer* createNew(UsageEnvironment& env,
                                        Port ourPort = 554,
                                        UserAuthenticationDatabase* authDB = NULL,
                                        unsigned reclamationSeconds = 65);

    virtual ClientConnection*
    createNewClientConnection(int clientSocket,
                              struct sockaddr_storage const& clientAddr);

protected:
    LoggingRTSPServer(UsageEnvironment& env, int ourSocket, int domain,
                      Port ourPort,
                      UserAuthenticationDatabase* authDB,
                      unsigned reclamationSeconds);
    virtual ~LoggingRTSPServer();

private:
    int fClientCount;
};

// ----- 实现 -----
LoggingRTSPServer* LoggingRTSPServer::createNew(UsageEnvironment& env,
                                                 Port ourPort,
                                                 UserAuthenticationDatabase* authDB,
                                                 unsigned reclamationSeconds)
{
    int ourSocket = setUpOurSocket(env, ourPort, AF_INET);
    if (ourSocket == -1) return NULL;
    return new LoggingRTSPServer(env, ourSocket, AF_INET, ourPort,
                                  authDB, reclamationSeconds);
}

LoggingRTSPServer::LoggingRTSPServer(UsageEnvironment& env, int ourSocket,
                                       int domain, Port ourPort,
                                       UserAuthenticationDatabase* authDB,
                                       unsigned reclamationSeconds)
    : RTSPServer(env, ourSocket, domain, ourPort, authDB, reclamationSeconds),
      fClientCount(0)
{
}

LoggingRTSPServer::~LoggingRTSPServer()
{
}

GenericMediaServer::ClientConnection*
LoggingRTSPServer::createNewClientConnection(int clientSocket,
                                              struct sockaddr_storage const& clientAddr)
{
    fClientCount++;

    char clientIP[INET6_ADDRSTRLEN] = {0};
    uint16_t clientPort = 0;

    if (clientAddr.ss_family == AF_INET) {
        struct sockaddr_in const* addr4 = (struct sockaddr_in const*)&clientAddr;
        inet_ntop(AF_INET, &addr4->sin_addr, clientIP, sizeof(clientIP));
        clientPort = ntohs(addr4->sin_port);
    } else if (clientAddr.ss_family == AF_INET6) {
        struct sockaddr_in6 const* addr6 = (struct sockaddr_in6 const*)&clientAddr;
        inet_ntop(AF_INET6, &addr6->sin6_addr, clientIP, sizeof(clientIP));
        clientPort = ntohs(addr6->sin6_port);
    } else {
        snprintf(clientIP, sizeof(clientIP), "unknown_af_%d", clientAddr.ss_family);
    }

    LOG_INFO("===== RTSP Client #%d connected from %s:%d =====\n",
             fClientCount, clientIP, clientPort);

    return RTSPServer::createNewClientConnection(clientSocket, clientAddr);
}


// ============================================================
// RTSP 服务线程主函数
// ============================================================
int RTSPH264VideoServerMediaSubsession_thread_main()
{
    st::sense_koks::Isp::GetInstance().WaitForBootDayNightJudge();
    std::vector<unsigned char> H264Databuf(300 * 1024);

    //初始化音视频流
    get_audio_data_thread_start();
    MediaUvc::GetInstance().MediaStreamConfigVideo(VIDEO_STREAM_H265, true, 1080, 1920); 
    if (RTSP_VIDEO_CODEC == VIDEO_CODEC_H264) {
        LOG_INFO("Configuring stream for H.264 codec\n");
        if (MediaUvc::GetInstance().updateStreamCodecStandard(PT_H264) != 0) {
            LOG_ERROR("Failed to set stream codec standard to H.264\n");
            return -1;
        }
    }

    // 请求关键帧
    MediaUvc::GetInstance().SetFrameIDR(); 

    // ----- 创建 live555 环境 -----
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    // ----- 创建 RTSP 服务器 -----
    LoggingRTSPServer* rtspServer = LoggingRTSPServer::createNew(*env, RTSP_DEFAULT_PORT, NULL);
    if (rtspServer == NULL) {
        LOG_ERROR("Failed to create RTSP server: %s\n", env->getResultMsg());
        return -1;
    }

    // ----- 创建 ServerMediaSession（回调模式推流）-----
    ServerMediaSession* sms = ServerMediaSession::createNew(
        *env, STREAM_NAME, STREAM_INFO, STREAM_DESC);

    sms->addSubsession(
        RTSPH264VideoServerMediaSubsession::createNew(
            *env,
            VideoFrameAcquireCallback,
            &H264Databuf,
            DEFAULT_FPS,
            ESTIMATED_BITRATE,
            False,
            RTSP_VIDEO_CODEC
        )
    );

    sms->addSubsession(
        RTSPPCMAAudioServerMediaSubsession::createNew(
            *env,
            AudioFrameAcquireCallback,
            AUDIO_SAMPLE_RATE,
            AUDIO_CHANNELS
        )
    );

    rtspServer->addServerMediaSession(sms);
    
    LOG_INFO("RTSP server started on port %d, stream: %s, codec: %s\n",
             RTSP_DEFAULT_PORT, STREAM_NAME,RTSP_VIDEO_CODEC==VIDEO_CODEC_H264 ?"H.264" :"H.265"
    );

    char* url = rtspServer->rtspURL(sms);
    LOG_INFO("Stream URL: %s\n", url);

    // ----- 进入事件循环（阻塞）-----
    env->taskScheduler().doEventLoop();
    LOG_INFO("rtsp thread exit\n");

    return 0;
}