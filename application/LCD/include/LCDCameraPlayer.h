#pragma once
#include "LCD.h"
#include "sys_logger.h"
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <stdint.h>
#include <sys/stat.h>
#include <thread>
#include "st_koks_uvc.h"
#include "st_koks_isp.h"
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE

#define LCD_WIDTH 320
#define LCD_HEIGHT 480
#define FBDEV_PATH "/dev/fb0"

class LCDCameraPlayer
{
public:
    static LCDCameraPlayer * getInstance()
    {
        static LCDCameraPlayer instance;
        return &instance;
    }

    int32_t StartPlay();
    void StopPlay();
    

private:
    LCDCameraPlayer();
    ~LCDCameraPlayer();
    std::atomic<bool> isPlaying{false};
    std::chrono::steady_clock::time_point m_playStartTime;
    
    std::thread m_playerThread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_threadExit{false};   // 通知线程退出（析构时使用）
    
    void PlayerThreadFunc();    // 线程主函数
};

#endif // KOKS_FEATURE_UVC_MEDIA_STREAM_MODE