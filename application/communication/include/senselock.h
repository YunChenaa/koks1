/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 23:54:28
 * @LastEditors: 洪照
 * @LastEditTime: 2026-01-29 18:19:40
 * @FilePath: /sense-koks/application/communication/include/senselock.h
 */
#pragma once
#include <stdint.h>
#include "koks_project_def.h"
// #include "os_task.h"
// #include "os_event.h"
// #include "host_interface.h"
#include "sense_control.h"
#include "fast_frame_info.h"

#ifdef KOKS_SDK_VERSION_V2
#include "st_palm_module.h"
#else
#include "st_face.h"
#include "st_face_data.h"
#include "st_face_module.h"
#endif
// #include "mb_msg.h"
// #include "file_utils.h"
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

//#define LOGDD_DEBUG
#ifdef LOGDD_DEBUG
#define LOGDD(fmt, args...)     LOGI(fmt, ##args)
#else
#define LOGDD(fmt, args...)
#endif
#define FAKE_DEVICE_ID "COBA138901240789321"
#define SERIALNUMBER_PATH "sn.bin"
#define FACE_USER_LIST "userlist.bin"
#define NOTE_FREQ_FAST
int st_module_callback_face_rect(st_image_t* image, const std::vector<st_rect_t>* rect);
#ifdef KOKS_SDK_VERSION_V2
typedef enum {
    FLG_FACE = 0x1,
    FLG_PALM = 0x2,
} EnrollUnlockFlag;

void draw_rc(uint8_t* yuv, int width, int height, int x, int y, int w, int h, int borderWidth);
void rc_scalar_internal(st_rect_t& rc, float left, float right, float top, float bottom);
void rc_scalar(st_image_t& img, st_rect_t& rc, float left, float right, float top, float bottom);
#endif


enum SenseCameraEveneType {
    SENSECAM_EVENT_IR_MAIN_READY = 0,
    SENSECAM_EVENT_IR_SCND_READY,
    SENSECAM_EVENT_RGB_READY,
};

// int sense_system(const char *cmd);
enum s_framestate {
    C_STATE_NO_FRAME = 0 << 0,
    C_STATE_IR1_FRAME = 1 << SENSECAM_EVENT_IR_MAIN_READY,
    C_STATE_IR2_FRAME = 1 << SENSECAM_EVENT_IR_SCND_READY,
    C_STATE_BINOCULAR_FRAME = C_STATE_IR1_FRAME | C_STATE_IR2_FRAME,
};

typedef struct _brightness_limit_info {
    double brightness_min;
    double brightness_max;
} brightness_limit_info_t;

#define IMAGE_STATE_READY C_STATE_BINOCULAR_FRAME
// single instance
class Senselock : public HostListener
{
    public:
    ~Senselock();
    static Senselock* getInstance();
    static void setUTCTime(uint32_t Second);
    s_status init(int width, int height);
    s_status deinit();
    s_status setConfig();
    s_status setHostProxy(HostProxy* host);
    uint8_t getState();
    s_status start();
    void earlyGetFrame();
    void earlyVerify();

    // HostListener
    virtual void onMsg(s_msg& msg);
    void reply(s_msg_id cmd, uint8_t result, void* data, uint16_t size, s_type type = s_type::TYPE_UART);
    void replyNoEncrypt(s_msg_id cmd, uint8_t result, void* data, uint16_t size, s_type type = s_type::TYPE_UART);
    void sendMessage(s_msg_id cmd, void* data, uint16_t size, s_type type = s_type::TYPE_UART);
    s_status sendImage(void* image, int size);
    void uart_msg_task(void* arg);
    int startCamera();
    int stopCamera();

    void process_ir_buffer(void);
    int releaseCameraBufs(void);
    int RequestHwCheck(void);
    int GetCameraFrame();
#ifdef KOKS_SDK_VERSION_V2
    int getFaceAeTarget(bool isIr, int enable, double brightness, int defTarget, EnrollUnlockFlag flag);
    int setCameraROI(st_rect_t* rc, st_rect_t* rgb, double brightness, EnrollUnlockFlag flag);
    int GetCameraFrame(st_image_t& img_rgb, st_image_t& img_ir, uint8_t face = 1);
    void switchEffectBin(bool isPalm = true);
    void setImageState(int state) {
        std::unique_lock<std::mutex> lock(mFrameQLock);
        mImageState = state;
    }
#else
    int setCameraROI(const std::vector<st_rect_t>* rect);
#endif

    public:
    std::list<s_msg*> mCmdQueue;
    std::mutex mFrameQLock;
    SenseControl* mHost;
    volatile uint8_t mState;
    uint8_t mPowerMode;
    long lastTimeStamp;
    unsigned int framesDiffMaxTime;
    int mImageState;
    uint8_t mDemoMode;  // '0' is normal mode, '1' is demo mode
    uint8_t mDebugMode; // '0' indicates disable, non-zero indicates enable
    uint8_t hardware_check{ 0 };
    static std::atomic<bool> mAbort;
    unsigned char* mRGBImageBuffer{ nullptr };
    unsigned int mRGBImageBufferSize;
    unsigned char* mPaddingBuffer{ nullptr };
    unsigned int mPaddingWidth;
    unsigned int mPaddingHeight;
    std::mutex UartMSGHandleMutex;
    std::condition_variable UartMSGHandle_cv;
    // os_event_h gEventFrameReadyHandle;
    st_module_image_t mImage;
    bool mLicenseFlag;
#ifdef KOKS_SDK_VERSION_V2
    int  mCnt { 0 };
#endif
    struct fast_frame_ev_info ev_info{ 0 };

#ifdef ENABLE_SAVE_LOG
    st_log_info* mLogInfo;
#endif
    volatile bool mCameraReady;

    public:
    uint8_t mEarlyVerifyState = 0; // 0: not start, 1: verifying, 2: done
    brightness_limit_info_t mFaceBrightnessLimit;
    brightness_limit_info_t mPalmBrightnessLimit;

    private:
    Senselock();
#ifdef KOKS_SDK_VERSION_V2
    void handleEnrollUnlock(s_msg* msg, st_module_detect_mode mode, int flg);
#endif
    std::vector<st_rect_t> mEarlyROI;
    std::once_flag mEarlyVerifyOnceFlag;
    int mIrRoiLuma;
    int mIrRoiTargetLuma;
    int mRgbRoiLuma;
    int mRgbRoiTargetLuma;
};
