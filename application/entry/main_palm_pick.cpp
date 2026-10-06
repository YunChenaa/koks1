/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-11 17:19:22
 * @LastEditors: 洪照
 * @LastEditTime: 2026-01-14 17:44:20
 * @FilePath: /sense-koks/application/entry/main_palm_pick.cpp
 */

#include "faceunlock_wrapper.h"
#include "koks_project_def.h"
#include "koks1_config.h"
#include "os_utils.h"
#include "senselock.h"
#include "st_koks_db.h"
#include "st_koks_led.h"
#include "st_koks_mediamanager.h"
#include "st_koks_uvc.h"
#include "sys_logger.h"
#include "sys_version.h"
#include "uart.h"
#include <chrono>

using namespace st::sense_koks;

#define DEFAULT_VERIFY_LEVEL (1)
#define DEFAULT_LIVENESS_LEVEL (1)

const static float palm_verify_threshold_level[5] = { 0.4, 0.5, 0.7, 0.9, 0.95 }; // 8.13更新
const static float palm_verify_rgb_threshold_level[5] = { 0.4, 0.5, 0.7, 0.9, 0.95 };
const static float palm_liveness_threshold_level[5] = { 0.99, 0.98, 0.96, 0.92, 0.91 };
static st_module_palm_param_config_t default_palm_param_config = {
    .config_detect_threshold = 0.27f,
    .config_align_threshold = 1.0f,
    .config_hackness_threshold = palm_liveness_threshold_level[DEFAULT_LIVENESS_LEVEL],
    .config_verify_threshold = palm_verify_threshold_level[DEFAULT_VERIFY_LEVEL],
    .config_enroll_verify_threshold = palm_verify_rgb_threshold_level[DEFAULT_VERIFY_LEVEL],
    .config_quality_brightness_max = 0.15,
    .config_quality_brightness_min = -0.35f,
    .config_palm_close_threshold = 300.0f,
    .config_palm_far_threshold = 50.0f,
    .config_palm_sharpness_threshold = 0.05f,
    .config_pixel_to_image_edge = 5,
    .config_enroll_yaw_min = -20,
    .config_enroll_yaw_max = 20,
    .config_enroll_roll_min = -180,
    .config_enroll_roll_max = 180,
    .config_enroll_pitch_min = -35,
    .config_enroll_pitch_max = 40,
    .config_occlusion_ratio = 0.3f,
    .config_verify_yaw_min = -20,
    .config_verify_yaw_max = 20,
    .config_verify_roll_min = -180,
    .config_verify_roll_max = 180,
    .config_verify_pitch_min = -35,
    .config_verify_pitch_max = 40,
    .enable_dup_enroll_ = 1,
    .enable_blur_ = 0,
};

static st_module_palm_config_t st_palm_module_config_640_480 = {
    .template_save_path = (char*)"/config/db/palm/",
    .liveness_enable = false,
    .calc_pose = nullptr,
    .palm_rect = nullptr,
    .algo_abort = nullptr,
    .palm_levle = { _kLow, _kLow, _kLow, _kLow }, //{ _kNormal, _kNormal, _kNormal, _kNormal },
};

#define DEVICE_CONFIG_FILE "/config/koks1.json"
// constexpr const char* const SERIAL_PORT = "/dev/ttyUSB0";

std::atomic<bool> running(true);

static int sum_row(const uint8_t* yuv, uint16_t width, uint16_t top, uint16_t left, uint16_t right)
{
    int ret = 0;
    yuv += width * top;
    while (left < right)
        ret += yuv[left++];
    return ret;
}

static int sum_col(const uint8_t* yuv, uint16_t width, uint16_t col, uint16_t top, uint16_t bottom)
{
    int ret = 0;
    yuv += col;
    while (top < bottom) {
        ret += yuv[width * top];
        top++;
    }
    return ret;
}

// extern void rc_scalar_internal(st_rect_t& rc, float left, float right, float top, float bottom);

static int per_col(st_image_t& img, st_rect_t& rc)
{
    int l = 1, r = 1;
    for (int i = 0; i < 6; i += 2) {
        l += sum_col(img.data, img.width, rc.left + i, rc.top, rc.bottom);
        r += sum_col(img.data, img.width, rc.right - i, rc.top, rc.bottom);
    }
    return l * 100 / r;
}

