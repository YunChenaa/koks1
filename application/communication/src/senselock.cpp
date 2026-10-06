/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 20:05:39
 * @LastEditors: 洪照
 * @LastEditTime: 2026-03-13 11:29:23
 * @FilePath: /sense-koks/application/communication/src/senselock.cpp
 */
#include "senselock.h"
#include "app_flash_otp.h"
#include "os_utils.h"
#include "partition_guard.h"
#include "senselock_handler.h"
#include "st_koks_isp.h"
#include "st_koks_led.h"
#include "koks_project_def.h"
#include "st_koks_mediamanager.h"
#include "su_base.h"
#include "sys_logger.h"
#include "updater.h"
#include "factory.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <sys/time.h>
#include <thread>
using namespace st::sense_koks;

#if KOKS_FEATURE_MAX_SENSOR_NUM == 3
#define ENROLL_SKIP_FRAMES 3
#else
#define ENROLL_SKIP_FRAMES 10
#endif
#define VERIFY_SKIP_FRAMES 1

#ifndef NOTE_FREQ_FAST
typedef struct {
    uint8_t message[MAX_NOTE_DATA_SIZE];
    uint16_t valid;
    uint32_t timestamp;
} last_note_message_info;

last_note_message_info last_note_msg = { 0 };
uint32_t MIN_REPEAT_NOTE_MSG_INTERVAL_MS = 3000; // 3 seconds
#endif

static unsigned char* upload_image_buf = nullptr;
#define APP_SN_SIZE 20

class RunTime
{
    public:
    RunTime(const char* fun_name) : s_time(0), e_time(0), name{ fun_name }
    {
        s_time = GetCurTimeMs();
        if (log == true)
            LOG_INFO("%s start!\n", name.c_str());c
    }

    RunTime(const char* fun_name, bool enable) : s_time(0), e_time(0), name{ fun_name }, log{ enable }
    {
        if (log == true)
            LOG_INFO("%s start!\n", name.c_str());
        s_time = GetCurTimeMs();
    }

    ~RunTime()
    {
        e_time = GetCurTimeMs();
        if (log == true)
            LOG_INFO("%s end %lld, %ld ms\n", name.c_str(), e_time, e_time - s_time);
    }

    private:
    std::string name;
    uint64_t s_time{ 0 };
    uint64_t e_time{ 0 };
    bool log{ false };
};

static bool is_note_drop(int16_t state, uint32_t ms_timeout = 1000) {
    static int16_t last_state = 0;
    static uint32_t last_ms = 0;
    uint32_t now = GetCurTimeMs();
    if (last_ms && last_state == state && GET_TIMEDIFF(last_ms, now) < ms_timeout) {
        LOGW("repeated message(%d), drop it", state);
        return true;
    }
    last_ms = now;
    last_state = state;
    return false;
}


std::atomic<bool> Senselock::mAbort{ false };
static Senselock* gInstance = NULL;
Senselock::Senselock()
        : mHost(NULL), mState(MS_INVALID), mPowerMode(PM_DEFAULT), lastTimeStamp(0), framesDiffMaxTime(0), mImageState(0), mDemoMode(0), mDebugMode(0), mRGBImageBuffer(NULL),
          mRGBImageBufferSize(0), mLicenseFlag(true), mCameraReady(false)
{
    LOG_DEBUG("in\n");
    mCmdQueue.clear();
#ifdef LOGDD_DEBUG
    auto isp_monitor = std::thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        while (1) {
            /*
            1，IMP_ISP_Tuning_GetFaceAeLuma  获取roi和global的值，主要是确认他是否在按照既定的roi  target 设定的在收敛
            2，IMP_ISP_Tuning_GetAeOnlyReadAttr  获取当前帧的ev值
            3，IMP_ISP_Tuning_GetISPRunningMode  获取当前ISP运行在哪个模式；
            4，IMP_ISP_Tuning_GetAeExprInfo  获取当前的ae模式*/
            IMPISPFaceAeLuma luma;
            IMPISPAeOnlyReadAttr ae_attr;
            IMPISPRunningMode mode;
            IMPISPAEExprInfo exprinfo;
            for (int i = 0; i < 2; i++) {
                IMP_ISP_Tuning_GetFaceAeLuma((IMPVI_NUM)i, &luma);
                IMP_ISP_Tuning_GetISPRunningMode((IMPVI_NUM)i, &mode);
                IMP_ISP_Tuning_GetAeOnlyReadAttr((IMPVI_NUM)i, &ae_attr);
                IMP_ISP_Tuning_GetAeExprInfo((IMPVI_NUM)i, &exprinfo);
                LOGDD("%d: mode=%d, luma.global=%d, luma.roi=%d, AeMode=%d, ae_mean=%d, bv=%d, ev=%lld, luma=%d, luma_scence=%d, target=%d, TotalGainDb=%d",
                    i, mode, luma.global, luma.roi, exprinfo.AeMode,
                    ae_attr.ae_mean, ae_attr.bv, ae_attr.ExposureValue, ae_attr.luma, ae_attr.luma_scence, ae_attr.target, ae_attr.TotalGainDb);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / SENSOR_FRAME_RATE_NUM));
        }
    });
    isp_monitor.detach();
#endif
}

Senselock::~Senselock()
{
    LOG_DEBUG("in\n");
}

s_status Senselock::init(int width, int height)
{
    mLicenseFlag = true;
    mCameraReady = true;
    mImage.height = height;
    mImage.width = width;
    mImage.orientation = ST_FACE_UP;
    mImage.first_pixel_format = ST_PIX_FMT_GRAY8;
    mImage.second_pixel_format = ST_PIX_FMT_NV12;
    mImage.first_image = new unsigned char[mImage.width * mImage.height * 3 / 2];
    mImage.second_image = new unsigned char[mImage.width * mImage.height * 3 / 2];
    mImage.second_stride = mImage.width;
    mImage.first_stride = mImage.width;
}


s_status Senselock::start()
{
    LOG_DEBUG("in\n");
    std::thread uart_msg_thread([this] { uart_msg_task(reinterpret_cast<void*>(this)); });
    uart_msg_thread.detach();
    mHost = SenseControl::getInstance();
    mHost->setListener(this);
    mState = MS_STANDBY;
#ifdef ENABLE_SAVE_LOG
    mLogInfo = slLoginfo;
#endif
    return OK;
}


void Senselock::setUTCTime(uint32_t Second)
{
    // 快速过滤掉无效的时间戳
    const uint32_t MIN_SECONDS = 1782864000;  // 2026-07-01 08:00:00
    if (Second < MIN_SECONDS) {
        LOG_ERROR("Invalid utc seconds %u\n", Second);
        return;
    }

    SUTime time;
    time_t seconds = static_cast<time_t>(Second);
    struct tm* ptm;

    // 将秒数转换为 tm 结构体
    ptm = gmtime(&seconds); // 使用 gmtime 来获取 UTC 时间

    // 将 tm 结构体的值赋给 SUTime 结构体
    time.sec = ptm->tm_sec;          // 秒数，范围：0~59
    time.min = ptm->tm_min;          // 分钟数，范围：0~59
    time.hour = ptm->tm_hour;        // 小时数，范围：0~23
    time.mday = ptm->tm_mday;        // 一个月中的第几天，范围：1~31
    time.mon = ptm->tm_mon + 1;      // 月份，范围：1~12（tm_mon 是从 0 开始的）
    time.year = ptm->tm_year + 1900; // 年份，范围：>1900

    // 调用设置时间的函数
    // SU_Base_SetTime(&time);
    LOG_INFO("Set UTC time: %04d-%02d-%02d %02d:%02d:%02d\n", time.year, time.mon, time.mday, time.hour, time.min, time.sec);
    // 这里可以添加调用设置时间的函数，不能使用 SU_Base_SetTime(&time);建议使用操作系统自带的
    struct timeval tv;
    tv.tv_sec = seconds; // 设置秒数
    tv.tv_usec = 0;      // 设置微秒数为 0
    if (settimeofday(&tv, NULL) < 0) {
        LOG_ERROR("Failed to set UTC time: %s\n", strerror(errno));
    } else {
        LOG_INFO("UTC time set successfully.\n");
    }
}

void Senselock::reply(s_msg_id cmd, uint8_t result, void* data, uint16_t size, s_type type)
{
    uint8_t reply_buff[MAX_REPLY_DATA_SIZE];
    s_msg_reply_data* rdata = (s_msg_reply_data*)reply_buff;

    CLEAR(reply_buff);
    uint16_t dsize = 0;
    rdata->mid = cmd;
    if (mLicenseFlag == true) {
        rdata->result = result;
        if (size && data != NULL) {
            memcpy(rdata->data, data, size);
        }
        dsize = sizeof(s_msg_reply_data) + size;
    } else {

        rdata->result = MR_FAILED4_AUTHORIZATION;
        dsize = sizeof(s_msg_reply_data) + 0;
    }
    mHost->sendReply(rdata, dsize, type);
#ifndef NOTE_FREQ_FAST
    last_note_msg.valid = 0;
#endif
    LOG_DEBUG("[REPLY]MID:0x%x result:%d\n", rdata->mid, rdata->result);
    return;
}

