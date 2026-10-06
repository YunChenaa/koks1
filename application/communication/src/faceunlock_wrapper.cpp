/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-07 14:29:14
 * @LastEditors: 洪照
 * @LastEditTime: 2026-04-08 18:59:48
 * @FilePath: /sense-koks/application/communication/src/faceunlock_wrapper.cpp
 */
#include "faceunlock_wrapper.h"
#include "crypto.h"
#include "os_utils.h"
#include "rota_common.h"
#include "senselock.h"
#include "senselock_handler.h"
#include "st_koks_db.h"
#include "st_koks_isp.h"
#include "sys_logger.h"
#include <chrono>
#include <string>
#include <thread>
#define SAVE_ATTRIBUTE_STATE "/data/AttState"
#define TEMPLATE_PATH "/data"
#define MODULE_AES_KEY "Y6nkaNQkW+jAAUe="
#ifdef KOKS_SDK_VERSION_V2
const static PoseAngle pose_angle = { .enroll_angle = { .roll = { -20, 20 }, .pitch = { -20, 20 }, .yaw = { -20, 20 } },
#elif defined(KOKS_FEATURE_KDS)
const static PoseAngle pose_angle = { .enroll_angle = { .roll = { -20, 20 }, .pitch = { -20, 20 }, .yaw = { -20, 20 } },
#else
const static PoseAngle pose_angle = { .enroll_angle = { .roll = { -15, 15 }, .pitch = { -15, 15 }, .yaw = { -15, 15 } },
#endif
                                      .unlock_angle = { .roll = { -20, 20 }, .pitch = { -20, 20 }, .yaw = { -20, 20 } } };
#ifdef KOKS_SDK_VERSION_V2
// 支持脸+掌的SDK在这里配置
#define DEFAULT_VERIFY_LEVEL (2)
#if defined(KOKS_PROJECT_M53A) || defined(KOKS_PROJECT_M24CP) || defined(KOKS_PROJECT_M25P)
#define DEFAULT_LIVENESS_LEVEL (3)
#else
#define DEFAULT_LIVENESS_LEVEL (2)
#endif
#define FAKE_THRESHOLD_LEVEL (255)
#else
// 单人脸的SDK在这里配置
#define DEFAULT_VERIFY_LEVEL (2)
#define DEFAULT_LIVENESS_LEVEL (2)
#define FAKE_THRESHOLD_LEVEL (255)
#endif

#ifdef KOKS_FEATURE_DSM
#define KEYPOINT_STABILIZER_WIN_SIZE 5
#else
#define KEYPOINT_STABILIZER_WIN_SIZE 3
#endif

using namespace std;

void st_module_callback_calc_pose(st_module_face_info_t* face_info)
{
    SenselockHandler::getInstance()->handleSetFaceInfo(face_info);
}

#ifndef KOKS_SDK_VERSION_V2
int st_module_callback_face_rect(st_image_t* image, const std::vector<st_rect_t>* rect)
{
    Senselock* slock = Senselock::getInstance();
    if (slock && rect)
        // `image` is face up, need rotate
        slock->setCameraROI(rect);

    return 0;
}
#endif

int st_module_callback_algo_abort()
{
    LOG_INFO("Is algo abort %d\n", Senselock::mAbort.load());
    return Senselock::mAbort.load();
}

void st_modle_log(const char* msg)
{
    LOG_WARNING("[sdk]%s\n", msg);
}

#ifdef KOKS_PROJECT_M24B3
// true：day   false：night
bool st_module_callback_day_mode()
{
    return st::sense_koks::Isp::GetInstance().isBright(1);
}
#endif

#ifdef KOKS_SDK_VERSION_V2
#if defined(KOKS_PROJECT_M53A) || defined(KOKS_PROJECT_M24CP) || defined(KOKS_PROJECT_M25P)
const static float verify_threshold_level[7] = { 0.7, 0.8, 0.9, 0.93, 0.95, 0.97, 1 }; // 8.13更新
const static float verify_rgb_threshold_level[7] = { 0.7, 0.8, 0.9, 0.93, 0.95, 0.97, 1 };
const static float liveness_threshold_level[7] = { 0.99, 0.98, 0.95, 0.94, 0.93, 0.9, 0.8 };
#else
const static float verify_threshold_level[5] = { 0.7, 0.8, 0.9, 0.93, 0.95 }; // 8.13更新
const static float verify_rgb_threshold_level[5] = { 0.7, 0.8, 0.9, 0.93, 0.95 };
const static float liveness_threshold_level[5] = { 0.999, 0.99, 0.98, 0.95, 0.9 };
#endif
#elif defined(KOKS_FEATURE_KDS)
const static float verify_threshold_level[5] = { 0.7, 0.8, 0.9, 0.93, 0.95 }; // 8.13更新
const static float verify_rgb_threshold_level[5] = { 0.7, 0.8, 0.9, 0.93, 0.95 };
const static float liveness_threshold_level[5] = { 0.99, 0.98, 0.95, 0.94, 0.93 };
#else
const static float verify_threshold_level[7] = {0.7, 0.8, 0.9, 0.93, 0.95, 0.97, 1}; // 8.13更新
const static float verify_rgb_threshold_level[7] = {0.7, 0.8, 0.9, 0.93, 0.95, 0.97, 1};
const static float liveness_threshold_level[7] = { 0.99, 0.98, 0.95, 0.94, 0.93, 0.9, 0.8 };
#endif

#ifdef KOKS_SDK_VERSION_V2
static st_module_config_t st_module_config_640_480 = { .template_save_path = (char*)"/config/db/face/",
                                                       .resize_image = ST_FACE_RESIZE_IMG_640x480,
                                                       .liveness_LOGIc_method = LIVENESS_LOGIC_METHOD,
                                                       .load_verify = true,
                                                       .aes_enc_key = (char*)MODULE_AES_KEY,
                                                       .calc_pose = nullptr,
                                                       .face_rect = nullptr,
                                                       .algo_abort = st_module_callback_algo_abort,
                                                       .aes_crypto = aes_crypto };
#else
static st_module_config_t st_module_config_640_480 = { .template_save_path = (char*)"/data",
                                                       .resize_image = ST_FACE_RESIZE_IMG_640x480,
                                                       .liveness_LOGIc_method = LIVENESS_LOGIC_METHOD,
                                                       .load_verify = true,
                                                       .aes_enc_key = (char*)MODULE_AES_KEY,
                                                       .calc_pose = nullptr,
                                                       .face_rect = st_module_callback_face_rect,
                                                       .algo_abort = st_module_callback_algo_abort,
                                                       .aes_crypto = aes_crypto,
                                                       .x_log = st_modle_log,
#ifdef KOKS_PROJECT_M24B3
                                                       .is_daymode = st_module_callback_day_mode
#endif
                                                    };
#endif

st_module_param_config_t st_module_param_config = {
#ifdef KOKS_SDK_VERSION_V2
                                                    .config_face_detect_enroll_threshold = 0.35,
                                                    .config_face_detect_unlock_threshold = 0.25,
                                                    .config_palm_detect_enroll_threshold = 0.35,
                                                    .config_palm_detect_unlock_threshold = 0.25,
                                                    .config_opisthenar_detect_threshold = 0.4,
                                                    .config_glove_detect_threshold = 0.25,
#else
                                                    .config_detect_threshold = 0.27,
#endif
                                                    .config_align_threshold = 1.0, // 7.14给出
                                                    .config_hackness_threshold = liveness_threshold_level[DEFAULT_LIVENESS_LEVEL],
                                                    .config_verify_threshold = verify_threshold_level[DEFAULT_VERIFY_LEVEL], // 8.13更新 verify_threshold_level[1]
                                                    .config_verify_rgb_threshold = verify_rgb_threshold_level[DEFAULT_VERIFY_LEVEL],
                                                    .config_enroll_verify_threshold = 0.5, // 7.14给出
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
                                                    .config_quality_brightness_max = 0.4,
                                                    .config_quality_brightness_min = -0.5,
#elif defined(KOKS_FEATURE_KDS)
                                                    .config_quality_brightness_max = 0.5,
                                                    .config_quality_brightness_min = -0.5,
#else
                                                    .config_quality_brightness_max = 0.1,
                                                    .config_quality_brightness_min = -0.7,
#endif
                                                    .config_sharpness_threshold = 0.227,
                                                    .config_face_middle_max = pose_angle.enroll_angle.pitch.max, //入库
                                                    .config_face_middle_min = pose_angle.enroll_angle.pitch.min,
                                                    .config_face_middle_yaw_max = pose_angle.enroll_angle.yaw.max, //入库
                                                    .config_face_middle_yaw_min = pose_angle.enroll_angle.yaw.min,
                                                    .config_face_left_max = -8,
                                                    .config_face_left_min = -60,
                                                    .config_face_right_max = 60,
                                                    .config_face_right_min = 8,
                                                    .config_face_up_max = -18,
                                                    .config_face_up_min = -60,
                                                    .config_face_down_max = 60,
                                                    .config_face_down_min = 8,
                                                    .config_face_roll_max = pose_angle.enroll_angle.roll.max, //入库
                                                    .config_face_roll_min = pose_angle.enroll_angle.roll.min,
                                                    .config_face_up_padding = 3,
                                                    .config_face_down_padding = 5,
                                                    .config_face_up_range = 50,
                                                    .config_face_down_range = 50,
                                                    .config_face_middle_range = 20,
#ifdef KOKS_SDK_VERSION_V2
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M35)
                                                    .config_face_close_threshold = 195, // 2026.02.05更新
                                                    .config_face_far_threshold = 54,    // 2026.02.05更新
                                                    .config_face_iou_threshold = 0.27,  // 0.3
#elif defined(KOKS_PROJECT_M33)
                                                    .config_face_close_threshold = 185, // 2026.02.05更新
                                                    .config_face_far_threshold = 36,    // 2026.03.17更新
                                                    .config_face_iou_threshold = 0.27,  // 0.3
#else
                                                    .config_face_close_threshold = 175, // 7.14更新
                                                    .config_face_far_threshold = 47,    // 7.14更新
                                                    .config_face_iou_threshold = 0.27,  // 0.3
#endif
#elif defined(KOKS_PROJECT_M55) // KDS M5-5 项目
                                                    .config_face_close_threshold = 165, // 7.14更新
                                                    .config_face_far_threshold = 42,    // 7.14更新
                                                    .config_face_iou_threshold = 0.1,     // 0.3
#elif defined(KOKS_FEATURE_KDS) // 其他 KDS 项目
                                                    .config_face_close_threshold = 175, // 7.14更新
                                                    .config_face_far_threshold = 53,    // 7.14更新
                                                    .config_face_iou_threshold = 0.27,  // 0.3
#elif defined(KOKS_PROJECT_M53B)
                                                    .config_face_close_threshold = 165, // 7.14更新
                                                    .config_face_far_threshold = 53,    // 7.14更新
                                                    .config_face_iou_threshold = 0.1,     // 0.3
#else
                                                    .config_face_close_threshold = 165, // 7.14更新
                                                    .config_face_far_threshold = 42,    // 7.14更新
                                                    .config_face_iou_threshold = 0.1,     // 0.3
#endif
                                                    .config_left_eye_occlusion_max_count = 3,
                                                    .config_right_eye_occlusion_max_count = 3,
                                                    .config_left_eyebrow_occlusion_max_count = 15,
                                                    .config_right_eyebrow_occlusion_max_count = 15,
                                                    .config_face_occlusion_max_count = 5,
                                                    .config_enable_enroll_hackness = 1,
                                                    .config_spot_detect_threshold = 0.05 };

Senselock* FaceUnlockWrapper::sLock = NULL;
static FaceUnlockWrapper* gInstance = NULL;

FaceUnlockWrapper::FaceUnlockWrapper() : m_attribute_state_info(0)
#ifndef KOKS_SDK_VERSION_V2
	, mVerifyModelStatus(VERIFY_MODEL_STANDBY)
#endif
{
    sLock = Senselock::getInstance();
    m_moduleConfig = &st_module_config_640_480;
#ifndef KOKS_SDK_VERSION_V2
    CLEAR(mVerifyModelInfo);
#endif
}

int FaceUnlockWrapper::init()
{
    if (rota_has_flag()) {
        LOG_ERROR("Last OTA incomplete, disable face\n");
        return -1;
    }

    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::ThresholdLevelStrategy levelStrategy;
    m_moduleConfig = &st_module_config_640_480;
    int sdk_init_result = st_face_module_init(m_moduleConfig);
    if (ST_OK != sdk_init_result) {
        LOG_INFO("sdk init failed, sdk_init_result = %d\n", sdk_init_result);
        return -1;
    }

#ifdef KOKS_FEATURE_PALM
    if(ST_OK != SenselockHandler::getInstance()->palmInit()) {
        return -1;
    }
#endif

    s_msg_algo_threshold_level level;
    if (db.readData(&levelStrategy, &level) == ST_ERRNO_SUCCESS) {
        LOG_DEBUG("verify_threshold_level %d,liveness_threshold_level %d\n", level.verify_threshold_level, level.liveness_threshold_level);
#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
        if (level.verify_threshold_level > 4 || level.liveness_threshold_level > 4) {
#elif defined(KOKS_FEATURE_KDS)
        if (level.verify_threshold_level > 4 || level.liveness_threshold_level > 4) {
#else
        if (level.verify_threshold_level > 6 || level.liveness_threshold_level > 6) {
#endif
            level.verify_threshold_level = DEFAULT_VERIFY_LEVEL;
            level.liveness_threshold_level = DEFAULT_LIVENESS_LEVEL;
            db.writeData(&levelStrategy, &level);
        }
        st_module_param_config.config_hackness_threshold = liveness_threshold_level[level.liveness_threshold_level];
        st_module_param_config.config_verify_threshold = verify_threshold_level[level.verify_threshold_level];
        st_module_param_config.config_verify_rgb_threshold = verify_rgb_threshold_level[level.verify_threshold_level];
    }

    st_module_param_config.config_verify_yaw_min = pose_angle.unlock_angle.yaw.min;
    st_module_param_config.config_verify_yaw_max = pose_angle.unlock_angle.yaw.max;
    st_module_param_config.config_verify_roll_min = pose_angle.unlock_angle.roll.min;
    st_module_param_config.config_verify_roll_max = pose_angle.unlock_angle.roll.max;
    st_module_param_config.config_verify_pitch_min = pose_angle.unlock_angle.pitch.min;
    st_module_param_config.config_verify_pitch_max = pose_angle.unlock_angle.pitch.max;

#ifndef KOKS_SDK_VERSION_V2
    st_module_param_config.keypointStabilizer_win_size = KEYPOINT_STABILIZER_WIN_SIZE;
    st_module_param_config.keypointStabilizer_thresh = 10;
#endif

    s_msg_blur_config blur;
    FaceUnlockWrapper::getInstance()->get_blur_config(&blur);
    st_face_module_set_enable_blur(blur.enable);
    LOG_INFO("st_face_module_set_enable_blur %d, threshold %.3f\n", blur.enable, blur.blur_threshold);
    st_module_param_config.config_sharpness_threshold = blur.blur_threshold;
    st_face_module_set_param_config(&st_module_param_config);

    s_msg_misc_ai_config config;
    get_misc_ai_config(&config);
    LOG_INFO("get_misc_ai_config beard_enable %d, device_type %d\n", config.beard_enable, config.device_type);
    st_face_module_set_enable_full_beard(config.beard_enable);
    LOG_DEBUG("st_face_module_set_param_config\n");
#ifdef DISABLE_LIVENESS
    st_face_module_set_enable_liveness(0);
#else
    st_face_module_set_enable_liveness(1);
#endif
    LOG_DEBUG("st_face_module_set_enable_liveness\n");
    sLock->mFaceBrightnessLimit.brightness_max = st_module_param_config.config_quality_brightness_max;
    sLock->mFaceBrightnessLimit.brightness_min = st_module_param_config.config_quality_brightness_min;

    st_face_module_set_max_user_count(MAX_FACE_USER_COUNTS);
    LOG_DEBUG("config_hackness_threshold %f,config_verify_threshold %f,config_verify_rgb_threshold %f,config_face_middle_max %f,config_face_middle_min %f\n",
              st_module_param_config.config_hackness_threshold, st_module_param_config.config_verify_threshold, st_module_param_config.config_verify_rgb_threshold,
              st_module_param_config.config_face_middle_max, st_module_param_config.config_face_middle_min);
    return 0;
}
#ifndef KOKS_SDK_VERSION_V2
int FaceUnlockWrapper::initFaceModule()
{
    return st_face_module_init(m_moduleConfig);
}

void FaceUnlockWrapper::outputFaceRect(st_image_t* image, std::vector<st_rect_t>* rect)
{
    m_moduleConfig->face_rect(image, rect);
    return;
}

void FaceUnlockWrapper::getParamConfig(st_module_param_config_t& paramConfig)
{
    memcpy(&paramConfig, &st_module_param_config, sizeof(st_module_param_config_t));
    return;
}
#endif
FaceUnlockWrapper* FaceUnlockWrapper::getInstance()
{
    if (gInstance == NULL)
        gInstance = new FaceUnlockWrapper();

    return gInstance;
}

void FaceUnlockWrapper::updateParamConfig(s_msg_algo_threshold_level* threLevel)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::ThresholdLevelStrategy levelStrategy;
    st_module_param_config.config_verify_threshold = verify_threshold_level[threLevel->verify_threshold_level];
    st_module_param_config.config_verify_rgb_threshold = verify_rgb_threshold_level[threLevel->verify_threshold_level];
    st_module_param_config.config_hackness_threshold = liveness_threshold_level[threLevel->liveness_threshold_level];
    st_face_module_set_param_config(&st_module_param_config);
    db.writeData(&levelStrategy, threLevel);
    return;
}

void FaceUnlockWrapper::getLevelParamConfig(s_msg_algo_threshold_level* threLevel)
{
    int verify_level_ = DEFAULT_VERIFY_LEVEL;
    int liveness_level_ = DEFAULT_LIVENESS_LEVEL;
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::ThresholdLevelStrategy levelStrategy;
    if (db.readData(&levelStrategy, threLevel) == ST_ERRNO_SUCCESS) {
        verify_level_ = threLevel->verify_threshold_level;
        liveness_level_ = threLevel->liveness_threshold_level;
        LOG_INFO("saved the threshold level verify: %d liveness %d\n", threLevel->verify_threshold_level, threLevel->liveness_threshold_level);
    }
    threLevel->verify_threshold_level = verify_level_;
    threLevel->liveness_threshold_level = liveness_level_;
    return;
}

void FaceUnlockWrapper::updateAlgoThreshold(s_msg_algo_threshold* threshold)
{
    s_msg_algo_threshold_level level = { 5, 5 };
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::ThresholdLevelStrategy levelStrategy;
    if (db.writeData(&levelStrategy, &level)) {
        LOG_ERROR("store the threLevel failed\n");
    }
    st_module_param_config.config_verify_threshold = threshold->verify_threshold;
    st_module_param_config.config_verify_rgb_threshold = threshold->verify_threshold;
    st_module_param_config.config_hackness_threshold = threshold->liveness_threshold;
    st_face_module_set_param_config(&st_module_param_config);
    st::sense_koks::ThresholdStrategy thresholdStrategy;
    if (db.writeData(&thresholdStrategy, threshold)) {
        LOG_ERROR("store the threshold failed\n");
        return;
    }
    return;
}

void FaceUnlockWrapper::getAlgoThreshold(s_msg_algo_threshold* threshold)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    float verify_threshold_ = 0.0f;
    float liveness_threshold_ = 0.0f;
    st::sense_koks::ThresholdStrategy thresholdStrategy;
    if (db.readData(&thresholdStrategy, threshold) == ST_ERRNO_SUCCESS) {
        verify_threshold_ = threshold->verify_threshold;
        liveness_threshold_ = threshold->liveness_threshold;
        LOG_DEBUG("saved the threshold  verify: %f liveness %f\n", threshold->verify_threshold, threshold->liveness_threshold);
    }
    threshold->verify_threshold = verify_threshold_;
    threshold->liveness_threshold = liveness_threshold_;
    return;
}

#ifndef KOKS_SDK_VERSION_V2
void FaceUnlockWrapper::loadVerifyModel(void)
{
    if (mVerifyModelStatus == VERIFY_MODEL_STANDBY) {
        std::string model_name = MODEL_NAME_VERIFY;
        if (st_face_module_get_model_info(model_name.c_str(), &mVerifyModelInfo)) {
            LOG_INFO("get veirfy model info ERROR\n");
            mVerifyModelStatus = VERIFY_MODEL_ERROR;
        } else {
            LOG_INFO("verify model: %s %s, w_point 0x%x, w_size %d, code_point 0x%x, code_size %d\n", mVerifyModelInfo.w_name, mVerifyModelInfo.code_name,
                     mVerifyModelInfo.weight_point, mVerifyModelInfo.weight_size, mVerifyModelInfo.code_point, mVerifyModelInfo.code_size);
            Senselock* slock = Senselock::getInstance();
            mVerifyModelStatus = VERIFY_MODEL_LOADING;
        }
    }
    return;
}

void FaceUnlockWrapper::updateVerifyModel(void)
{
    std::string model_name = MODEL_NAME_VERIFY;
    if (!st_face_module_update_model(model_name.c_str(), mVerifyModelInfo)) {
        mVerifyModelStatus = VERIFY_MODEL_LOADED;
    } else {
        LOG_DEBUG("update veirfy model info ERROR\n");
        mVerifyModelStatus = VERIFY_MODEL_ERROR;
    }
    return;
}

verify_model_status FaceUnlockWrapper::waitVerifyModelLoaded(void)
{
    int times = 20000;

    if (mVerifyModelStatus == VERIFY_MODEL_LOADED || mVerifyModelStatus == VERIFY_MODEL_ERROR)
        return mVerifyModelStatus;

    while (mVerifyModelStatus != VERIFY_MODEL_LOADED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        times--;
        if (times < 0) {
            mVerifyModelStatus = VERIFY_MODEL_ERROR;
            break;
        }
    }

    return mVerifyModelStatus;
}
#endif

void FaceUnlockWrapper::set_misc_ai_config(s_msg_misc_ai_config config)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::AimiscStrategy aimiscStrategy;
    if (db.writeData(&aimiscStrategy, &config)) {
        LOG_ERROR("store the misc ai config failed\n");
        return;
    }
    st_face_module_set_enable_full_beard(static_cast<int>(config.beard_enable));
    LOG_INFO("set misc ai config beard_enable %d device_type %d\n", config.beard_enable, config.device_type);
    return;
}