static int per_row(st_image_t& img, st_rect_t& rc)
{
    int t = 1, b = 1;
    for (int i = 0; i < 6; i += 2) {
        t += sum_row(img.data, img.width, rc.top + i, rc.left, rc.right);
        b += sum_row(img.data, img.width, rc.bottom - i, rc.left, rc.right);
    }
    return t * 100 / b;
}

static void rc_move(st_image_t& img, st_rect_t& rc, float per)
{
    int per_h = per_row(img, rc);
    int per_w = per_col(img, rc);
    int dw = (rc.right - rc.left) * per;
    int dh = (rc.bottom - rc.top) * per;
    if (per_h > 105)
        dh = -dh;
    else if (per_h > 95)
        dh = 0;
    if (per_w > 105)
        dw = -dw;
    else if (per_h > 95)
        dw = 0;
    // LOGI("per_h=%.3f, per_w=%.3f, dw=%d, dh=%d", per_h, per_w, dw, dh);
    rc.left += dw;
    rc.right += dw;
    rc.top += dh;
    rc.bottom += dh;
}

extern void rc_scalar(st_image_t& img, st_rect_t& rc, float left, float right, float top, float bottom);

// static void signalHandler(int signum)
// {
//     std::cout << "Interrupt signal (" << signum << ") received.\n";
//     running = false;
// }

static st_rect_t rc_big, rc_ae;
#define IMAGE_SIZE (608 * 800 * 3 / 2)
static unsigned char frameBuffer[IMAGE_SIZE] = {0};
extern void draw_rc(uint8_t* yuv, int width, int height, int x, int y, int w, int h, int borderWidth);

static void rotate(const unsigned char* src, int src_width, int src_height, char* dst)
{
    const int dst_width = src_height;
    const int dst_height = src_width;

    for (int y = 0; y < src_height; ++y) {
        for (int x = 0; x < src_width; ++x) {
            unsigned char val = src[y * src_width + x];
            int dst_x = src_height - 1 - y; // 原始的高度减去当前行号再减一，以实现逆时针旋转
            int dst_y = x;                  // 当前列号变为新行号
            dst[dst_y * dst_width + dst_x] = val;
        }
    }

    const int uv_src_width = src_width / 2 * 2;
    const int uv_src_height = src_height / 2;
    const int uv_dst_width = dst_width / 2 * 2;
    const int uv_dst_height = dst_height / 2;
    const unsigned char* src_uv = src + src_width * src_height;
    char* dst_uv = dst + dst_width * dst_height;

    for (int y = 0; y < uv_src_height; ++y) {
        for (int x = 0; x < uv_src_width; x += 2) {
            unsigned char u = src_uv[y * uv_src_width + x];
            unsigned char v = src_uv[y * uv_src_width + x + 1];
            int dst_x = uv_src_height - 1 - y;
            int dst_y = x / 2;
            dst_uv[dst_y * uv_dst_width + dst_x * 2] = u;
            dst_uv[dst_y * uv_dst_width + dst_x * 2 + 1] = v;
        }
    }
}

int yuv_push_callback(int encoder_index, char* buf)
{
    const int UVC_SOURCE = DEV_FRAME_SOURCE_2;
    IMPFrameInfo* fr = nullptr;
    auto& media_manager = MediaManager::GetInstance();
    ST_ERR ret = media_manager.GetDeviceFrameSource(UVC_SOURCE)->GetFrame(&fr);
    if (ret == ST_ERRNO_SUCCESS) {
        int len = (fr->width * fr->height) * 3 / 2;
        static long long s_frame_idx = 0;
        ++s_frame_idx;
        memcpy(frameBuffer, (void*)fr->virAddr, len);
        rotate(frameBuffer, fr->width, fr->height, buf);

        if (Senselock::getInstance()->mDebugMode && rc_big.right) {
            IMPISPFaceAeLuma luma;
            IMPISPFaceAttr face_attr;
            // Face ROI
            if (IMP_ISP_Tuning_GetFaceAe((IMPVI_NUM)0, &face_attr) == 0) {
                LOGI("Face ROI: left=%d, top=%d, right=%d, bottom=%d, target=%d", face_attr.left, face_attr.top, face_attr.right, face_attr.bottom, face_attr.target);
            }
            // luma / EV / gain
            if (IMP_ISP_Tuning_GetFaceAeLuma((IMPVI_NUM)0, &luma) == 0) {
                LOGI("luma.global=%d, luma.roi=%d ", luma.global, luma.roi);
            }
            draw_rc((uint8_t*)buf, fr->height, fr->width, rc_big.left, rc_big.top, rc_big.right - rc_big.left, rc_big.bottom - rc_big.top, 1);
            draw_rc((uint8_t*)buf, fr->height, fr->width, rc_ae.left, rc_ae.top, rc_ae.right - rc_ae.left, rc_ae.bottom - rc_ae.top, 1);
        }

        media_manager.GetDeviceFrameSource(UVC_SOURCE)->ReleaseFrame(fr);
        return len;
    }
    LOG_ERROR("GetFrame failed\n");
    return -1;
}