void Senselock::replyNoEncrypt(s_msg_id cmd, uint8_t result, void* data, uint16_t size, s_type type){
    LOG_INFO("replyNoEncrypt cmd:0x%x, result:%d\n", cmd, result);
    uint8_t reply_buff[MAX_REPLY_DATA_SIZE];
    s_msg_reply_data* rdata = (s_msg_reply_data*)reply_buff;

    CLEAR(reply_buff);
    uint16_t dsize = 0;
    rdata->mid = cmd;
    if (mLicenseFlag == true) {
        rdata->result = result;
        if (size && data != NULL) {
            memcpy(rdata->data, data, size);
        }
        dsize = sizeof(s_msg_reply_data) + size;
    } else {

        rdata->result = MR_FAILED4_AUTHORIZATION;
        dsize = sizeof(s_msg_reply_data) + 0;
    }
    mHost->sendReplyNoEncrypt(rdata, dsize, type);

#ifndef NOTE_FREQ_FAST
    last_note_msg.valid = 0;
#endif
    LOG_DEBUG("[REPLY]MID:0x%x result:%d\n", rdata->mid, rdata->result);
    return;
}

void Senselock::sendMessage(s_msg_id cmd, void* data, uint16_t size, s_type type)
{
    uint8_t message_buff[MAX_NOTE_DATA_SIZE];
    s_msg_note_data* ndata = (s_msg_note_data*)message_buff;
    assert(size < MAX_NOTE_DATA_SIZE - sizeof(s_msg_note_data));
    CLEAR(message_buff);
    uint16_t dsize = 0;
    if (cmd != NID_READY) {
    }
    if (mLicenseFlag == true || cmd == NID_READY) {
        ndata->nid = cmd;
        if (size && data != NULL) {
            memcpy(ndata->data, data, size);
        }
        dsize = sizeof(s_msg_note_data) + size;
    } else {
        ndata->nid = NID_AUTHORIZATION;
        dsize = sizeof(s_msg_note_data) + 0;
    }

#ifndef NOTE_FREQ_FAST
    if (cmd == NID_FACE_STATE && last_note_msg.valid) {
        if (GET_TIMEDIFF(last_note_msg.timestamp, GetCurTimeMs()) < MIN_REPEAT_NOTE_MSG_INTERVAL_MS) {
            // drop the repeated NID_FACE_STATE
            if (((s_note_data_face*)(data))->state == ((s_note_data_face*)(&last_note_msg))->state) {
                LOG_WARNING("repeated message, drop it\n");
                return;
            }
        }
    }
#endif

    mHost->sendMsg(ndata, dsize, type);

#ifndef NOTE_FREQ_FAST
    if (cmd == NID_FACE_STATE) {
        // save the cmd and message
        memcpy(last_note_msg.message, data, size);
        last_note_msg.valid = 1;
        last_note_msg.timestamp = GetCurTimeMs();
    }
#endif

    LOGD("[NOTE]NID:0x%x", ndata->nid);
}

s_status Senselock::sendImage(void* image, int size)
{
    if (size > MAX_SEND_IMAGE_SIZE) {
        return INVALID_PARAM;
    }
    if (mLicenseFlag == true) {
        mHost->sendImg((unsigned char*)image, size);
    } else {
        mHost->sendImg(NULL, 0);
    }

    LOG_VERBOSE("send image down");
    return OK;
}