void FaceUnlockWrapper::get_misc_ai_config(s_msg_misc_ai_config* config)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::AimiscStrategy aimiscStrategy;
    s_msg_misc_ai_config _config = { 0, 0 };
    int ret = db.readData(&aimiscStrategy, &_config);
    if (ret == 0) {
        config->beard_enable = _config.beard_enable;
        config->device_type = _config.device_type;
    } else {
        config->beard_enable = _config.beard_enable;
        config->device_type = _config.device_type;
        LOG_INFO("readRawFile failed!default set {0,0}\n");
    }
    return;
}

void FaceUnlockWrapper::set_blur_config(s_msg_blur_config blur)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::BlurStrategy blurStrategy;
    if (blur.enable > 1) {
        LOG_INFO("blur set failed with enable=%d\n", blur.enable);
        return;
    }
    if (db.writeData(&blurStrategy, &blur)) {
        LOG_ERROR("store the blur config failed\n");
        return;
    }
    st_face_module_set_enable_blur(static_cast<int>(blur.enable));
    st_module_param_config.config_sharpness_threshold = blur.blur_threshold;
    st_face_module_set_param_config(&st_module_param_config);
    LOG_INFO("blur enable %d, blur threshold %f\n", blur.enable, blur.blur_threshold);
    return;
}

void FaceUnlockWrapper::get_blur_config(s_msg_blur_config* config)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::BlurStrategy blurStrategy;
    s_msg_blur_config blur;
    if (db.readData(&blurStrategy, &blur) == ST_ERRNO_SUCCESS) {
        config->enable = blur.enable;
        config->blur_threshold = blur.blur_threshold;
    } else {
        config->enable = 0;
        config->blur_threshold = 0.0f;
        LOG_WARNING("read blur config failed!default set {0,0}\n");
        st_face_module_set_enable_blur(0);
        st_module_param_config.config_sharpness_threshold = 0.0f;
        st_face_module_set_param_config(&st_module_param_config);
    }
    return;
}