int main(void)
{
    // std::string test_sn ="koks12345";
    // app_flash_write_sn((uint8_t *)test_sn.data(), test_sn.size());
    // std::string license = "36530F0DAA4367000BFF5332D5C66A248531E1B2C90963B127665051A82A8C53";
    // std::string license = "2A4572063187426A9E4345EC181DC80AEAB31E4E04583C6B3568D45E40A29351";
    // std::string license = "6676C205092851BC2D0A7AFEF096D270A75E79CFB783F8920E17F8E80F300400";
    // app_flash_write_licence((uint8_t*)license.data(), license.size());
    int ret = 0, status = 0;
    // printf("                                 __         __       \n");
    // printf("   ________  ____  ________     / /______  / /_______\n");
    // printf("  / ___/ _ \\/ __ \\/ ___/ _ \\   / //_/ __ \\/ //_/ ___/\n");
    // printf(" (__  )  __/ / / (__  )  __/  / ,< / /_/ / ,< (__  ) \n");
    // printf("/____/\\___/_/ /_/____/\\___/  /_/|_|\\____/_/|_/____/  \n");
    // printf("\n\n");
    printf("cpu %s sense koks compile at %s %s, version %s\n", GetCPUID().data(), __DATE__, __TIME__, SENSELOCK_V3X_VERSION); // 必须要打印出来，以便排查问题时核对版本
    // LibSerial::SerialPort serial_port;
    // serial_port.Open(SERIAL_PORT);
    // serial_port.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
    // serial_port.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
    // serial_port.SetParity(LibSerial::Parity::PARITY_NONE);
    // serial_port.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
    // std::signal(SIGINT, signalHandler);
    std::string version = std::string(SENSELOCK_V3X_VERSION);

    if (koks_config.log.path != "null") {
        if (SysLogger::GetInstance()->InitLogger((koks_config.log.path + "/test_" + version).c_str(), koks_config.log.level.level, koks_config.log.max_num,
                                                 koks_config.log.max_size)) {
            LOG_WARNING("sense koks compile at %s %s\n", __DATE__, __TIME__); // 必须要打印出来，以便排查问题时核对版本
        } else {
            std::cerr << "init log failed\n";
            return -1;
        }
    } else {
        SysLogger::GetInstance()->SetLoggerLevel(koks_config.log.level.level);
    }
    auto& db = koks_db::GetInstance();
    db.Init(koks_config.db.path);
    Senselock* sLock = Senselock::getInstance();
    sLock->init(608, 800);

    FaceUnlockWrapper* faceUnlockWrapper = FaceUnlockWrapper::getInstance();
    if (faceUnlockWrapper->init()) {
        LOG_ERROR("faceSdk init failed\n");
        sLock->mState = MS_INVALID;
    }

    std::string serial_path = koks_config.serial.path;
    uint32_t serial_brate = koks_config.serial.BaudRate;
    auto& serial = Uart::GetInstance();
    serial.SetConfig(serial_path, Uart::int2rate(serial_brate));
    serial.Init();

    auto& media_manager = MediaManager::GetInstance();

    sLock->start();
    sLock->mHost->checkRlsKey();

    st::sense_koks::PalmStrategy palm;
    db.readData(&palm, &st_palm_module_config_640_480.palm_levle);
    ret = st_palm_module_init(&st_palm_module_config_640_480);
    if (ret != ST_OK) {
        LOGE("st_palm_module_init failed %d", ret);
        return -1;
    }
    ret = st_palm_module_set_param_config(&default_palm_param_config);
    if (ret != ST_OK) {
        LOGE("st_palm_module_set_param_config failed %d", ret);
        return -1;
    }

    st::sense_koks::LedProxy::Init();
    sLock->sendMessage(NID_READY, NULL, 0);
    LOG_INFO("sensen koks READY!\n");

    std::thread standby([&sLock, &media_manager, &ret]() {
        int sdk_status = 0;
        int sdk_user_id = 0;
        int uid = 0;
        static bool s_has_roi = false;
        static st_rect_t s_last_rc_ae = { 0 };
        static st_rect_t s_last_rc_big = { 0 };
        static int s_last_status = -1;
        static uint64_t s_frame_idx = 0;
        auto rect_valid = [](const st_rect_t& r) { return (r.right > r.left) && (r.bottom > r.top); };
        uint8_t occlusion_percent = 0;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        st_image_t img_rgb, img_ir;

        ret = media_manager.Init(koks_config.media, MediaType::AUDIO);
        if (ret)
            LOG_ERROR("media_manager.Init failed with %d\n", ret);
        ret = media_manager.Start(MediaType::AUDIO);
        if (ret)
            LOG_ERROR("media_manager.Start failed with %d\n", ret);

        LOG_INFO("sensen koks startup!\n");
        sLock->switchEffectBin();

        system("insmod /lib/modules/inlock_camera.ko");
        media_uvc_factory_init();

        const float SCALE_SIDE = 0.35f;
        while (running) {
#if 1
            ++s_frame_idx;
            LOGI("start GetCameraFrame");
            if (!sLock->GetCameraFrame(img_rgb, img_ir)) {
                LOGI("Get one frame, sending to detect");
                st_rect_t rc_face_ir = { 0 }, rc_palm_ir = { 0 }, rc{ 0 }, rc_opisthenar_ir{ 0 }, rc_align = { 0 };
                int status = 0;
                ret = st_face_plam_detect(&img_ir, &rc_face_ir, &rc_palm_ir, &rc_opisthenar_ir, &status, ST_MODULE_DETECT_MODE_ENROLL);
                LOGI("detect ret=%d status=%d palm=(%d,%d,%d,%d) back=(%d,%d,%d,%d)", ret, status, rc_palm_ir.left, rc_palm_ir.top, rc_palm_ir.right, rc_palm_ir.bottom,
                     rc_opisthenar_ir.left, rc_opisthenar_ir.top, rc_opisthenar_ir.right, rc_opisthenar_ir.bottom);
                if (ret == ST_OK) {
                    LOGI("return detect success");
                } else {
                    LOGI("st_face_plam_detect is false");
                }
                LOGI("%d", rc_palm_ir.right);
                st_rect_t* rc_detect = nullptr;
                bool is_back = rect_valid(rc_opisthenar_ir);
                if (is_back) {
                    rc_detect = &rc_opisthenar_ir; // 手背框
                    LOGI("Palm BACK detected");
                } else {
                    rc_detect = &rc_palm_ir; // 手心框
                    LOGI("Palm FRONT detected");
                }
                bool updated_roi = false;
                if (ret == ST_OK && rc_detect) {
                    int cross = 0;
                    rc_big = *rc_detect;

                    if (st_palm_module_get_align_rect(&img_ir, *rc_detect, &rc_align, &cross) != ST_OK || !rc_align.right) {
                        rc_scalar(img_ir, *rc_detect, SCALE_SIDE, -SCALE_SIDE, SCALE_SIDE, -SCALE_SIDE);
                        rc_ae = *rc_detect;
                    } else {
                        rc_ae = rc_align;
                    }
                    sLock->setCameraROI(&rc_ae, NULL, 0);
                    LOGI("setCameraROI success");
                }
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    standby.join();
    media_manager.Stop();
    media_manager.DeInit();
    serial.Deinit();
    koks_db::GetInstance().DeInit();
    return 0;
}