void Senselock::onMsg(s_msg& msg)
{
    LOG_DEBUG("cmd %x,msg size %d\n", msg.mid, (msg.size_heb << 8) | msg.size_leb);
    if (msg.mid == MID_FACTORY) {
        uint8_t req[256] = {0};
        uint8_t ack[256] = {0};
        int len = (msg.size_heb << 8) | msg.size_leb;
        memcpy(req, msg.data, len);
        len = factory_process((factory_req_t*)req, (factory_ack_t*)ack);
        if (len >= 0) {
            reply(msg.mid, MR_SUCCESS, ack, len);
        }
        return;
    }
    s_msg* msg_cp = (s_msg*)malloc(sizeof(s_msg) + ((msg.size_heb << 8) | msg.size_leb));
    memcpy(msg_cp, &msg, sizeof(s_msg) + ((msg.size_heb << 8) | msg.size_leb));
    // check state
    if (mState == MS_INVALID && msg.mid != MID_FACTORY_TEST) {
        LOG_ERROR("received cmd %d in invalid lock state. fixme!\n", msg.mid);
        // reply a message
        reply(msg.mid, MR_REJECTED, NULL, 0);
        free(msg_cp);
        return;
    }

    if (mState == MS_OTA && (msg.mid != MID_STOP_OTA) && (msg.mid != MID_START_OTA) && (msg.mid != MID_GET_OTA_STATUS) && (msg.mid != MID_OTA_HEADER) &&
        (msg.mid != MID_OTA_PACKET) && (msg.mid != MID_CONFIG_BAUDRATE)) {
        LOG_WARNING("Module is working on OTA process!\n");
        reply(msg.mid, MR_REJECTED, NULL, 0);
        free(msg_cp);
        return;
    }

#ifdef KOKS_FEATURE_KDS
    // check the encrypt key
    if (msg.mid == MID_SET_DEBUG_ENC_KEY) {
        LOG_INFO("Set debug ENC KEY!\n");
        s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg.data;
        if (mHost->setKeyNumber(data->enc_key_number, DEBUG_ENC_KEY, false) == OK) {
            reply(msg.mid, MR_SUCCESS, NULL, 0);
        } else {
            reply(msg.mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
        free(msg_cp);
        return;
    }
    // check the encrypt key
    if (msg.mid == MID_SET_RELEASE_ENC_KEY) {
        LOG_INFO("Set release ENC KEY!\n");
        s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg.data;
        s_status ret = mHost->setKeyNumber(data->enc_key_number, RELEASE_ENC_KEY, false);
        if (ret == OK) {
            reply(msg.mid, MR_SUCCESS, NULL, 0);
        } else if (ret == EXCEED_LIMITATION) {
            reply(msg.mid, MR_REJECTED, NULL, 0);
        } else {
            reply(msg.mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
        free(msg_cp);
        return;
    }
#else //KOKS_FEATURE_KDS
#ifndef KOKS_FEATURE_RELEASE_ENC_KEY
    // check the encrypt key
    if (msg.mid == MID_SET_DEBUG_ENC_KEY) {
        LOG_INFO("Set release ENC KEY!\n");
        s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg.data;
        s_status ret = mHost->setKeyNumber(data->enc_key_number, RELEASE_ENC_KEY, true);
        if (ret == OK) {
            reply(msg.mid, MR_SUCCESS, NULL, 0);
        } else if (ret == EXCEED_LIMITATION) {
            reply(msg.mid, MR_REJECTED, NULL, 0);
        } else {
            reply(msg.mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
        free(msg_cp);
        return;
    }
#else // KOKS_FEATURE_RELEASE_ENC_KEY
    // check the encrypt key
    if (msg.mid == MID_SET_DEBUG_ENC_KEY) {
        LOG_INFO("Set debug ENC KEY!\n");
        s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg.data;
        if (mHost->setKeyNumber(data->enc_key_number, DEBUG_ENC_KEY, false) == OK) {
            reply(msg.mid, MR_SUCCESS, NULL, 0);
        } else {
            reply(msg.mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
        free(msg_cp);
        return;
    }

    // check the encrypt key
    if (msg.mid == MID_SET_RELEASE_ENC_KEY) {
        LOG_INFO("Set release ENC KEY!\n");
        s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg.data;
        s_status ret = mHost->setKeyNumber(data->enc_key_number, RELEASE_ENC_KEY, false);
        if (ret == OK) {
            reply(msg.mid, MR_SUCCESS, NULL, 0);
        } else if (ret == EXCEED_LIMITATION) {
            reply(msg.mid, MR_REJECTED, NULL, 0);
        } else {
            reply(msg.mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
        free(msg_cp);
        return;
    }
#endif // KOKS_FEATURE_RELEASE_ENC_KEY
#endif // KOKS_FEATURE_KDS

    /**
     * It's a special message which suppose to be the first message from HOST
     * to initialize everything. For considering the compatibility we must pay
     * attention to the data size. Use the data only when we received enough
     * size. Othewise it will crash.
     **/
#define OFFSET(TYPE, MEMBER) ((unsigned long)(&(((TYPE*)0)->MEMBER)))

    // check if starting encrypted communication
    if (msg.mid == MID_INIT_ENCRYPTION) {
        s_msg_init_encryption_data_v2* data = (s_msg_init_encryption_data_v2*)msg.data;
        int data_size = sizeof(s_msg_init_encryption_data_v2);
        int real_data_size = msg.size_heb << 8 | msg.size_leb;
        LOG_INFO("Starting encryption mode!\n");

        if (mHost->setKey(data->seed, 4) == OK) {
            if (real_data_size > OFFSET(s_msg_init_encryption_data_v2, mode)) {
                if (ST_ENCMODE_DEFAULT == (EncMode)data->mode) {
                    mHost->setEncryptionMode(ST_ENCMODE_AES);
                } else {
                    mHost->setEncryptionMode((EncMode)data->mode);
                }
            } else {
                mHost->setEncryptionMode(ST_ENCMODE_AES);
            }
        }

        s_msg_reply_init_encryption_data d;
        CLEAR(d);
        char sn_buffer[APP_SN_SIZE] = { 0 };
        auto app_flash_read = [&sn_buffer](char* data) -> bool { return app_flash_read_sn(reinterpret_cast<uint8_t*>(sn_buffer)); };
        int ret = app_flash_read(sn_buffer); //(OTP_APP_ID0, APP_SN_OFFSET, (uint8_t*)sn_buffer, APP_SN_SIZE);
        if (ret == true) {
            LOG_DEBUG("sn %s\n", sn_buffer);
            if (strlen(sn_buffer) >= 17 && strlen(sn_buffer) <= 20) {
                memcpy(d.device_id, sn_buffer, strlen(sn_buffer));
            } else {
                memcpy(d.device_id, FAKE_DEVICE_ID, sizeof(FAKE_DEVICE_ID));
            }
        } else {
            LOG_ERROR("Failed to read sn\n");
            memcpy(d.device_id, FAKE_DEVICE_ID, sizeof(FAKE_DEVICE_ID));
        }

        if (real_data_size > OFFSET(s_msg_init_encryption_data_v2, crttime)) {
            // sync the host time if valid
            uint32_t second_elapse = data->crttime[0] << 24 | data->crttime[1] << 16 | data->crttime[2] << 8 | data->crttime[3];
            setUTCTime(second_elapse);
        }

        reply(msg.mid, MR_SUCCESS, &d, sizeof(s_msg_reply_init_encryption_data));
        free(msg_cp);
        return;
    }

    // check some msgs don't have to get into thread loop
    if (msg.mid == MID_GETSTATUS) {
        LOG_DEBUG("handle msg MID_GETSTATUS\n");
        s_msg_reply_getstatus_data d;
        d.status = mState;
        reply(msg.mid, MR_SUCCESS, &d, sizeof(s_msg_reply_getstatus_data));
        free(msg_cp);
        return;
    }

    if (msg.mid == MID_RESTORE_FACTORY_SETTINGS) {
        LOG_DEBUG("restore factory settins!\n");
        SenselockHandler* sLock_handler = SenselockHandler::getInstance();
        sLock_handler->restore_factory_settings(msg_cp);
        free(msg_cp);
        return;
    }

#ifdef ENCRYPT_REQUIRED
    if (!mHost->getEncryptionMode() && (mState != MS_OTA)) {
        reply(msg.mid, MR_FAILED4_NO_ENCRYPT, NULL, 0);
        free(msg_cp);
        return;
    }
#endif

    if (msg.mid == MID_STOP_TEST && mState == MS_BUSY) {
        mAbort = true;
        LOG_ERROR("abort %d state!\n", mState);
    }

    if (msg.mid == MID_RESET && mState == MS_BUSY) {
        mAbort = true;
        LOG_ERROR("abort %d state!\n", mState);
    }
    // enqueue the message to main loop
    std::unique_lock<std::mutex> lk(UartMSGHandleMutex);
    mCmdQueue.push_back(msg_cp);
    UartMSGHandle_cv.notify_one();
    lk.unlock();
}

Senselock* Senselock::getInstance()
{
    if (gInstance == NULL)
        gInstance = new Senselock();

    return gInstance;
}

int Senselock::startCamera()
{
    LOG_INFO("\n");
    return 0;
}

int Senselock::stopCamera()
{
    // LOG_INFO("\n");
    // Led::GetInstance().turnOff();
    return 0;
}

void Senselock::earlyGetFrame()
{
    auto ret = GetFastFrame(mImage.first_image, mImage.second_image, mImage.height, mImage.width, &ev_info);
    if (ret != 0) {
        mEarlyVerifyState = 2;
    }
}

void Senselock::earlyVerify()
{
    std::call_once(mEarlyVerifyOnceFlag, [this]() {
        if (mEarlyVerifyState == 0) {
            mEarlyVerifyState = 1; // verifying
            if (mState != MS_INVALID) {
                SenselockHandler* sLock_handler = SenselockHandler::getInstance();
                sLock_handler->earlyVerify();
            }
            mEarlyVerifyState = 2;
        }
    });
}

static int rgbInvalidCount = 0;
#ifdef KOKS_SDK_VERSION_V2
static st_rect_t rc_big, rc_ae;
void draw_rc(uint8_t* yuv, int width, int height, int x, int y, int w, int h, int borderWidth) {
    uint8_t yValue = 255;
    uint8_t uValue = 0;
    uint8_t vValue = 255;

    if (x + w + borderWidth > width) w = width - x - borderWidth;
    if (y + h + borderWidth > height) h = height - y - borderWidth;
    for (int i = 0; i < borderWidth; i++) {
        for (int col = x; col < x + w; col++) {
            yuv[(y + i) * width + col] = yValue;
            yuv[(y + h - 1 - i) * width + col] = yValue;
        }
        for (int row = y; row < y + h; row++) {
            yuv[row * width + (x + i)] = yValue;
            yuv[row * width + (x + w - 1 - i)] = yValue;
        }
    }

    int uvWidth = width;
    int uvHeight = height / 2;
    int uvX = x / 2 * 2;
    int uvY = y / 2;
    int uvW = (w + 1) / 2 * 2;
    int uvH = h / 2;
    yuv += width * height;

    uvX = std::max(0, std::min(uvX, width - 2));
    uvY = std::max(0, std::min(uvY, uvHeight - 1));
    uvW = std::max(2, std::min(uvW, width - uvX));
    uvH = std::max(1, std::min(uvH, uvHeight - uvY));

    int uvBorderWidth = std::max(1, borderWidth / 2);
    int idx;

    for (int i = 0; i < uvBorderWidth; i++) {
        for (int col = uvX; col < uvX + uvW; col += 2) {
            int topRow = uvY + i;
            int bottomRow = uvY + uvH - 1 - i;
            if (topRow < uvHeight) {
                idx = topRow * uvWidth + col;
                yuv[idx] = uValue;
                yuv[idx + 1] = vValue;
            }
            if (bottomRow >= 0 && bottomRow < uvHeight) {
                idx = bottomRow * uvWidth + col;
                yuv[idx] = uValue;
                yuv[idx + 1] = vValue;
            }
        }

        for (int row = uvY; row < uvY + uvH; row++) {
            int leftCol = uvX + i * 2;
            int rightCol = uvX + uvW - 2 - i * 2;
            if (leftCol >= 0 && leftCol < uvWidth) {
                idx = row * uvWidth + leftCol;
                yuv[idx] = uValue;
                yuv[idx + 1] = vValue;
            }
            if (rightCol >= 0 && rightCol < uvWidth) {
                idx = row * uvWidth + rightCol;
                yuv[idx] = uValue;
                yuv[idx + 1] = vValue;
            }
        }
    }
}

static int sum_row(const uint8_t* yuv, uint16_t width, uint16_t top, uint16_t left, uint16_t right) {
    int ret = 0;
    yuv += width * top;
    while (left < right) ret += yuv[left++];
    return ret;
}

static int sum_col(const uint8_t* yuv, uint16_t width, uint16_t col, uint16_t top, uint16_t bottom) {
    int ret = 0;
    yuv += col;
    while (top < bottom) {
        ret += yuv[width * top];
        top++;
    }
    return ret;
}

void rc_scalar_internal(st_rect_t& rc, float left, float right, float top, float bottom) {
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    rc.left += w * left;
    rc.right += w * right;
    rc.top += h * top;
    rc.bottom += h * bottom;
}

static int per_col(st_image_t& img, st_rect_t& rc) {
    int l = 1, r = 1;
    for (int i = 0; i < 6; i += 2) {
        l += sum_col(img.data, img.width, rc.left + i, rc.top, rc.bottom);
        r += sum_col(img.data, img.width, rc.right - i, rc.top, rc.bottom);
    }
    return l * 100 / r;
}


static int per_row(st_image_t& img, st_rect_t& rc) {
    int t = 1, b = 1;
    for (int i = 0; i < 6; i += 2) {
        t += sum_row(img.data, img.width, rc.top + i, rc.left, rc.right);
        b += sum_row(img.data, img.width, rc.bottom - i, rc.left, rc.right);
    }
    return t * 100 / b;
}

static void rc_move(st_image_t& img, st_rect_t& rc, float per) {
    int per_h = per_row(img, rc);
    int per_w = per_col(img, rc);
    int dw = (rc.right - rc.left) * per;
    int dh = (rc.bottom - rc.top) * per;
    if (per_h > 105) dh = -dh;
    else if (per_h > 95) dh = 0;
    if (per_w > 105) dw = -dw;
    else if (per_h > 95) dw = 0;
    //LOGI("per_h=%.3f, per_w=%.3f, dw=%d, dh=%d", per_h, per_w, dw, dh);
    rc.left += dw;
    rc.right += dw;
    rc.top += dh;
    rc.bottom += dh;
}

void rc_scalar(st_image_t& img, st_rect_t& rc, float left, float right, float top, float bottom) {
    rc_scalar_internal(rc, left, right, top, bottom);
    float per = 0.6f;
    while (per > 0.15f) {
        rc_move(img, rc, per);
        per /= 2;
    }
}

int Senselock::GetCameraFrame(st_image_t& img_rgb, st_image_t& img_ir, uint8_t face) {
    IMPFrameInfo fr;
    auto& media_manager = MediaManager::GetInstance();
    int dt = 0;
    const int DT_S2 = 1000 / Isp::GetInstance().GetSensorFrameRate();
#ifdef KOKS_FEATURE_NOAESYNC
    static int cnt = 0;
    if (cnt++ == 0) {
        Isp::GetInstance().setAEMode(0);
        Isp::GetInstance().setAEMode(1);
    }
#endif

    do {
        // frame sync
        //if (dt) os_delay(2); // no delay because SnapFrame cost enough time
        auto startTime = std::chrono::high_resolution_clock::now();
        if ((!dt || dt < -DT_S2) || face == 0) {
            //只有刷脸才需要考虑取帧同步， 刷掌不需要取次摄的图像帧
            IMPISPFaceAeLuma luma;
            IMPISPAeOnlyReadAttr ae_attr;
            IMP_ISP_Tuning_GetFaceAeLuma((IMPVI_NUM)0, &luma);
            mIrRoiLuma = luma.roi;
            IMP_ISP_Tuning_GetAeOnlyReadAttr((IMPVI_NUM)0, &ae_attr);
            mIrRoiTargetLuma = ae_attr.target;

            #if 1
            IMPISPAEExprInfo ei;
            IMP_ISP_Tuning_GetAeExprInfo(static_cast<IMPVI_NUM>(0), &ei);
            LOGI("AeIntegrationTime=%d, AeAGain=%d, AeIspDGain=%d, AeMode = %d\n%d: luma.global=%d, luma.roi=%d, ev=%lld, luma=%d, luma_scence=%d, target=%d, TotalGainDb=%d, %lld",
                ei.AeIntegrationTime, ei.AeAGain, ei.AeIspDGain, ei.AeMode,
                mCnt, luma.global, luma.roi, ae_attr.ExposureValue, ae_attr.luma, ae_attr.luma_scence, ae_attr.target, ae_attr.TotalGainDb, img_ir.timestamp);
            #endif
            startTime = std::chrono::high_resolution_clock::now();
            media_manager.GetDeviceFrameSource(0)->SnapFrame(mImage.first_image, &fr);
            img_ir.data = mImage.first_image;
            img_ir.width = mImage.width;
            img_ir.height = mImage.height;
            img_ir.stride = mImage.first_stride;
            img_ir.pixel_format = mImage.first_pixel_format;
            img_ir.timestamp = fr.timeStamp;
        }

        if ((!dt || dt > DT_S2) && face == 1) {
            startTime = std::chrono::high_resolution_clock::now();
        #ifdef KOKS_FEATURE_NOAESYNC
            IMPISPFaceAeLuma luma;
            IMPISPAeOnlyReadAttr ae_attr;
            IMP_ISP_Tuning_GetFaceAeLuma((IMPVI_NUM)1, &luma);
            mRgbRoiLuma = luma.roi;
            IMP_ISP_Tuning_GetAeOnlyReadAttr((IMPVI_NUM)1, &ae_attr);
            mRgbRoiTargetLuma = ae_attr.target;
        #endif
            media_manager.GetDeviceFrameSource(1)->SnapFrame(mImage.second_image, &fr);
            img_rgb.data = mImage.second_image;
            img_rgb.width = mImage.width;
            img_rgb.height = mImage.height;
            img_rgb.stride = mImage.second_stride;
            img_rgb.pixel_format = mImage.second_pixel_format;
            img_rgb.timestamp = fr.timeStamp;
            auto endTime = std::chrono::high_resolution_clock::now();
            // dt = img_ir.timestamp - img_rgb.timestamp; //只有刷脸才需要考虑取帧同步， 刷掌不需要取次摄的图像帧
            dt = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        }
    } while(0);
    return 0;
}

void Senselock::switchEffectBin(bool isPalm) {
    const char PALM_BIN_PATH[] = "/etc/sensor/ov02b10-PRJ008-palm.bin";
    const char FACE_BIN_PATH[] = "/tmp/ov02f-PRJ008-night.bin";
    static bool s_face = true;
    if(isPalm) {
        if (s_face && access(PALM_BIN_PATH, F_OK) == 0) {
            int ret = 0;
            IMPISPBinAttr attr;
            attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
            strcpy(attr.bname, PALM_BIN_PATH);
            attr.mode = IMPISP_BIN_MODE_NIGHT;
            LOGI("IMP_ISP_Tuning_SwitchBin palm begin %s", attr.bname);
            ret = IMP_ISP_Tuning_SwitchBin(IMPVI_MAIN, &attr);
            LOGI("IMP_ISP_Tuning_SwitchBin palm end ret = %d, %s", ret, attr.bname);
            s_face = false;
        }
    }
    else {
        if (!s_face && access(FACE_BIN_PATH, F_OK) == 0) {
            int ret = 0;
            IMPISPBinAttr attr;
            attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
            strcpy(attr.bname, FACE_BIN_PATH);
            attr.mode = IMPISP_BIN_MODE_NIGHT;
            LOGI("IMP_ISP_Tuning_SwitchBin face begin %s", attr.bname);
            ret |= IMP_ISP_Tuning_SwitchBin(IMPVI_MAIN, &attr);
            LOGI("IMP_ISP_Tuning_SwitchBin face end ret = %d, %s", ret, attr.bname);
            s_face = true;
        }
    }
}

void Senselock::handleEnrollUnlock(s_msg* msg, st_module_detect_mode mode, int flg) {
    LOGI("handleEnrollUnlock entry");
    Isp::GetInstance().setFaceAe(true);
    uint64_t t0 = GetCurTimeMs();
    mState = MS_BUSY;
    if(mode == ST_MODULE_DETECT_MODE_UNLOCK) {
         if(SenselockHandler::getInstance()->checkEarlyVerifyResult(msg, flg) == 0) {
            return;
        }
    }

    auto& media_manager = MediaManager::GetInstance();
    if (flg == EnrollUnlockFlag::FLG_PALM) {
            switchEffectBin(true);
    }
    else if (flg & EnrollUnlockFlag::FLG_FACE) {
        st_face_module_set_enable_liveness(1);
        switchEffectBin(false);
    }

    st::sense_koks::LedGuard led_guard;
    const int SKIP_FRAME_NUM = mode == ST_MODULE_DETECT_MODE_UNLOCK ? 2 : 5;
    IMPFrameInfo rgb_frame, ir_frame;
    int imageSize = mImage.width * mImage.height * 3 / 2;
    std::vector<unsigned char> image_buffer(imageSize);
    for (int i = 0; i < SKIP_FRAME_NUM; ++i) {
        LOG_WARNING("skip frame %d\n", i);
        media_manager.GetDeviceFrameSource(1)->SnapFrame(image_buffer.data(), &rgb_frame);
        media_manager.GetDeviceFrameSource(0)->SnapFrame(image_buffer.data(), &ir_frame);
    }
    image_buffer.clear();
#ifndef KOKS_FEATURE_NOAESYNC
    IMPISPAEExprInfo exprinfo;
    IMP_ISP_Tuning_GetAeExprInfo((IMPVI_NUM)0, &exprinfo);
    LOGI("Ir led opened, set AE mode to auto");
    exprinfo.AeMode = IMPISP_TUNING_OPS_TYPE_AUTO;
    IMP_ISP_Tuning_SetAeExprInfo((IMPVI_NUM)0, &exprinfo);
#endif
    int fps = (flg & EnrollUnlockFlag::FLG_FACE) == EnrollUnlockFlag::FLG_FACE ? 15 : 30;
    Isp::GetInstance().SetSensorFrameRate(fps);
    SenselockHandler::getInstance()->handleEnrollUnlock(msg, mode, flg);
    mState = MS_STANDBY;
    Isp::GetInstance().setFaceAe(false);
#ifndef KOKS_FEATURE_NOAESYNC
    LOGI("Before close IrLed, restore AE mode to manual");
    IMP_ISP_Tuning_GetAeExprInfo((IMPVI_NUM)0, &exprinfo);
    exprinfo.AeMode = IMPISP_TUNING_OPS_TYPE_MANUAL;
    IMP_ISP_Tuning_SetAeExprInfo((IMPVI_NUM)0, &exprinfo);
#else
    switchEffectBin(false);
#endif
    LOGI("handleEnrollUnlock exit, elapse=%dms, mid=0x%X, %d", (uint32_t)(GetCurTimeMs() - t0), msg->mid, mCnt);
}

int Senselock::getFaceAeTarget(bool isIr, int enable, double brightness, int defTarget, EnrollUnlockFlag flag)
{
#ifdef KOKS_DYNAMIC_TARGET
    if(enable == IMPISP_TUNING_OPS_MODE_DISABLE) {
        return defTarget;
    }

    int lumaDiff = 0, roiTargetLuma = 0, roiLuma = 0;
    double max = 0, min = 0;
    if(isIr) {
        roiTargetLuma = mIrRoiTargetLuma;
        roiLuma = mIrRoiLuma;
    } else {
        roiTargetLuma = mRgbRoiTargetLuma;
        roiLuma = mRgbRoiLuma;
    }

    if(roiTargetLuma == 10) {
        return defTarget;
    }

    if(flag == FLG_FACE) {
        max = mFaceBrightnessLimit.brightness_max;
        min = mFaceBrightnessLimit.brightness_min;
    } else if(flag == FLG_PALM) {
        max = mPalmBrightnessLimit.brightness_max;
        min = mPalmBrightnessLimit.brightness_min;
    }

    lumaDiff = roiTargetLuma > roiLuma ? roiTargetLuma - roiLuma : roiLuma - roiTargetLuma;
    if(lumaDiff > 5) {
        return roiTargetLuma;
    }

    if(brightness >= max ) {
        return (roiTargetLuma > 24 ? roiTargetLuma - 8 : 16);
    } else if(brightness <= min ) {
        return (roiTargetLuma + 8);
    }

    return roiTargetLuma;
#else
    return defTarget;
#endif
}

int Senselock::setCameraROI(st_rect_t* ir_rc, st_rect_t* rgb_rc, double brightness, EnrollUnlockFlag flag)
{
    auto& isp = Isp::GetInstance();

    bool ir_rect_invalid = (ir_rc->left == 0 && ir_rc->right == 0 && ir_rc->top == 0 && ir_rc->bottom == 0);
    bool rgb_rect_invalid = (rgb_rc->left == 0 && rgb_rc->right == 0 && rgb_rc->top == 0 && rgb_rc->bottom == 0);

    if (ir_rect_invalid && rgb_rect_invalid) {
        LOG_WARNING("ir and rgb rect invalid\n");
        return 0;
    }
    else if (ir_rect_invalid) {
        LOG_WARNING("ir rect invalid, use rgb rect\n");
        ir_rc = rgb_rc;
    }
    else if (rgb_rect_invalid) {
        LOG_WARNING("rgb rect invalid, use ir rect\n");
        rgb_rc = ir_rc;
        rgbInvalidCount++;
    }

    auto raw_rect_rotate = [this](st_rect_t* rect, int rotate) -> st_rect_t {
        switch (rotate) {
            case 90:
                return { rect->top, mImage.width - rect->right, rect->bottom, mImage.width - rect->left };
            case 180:
                return { mImage.width - rect->right, mImage.height - rect->bottom, mImage.width - rect->left, mImage.height - rect->top };
            case 270:
                return { mImage.height - rect->bottom, rect->left, mImage.height - rect->top, rect->right };
            default:
                return { rect->left, rect->top, rect->right, rect->bottom };
        }
    };

    auto raw_rect_uncrop = [](st_rect_t* rect, int left, int top, int right, int bottom) -> st_rect_t {
        return { rect->left + left, rect->top + top, rect->right + left, rect->bottom + top };
    };

    if (ir_rc) {
        IMPISPFaceAttr ir_attr;
        isp.GetFaceAE(0, &ir_attr);
        auto ir_config = MediaManager::GetInstance().GetDeviceFrameSource(0)->GetChnConfig();
        auto angle = koks_config.media.video.frame_source[0].input._2d.rotate.angle;
        auto rect = raw_rect_rotate(ir_rc, angle);
        if(ir_config->fs_chn_attr.crop.enable) {
            rect = raw_rect_uncrop(&rect, ir_config->fs_chn_attr.crop.left, ir_config->fs_chn_attr.crop.top, ir_config->fs_chn_attr.crop.left, ir_config->fs_chn_attr.crop.top);
        }

        if(ir_config->fs_chn_attr.scaler.enable) {
            const float scale_x = SENSOR0_WIDTH * 1.0f / ir_config->fs_chn_attr.scaler.outwidth;
            const float scale_y = SENSOR0_HEIGHT * 1.0f / ir_config->fs_chn_attr.scaler.outheight;
            ir_attr.left = rect.left * scale_x;
            ir_attr.top = rect.top * scale_y;
            ir_attr.right = rect.right * scale_x;
            ir_attr.bottom = rect.bottom * scale_y;
        }
        unsigned int defTarget = flag == FLG_FACE ? (unsigned int)isp.GetAEConfig(0).face_ae_target : (unsigned int)isp.GetAEConfig(0).palm_ae_target;
        ir_attr.target = getFaceAeTarget(true, ir_attr.enable, brightness, defTarget, flag);
        LOGI("ir rect[%d %d, %d %d] -> [%d %d, %d %d], ir_attr.enable = %d, target = %d", ir_rc->left, ir_rc->top, ir_rc->right, ir_rc->bottom, ir_attr.left, ir_attr.top, ir_attr.right, ir_attr.bottom, ir_attr.enable, ir_attr.target);
        ir_attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
        isp.SetFaceAE(0, ir_attr);
    }
#ifdef KOKS_FEATURE_NOAESYNC
#if 1
    int lowEV = 1000;
    int highEV = 10000;
    IMPISPAeOnlyReadAttr ae_attr1;
    IMPISPRunningMode pmode;
    IMP_ISP_Tuning_GetISPRunningMode((IMPVI_NUM)1, &pmode);
    auto ret = IMP_ISP_Tuning_GetAeOnlyReadAttr((IMPVI_NUM)1, &ae_attr1);
    // LOG_INFO("RGB EV is %lld \n", ae_attr1.ExposureValue);
    if (ret == ST_ERRNO_SUCCESS && pmode == IMPISP_RUNNING_MODE_DAY && ae_attr1.ExposureValue > lowEV && ae_attr1.ExposureValue < highEV && rgbInvalidCount < 3) {
        LOG_INFO("RGB EV is %lld , switch to globalAE\n", ae_attr1.ExposureValue);
        isp.CloseFaceAe(1);
        return 0;
    }
#endif
    if(rgb_rc && rgb_rc->right > 0) {
        IMPISPFaceAttr rgb_attr;
        auto rgb_config = MediaManager::GetInstance().GetDeviceFrameSource(1)->GetChnConfig();
        auto angle = koks_config.media.video.frame_source[1].input._2d.rotate.angle;
        auto rect = raw_rect_rotate(rgb_rc, angle);
        if(rgb_config->fs_chn_attr.crop.enable) {
            rect = raw_rect_uncrop(&rect, rgb_config->fs_chn_attr.crop.left, rgb_config->fs_chn_attr.crop.top, rgb_config->fs_chn_attr.crop.left, rgb_config->fs_chn_attr.crop.top);
        }
        if(rgb_config->fs_chn_attr.scaler.enable) {
            const float scale_x = SENSOR1_WIDTH * 1.0f / rgb_config->fs_chn_attr.scaler.outwidth;
            const float scale_y = SENSOR1_HEIGHT * 1.0f / rgb_config->fs_chn_attr.scaler.outheight;
            rgb_attr.left = rect.left * scale_x;
            rgb_attr.top = rect.top * scale_y;
            rgb_attr.right = rect.right * scale_x;
            rgb_attr.bottom = rect.bottom * scale_y;
        }
        unsigned int defTarget = (unsigned int)isp.GetAEConfig(1).face_ae_target;
        rgb_attr.target = getFaceAeTarget(false, rgb_attr.enable, brightness, defTarget, FLG_FACE);
        LOGI("rgb rect[%d %d, %d %d] -> [%d %d, %d %d], target = %d", rgb_rc->left, rgb_rc->top, rgb_rc->right, rgb_rc->bottom, rgb_attr.left, rgb_attr.top, rgb_attr.right, rgb_attr.bottom, rgb_attr.target);
        rgb_attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
        isp.SetFaceAE(1, rgb_attr);
    }
#endif
    return 0;
}

#else
int Senselock::setCameraROI(const std::vector<st_rect_t>* rect)
{
    if (mEarlyVerifyState == 1) {
        LOG_DEBUG("Save early ROI\n");
        mEarlyROI = *rect;
        return 0;
    }

    auto ir_config = MediaManager::GetInstance().GetDeviceFrameSource(0)->GetChnConfig();
    auto rgb_config = MediaManager::GetInstance().GetDeviceFrameSource(1)->GetChnConfig();
    auto& isp = Isp::GetInstance();
    IMPISPFaceAttr ir_attr, rgb_attr;
    float ir_scaler = SENSOR0_WIDTH * 1.0f / ir_config->fs_chn_attr.scaler.outwidth;
    float rgb_scaler = SENSOR1_WIDTH * 1.0f / rgb_config->fs_chn_attr.scaler.outwidth;
    st_rect_t ir_rect = (*rect)[0];
    st_rect_t rgb_rect = (*rect)[1];

    bool ir_rect_invalid = (ir_rect.left == 0 && ir_rect.right == 0 && ir_rect.top == 0 && ir_rect.bottom == 0);
    bool rgb_rect_invalid = (rgb_rect.left == 0 && rgb_rect.right == 0 && rgb_rect.top == 0 && rgb_rect.bottom == 0);

    if (ir_rect_invalid && rgb_rect_invalid) {
        LOG_WARNING("ir and rgb rect invalid\n");
        return 0;
    }
    else if (ir_rect_invalid) {
        LOG_WARNING("ir rect invalid, use rgb rect\n");
        ir_rect = rgb_rect;
    }
    else if (rgb_rect_invalid) {
        LOG_WARNING("rgb rect invalid, use ir rect\n");
        rgb_rect = ir_rect;
        rgbInvalidCount++;
    }

    memset(&ir_attr, 0, sizeof(ir_attr));
    memset(&rgb_attr, 0, sizeof(rgb_attr));
    LOG_DEBUG("ir face roi %d %d %d %d\n", ir_rect.left, ir_rect.top, ir_rect.right, ir_rect.bottom);
    LOG_DEBUG("rgb face roi %d %d %d %d\n", rgb_rect.left, rgb_rect.top, rgb_rect.right, rgb_rect.bottom);
    auto angle = koks_config.media.video.frame_source[0].input._2d.rotate.angle;

    auto get_raw_rect = [this](st_rect_t* rect, const FrameSourceChnConfig* chn_config, const int rotate) -> st_rect_t {
        st_rect_t raw_rect;
        switch(rotate) {
            case 90:
                raw_rect.left = rect->top;
                raw_rect.top = mImage.width - rect->right;
                raw_rect.right = rect->bottom;
                raw_rect.bottom = mImage.width - rect->left;
                break;
            case 180:
                raw_rect.left = mImage.width - rect->right;
                raw_rect.top = mImage.height - rect->bottom;
                raw_rect.right = mImage.width - rect->left;
                raw_rect.bottom = mImage.height - rect->top;
                break;
            case 270:
                raw_rect.left = mImage.height - rect->bottom;
                raw_rect.top = rect->left;
                raw_rect.right = mImage.height - rect->top;
                raw_rect.bottom = rect->right;
                break;
            default:
                raw_rect.left = rect->left;
                raw_rect.top = rect->top;
                raw_rect.right = rect->right;
                raw_rect.bottom = rect->bottom;
        }
        raw_rect.left += (chn_config->fs_chn_attr.crop.enable ? chn_config->fs_chn_attr.crop.left : 0);
        raw_rect.top += (chn_config->fs_chn_attr.crop.enable ? chn_config->fs_chn_attr.crop.top : 0);
        raw_rect.right += (chn_config->fs_chn_attr.crop.enable ? chn_config->fs_chn_attr.crop.left : 0);
        raw_rect.bottom += (chn_config->fs_chn_attr.crop.enable ? chn_config->fs_chn_attr.crop.top : 0);
        return raw_rect;
    };

    st_rect_t ir_raw_rect_ = get_raw_rect(&ir_rect, ir_config, angle);
#ifdef KOKS_PROJECT_M24B2
    st_rect_t rgb_raw_rect_ = ir_raw_rect_;
#else
    st_rect_t rgb_raw_rect_ = get_raw_rect(&rgb_rect, rgb_config, angle);
#endif
    ir_attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
    ir_attr.left = ir_raw_rect_.left * ir_scaler;
    ir_attr.top = ir_raw_rect_.top * ir_scaler;
    ir_attr.right = ir_raw_rect_.right * ir_scaler;
    ir_attr.bottom = ir_raw_rect_.bottom * ir_scaler;
    ir_attr.target = static_cast<unsigned int>(isp.GetAEConfig(0).face_ae_target);
    isp.SetFaceAE(0, ir_attr);

#if 1
    int lowEV = 1000;
    int highEV = 10000;
#if defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M55)
    lowEV = 1000;
    highEV = 50000;
#endif
    IMPISPAeOnlyReadAttr ae_attr1;
    IMPISPRunningMode pmode;
    IMP_ISP_Tuning_GetISPRunningMode((IMPVI_NUM)1, &pmode);
    auto ret = IMP_ISP_Tuning_GetAeOnlyReadAttr((IMPVI_NUM)1, &ae_attr1);
    // LOG_INFO("RGB EV is %lld \n", ae_attr1.ExposureValue);
    if (ret == ST_ERRNO_SUCCESS && pmode == IMPISP_RUNNING_MODE_DAY && ae_attr1.ExposureValue > lowEV && ae_attr1.ExposureValue < highEV && rgbInvalidCount < 3) {
        LOG_INFO("RGB EV is %lld , switch to globalAE\n", ae_attr1.ExposureValue);
        isp.CloseFaceAe(1);
        return 0;
    }
#endif

    rgb_attr.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
    rgb_attr.left = rgb_raw_rect_.left * rgb_scaler;
    rgb_attr.top = rgb_raw_rect_.top * rgb_scaler;
    rgb_attr.right = rgb_raw_rect_.right * rgb_scaler;
    rgb_attr.bottom = rgb_raw_rect_.bottom * rgb_scaler;
    rgb_attr.target = static_cast<unsigned int>(isp.GetAEConfig(1).face_ae_target);
    isp.SetFaceAE(1, rgb_attr);

    // LOG_INFO("raw ir face roi %d %d %d %d, target %d\n", ir_attr.left, ir_attr.top, ir_attr.right, ir_attr.bottom, ir_attr.target);
    // LOG_INFO("raw rgb face roi %d %d %d %d, target %d\n", rgb_attr.left, rgb_attr.top, rgb_attr.right, rgb_attr.bottom, rgb_attr.target);
    return 0;
}
#endif

int Senselock::releaseCameraBufs(void)
{
    LOG_INFO("\n");
    return 0;
}

int Senselock::RequestHwCheck(void)
{
    LOG_ERROR("\n");
    return 0;
}

static int g_skip_frames = 0;
int Senselock::GetCameraFrame()
{
    // st::sense_koks::LedGuard led_guard;
    constexpr uint8_t SKIP_FRAME_NUM_IR = 0;
    constexpr uint8_t SKIP_FRAME_NUM_RGB = 0;
    RunTime runtime("GetCameraFrame", true);
    int ret = 0;
    IMPFrameInfo ir_frame;
    IMPFrameInfo rgb_frame;

    static int cnt = 0;

    auto& media_manager = MediaManager::GetInstance();
#ifndef KOKS_SDK_VERSION_V2
    if (cnt == 0 && !mEarlyROI.empty()) {
        if (GetCurTimeMs() < 1000) {
            LOG_DEBUG("Set early ROI\n");
            setCameraROI(&mEarlyROI);
            cnt = 1;
        }
        mEarlyROI = {};
    }
#endif
#if 0
    auto future = std::async(std::launch::async, [&media_manager, this, &rgb_frame](void) -> int {
        (void)std::ignore;
        for (uint8_t i = 0; i < SKIP_FRAME_NUM_RGB; i++) {
            RunTime runtime("GetCameraFrame rgb 1", true);
            media_manager.GetDeviceFrameSource(1)->SnapFrame(mImage.second_image, &rgb_frame);
        }
        {
            RunTime runtime("GetCameraFrame rgb 2", true);
            return media_manager.GetDeviceFrameSource(1)->SnapFrame(mImage.second_image, &rgb_frame);
        }
    });
    auto future2 = std::async(std::launch::async, [&media_manager, this, &ir_frame](void) -> int {
        (void)std::ignore;
        for (uint8_t i = 0; i < SKIP_FRAME_NUM_IR; i++) {
            RunTime runtime("GetCameraFrame ir 1", true);
            media_manager.GetDeviceFrameSource(0)->SnapFrame(mImage.first_image, &ir_frame);
        }
        {
            RunTime runtime("GetCameraFrame ir 2", true);
            return media_manager.GetDeviceFrameSource(0)->SnapFrame(mImage.first_image, &ir_frame);
        }
    });
    ret = future.get();
    if (ret != 0) {
        LOG_ERROR("SnapFrame failed with %#x\n", ret);
        return ret;
    }
    ret = future2.get();
    if (ret != 0) {
        LOG_ERROR("SnapFrame failed with %#x\n", ret);
        return ret;
    }
#endif

#ifndef KOKS_FEATURE_PALM
    for (int i = 0; i < g_skip_frames; ++i) {
        LOG_INFO("skip frame cnt %d\n", i + 1);
        media_manager.GetDeviceFrameSource(1)->SnapFrame(mImage.second_image, &rgb_frame);
        media_manager.GetDeviceFrameSource(0)->SnapFrame(mImage.first_image, &ir_frame);
    }
    g_skip_frames = 0;
#endif

    auto s_time = GetCurTimeMs();
    {
        // RunTime runtime("GetCameraFrame rgb 1", true);
        media_manager.GetDeviceFrameSource(1)->SnapFrame(mImage.second_image, &rgb_frame);
#ifdef KOKS_PROJECT_M24B3
        Isp::GetInstance().checkBrightness(1); // 检测RGB亮度
#endif
    }
    auto elapsed_time = GetCurTimeMs() - s_time;
    auto sensor_frame_rate = Isp::GetInstance().GetSensorFrameRate();
    if (elapsed_time < 1000 / sensor_frame_rate) {
        os_delay(1000 / sensor_frame_rate - elapsed_time);
    }
    {
        // RunTime runtime("GetCameraFrame ir 1", true);
        media_manager.GetDeviceFrameSource(0)->SnapFrame(mImage.first_image, &ir_frame);
    }

    if (cnt++ == 0) {
        // printf("----cnt %d-----",cnt);
        Isp::GetInstance().setAEMode(0);
        Isp::GetInstance().setAEMode(1);
    }

    LOG_DEBUG("ir frame size %dx%d,timestamp %lld\n", ir_frame.width, ir_frame.height, ir_frame.timeStamp);
    LOG_DEBUG("rgb frame size %dx%d,timestamp %lld\n", rgb_frame.width, rgb_frame.height, rgb_frame.timeStamp);
    // LOG_INFO("GetCameraFrame success!\n");
    return 0;
}

void Senselock::uart_msg_task(void* arg)
{
    Senselock* sLock = static_cast<Senselock*>(arg);
    SenselockHandler* sLock_handler = SenselockHandler::getInstance();
    UpdaterHandler* sUpdater_handler = UpdaterHandler::getInstance();
    s_msg* msg;
    uint32_t ret = 0;
    int upload_image_len;
    int upload_debug_info_total_len;
    uint32_t event_flag;
    unsigned char* upload_log_buf = NULL;
    int upload_log_len = 0;
    LOG_DEBUG("uart_msg_task startup!\n");
    while (1) {
        // get uart cmd
        std::unique_lock<std::mutex> lk(sLock->UartMSGHandleMutex);
        sLock->UartMSGHandle_cv.wait(lk, [sLock]() { return !sLock->mCmdQueue.empty(); });
        msg = sLock->mCmdQueue.front();
        sLock->mCmdQueue.pop_front();

        switch (msg->mid) {
        case MID_GET_VERSION:
        case MID_SET_UTC:
        case MID_GETLIBRARY_VERSION:
        case MID_CONFIG_BAUDRATE:
        case MID_FACTORY_TEST:
            break;
        default:
            earlyVerify(); // only call once
            break;
        }

        // process the uart cmd
        switch (msg->mid) {
        case MID_FACERESET: {
            RunTime runtime("handleMsgFaceReset");
            sLock_handler->handleMsgFaceReset(msg);
            break;
        }
        case MID_GETUSERINFO: {
            RunTime runtime("handleMsgGetUserInfo");
            sLock_handler->handleMsgGetUserInfo(msg);
            break;
        }
#ifndef KOKS_SDK_VERSION_V2
        case MID_GET_ALL_USERID: {
            RunTime runtime("handleGetAllUserId");
            sLock_handler->handleGetAllUserId();
            break;
        }
        case MID_ENROLL: {
            st::sense_koks::LedGuard led_guard;
            {
                RunTime runtime("handleMsgEnroll");
                PartitionGuard configGuard(CONFIG_FACE_DB);
                g_skip_frames = Isp::GetInstance().IsAutoAEMode() ? ENROLL_SKIP_FRAMES : 1;
                sLock_handler->handleMsgEnroll(msg, FACE_ENROLL_TYPE_INTERACTIVE);
                Isp::GetInstance().CloseFaceAe();
                rgbInvalidCount = 0;
            }
            break;
        }
        case MID_ENROLL_SINGLE: {
            st::sense_koks::LedGuard led_guard;
            {
                RunTime runtime("handleMsgEnroll", true);
                PartitionGuard configGuard(CONFIG_FACE_DB);
                g_skip_frames = Isp::GetInstance().IsAutoAEMode() ? ENROLL_SKIP_FRAMES : 1;
                sLock_handler->handleMsgEnroll(msg, FACE_ENROLL_TYPE_SINGLE);
                Isp::GetInstance().CloseFaceAe();
                rgbInvalidCount = 0;
            }
            break;
        }
        case MID_SAVE_FEARURE: {
            RunTime runtime("handleMsgEnrollFeature");
            sLock_handler->handleMsgEnrollFeature(msg);
            break;
        }
        case MID_SET_RGBIMAGE_SIZE: {
            RunTime runtime("handleMsgSetRgbImageSize");
            sLock_handler->handleMsgSetRgbImageSize(msg);
            break;
        }
        case MID_TRANSFER_RGBIMAGE: {
            RunTime runtime("handleMsgTransferRGBImage");
            sLock_handler->handleMsgTransferRGBImage(msg);
            break;
        }
        case MID_ENROLL_RGB: {
            RunTime runtime("handleMsgEnrollRGB");
            PartitionGuard configGuard(CONFIG_FACE_DB);
            g_skip_frames = Isp::GetInstance().IsAutoAEMode() ? ENROLL_SKIP_FRAMES : 1;
            sLock_handler->handleMsgEnrollRGB(msg, 0);
            rgbInvalidCount = 0;
            break;
        }
        case MID_ENROLL_ITG: {
            st::sense_koks::LedGuard led_guard;
            {
                RunTime runtime("handleMsgEnrollItg");
                PartitionGuard configGuard(CONFIG_FACE_DB);
                g_skip_frames = Isp::GetInstance().IsAutoAEMode() ? ENROLL_SKIP_FRAMES : 1;
                sLock_handler->handleMsgEnrollItg(msg);
                Isp::GetInstance().CloseFaceAe();
            }
            break;
        }
        case MID_VERIFY: {
            st::sense_koks::LedGuard led_guard;
            {
                RunTime runtime("handleMsgVerify", true);
                g_skip_frames = VERIFY_SKIP_FRAMES;
                sLock_handler->handleMsgVerify(msg);
                Isp::GetInstance().CloseFaceAe();
                rgbInvalidCount = 0;
            }
            break;
        }
        case MID_DELUSER: {
            RunTime runtime("handleMsgDelUser", true);
            sLock_handler->handleMsgDelUser(msg);
            break;
        }
        case MID_DELALL: {
            RunTime runtime("handleMsgDelAll", true);
            sLock_handler->handleMsgDelAll();
            sLock_handler->handleMsgDelOVDConfig();
            break;
        }
        case MID_GET_VERSION: {
            RunTime runtime("getSWVersion");
            sLock_handler->getSWVersion();
            break;
        }
#ifdef KOKS_FEATURE_VIDEO
        case MID_SET_VIDEO_PARAM: {
            RunTime runtime("handleSetVideoParam");
            sLock_handler->handleSetVideoParam(msg, s_type::TYPE_UART);
            break;
        }
        case MID_SET_AUDIO_PARAM: {
            RunTime runtime("handleSetAudioParam");
            sLock_handler->handleSetAudioParam(msg);
            break;
        }
        case MID_SET_UVC_CONTROL: {
            RunTime runtime("handleSetUVCParam");
            sLock_handler->handleSetUVCParam(msg);
            break;
        }
        case MID_SET_UVC_SPECIFIC_FRAME: {
            RunTime runtime("handleSetUVCStreamType");
            sLock_handler->handleSetUVCStreamType(msg, s_type::TYPE_UART);
            break;
        }
        case MID_UVC_SWITCH: {
            RunTime runtime("handleUVCSwitch");
            sLock_handler->handleUVCSwitch(msg, s_type::TYPE_UART);
            break;
        }
        case MID_GET_VIDEO_MODE: {
            RunTime runtime("handleGetVideoMode");
            sLock_handler->handleGetVideoMode(msg, s_type::TYPE_UART);
            break;
        }
#endif
        case MID_DEMOMODE: {
            RunTime runtime("handleDemoMode");
            // erase_raw_file_sector(); //only for test
            sLock_handler->handleDemoMode(msg);
            break;
        }
        case MID_DEBUG_MODE: {
            RunTime runtime("handleMsgDebugMode");
            sLock_handler->handleMsgDebugMode(msg);
            break;
        }
#endif
        // case MID_CHANGE_RELEASE_ENC_KEY: {
        //     RunTime runtime("handleChangeReleaseKey");
        //     sLock_handler->handleChangeReleaseKey(msg);
        //     break;
        // }
        case MID_WRITE_SN: {
            RunTime runtime("handleMsgwriteSN");
            sLock_handler->handleMsgwriteSN(msg);
            break;
        }
        case MID_SET_UTC: {
            RunTime runtime("handleSetUTCTime");
            sLock_handler->handleSetUTCTime(msg);
            break;
        }
        case MID_GETLIBRARY_VERSION: {
            RunTime runtime("getLibraryVersion");
            sLock_handler->getLibraryVersion();
            break;
        }
        case MID_SNAP_DUAL_IMAGE: {
            RunTime runtime("handleMsgSnapDualImage");
            sLock_handler->handleMsgSnapDualImage(msg);
            break;
        }
        case MID_SNAPIMAGE: {
            RunTime runtime("handleMsgSnapImage");
            sLock_handler->handleMsgSnapImage(msg);
            break;
        }
        case MID_POWERDOWN: {
            RunTime runtime("handlePowerDown");
            sLock_handler->handlePowerDown(msg);
            break;
        }
        case MID_GETSAVEDIMAGE: {
            RunTime runtime("handleMsgGetSavedImage");
            if (!upload_image_buf) {
                upload_image_buf = new unsigned char[sLock->mImage.width * sLock->mImage.height * 5 / 2];
            }
            sLock_handler->handleMsgGetSavedImage(msg, upload_image_buf, upload_image_len);
            break;
        }
        case MID_GETIMAGE: {
            RunTime runtime("handleMsgGetImage");
            if (!upload_image_buf) {
                upload_image_buf = new unsigned char[sLock->mImage.width * sLock->mImage.height * 5 / 2];
            }
            sLock_handler->handleMsgGetImage(upload_image_buf, upload_image_len);
            break;
        }
        case MID_UPLOADIMAGE: {
            RunTime runtime("handleMsgUploadImage", false);
            if (!upload_image_buf) {
                upload_image_buf = new unsigned char[sLock->mImage.width * sLock->mImage.height * 5 / 2];
            }
            sLock_handler->handleMsgUploadImage(msg, upload_image_buf, upload_image_len);
            break;
        }
        case MID_GETRAWIMAGE: {
            RunTime runtime("handleMsgGetRawImage", true);
            if (!upload_image_buf) {
                upload_image_buf = new unsigned char[sLock->mImage.width * sLock->mImage.height * 5 / 2];
            }
            sLock_handler->handleMsgGetRawImage(upload_image_buf, upload_image_len);
            break;
        }
        case MID_CONFIG_BAUDRATE: {
            RunTime runtime("handleConfigBaudrate");
            sLock_handler->handleConfigBaudrate(msg);
            break;
        }

        case MID_GET_LOGFILE: {
            sLock_handler->handleMsgGetLogFile(msg, &upload_log_buf, upload_log_len);
            break;
        }
        case MID_UPLOAD_LOGFILE: {
            sLock_handler->handleMsgUploadLogFile(msg, upload_log_buf, upload_log_len);
            break;
        }
#ifdef KOKS_FEATURE_LCD_EMOTION
        case MID_EMOTION: {
            sLock_handler->handleMsgEmotion(msg);
            break;
        }
#endif
        case MID_START_OTA: {
            RunTime runtime("startOTA");
#ifndef KOKS_FEATURE_VIDEO
            auto& media_manager = MediaManager::GetInstance();
            media_manager.Stop();
            media_manager.DeInit();
#endif
            sUpdater_handler->startOTA();
            break;
        }
        case MID_GET_OTA_STATUS: {
            RunTime runtime("getOTAStatus");
            sUpdater_handler->getOTAStatus();
            break;
        }
        case MID_FORMAT_FATFS: {
            RunTime runtime("format_fs");
            sLock_handler->format_fs(msg);
            break;
        }
        case MID_OTA_HEADER: {
            RunTime runtime("handleMsgOTAHeader");
            sUpdater_handler->handleMsgOTAHeader(msg);
            break;
        }
        case MID_STOP_OTA: {
            RunTime runtime("stopOTA");
            sUpdater_handler->stopOTA();
            break;
        }
        case MID_OTA_PACKET: {
            RunTime runtime("OTAPacketTransmit", false);
            sUpdater_handler->OTAPacketTransmit(msg);
            break;
        }
        case MID_REBOOT_CMD: {
            RunTime runtime("handleReboot");
            sLock_handler->handleReboot();
            break;
        }
        case MID_SET_THRESHOLD_LEVEL: {
            RunTime runtime("handleMsgSetThreLevel");
            sLock_handler->handleMsgSetThreLevel(msg);
            break;
        }
        case MID_GET_THRESHOLD_LEVEL: {
            RunTime runtime("handleMsgGetThreLevel");
            sLock_handler->handleMsgGetThreLevel(msg);
            break;
        }
        case MID_SET_THRESHOLD: {
            RunTime runtime("handleMsgSetThreshold");
            sLock_handler->handleMsgSetThreshold(msg);
            break;
        }
        case MID_GET_THRESHOLD: {
            RunTime runtime("handleMsgGetThreshold");
            sLock_handler->handleMsgGetThreshold(msg);
            break;
        }

        case MID_MISC_AI_CONFIG: {
            RunTime runtime("handleMsgMiscAiConfig");
            sLock_handler->handleMsgMiscAiConfig(msg);
            break;
        }

        case MID_BLUR_CONFIG: {
            RunTime runtime("handleMsgBlurConfig");
            sLock_handler->handleMsgBlurConfig(msg);
            break;
        }
        case MID_GET_DEBUG_INFO: {
            RunTime runtime("handleMsgGetDebugInfo");
            sLock_handler->handleMsgGetDebugInfo(msg, upload_debug_info_total_len);
            break;
        }
        case MID_UPLOAD_DEBUG_INFO: {
            RunTime runtime("handleMsgUploadDebugInfo");
            sLock_handler->handleMsgUploadDebugInfo(msg, upload_debug_info_total_len);
            break;
        }
#ifdef KOKS_SDK_VERSION_V2
#ifdef KOKS_FEATURE_PALM
        case MID_PALM_ENROLL:
        case MID_PALM_ENROLL_SINGLE:
        case MID_PALM_ENROLL_BYID:         // enroll palm by id [admin, user name[32], memberid[2], timeout, id]
        case MID_PALM_ENROLL_BYID1:        // enroll palm by id [admin, user name[32], memberid[2], timeout, id]
        case MID_PALM_ENROLL_BYID_NO_OVERWRITE: // enroll palm by id no overwrite [admin, user name[32], memberid[2], timeout, id]
        {
            PartitionGuard configGuard(CONFIG_PALM_DB);
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_ENROLL, FLG_PALM);
            break;
        }
        case MID_PALM_VERIFY_ONLY:
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_UNLOCK, FLG_PALM);
            break;
    #ifdef KOKS_FEATURE_KDS
        case MID_PALM_VERIFY:
    #endif
        case MID_VERIFY:
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_UNLOCK, FLG_FACE|FLG_PALM);
            rgbInvalidCount = 0;
            break;
#else
        case MID_VERIFY:
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_UNLOCK, FLG_FACE);
            rgbInvalidCount = 0;
            break;
#endif
        case MID_GET_VERSION:
        case MID_DEBUG_MODE:
        case MID_DEMOMODE:
        case MID_DELUSER:
        case MID_DELALL:
        case MID_GET_ALL_USERID:
#ifdef KOKS_FEATURE_PALM
        case MID_PALM_QUERY:
        case MID_PALM_DELETE:
        case MID_PALM_DELALL:
        case MID_PALM_SWITCH:
        case MID_PALM_LEVEL:
#endif
        case MID_MEMBERSHIP:
        case MID_FACE_DEL_MULTI:           // delete face by multi-id [id, id, ...]
        case MID_PALM_DEL_MULTI:           // delete palm by multi-id [id, id, ...]
            mState = MS_BUSY;
            sLock_handler->handleCommon(msg);
            mState = MS_STANDBY;
            break;
        case MID_VERIFY_FACE_SINGLE:
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_UNLOCK, FLG_FACE);
            rgbInvalidCount = 0;
            break;
        case MID_ENROLL:
        case MID_ENROLL_SINGLE:
        case MID_ENROLL_FACE_UP:
        case MID_ENROLL_FACE_DN:
        case MID_ENROLL_FACE_LEFT:
        case MID_ENROLL_FACE_RIGHT:
        case MID_ENROLL_FACE_SINGLE:
    #ifndef KOKS_FEATURE_KDS
        case MID_FACE_ENROLL_BYID:         // enroll by id [admin, user name[32], direction, memberid[2], timeout, id]
    #endif
        case MID_FACE_ENROLL_BYID1:        // enroll face by id [admin, user name[32], direction, memberid[2], timeout, id]
        case MID_FACE_ENROLL_BYID_NO_OVERWRITE:// enroll face by id [admin, user name[32], direction, memberid[2], timeout, id]
        {
            PartitionGuard configGuard(CONFIG_FACE_DB);
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_ENROLL, FLG_FACE);
            rgbInvalidCount = 0;
            break;
        }
    #ifdef KOKS_FEATURE_KDS
        case MID_PALM_ENROLL_ITG:
    #endif
        case MID_ENROLL_MULTI_MODE:        // enroll multimode [admin, user name[32], direction, memberid[2], timeout]
        {
            PartitionGuard configGuard(CONFIG_FACE_DB | CONFIG_PALM_DB);
            handleEnrollUnlock(msg, ST_MODULE_DETECT_MODE_ENROLL, FLG_FACE|FLG_PALM);
            rgbInvalidCount = 0;
            break;
        }
        case MID_REPAIR_MODE:              // repair mode
            sLock_handler->restore_factory_settings(msg); // ?
            break;
        case MID_GET_MODELS_MD5: {
            RunTime runtime("handleMsgGetModelMD5");
            sLock_handler->handleMsgGetModelMD5(msg);
            break;
        }
#endif
        case MID_RESET: {
            RunTime runtime("handleReset");
            sLock_handler->handleReset();
            break;
        }
        case MID_HW_CHECK: {
            RunTime runtime("handleReset");
            sLock_handler->handleHardwareCheck();
            break;
        }
        case MID_FACTORY_TEST: {
            RunTime runtime("handleFactoryTest");
            sLock_handler->handleFactoryTest(msg);
            break;
        }
        default:
            LOG_DEBUG("unsupport mid 0x%x\n", msg->mid);
#ifdef KOKS_FEATURE_KDS
            if (mHost->isFirstCmdReceived())
                sLock->reply(msg->mid, MR_REJECTED, NULL, 0);
#else
            sLock->reply(msg->mid, MR_REJECTED, NULL, 0);
#endif
        }

        // release the uart message
        if (msg != NULL) {
            free(msg);
        }
    }

    if (upload_image_buf != NULL) {
        delete[] upload_image_buf;
    }
    if (upload_log_buf != NULL) {
        free(upload_log_buf);
    }
}

void Senselock::process_ir_buffer(void)
{
    LOG_INFO("\n");
}
