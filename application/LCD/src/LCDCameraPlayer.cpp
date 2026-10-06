#include "LCDCameraPlayer.h"
#include "sys_logger.h"
#include <sys/stat.h>

#ifdef KOKS_FEATURE_UVC_MEDIA_STREAM_MODE

static uint8_t frameBuffer[LCD_WIDTH * LCD_HEIGHT * 3 / 2]; // NV12格式的帧缓冲区

LCDCameraPlayer::LCDCameraPlayer()
{
    system("insmod /lib/fb_gf.ko");
    system("insmod /lib/lcd_m53d.ko");
    LCD::GetInstance()->init(FBDEV_PATH);
    
    m_playerThread = std::thread(&LCDCameraPlayer::PlayerThreadFunc, this);
}

LCDCameraPlayer::~LCDCameraPlayer()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threadExit = true;
    }
    m_cv.notify_all();                      // 唤醒可能在等待的线程
    if (m_playerThread.joinable()) {
        m_playerThread.join();              // 等待线程安全退出
    }
    
    system("rmmod lcd_m53d.ko");
    system("rmmod fb_gf.ko");
}

void LCDCameraPlayer::PlayerThreadFunc()
{
    bool is_key_frame = false;
    bool isfirstframe = true;

    while (true) {
        // --- 等待 StartPlay() 发来的播放信号 ---
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() {
                return isPlaying.load() || m_threadExit;
            });
        }
        
        // 如果是析构发出的退出信号，结束线程
        if (m_threadExit) {
            break;
        }

        isfirstframe = true;  // 每次重新播放时，丢弃第一帧

        while (isPlaying) {
            int ret = MediaUvc::GetInstance().MediaStreamGetVideo(
                VIDEO_STREAM_NV12, frameBuffer, is_key_frame);
            if (ret == -1) {
                LOG_ERROR("MediaStreamGetVideo failed, ret: %d \n", ret);
            } else {
                if (isfirstframe) {
                    isfirstframe = false;   // 丢弃首帧
                } else {
                    LCD::GetInstance()->DisplayNV12(frameBuffer, LCD_WIDTH, LCD_HEIGHT);
                }
            }
        }

        LCD::GetInstance()->SetBacklightBrightnessPWM(0);
        LCD::GetInstance()->DisplayColor(0, 0, 0, LCD_WIDTH, LCD_HEIGHT);
        LOG_INFO("Stop video stream\n");
    }
}

int32_t LCDCameraPlayer::StartPlay()
{
    // 原子交换：如果之前已经是 true，说明已经在播放
    if (isPlaying.exchange(true)) {
        LOG_INFO("Camera is already playing. \n");
        return 0;
    }
    m_playStartTime = std::chrono::steady_clock::now();

    LCD::GetInstance()->draw_nv12picturefile(0, 0, "/cmcc_fs/screen.nv12",
        LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH, LCD_HEIGHT);

    LCD::GetInstance()->SetBacklightBrightnessPWM(180);

    // --- 首次打开时配置视频流（整个生命周期只调用一次） ---
    static bool isFirstOpen = true;
    if (isFirstOpen) {
        isFirstOpen = false;
        int ret = MediaUvc::GetInstance().MediaStreamConfigVideo(
            VIDEO_STREAM_NV12, true, LCD_WIDTH, LCD_HEIGHT);
        if (ret != 0) {
            LOG_ERROR("MediaStreamConfigVideo failed, ret: %d \n", ret);
            isPlaying = false;
            isFirstOpen = true;     // 允许再次尝试
            return ret;
        }
    }



    st::sense_koks::Isp::GetInstance().WaitForBootDayNightJudge();


    m_cv.notify_one();
    return 0;
}

void LCDCameraPlayer::StopPlay()
{
    auto elapsed = std::chrono::steady_clock::now() - m_playStartTime;
    if (elapsed < std::chrono::milliseconds(200)) {
        LOG_INFO("StopPlay ignored: within 200ms cooldown.\n");
        return;
    }
    isPlaying = false;
}

#endif