/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 20:11:30
 * @LastEditors: 洪照
 * @LastEditTime: 2026-07-21 15:26:50
 * @FilePath: /koks1/application/communication/src/senselock_handler.cpp
 */
#include "senselock_handler.h"
#include "app_flash_otp.h"
#include "factory.h"
#include "os_utils.h"
#include "partition_guard.h"
#include "rota_common.h"
#include "soft_codec.h"
#include "st_face_data.h"
#include "st_koks_db.h"
#include "st_koks_isp.h"
#include "st_koks_led.h"
#include "st_koks_mediamanager.h"
#include "st_koks_uvc.h"
#include "sys_logger.h"
#include "rota_common.h"
#include "koks_project_def.h"
#include "uart.h"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#define BYTE2WORD(b, offset) ((b[offset] << 8) | b[offset + 1])

#ifdef KOKS_FEATURE_PALM
#define DEFAULT_VERIFY_LEVEL (1)
#define DEFAULT_LIVENESS_LEVEL (3)

const static float palm_verify_threshold_level[5] = { 0.65, 0.67, 0.68, 0.69, 0.70 };
const static float palm_verify_rgb_threshold_level[5] = { 0.7, 0.8, 0.9, 0.93, 0.95 };
const static float palm_liveness_threshold_level[6] = {1.0, 0.755503, 0.575799 , 0.404802, 0.124198 , 0.085120 };
static st_module_palm_param_config_t default_palm_param_config = {
    .config_align_threshold = 0.1f,
    .config_landmark_threshold = 0.7f,
    .config_hackness_threshold = palm_liveness_threshold_level[DEFAULT_LIVENESS_LEVEL],
    .config_verify_threshold = palm_verify_threshold_level[DEFAULT_VERIFY_LEVEL],
    .config_enroll_verify_threshold = palm_verify_rgb_threshold_level[DEFAULT_VERIFY_LEVEL],
    .config_quality_brightness_max = 0.17f,
    .config_quality_brightness_min = -0.36f,
#if defined(KOKS_PROJECT_M33)
    .config_palm_close_threshold = 236.0f,
    .config_palm_far_threshold = 52.0f,
#elif defined(KOKS_PROJECT_M24CP) || defined(KOKS_PROJECT_M53A)
    .config_palm_close_threshold = 254.0f,
    .config_palm_far_threshold = 80.0f,
#else
    .config_palm_close_threshold = 270.0f,
    .config_palm_far_threshold = 85.0f,
#endif
    .config_palm_detect_rect_margin = 20.0f,
    .config_palm_align_rect_margin = 30.0f,
    .config_palm_sharpness_threshold = 0.05f,
    .config_pixel_to_image_edge = 5,
    .config_palm_min_drift_threshold = 15,
    .config_palm_max_drift_threshold = 30,
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
    .palm_levle = { _kLow, _kLow, (PalmConfidenceLevel)DEFAULT_VERIFY_LEVEL, (PalmConfidenceLevel)DEFAULT_LIVENESS_LEVEL }, //{ _kNormal, _kNormal, _kNormal, _kNormal },
};
#endif

#ifdef KOKS_LOAD_MODEL_FROM_SFC
const uint32_t model_offset = 0x100000;
constexpr const char *sfc_device = "/dev/ingenic_sfc1";
#endif
uint8_t g_sdk_verify = 0;
constexpr const char* data_save_path = "/tmp/";
using namespace st::sense_koks;
s_note_data_face face_data = { .state = 0 | 0, .left = 0, .top = 0, .right = 0, .bottom = 0, .yaw = 0, .pitch = 0, .roll = 0 };

void SenselockHandler::nv12_to_I420(const st_image_t* image_src, st_image_t* image_dst)
{
    int frame_size = image_src->width * image_src->height;
    int chroma_size = frame_size / 4;

    unsigned char* nv12_y = image_src->data;
    unsigned char* nv12_uv = image_src->data + frame_size;

    unsigned char* i420_y = image_dst->data;
    unsigned char* i420_u = i420_y + frame_size;
    unsigned char* i420_v = i420_u + chroma_size;

    // Copy Y plane
    memcpy(i420_y, nv12_y, frame_size);

    // Convert UV planes
    for (int i = 0; i < chroma_size; i++) {
        i420_u[i] = nv12_uv[2 * i];
        i420_v[i] = nv12_uv[2 * i + 1];
    }
}

// Senselock* SenselockHandler::sLock = NULL;
static SenselockHandler* gInstance = NULL;
bool JpgImageReady = false;

SenselockHandler::SenselockHandler()
{
    sLock = Senselock::getInstance();
}

SenselockHandler* SenselockHandler::getInstance()
{
    if (gInstance == NULL)
        gInstance = new SenselockHandler();

    return gInstance;
}

void SenselockHandler::handleSetUTCTime(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    uint32_t second_elapse = msg->data[0] << 24 | msg->data[1] << 16 | msg->data[2] << 8 | msg->data[3];
    LOG_INFO("set UTC time: %u\n", second_elapse);
    sLock->setUTCTime(second_elapse);
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::getLibraryVersion()
{
    LOG_DEBUG("get library version\n");
    sLock->mState = MS_BUSY;
    s_msg_reply_library_version_data version_data = { 0 };
    st_result_t ret = st_face_module_get_version(version_data.library_version_info);
    if (ret != ST_OK) {
        LOG_ERROR("fet sdk library version failed.\n");
        sLock->reply(MID_GETLIBRARY_VERSION, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    } else {
        LOG_DEBUG("library version: %s\n", version_data.library_version_info);
        sLock->reply(MID_GETLIBRARY_VERSION, MR_SUCCESS, &version_data, sizeof(s_msg_reply_library_version_data));
    }
    sLock->mState = MS_STANDBY;
    return;
}

int SenselockHandler::mount_pictures_partition()
{
#if 0
    auto isMounted = [](const std::string& path) {
        std::string command = "mountpoint -q " + path + " " + "&& echo \"y\" || echo \"n\"";
        char result[64] = { 0 };
        LOG_INFO("cmd %s\n", command.c_str());
        if (utils_shell(command.data(), result, sizeof(result) - 1) != 0) {
            printf("Init failed\n");
            return -1;
        }
        if (result[0] == 'y') {
            LOG_INFO("Mount %s is ok\n", path.c_str());
            return 0;
        } else {
            LOG_WARNING("Mount %s is not mounted\n", path.c_str());
            return -1;
        }
    };
    if (isMounted(data_save_path) == 0) {
        LOG_INFO("Mount %s is ok\n", data_save_path);
        return 0;
    } else {
        std::string command = "mount -t jffs2 -o ro /dev/mtdblock6 " + std::string(data_save_path);
        char result[64] = { 0 };
        LOG_INFO("cmd %s\n", command.c_str());
        if (utils_shell(command.data(), result, sizeof(result) - 1) != 0) {
            printf("Mount failed\n");
            return -1;
        } else {
            LOG_INFO("Mount %s is ok\n", data_save_path);
            return 0;
        }
    }
#endif
    return 0;
}

void SenselockHandler::handleMsgEnroll(s_msg* msg, uint8_t enroll_type)
{
#ifndef KOKS_SDK_VERSION_V2
    uint32_t total_start_time, total_end_time, single_start_time;
    uint32_t event_flag;
    st_module_algo_info sdkLogInfo = { 0 };
    s_msg_reply_enroll_data reply_enroll_data = { 0 };
    s_msg_enroll_data* enroll_data = (s_msg_enroll_data*)msg->data;
    uint8_t msgEnroll = msg->mid;
    int ret = ST_OK;

    mEarlyVerifyResult.ret = -1;

    if (enroll_type == FACE_ENROLL_TYPE_INTERACTIVE) {
        // LOG_ERROR("ST_MODULE_ENROLL_MODE_INTERACTIVE is not exist\n");
        st_face_module_set_enroll_mode(ST_MODULE_ENROLL_MODE_INTERACTIVE);
    } else if (enroll_type == FACE_ENROLL_TYPE_SINGLE) {
        LOG_DEBUG("st_face_module_set_enroll_mode ST_MODULE_ENROLL_MODE_NORMAL\n");
        st_face_module_set_enroll_mode(ST_MODULE_ENROLL_MODE_NORMAL);
    } else {
        LOG_ERROR("unsupport msgEnroll, msg = %d\n", msgEnroll);
        sLock->reply(msgEnroll, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    }

    int sdk_status = 0;
    int sdk_user_id = 0;
    int frame_id = 0;
    bool first_enroll = true;
    int timeout = 10 * 1000;
    if (enroll_data->timeout > 0) {
        timeout = enroll_data->timeout * 1000 - 1300;
    }
    LOG_DEBUG("enroll_data->timeout %d\n", enroll_data->timeout);
    uint8_t occlusion_percent = 0;
    int failed_detection_count = 0;
    LOG_DEBUG("%s in", __func__);
    total_start_time = GetCurTimeMs();
    total_end_time = total_start_time;
    sLock->mState = MS_BUSY;

    std::vector<st_rect_t> roi_frames = {
        { 0, 0, 160, 214 },     
        { 160, 0, 320, 214 },   
        { 320, 0, 480, 214 },   
        { 0, 214, 160, 428 },   
        { 160, 214, 320, 428 }, 
        { 320, 214, 480, 428 }, 
        { 0, 428, 160, 640 },   
        { 160, 428, 320, 640 }, 
        { 320, 428, 480, 640 }  
    };
    int current_roi_index = 0; // 用来循环选择框

    // mount blockX to save pictures
    if (sLock->mDebugMode) {
        if (mount_pictures_partition()) {
            sLock->reply(msgEnroll, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            sLock->mState = MS_STANDBY;
            return;
        }
    }

    // start camera TODO: check the result
    this->StartMessageNotifyThread();
    // wait until all enrolling process done
    while (!Senselock::mAbort) {
        if (GET_TIMEDIFF(total_start_time, total_end_time) >= timeout) {
            LOG_WARNING("E:Timeout\n");
            sLock->reply(msgEnroll, MR_FAILED4_TIMEOUT, NULL, 0);
            break;
        }
        if (KOKS_RET_OK != sLock->GetCameraFrame()) {
            LOG_ERROR("open camera failed\n");
            sLock->reply(msgEnroll, MR_FAILED4_CAMERA, NULL, 0);
            sLock->mState = MS_STANDBY;
            return;
        }
        std::unique_lock<std::mutex> lock(sLock->mFrameQLock);
        sLock->mImageState = C_STATE_NO_FRAME;
        lock.unlock();

        single_start_time = GetCurTimeMs();
        LOG_DEBUG("buffer is ready!\n");

        LOG_DEBUG("first_image = 0x%lx, second_image = 0x%lx, first_pixel_format= %d, second_pixel_format= %d,orientation = %d, width = %d, height = %d\n",
                  sLock->mImage.first_image, sLock->mImage.second_image, sLock->mImage.first_pixel_format, sLock->mImage.second_pixel_format, sLock->mImage.orientation,
                  sLock->mImage.width, sLock->mImage.height);

        // timeout = 10;
        LOG_DEBUG("msg->data enroll direction: %d\n", enroll_data->face_direction);
        if (sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE || sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_FAR) {
            s_note_data_face _face_state;
            memset(&_face_state, 0, sizeof(s_note_data_face));
            int16_t state;
            if (sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE) {
                state = FACE_STATE_TOOCLOSE;
            } else {
                state = FACE_STATE_TOOFAR;
            }
            _face_state.state = state;
            _face_state.state |= (0 << 8);
            sLock->sendMessage(NID_FACE_STATE, &_face_state, sizeof(s_note_data_face));
        }
        st_face_module_set_enroll_direction(enroll_data->face_direction);
        ret = st_face_module_enroll(&sLock->mImage, &sdk_status, &sdk_user_id, &occlusion_percent, first_enroll);
        first_enroll = false;
        occlusion_percent = 0;
        total_end_time = GetCurTimeMs();
        LOG_INFO("enroll time : %d ms, sdk_status:%d result:%d\n", total_end_time - single_start_time, sdk_status, ret);
        if (sdk_status == ST_FACE_MODULE_STATUS_DETECT_FAILED) {
            failed_detection_count++; // 检测失败次数增加
            LOG_DEBUG("Face detection failed, sdk_status = %d\n", sdk_status);
#if 0
            if (failed_detection_count >= 5) {
                // 检测失败超过5次时，手动调用setCameraROI，按顺序传入 ROI
                LOG_INFO("Detection failed 5 times, switching to manual ROI\n");
                std::vector<st_rect_t> manual_roi = { roi_frames[current_roi_index], roi_frames[current_roi_index] }; // 传入当前ROI
                sLock->setCameraROI(&manual_roi);                                                                     
                current_roi_index = (current_roi_index + 1) % roi_frames.size();                                      // 循环使用九个框                                                                                        
            }
#endif
            // 由于30帧的IR图像噪点多，当检测失败达到5次时切换到效果更好的15帧。
            if (failed_detection_count == 3) {
                Isp::GetInstance().SetSensorFrameRate(15);
            }
        } else {
            // 重置失败计数
            failed_detection_count = 0;
            current_roi_index = 0;
        }
        int enroll_status = 0;
        st_face_module_get_enroll_status(&enroll_status);
        LOG_DEBUG(">>>>Enroll status = %d result:%d enroll_status:%d\n", sdk_status, ret, enroll_status);
        LOG_DEBUG("E:f=%d t=%d st= %d\n", frame_id, GET_TIMEDIFF(single_start_time, total_end_time), sdk_status);
        if ((sdk_status == ST_FACE_MODULE_STATUS_ENROLL_OK) || (sdk_status == ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE) ||
            (sdk_status == ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH) || (sdk_status == ST_FACE_MODULE_STATUS_FACE_DIRECTION_UP) ||
            (sdk_status == ST_FACE_MODULE_STATUS_FACE_DIRECTION_DOWN) || (sdk_status == ST_FACE_MODULE_STATUS_FACE_DIRECTION_LEFT)) {
            st_face_module_get_algo_info(&sdkLogInfo);
            LOG_DEBUG("S:i=%d, q=%d,v=%d,l=%d\n", int(sdkLogInfo.iou_score * 1000), int(sdkLogInfo.quality_score * 1000), int(sdkLogInfo.verify_score * 1000),
                      int(sdkLogInfo.liveness_score * 1000));
            LOG_DEBUG("S:v_id=%d,f_r= [%d,%d,%d,%d]\n", sdkLogInfo.verify_id, sdkLogInfo.face_rect.left, sdkLogInfo.face_rect.top, sdkLogInfo.face_rect.right,
                      sdkLogInfo.face_rect.bottom);
        }
        if (ST_OK != ret) {
            sLock->reply(msgEnroll, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }

        // the frame_id indicate how much frames we checked.
        frame_id++;

        // save images in debug mode
        if (sLock->mDebugMode) {
            int debug_ret = saveDebugImage(&sLock->mImage);
            if (debug_ret != 0) {
                LOG_ERROR("save debug image failed\n");
            }
        }

        // the following cases no need to break the while
        if (ST_OK == ret) {

            switch (sdk_status) {
            case ST_FACE_MODULE_STATUS_DETECT_FAILED:
            case ST_FACE_MODULE_STATUS_ALIGN_FAILED:
            case ST_FACE_MODULE_STATUS_FACE_IOU_ERROR:
                memset(&face_data, 0, sizeof(s_note_data_face));
                face_data.state = FACE_STATE_NOFACE;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_VERIFY_FAILED:
            case ST_FACE_MODULE_STATUS_HACKNESS_FAILED:
            case ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED:
            case ST_FACE_MODULE_STATUS_QUALITY_FAILED:
            case ST_FACE_MODULE_STATUS_GET_FEATURE_FAILED:
            case ST_FACE_MODULE_STATUS_UNFINISHED:
                face_data.state = FACE_STATE_NORMAL;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                LOG_DEBUG("Abnormal SDK status %d, continue\n", sdk_status);
                if (sdk_status == ST_FACE_MODULE_STATUS_HACKNESS_FAILED || sdk_status == ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED) {
                    LOG_WARNING("hackness failed %d\n", sdk_status);
                }
                break;
            case ST_FACE_MODULE_STATUS_FACE_UP:
                face_data.state = FACE_STATE_TOOUP;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_DOWN:
                face_data.state = FACE_STATE_TOODOWN;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_LEFT:
                face_data.state = FACE_STATE_TOOLEFT;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_RIGHT:
                face_data.state = FACE_STATE_TOORIGHT;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE:
                face_data.state = FACE_STATE_TOOCLOSE;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_TOO_FAR:
                face_data.state = FACE_STATE_TOOFAR;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_LEFT_EYEBROW_OCCLUSION:
            case ST_FACE_MODULE_STATUS_RIGHT_EYEBROW_OCCLUSION:
                face_data.state = FACE_STATE_EYEBROW_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_LEFT_EYE_OCCLUSION:
            case ST_FACE_MODULE_STATUS_RIGHT_EYE_OCCLUSION:
            case ST_FACE_MODULE_STATUS_BOTH_EYE_OCCLUSION:
                face_data.state = FACE_STATE_EYE_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_OCCLUSION:
                face_data.state = FACE_STATE_FACE_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_BLUR_FAILED:
                face_data.state = FACE_STATE_BLUR;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_DIRECTION_ERROR:
                face_data.state = FACE_STATE_DIRECTION_ERROR;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
            default:
                break;
            }
#ifdef KOKS_FEATURE_DSM
            this->SetNotifyMessagedata(NID_FACE_STATE, &face_data);
#else
            sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
#endif // DEBUG
        }

        if (sdk_status >= ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE && sdk_status <= ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH) {
            sync();
        }

        if (ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE == sdk_status) {
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }
        if (ST_FACE_MODULE_STATUS_FACE_DIRECTION_UP == sdk_status) {
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }
        if (ST_FACE_MODULE_STATUS_FACE_DIRECTION_DOWN == sdk_status) {
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }
        if (ST_FACE_MODULE_STATUS_FACE_DIRECTION_LEFT == sdk_status) {
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }
        if (ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH == sdk_status) {
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }

        if (ST_FACE_MODULE_STATUS_CREATE_FOLDER_FAILED == sdk_status || ST_FACE_MODULE_STATUS_SAVE_TEMPLATE_FAILED == sdk_status) {
            LOG_ERROR("Enroll file opperation failed\n");
            sLock->reply(msgEnroll, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }

        if (ST_FACE_MODULE_STATUS_FACE_ENROLLED == sdk_status) {
            sLock->reply(msgEnroll, MR_FAILED4_FACEENROLLED, NULL, 0);
            break;
        }

        if (ST_FACE_MODULE_STATUS_MAXIMUM_USER_NUMBER == sdk_status) {
            sLock->reply(msgEnroll, MR_FAILED4_MAXUSER, NULL, 0);
            break;
        }

        if (ST_FACE_MODULE_STATUS_MORE_HACKNESS_FAILED == sdk_status) {
            LOG_DEBUG("Reached limited hackness count, enroll failed...\n");
            LOG_WARNING("hack limited\n");
            sLock->reply(msgEnroll, MR_FAILED4_LIVENESSCHECK, NULL, 0);
            break;
        }

        if (ST_FACE_MODULE_STATUS_ENROLL_OK == sdk_status) {
            // TODO: reply the enroll command
            memset(&face_data, 0, sizeof(s_note_data_face));
            st_module_user_t user_info;
            memset(&user_info, 0, sizeof(st_module_user_t));
            user_info.id = sdk_user_id;
            user_info.admin = enroll_data->admin;
            user_info.rgb_enroll = 0;
            memcpy(user_info.name, enroll_data->user_name, sizeof(uint8_t) * USER_NAME_SIZE);
            ret = st_face_module_update_user(&user_info);
            if (ret != ST_OK) {
                LOG_ERROR(">>>>Enroll failed to update user info\n");
            }
            sync();

            if (sLock->mDemoMode == 1) {
                LOG_INFO("Exit demo mode\n");
                sLock->mDemoMode = 0;
                st_face_module_set_unlock_mode(ST_MODULE_UNLOCK_MODE_NORMAL);
            }

            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = 31;
            sLock->reply(msgEnroll, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        }
        if (ST_FACE_MODULE_STATUS_ABORTED == sdk_status) {
            sLock->reply(msgEnroll, MR_ABORTED, NULL, 0);
            break;
        }
    }
    this->StopMessageNotifyThread();
    sLock->mState = MS_STANDBY;
#endif
}

/**
 * @name: handleSaveFeature
 * @brief: 特征入库
 * @param {s_msg} *msg 携带加密后的特征数据
 * @return {*}
 */
void SenselockHandler::handleMsgEnrollFeature(s_msg* msg)
{
    uint32_t total_start_time, total_end_time;
    uint32_t event_flag;

    st_module_algo_info sdkLogInfo = { 0 };
    s_msg_reply_enroll_data reply_enroll_data = { 0 };
    s_msg_enroll_feature_data* enroll_feature_data = (s_msg_enroll_feature_data*)(msg->data);
    uint8_t msgEnrolFeature = msg->mid;

    int sdk_status = 0;
    int sdk_user_id = 0;
    int timeout = 10;
    int ret = ST_OK;

    LOG_DEBUG("%s in\n", __func__);
    mEarlyVerifyResult.ret = -1;
    total_start_time = GetCurTimeMs();
    // gettimeofday(&total_start_time, NULL);
    sLock->mState = MS_BUSY;

    if (enroll_feature_data->timeout > 0) {
        timeout = enroll_feature_data->timeout * 1000;
    }

    LOG_DEBUG("enroll_data->timeout %d\n", enroll_feature_data->timeout);

    LOG_DEBUG("msg->data enroll direction: %d\n", enroll_feature_data->face_direction);
    st_face_module_set_enroll_direction(enroll_feature_data->face_direction);
    st_face_module_set_enroll_mode(ST_MODULE_ENROLL_MODE_NORMAL);
    st_face_module_set_enable_liveness(0);

    char aes_feature[AES_BASE64_FEATURE_SIZE];
#ifdef KOKS_SDK_VERSION_V2
    st_module_user_t user_info = { 0 };
    user_info.admin = enroll_feature_data->admin;
    memcpy(user_info.name, enroll_feature_data->user_name, sizeof(user_info.name));
    memcpy(aes_feature, enroll_feature_data->aes_feature, AES_BASE64_FEATURE_SIZE);
	ret = st_face_module_feature_enroll(aes_feature, (int)AES_BASE64_FEATURE_SIZE, &sdk_status, ST_MODULE_ENROLL_ID_MODE_INTERNAL, &sdk_user_id, user_info);
#else
    ret = st_face_module_feature_enroll(aes_feature, (int)AES_BASE64_FEATURE_SIZE, &sdk_status, &sdk_user_id);
#endif
    st_face_module_set_enable_liveness(1);
    total_end_time = GetCurTimeMs();

    int enroll_status = 0;
    st_face_module_get_enroll_status(&enroll_status);
    LOG_DEBUG(">>>>Enroll status = %d result:%d enroll_status:%d\n", sdk_status, ret, enroll_status);

    if (ST_OK != ret) {
        sLock->reply(msgEnrolFeature, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    } else {
        switch (sdk_status) {
        case ST_FACE_MODULE_STATUS_ENROLL_OK:
#ifndef KOKS_SDK_VERSION_V2
            st_module_user_t user_info;
            memset(&user_info, 0, sizeof(st_module_user_t));
            user_info.id = sdk_user_id;
            user_info.admin = enroll_feature_data->admin;
            user_info.rgb_enroll = 0;
            memcpy(user_info.name, enroll_feature_data->user_name, sizeof(uint8_t) * USER_NAME_SIZE);
            ret = st_face_module_update_user(&user_info);
            if (ret != ST_OK) {
                LOG_ERROR(">>>>Enroll failed to update user info\n");
            }
#endif
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = 31;
            sLock->reply(msgEnrolFeature, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_UP:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_DOWN:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_LEFT:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH:
            reply_enroll_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_enroll_data.user_id_leb = sdk_user_id & 0xFF;
            reply_enroll_data.face_direction = enroll_status;
            sLock->reply(msgEnrolFeature, MR_SUCCESS, &reply_enroll_data, sizeof(s_msg_reply_enroll_data));
            break;
        case ST_FACE_MODULE_STATUS_CREATE_FOLDER_FAILED:
        case ST_FACE_MODULE_STATUS_SAVE_TEMPLATE_FAILED:
            sLock->reply(msgEnrolFeature, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;

        case ST_FACE_MODULE_STATUS_FACE_ENROLLED:
            sLock->reply(msgEnrolFeature, MR_FAILED4_FACEENROLLED, NULL, 0);
            break;

        case ST_FACE_MODULE_STATUS_MAXIMUM_USER_NUMBER:
            sLock->reply(msgEnrolFeature, MR_FAILED4_MAXUSER, NULL, 0);
            break;
        default:
            sLock->reply(msgEnrolFeature, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }
    }

    if (GET_TIMEDIFF(total_start_time, total_end_time) >= timeout) {
        LOG_WARNING("E:Timeout\n");
        LOG_WARNING("total_start_time %ld,total_end_time %ld\n", total_start_time, total_end_time);
        sLock->reply(msgEnrolFeature, MR_FAILED4_TIMEOUT, NULL, 0);
    }

    sLock->mState = MS_STANDBY;
    return;
}

int SenselockHandler::getSavingImageId(uint8_t* image_id)
{
    int ret_value = 0;
    // uint8_t last_image_id = 0;
    // FRESULT res;
    // UINT br;
    // // write file
    // res = f_open(&file_fp, DEBUG_PIC_ID_FILE, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    // if (res) {
    //     LOG_ERROR("can not open %s", DEBUG_PIC_ID_FILE);
    //     ret_value = -1;
    // } else {

    //     f_read(&file_fp, &last_image_id, sizeof(uint8_t), &br);
    //     if (br) {
    //         if (last_image_id >= DEBUG_PIC_START_ID && last_image_id < DEBUG_PIC_MAX_ID) {
    //             *image_id = last_image_id + 1;
    //         } else {
    //             *image_id = DEBUG_PIC_START_ID;
    //         }
    //     } else {
    //         *image_id = DEBUG_PIC_START_ID;
    //     }
    //     f_lseek(&file_fp, 0);
    //     f_write(&file_fp, image_id, sizeof(uint8_t), &br);
    //     f_close(&file_fp);
    //     ret_value = 0;
    // }
    return ret_value;
}

int SenselockHandler::saveDebugImage(st_module_image_t* raw_image)
{
#if 0
    uint8_t debug_pic_id;
    unsigned char* image_jpeg = NULL;
    int file_len = 0;
    int ret = getSavingImageId(&debug_pic_id);
    LOG_DEBUG("P:id=%d\n", debug_pic_id);
    if (ret == 0) {
        char first_pic_path[32] = { 0 };
        snprintf(first_pic_path, sizeof(first_pic_path), "%s/%d_f.jpg", DEBUG_INFO_PATH, debug_pic_id);

        ret = enc_raw_to_jpeg(sLock->mImage.first_image, sLock->mImage.width, sLock->mImage.height, &image_jpeg, &file_len, JPEG_FW_YUV400);
        LOGD("gray8 translate jpeg ret = %d upload_image_len = %d", ret, file_len);
        ret = writeFile(first_pic_path, (char*)image_jpeg, file_len);
        if (image_jpeg)
            os_mem_free(image_jpeg);
        if (ret != 0) {
            LOGE("%s save debug picture failed", __func__);
            return -1;
        }
        char second_pic_path[32] = { 0 };
        snprintf(second_pic_path, sizeof(second_pic_path), "%s/%d_s.jpg", DEBUG_INFO_PATH, debug_pic_id);

        st_image_t st_image = { sLock->mImage.second_image,
                                sLock->mImage.second_pixel_format, // ST_PIX_FMT_NV12
                                sLock->mImage.width,
                                sLock->mImage.height,
                                sLock->mImage.width,
                                0 };
        st_image_guard face_resize_guard;
        st_image_t* face_resize = nullptr;
        if (st_image.pixel_format == ST_PIX_FMT_NV12 && st_image.pixel_format != ST_PIX_FMT_BGR888) {
            image_allocate(st_image.width, st_image.height, ST_PIX_FMT_NV12, &face_resize);
            nv12_to_I420(&st_image, face_resize);
            face_resize_guard.reset(face_resize);
        } else {
            face_resize = const_cast<st_image_t*>(&st_image);
        }

        ret = enc_raw_to_jpeg(face_resize->data, sLock->mImage.width, sLock->mImage.height, &image_jpeg, &file_len, JPEG_FW_YUV420);
        LOGD("gray8 translate jpeg ret = %d upload_image_len = %d", ret, file_len);
        ret = writeFile(second_pic_path, (char*)image_jpeg, file_len);
        if (image_jpeg)
            os_mem_free(image_jpeg);
        if (ret != 0) {
            LOGE("%s save debug picture failed", __func__);
            return -1;
        }
    } else {
        LOGE("%s get debug picture id failed", __func__);
        return -1;
    }
#endif
    return 0;
}

void SenselockHandler::handleMsgFaceReset(s_msg* msg)
{
    int ret = ST_OK;

    LOG_DEBUG("The command of protocal is MID_FACERESET\n");
    sLock->mState = MS_BUSY;
    ret = st_face_module_reset();
    LOG_INFO("sdk_result: %d\n", ret);
    if (ret != ST_OK) {
        sLock->reply(MID_FACERESET, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    } else {
        sLock->reply(MID_FACERESET, MR_SUCCESS, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    memset(&face_data, 0, sizeof(s_note_data_face));
    return;
}

void SenselockHandler::handleMsgEnrollItg(s_msg* msg)
{
    s_msg_enroll_itg* enroll_itg = (s_msg_enroll_itg*)msg->data;
    if (enroll_itg->enable_duplicate) {
        st_face_module_set_enable_dup_enroll(1);
    } else {
        st_face_module_set_enable_dup_enroll(0);
    }

    s_msg_enroll_data enroll_data;
    enroll_data.admin = enroll_itg->admin;
    enroll_data.face_direction = enroll_itg->face_direction;
    enroll_data.timeout = enroll_itg->timeout;
    memcpy(enroll_data.user_name, enroll_itg->user_name, USER_NAME_SIZE * sizeof(uint8_t));

    int msg_buf_size = sizeof(s_msg) + sizeof(s_msg_enroll_data);
    uint8_t msg_buf[msg_buf_size];
    memset(msg_buf, 0, msg_buf_size * sizeof(uint8_t));
    s_msg* enroll_msg = (s_msg*)msg_buf;
    enroll_msg->mid = msg->mid;
    memcpy(enroll_msg->data, &enroll_data, sizeof(s_msg_enroll_data));

    switch (enroll_itg->enroll_type) {
    case FACE_ENROLL_TYPE_INTERACTIVE:
        handleMsgEnroll(enroll_msg, FACE_ENROLL_TYPE_INTERACTIVE);
        // LOG_ERROR("ST_MODULE_ENROLL_MODE_INTERACTIVE is not exist");
        break;
    case FACE_ENROLL_TYPE_SINGLE:
        handleMsgEnroll(enroll_msg, FACE_ENROLL_TYPE_SINGLE);
        break;
    case FACE_ENROLL_TYPE_RGB:
        handleMsgEnrollRGB(enroll_msg, enroll_itg->enable_duplicate);
        break;
    default:
        LOG_ERROR("unsupport msg = %d", msg->mid);
        sLock->reply(msg->mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        break;
    }

    st_face_module_set_enable_dup_enroll(1);
    return;
}

void SenselockHandler::handleMsgVerify(s_msg* msg)
{
#ifndef KOKS_SDK_VERSION_V2
    // usleep(40 * 1000); // wait for led on
    int ret = ST_OK;
    uint8_t occlusion_percent = 0;
    uint32_t event_flag;
    uint32_t total_start_time, total_end_time, single_start_time, eye_close_test_start_time;
    st_module_algo_info sdkLogInfo = { 0 };
    uint8_t attribute_state_info;
    int sdk_status = 0;
    int sdk_user_id = 0;
    int sdk_eye_close_user_id = 0;
    int frame_id = 0;
    int timeout = 10 * 1000;
    int verify_frame_count = 0;
    int unlock_eye_close_count = 0;
    bool unlock_close_eye_flag = false;
    uint8_t senselock_eye_close_mode = 0;
    int failed_detection_count = 0;

    LOG_DEBUG("Handling VERIFY...\n");
    g_sdk_verify = 1;
    total_start_time = GetCurTimeMs();

    sLock->mState = MS_BUSY;
    std::vector<st_rect_t> roi_frames = { { 0, 0, 160, 214 },     { 160, 0, 320, 214 }, { 320, 0, 480, 214 },   { 0, 214, 160, 428 },  { 160, 214, 320, 428 },
                                          { 320, 214, 480, 428 }, { 0, 428, 160, 640 }, { 160, 428, 320, 640 }, { 320, 428, 480, 640 } };
    int current_roi_index = 0; // 用来循环选择框

    st_face_module_set_enable_liveness(1);
    s_msg_verify_data* verify_data = (s_msg_verify_data*)msg->data;
    if (verify_data->timeout > 0) {
        timeout = verify_data->timeout * 1000 - 700 - 15 * verify_data->timeout;
    }
    // sdkWrapper->getAttriStateInfo(attribute_state_info);

    // mount blockX to save pictures
    if (sLock->mDebugMode && mount_pictures_partition()) {
        sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    // start the camera TODO: check the result
    this->StartMessageNotifyThread();
    // wait until all verifing process done
    while (!Senselock::mAbort) {
        if (GET_TIMEDIFF(total_start_time, GetCurTimeMs()) >= timeout) {
            LOG_WARNING("Timeout, verify failed!\n");
            sLock->reply(MID_VERIFY, MR_FAILED4_TIMEOUT, NULL, 0);
            break;
        }
        if (mEarlyVerifyResult.ret == 0 && (GetCurTimeMs() < 1000 || mEarlyVerifyResult.sdk_status == ST_FACE_MODULE_STATUS_NO_TEMPLATE)) {
            LOG_WARNING("Use early verify result\n");
            sdk_status = mEarlyVerifyResult.sdk_status;
            sdk_user_id = mEarlyVerifyResult.sdk_user_id;
            occlusion_percent = mEarlyVerifyResult.occlusion_percent;
            ret = mEarlyVerifyResult.ret;
            mEarlyVerifyResult.ret = -1;
            single_start_time = GetCurTimeMs();
        }
        else {
            mEarlyVerifyResult.ret = -1;
            if (KOKS_RET_OK != sLock->GetCameraFrame()) {
                LOG_ERROR("open camera failed\n");
                sLock->reply(MID_VERIFY, MR_FAILED4_CAMERA, NULL, 0);
                sLock->mState = MS_STANDBY;
                return;
            }
            std::unique_lock<std::mutex> lock(sLock->mFrameQLock);
            sLock->mImageState = C_STATE_NO_FRAME;
            lock.unlock();
            
            single_start_time = GetCurTimeMs();
            if (sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE || sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_FAR) {
                s_note_data_face _face_state;
                memset(&_face_state, 0, sizeof(s_note_data_face));
                int16_t state;
                if (sdk_status == ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE) {
                    state = FACE_STATE_TOOCLOSE;
                } else {
                    state = FACE_STATE_TOOFAR;
                }
                _face_state.state = state;
                _face_state.state |= (0 << 8);
                sLock->sendMessage(NID_FACE_STATE, &_face_state, sizeof(s_note_data_face));
            }
            ret = st_face_module_unlock(&sLock->mImage, &sdk_status, &sdk_user_id, &occlusion_percent);
        }
        occlusion_percent = 0;
        if (sdk_status == ST_FACE_MODULE_STATUS_DETECT_FAILED) {
            failed_detection_count++; // 检测失败次数增加
            LOG_DEBUG("Face detection failed, sdk_status = %d\n", sdk_status);
#if 0
            if (failed_detection_count >= 5) {
                // 检测失败超过5次时，手动调用setCameraROI，按顺序传入 ROI
                LOG_INFO("Detection failed 5 times, switching to manual ROI\n");
                std::vector<st_rect_t> manual_roi = { roi_frames[current_roi_index], roi_frames[current_roi_index] }; // 传入当前ROI
                sLock->setCameraROI(&manual_roi);
                current_roi_index = (current_roi_index + 1) % roi_frames.size(); // 循环使用九个框
            }
#endif
            // 由于30帧的IR图像噪点多，当检测失败达到5次时切换到效果更好的15帧。
            if (failed_detection_count == 3) {
                Isp::GetInstance().SetSensorFrameRate(15);
            }
        } else {
            // 重置失败计数
            failed_detection_count = 0;
            current_roi_index = 0;
        }

        total_end_time = GetCurTimeMs();
        LOG_INFO("unlock time : %d ms\n", total_end_time - single_start_time);

        LOG_VERBOSE("first_image = 0x%lx, second_image = 0x%lx, first_pixel_format= %d, second_pixel_format= %d,orientation = %d, width = %d, height = %d\n",
                    sLock->mImage.first_image, sLock->mImage.second_image, sLock->mImage.first_pixel_format, sLock->mImage.second_pixel_format, sLock->mImage.orientation,
                    sLock->mImage.width, sLock->mImage.height);
        LOG_DEBUG(">>>>Verifing sdk_status = %d sdk_result:%d\n", sdk_status, ret);
        // LOG_DEBUG("V:f=%d t=%d st=%d", frame_id, GET_TIMEDIFF(single_start_time, total_end_time), sdk_status);
        if ((sdk_status == ST_FACE_MODULE_STATUS_UNLOCK_OK) || (sdk_status == ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE)) {
        }
        if (ST_OK != ret) {
            sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }

        frame_id++;
        // save images in debug mode
        if (sLock->mDebugMode) {
            int debug_ret = saveDebugImage(&sLock->mImage);
            if (debug_ret != 0) {
                LOG_DEBUG("save debug image failed\n");
            }
        }
        // the following cases no need to break the while
        if (ST_OK == ret && false == unlock_close_eye_flag) {

            switch (sdk_status) {
            case ST_FACE_MODULE_STATUS_DETECT_FAILED:
            case ST_FACE_MODULE_STATUS_FACE_IOU_ERROR:
                memset(&face_data, 0, sizeof(s_note_data_face));
                face_data.state = FACE_STATE_NOFACE;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_VERIFY_FAILED:
            case ST_FACE_MODULE_STATUS_HACKNESS_FAILED:
            case ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED:
            case ST_FACE_MODULE_STATUS_QUALITY_FAILED:
            case ST_FACE_MODULE_STATUS_GET_FEATURE_FAILED:
                face_data.state = FACE_STATE_NORMAL;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                LOG_DEBUG("Abnormal SDK status %d, continue\n", sdk_status);
                if (sdk_status == ST_FACE_MODULE_STATUS_HACKNESS_FAILED || sdk_status == ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED) {
                    LOG_WARNING("hackness failed %d\n", sdk_status);
                }
                break;
            case ST_FACE_MODULE_STATUS_FACE_UP:
                face_data.state = FACE_STATE_TOOUP;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_DOWN:
                face_data.state = FACE_STATE_TOODOWN;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_LEFT:
                face_data.state = FACE_STATE_TOOLEFT;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_RIGHT:
                face_data.state = FACE_STATE_TOORIGHT;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE:
                face_data.state = FACE_STATE_TOOCLOSE;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_TOO_FAR:
                face_data.state = FACE_STATE_TOOFAR;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_LEFT_EYEBROW_OCCLUSION:
            case ST_FACE_MODULE_STATUS_RIGHT_EYEBROW_OCCLUSION:
                face_data.state = FACE_STATE_EYEBROW_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_LEFT_EYE_OCCLUSION:
            case ST_FACE_MODULE_STATUS_RIGHT_EYE_OCCLUSION:
            case ST_FACE_MODULE_STATUS_BOTH_EYE_OCCLUSION:
                face_data.state = FACE_STATE_EYE_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_FACE_OCCLUSION:
                face_data.state = FACE_STATE_FACE_OCCLUSION;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            case ST_FACE_MODULE_STATUS_POSE_ERROR:
                face_data.state = FACE_STATE_DIRECTION_ERROR;
                face_data.state |= (occlusion_percent << 8);
                // sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
                break;
            default:
                break;
            }

#ifdef KOKS_FEATURE_DSM       
            this->SetNotifyMessagedata(NID_FACE_STATE, &face_data);
#else
            sLock->sendMessage(NID_FACE_STATE, &face_data, sizeof(s_note_data_face));
#endif 
        }
        // 在闭眼检测模式下的超时判断
        if (unlock_close_eye_flag == true && GET_TIMEDIFF(eye_close_test_start_time, single_start_time) / 1000 >= 1) {
            LOG_DEBUG("eye_close_status time out\n");
            if (unlock_eye_close_count > 1) {
                LOG_DEBUG("FACE_STATE_EYE_CLOSE_STATUS....\n");
                senselock_eye_close_mode = FACE_STATE_EYE_CLOSE_STATUS;
            } else {
                LOG_DEBUG("FACE_STATE_EYE_CLOSE_UNKNOW_STATUS....\n");
                senselock_eye_close_mode = FACE_STATE_EYE_CLOSE_UNKNOW_STATUS;
            }

            sLock->sendMessage(NID_EYE_STATE, &senselock_eye_close_mode, sizeof(senselock_eye_close_mode));
            st_face_module_set_enable_liveness(1);
            break;
        }
        if ((ST_FACE_MODULE_STATUS_UNLOCK_OK == sdk_status) || (ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE == sdk_status)) {
            // TODO: reply the verify command
            st_module_user_t user_info;
            memset(&user_info, 0, sizeof(st_module_user_t));
            ret = st_face_module_get_user(sdk_user_id, &user_info);
            if (ret != ST_OK) {
                LOG_DEBUG("get user informaiton failed, ret = %d\n", ret);
            }
            s_msg_reply_verify_data reply_verify_data;
            reply_verify_data.user_id_heb = (sdk_user_id >> 8) & 0xFF;
            reply_verify_data.user_id_leb = sdk_user_id & 0xFF;
            memcpy(reply_verify_data.user_name, user_info.name, sizeof(uint8_t) * USER_NAME_SIZE);
            reply_verify_data.admin = user_info.admin;
            reply_verify_data.unlockStatus = sdk_status;

            if (0 == unlock_eye_close_count) {
                sLock->reply(MID_VERIFY, MR_SUCCESS, &reply_verify_data, sizeof(s_msg_reply_verify_data));
                if (ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE == sdk_status) {
                    LOG_DEBUG("start the eye close status....\n");
                    // 设置闭眼检测标志位
                    unlock_close_eye_flag = true;
                    // 保存闭眼检测成功的user_id
                    sdk_eye_close_user_id = sdk_user_id;
                    // 闭眼解锁计时开始
                    // gettimeofday(&eye_close_test_start_time, NULL);
                    eye_close_test_start_time = GetCurTimeMs();
                    // eyeclose模式1秒超时退出
                    timeout += 1000;
                    // 关闭活体检测
                    st_face_module_set_enable_liveness(0);
                }

            } else if ((unlock_close_eye_flag == true) && (ST_FACE_MODULE_STATUS_UNLOCK_OK == sdk_status)) {
                LOG_DEBUG("FACE_STATE_EYE_CLOSE_STATUS_OPEN_EYE....\n");
                senselock_eye_close_mode = FACE_STATE_EYE_CLOSE_STATUS_OPEN_EYE;
                sLock->sendMessage(NID_EYE_STATE, &senselock_eye_close_mode, sizeof(senselock_eye_close_mode));
                st_face_module_set_enable_liveness(1);
                break;
            }

            if ((ST_FACE_MODULE_STATUS_UNLOCK_OK == sdk_status) && (false == unlock_close_eye_flag)) {
                break;
            } else if (ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE == sdk_status) {

                if (sdk_eye_close_user_id == sdk_user_id) {
                    unlock_eye_close_count++;
                }
                LOG_DEBUG("unlock_eye_close++ = [%d]\n", unlock_eye_close_count);
                continue;
            }
        }
        if (false == unlock_close_eye_flag) {
            if (sdk_status == ST_FACE_MODULE_STATUS_UNLOCK_FAILED || sdk_status == ST_FACE_MODULE_STATUS_TEMPLDATE_FILE_DAMAGED) {
                sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNREASON, NULL, 0);
                break;
            }

            if (ST_FACE_MODULE_STATUS_NO_TEMPLATE == sdk_status) {
                sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNUSER, NULL, 0);
                break;
            }

            if (ST_FACE_MODULE_STATUS_ABORTED == sdk_status) {
                sLock->reply(MID_VERIFY, MR_ABORTED, NULL, 0);
                break;
            }

            if (ST_FACE_MODULE_STATUS_MORE_HACKNESS_FAILED == sdk_status) {
                LOG_DEBUG("Reached limited hackness count, verify failed...\n");
                LOG_WARNING("V:hack limited");
                sLock->reply(MID_VERIFY, MR_FAILED4_LIVENESSCHECK, NULL, 0);
                break;
            }

            if (ST_FACE_MODULE_STATUS_VERIFY_FAILED == sdk_status) {
                if (verify_frame_count++ >= VERIFY_LIMITED_FRAME_COUNT - 1) {
                    LOG_DEBUG("Reached limited verify count, verify failed...\n");
                    LOG_WARNING("V:verify more");
                    sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNUSER, NULL, 0);
                    break;
                }
            }

            if (GET_TIMEDIFF(total_start_time, total_end_time) >= timeout) {
                LOG_WARNING("Timeout, verify failed!\n");
                if (ST_FACE_MODULE_STATUS_VERIFY_FAILED == sdk_status)
                    sLock->reply(MID_VERIFY, MR_FAILED4_UNKNOWNUSER, NULL, 0);
                else
                    sLock->reply(MID_VERIFY, MR_FAILED4_TIMEOUT, NULL, 0);
                break;
            }
        }
    }
    this->StopMessageNotifyThread();
    sLock->mState = MS_STANDBY;
    LOG_DEBUG("Handling VERIFY done...\n");
#endif //KOKS_SDK_VERSION_V2
}

void SenselockHandler::handleMsgUploadImage(s_msg* msg, unsigned char* image_buf, int image_len)
{
    LOG_VERBOSE("start upload image ... \n");
    sLock->mState = MS_BUSY;
    s_msg_upload_image_data* upload_image_data = (s_msg_upload_image_data*)msg->data;
    int image_offset = (upload_image_data->upload_image_offset[0] << 24) + (upload_image_data->upload_image_offset[1] << 16) + (upload_image_data->upload_image_offset[2] << 8) +
                       upload_image_data->upload_image_offset[3];
    int image_size = (upload_image_data->upload_image_size[0] << 24) + (upload_image_data->upload_image_size[1] << 16) + (upload_image_data->upload_image_size[2] << 8) +
                     upload_image_data->upload_image_size[3];
    LOG_VERBOSE("upload image offset = %d, upload image size = %d\n", image_offset, image_size);
    if (image_offset > image_len) {
        sLock->reply(MID_UPLOADIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    if (image_size > image_len - image_offset) {
        sLock->reply(MID_UPLOADIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    s_status ret = sLock->sendImage(image_buf + image_offset, image_size);
    if (OK != ret) {
        sLock->reply(MID_UPLOADIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgGetDebugInfo(s_msg* msg, int& upload_debug_info_total_len)
{
#if 0
    LOG_DEBUG("start get debug informaiton");
    sLock->mState = MS_BUSY;
    int ret = ST_OK;
    int size = 0;
    if (!debug_info_buf)
        debug_info_buf = (char*)malloc(WQ_APP_DEMO_MID, 1 * 1024 * 1024);

#ifdef ENABLE_SAVE_LOG
    sync_logfile(sLock->mLogInfo, TRUE);
#endif

    if (debug_info_buf) {
        size = st_tar(DEBUG_INFO_PATH, debug_info_buf);
    } else {
        LOG_ERROR("malloc failed !");
        sLock->reply(MID_GET_DEBUG_INFO, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    auto encByte = [](char* data, int length) {
        char encrypt_xor[4] = { 0x41, 0x94, 0x31, 0x42 };
        for (int index = 0; index < length; index++) {
            data[index] = data[index] ^ encrypt_xor[index % 4];
            data[index] = ~data[index];
        }
    };
    encByte(debug_info_buf, size);

    upload_debug_info_total_len = size; // save debug file total size for uploading
    s_msg_reply_get_debug_info_data reply_get_debug_info_data;
    reply_get_debug_info_data.debug_file_size[0] = (size >> 24) & 0xFF;
    reply_get_debug_info_data.debug_file_size[1] = (size >> 16) & 0xFF;
    reply_get_debug_info_data.debug_file_size[2] = (size >> 8) & 0xFF;
    reply_get_debug_info_data.debug_file_size[3] = size & 0xFF;
    sLock->reply(MID_GET_DEBUG_INFO, MR_SUCCESS, &reply_get_debug_info_data, sizeof(s_msg_reply_get_debug_info_data));
    sLock->mState = MS_STANDBY;
#endif
}

void SenselockHandler::handleMsgUploadDebugInfo(s_msg* msg, int upload_debug_info_total_len)
{
#if 0
    LOG_DEBUG("start upload debug informaiton");
    sLock->mState = MS_BUSY;
    s_msg_upload_debug_info_data* upload_debug_info = (s_msg_upload_debug_info_data*)msg->data;
    int debug_info_offset = (upload_debug_info->upload_debug_info_offset[0] << 24) + (upload_debug_info->upload_debug_info_offset[1] << 16) +
                            (upload_debug_info->upload_debug_info_offset[2] << 8) + upload_debug_info->upload_debug_info_offset[3];
    int debug_info_size = (upload_debug_info->upload_debug_info_size[0] << 24) + (upload_debug_info->upload_debug_info_size[1] << 16) +
                          (upload_debug_info->upload_debug_info_size[2] << 8) + upload_debug_info->upload_debug_info_size[3];
    LOG_DEBUG("debug info total size = %d, offset = %d, upload size = %d", upload_debug_info_total_len, debug_info_offset, debug_info_size);
    if (debug_info_offset > upload_debug_info_total_len) {
        sLock->reply(MID_UPLOAD_DEBUG_INFO, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    if (debug_info_size > upload_debug_info_total_len - debug_info_offset) {
        sLock->reply(MID_UPLOAD_DEBUG_INFO, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    if (debug_info_buf != NULL) {
        s_status ret = sLock->sendImage(debug_info_buf + debug_info_offset, debug_info_size);
        if (OK != ret) {
            LOG_ERROR("send debug information file failed !");
            sLock->reply(MID_UPLOAD_DEBUG_INFO, MR_FAILED4_INVALIDPARAM, NULL, 0);
        }
    } else {
        LOG_ERROR("buffer is null");
        sLock->reply(MID_UPLOAD_DEBUG_INFO, MR_FAILED4_INVALIDPARAM, NULL, 0);
    }

    if (upload_debug_info_total_len <= (debug_info_size + debug_info_offset)) {
        int res = delDir(DEBUG_INFO_PATH);
        LOG_DEBUG("res: %d", res);
    }

    sLock->mState = MS_STANDBY;
#endif
}

void SenselockHandler::handleMsgGetImage(unsigned char* image_buf, int& image_len)
{
    int ret = ST_OK;
    LOG_DEBUG("start to copy image_jpeg\n");
    unsigned long image0_len = 0;
    unsigned long image1_len = 0;
    unsigned char* rgb0 = nullptr;
    unsigned char* rgb1 = nullptr;
    unsigned char* image0_jpeg = nullptr;
    unsigned char* image1_jpeg = nullptr;

    // Convert first image from YUV400 to RGB
    ret = st::sense_koks::SoftCodec::ConvertYUV400ToRGB(sLock->mImage.first_image, &rgb0, sLock->mImage.width, sLock->mImage.height);
    if (ret != ST_OK) {
        LOG_ERROR("Failed to convert YUV400 to RGB\n");
        goto cleanup;
    }

    // Encode first RGB image to JPEG
    ret = st::sense_koks::SoftCodec::EncodeRGBToJPEG(rgb0, sLock->mImage.width, sLock->mImage.height, &image0_jpeg, &image0_len, 70);
    if (ret != ST_OK) {
        LOG_ERROR("Failed to encode RGB to JPEG\n");
        goto cleanup;
    }

    // Convert second image from NV12 to RGB
    ret = st::sense_koks::SoftCodec::ConvertYUV400ToRGB(sLock->mImage.second_image, &rgb1, sLock->mImage.width, sLock->mImage.height);
    if (ret != ST_OK) {
        LOG_ERROR("Failed to convert NV12 to RGB\n");
        goto cleanup;
    }

    // Encode second RGB image to JPEG
    ret = st::sense_koks::SoftCodec::EncodeRGBToJPEG(rgb1, sLock->mImage.width, sLock->mImage.height, &image1_jpeg, &image1_len, 70);
    if (ret != ST_OK) {
        LOG_ERROR("Failed to encode RGB to JPEG\n");
        goto cleanup;
    }

    LOG_DEBUG("gray8 translate jpeg ret = %d image_len = %d", ret, image1_len);
    memset(image_buf, 0, sLock->mImage.width * sLock->mImage.height);
    LOG_DEBUG("start to copy image0_jpeg");
    memcpy(image_buf, image0_jpeg, image0_len);
    memcpy(image_buf + image0_len, image1_jpeg, image1_len);

    s_msg_reply_get_image_data reply_get_image_data;
    reply_get_image_data.image_size[0] = (image0_len >> 24) & 0xFF;
    reply_get_image_data.image_size[1] = (image0_len >> 16) & 0xFF;
    reply_get_image_data.image_size[2] = (image0_len >> 8) & 0xFF;
    reply_get_image_data.image_size[3] = image0_len & 0xFF;

    reply_get_image_data.image2_size[0] = (image1_len >> 24) & 0xFF;
    reply_get_image_data.image2_size[1] = (image1_len >> 16) & 0xFF;
    reply_get_image_data.image2_size[2] = (image1_len >> 8) & 0xFF;
    reply_get_image_data.image2_size[3] = image1_len & 0xFF;
    image_len = image0_len + image1_len;

    sLock->reply(MID_GETIMAGE, MR_SUCCESS, &reply_get_image_data, sizeof(s_msg_reply_get_image_data));

cleanup:
    if (rgb0 != nullptr) {
        free(rgb0);
        rgb0 = nullptr;
    }

    if (rgb1 != nullptr) {
        free(rgb1);
        rgb1 = nullptr;
    }

    if (image0_jpeg != nullptr) {
        free(image0_jpeg);
        image0_jpeg = nullptr;
    }

    if (image1_jpeg != nullptr) {
        free(image1_jpeg);
        image1_jpeg = nullptr;
    }

    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgGetRawImage(unsigned char* image_buf, int& image_len)
{
    LOG_DEBUG("get raw iamge ...\n");
    sLock->mState = MS_BUSY;
    memset(image_buf, 0, 2 * sLock->mImage.width * sLock->mImage.height);
    memcpy(image_buf, sLock->mImage.first_image, sLock->mImage.width * sLock->mImage.height); // GRAY8
    memcpy(image_buf + (sLock->mImage.width * sLock->mImage.height),                          // nv12
           sLock->mImage.second_image, sLock->mImage.width * sLock->mImage.height);
    image_len = 2 * sLock->mImage.width * sLock->mImage.height;
    s_msg_reply_get_raw_image_data reply_get_raw_image_data;
    reply_get_raw_image_data.image_size[0] = (image_len >> 24) & 0xFF;
    reply_get_raw_image_data.image_size[1] = (image_len >> 16) & 0xFF;
    reply_get_raw_image_data.image_size[2] = (image_len >> 8) & 0xFF;
    reply_get_raw_image_data.image_size[3] = image_len & 0xFF;
    sLock->reply(MID_GETRAWIMAGE, MR_SUCCESS, &reply_get_raw_image_data, sizeof(s_msg_reply_get_raw_image_data));
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleConfigBaudrate(s_msg* msg)
{
	uint8_t last_state = MS_STANDBY;
	if (sLock->mState == MS_OTA) {
		sLock->mState = MS_OTA;
	}
	sLock->mState = MS_BUSY;
    const uint32_t BR[] = {BANDRATE_115200, BANDRATE_230400, BANDRATE_460800, BANDRATE_1500000};
    uint8_t idx = msg->data[0] - 1;
    sLock->reply(MID_CONFIG_BAUDRATE, idx < 4 ? MR_SUCCESS : MR_FAILED4_INVALIDPARAM, NULL, 0);
    if (idx < 4) {
        LOGV("set baudrate to %d", BR[idx]);
        os_delay(20);
        sLock->mHost->config_uart(BR[idx]);
    }
    if (last_state == MS_OTA) {
        sLock->mState = MS_OTA;
    } else {
        sLock->mState = MS_STANDBY;
    }
}

void SenselockHandler::handleReboot(void)
{
    LOG_INFO("handleReboot\n");
    auto reboot_system = []() {
        sync();
        if (system("reboot") == -1) {
            LOG_ERROR("Failed to reboot the system");
        }
    };
    reboot_system();
}

void SenselockHandler::handleChangeReleaseKey(s_msg* msg)
{
    LOG_DEBUG("Change release ENC KEY!\n");
    sLock->mState = MS_BUSY;
    s_msg_enc_key_number_data* data = (s_msg_enc_key_number_data*)msg->data;
    if (sLock->mHost->checkKeyNumberLegal(data->enc_key_number)) {
        sLock->reply(msg->mid, MR_SUCCESS, NULL, 0);
        sLock->mHost->setKeyNumber(data->enc_key_number, RELEASE_ENC_KEY, true);
        sLock->mHost->ReConfigureKeyFromSavedMd5();
    } else {
        sLock->reply(msg->mid, MR_FAILED4_INVALIDPARAM, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgGetUserInfo(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    s_msg_getuserinfo_data* getuserinfo_data = (s_msg_getuserinfo_data*)(msg->data);
    uint16_t user_id = (getuserinfo_data->user_id_heb << 8) | getuserinfo_data->user_id_leb;
    if (user_id != 0) {
        st_module_user_t user_info;
        memset(&user_info, 0, sizeof(st_module_user_t));
        int ret = st_face_module_get_user(user_id, &user_info);
        LOG_DEBUG("user id: %d name: %s gender: %d\n", user_info.id, user_info.name, user_info.gender);
        if (ST_OK != ret) {
            if (ret == ST_FACE_MODULE_STATUS_NO_SUCH_USER) {
                sLock->reply(MID_GETUSERINFO, MR_FAILED4_UNKNOWNUSER, NULL, 0);
            } else {
                sLock->reply(MID_GETUSERINFO, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            }
        } else {
            s_msg_reply_getuserinfo_data user_reply_data;
            user_reply_data.user_id_heb = (user_info.id >> 8) & 0xFF;
            user_reply_data.user_id_leb = user_info.id & 0xFF;
            memcpy(user_reply_data.user_name, user_info.name, sizeof(uint8_t) * USER_NAME_SIZE);
            user_reply_data.admin = user_info.admin;
            sLock->reply(MID_GETUSERINFO, MR_SUCCESS, &user_reply_data, sizeof(s_msg_reply_getuserinfo_data));
        }
    } else {
        st_module_user_t* users_info = nullptr;
        int count = 0;
        int ret_users = st_face_module_get_all_users(&users_info, &count);
        if (ST_OK != ret_users) {
            sLock->reply(MID_GETUSERINFO, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        } else {
            uint8_t users_count = (uint8_t)count;
            sLock->reply(MID_GETUSERINFO, MR_SUCCESS, &users_count, sizeof(uint8_t));
        }
        if (users_info != nullptr) {
            int ret_delete = st_face_module_delete_users_buffer(users_info, count);
            if (ST_OK != ret_delete) {
                sLock->reply(MID_GETUSERINFO, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            }
            users_info = nullptr;
        }
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgwriteSN(s_msg* msg)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    SnStrategy snStrategy;
    sLock->mState = MS_BUSY;
    char* sn_data = (char*)msg->data;
    int sn_size = msg->size_heb << 8 | msg->size_leb;
    if (sn_size >= MAX_SN_SIZE - 1) {
        sLock->reply(MID_WRITE_SN, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }
    std::string sn(sn_data, sn_size);
    int ret = db.writeData(&snStrategy, &sn);
    if (ST_OK == ret) {
        sLock->reply(MID_WRITE_SN, MR_SUCCESS, NULL, 0);
    } else {
        sLock->reply(MID_WRITE_SN, MR_FAILED4_WRITE_FILE, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgSnapDualImage(s_msg* msg)
{
    LOG_DEBUG("snap dual image\n");
    uint32_t event_flag;
    sLock->mState = MS_BUSY;

    // int snap_image_counts = 0;
    // int save_image_sub_path = PICTURES_BEGIN_INDEX_NUMBER;
    // char save_path[32] = { 0 };
    // int frame_id = 0;

    // unsigned char* image_jpeg_first = NULL;
    // int len_jpeg_first = 0;
    // unsigned char* image_jpeg_second = NULL;
    // int len_jpeg_second = 0;

    // s_msg_snap_image_data* snap_image_data = (s_msg_snap_image_data*)msg->data;
    // snap_image_counts = snap_image_data->image_counts;
    // if (snap_image_counts < 1 || snap_image_counts > SINGLE_MAX_SNAP_IMAGE_COUNTS) {
    //     sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
    //     sLock->mState = MS_STANDBY;
    //     return;
    // }

    // if (KOKS_RET_OK != sLock->startCamera()) {
    //     sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_CAMERA, NULL, 0);
    //     sLock->mState = MS_STANDBY;
    //     return;
    // }

    // auto os_mem_deleter = [this](void* ptr) { free(ptr); };

    // while (!Senselock::mAbort) {
    //     MUTEX_LOCK(sLock->mFrameQLock);
    //     sLock->mImageState = C_STATE_NO_FRAME;
    //     MUTEX_UNLOCK(sLock->mFrameQLock);
    //     // int ret = os_wait_event(sLock->gEventFrameReadyHandle, 1000,&event_flag);
    //     int ret = os_pend_semaphore(sLock->gSemFrameReadyHandle, 1000);
    //     if (!ret) {
    //         LOG_ERROR("wait camera data timeout");
    //         sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_CAMERA, NULL, 0);
    //         break;
    //     }

    //     frame_id++;
    //     if (frame_id > IGNORED_FRAME_COUNTS + snap_image_counts) {
    //         LOG_DEBUG("snap %d images success", snap_image_counts);
    sLock->reply(MID_SNAP_DUAL_IMAGE, MR_SUCCESS, NULL, 0);
    //         break;
    //     }

    //     // @TODO: setCameraROI, Getimageinfo infterface isn't ready

    //     if (frame_id > IGNORED_FRAME_COUNTS) {
    //         if (save_image_sub_path > PICTURES_MAX_INDEX_NUMBER) {
    //             save_image_sub_path = PICTURES_BEGIN_INDEX_NUMBER;
    //         }
    //         // save fist image
    //         ret = enc_raw_to_jpeg(sLock->mImage.first_image, sLock->mImage.width, sLock->mImage.height, &image_jpeg_first, &len_jpeg_first, JPEG_FW_YUV400);
    //         std::unique_ptr<unsigned char[], decltype(os_mem_deleter)> image_jpeg_first_guard(image_jpeg_first, os_mem_deleter);
    //         if (ST_OK != ret) {
    //             sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    //         } else {
    //             // save image
    //             snprintf(save_path, sizeof(save_path), "%d.jpg", save_image_sub_path);
    //             LOG_DEBUG("save image path: %s", save_path);
    //             ret = writeFile(save_path, (char*)image_jpeg_first, len_jpeg_first);
    //             if (ST_OK != ret) {
    //                 sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_WRITE_FILE, NULL, 0);
    //             }
    //             save_image_sub_path++;
    //         }

    //         // save second image
    //         ret = enc_raw_to_jpeg(sLock->mImage.second_image, sLock->mImage.width, sLock->mImage.height, &image_jpeg_second, &len_jpeg_second, JPEG_FW_YUV420);
    //         std::unique_ptr<unsigned char[], decltype(os_mem_deleter)> image_jpeg_second_guard(image_jpeg_second, os_mem_deleter);
    //         if (ST_OK != ret) {
    //             sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    //         } else {
    //             // save image
    //             snprintf(save_path, sizeof(save_path), "%d.jpg", save_image_sub_path);
    //             LOG_DEBUG("save image path: %s", save_path);
    //             ret = writeFile(save_path, (char*)image_jpeg_second, len_jpeg_second);
    //             if (ST_OK != ret) {
    //                 sLock->reply(MID_SNAP_DUAL_IMAGE, MR_FAILED4_WRITE_FILE, NULL, 0);
    //             }
    //             save_image_sub_path++;
    //         }
    //     }
    // }

    // sLock->stopCamera();
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgSnapImage(s_msg* msg)
{
    LOG_DEBUG("snap image\n");
    mount_pictures_partition();
    uint32_t event_flag;
    int ret = ST_OK;
    unsigned long image0_len = 0;
    unsigned char* rgb0 = nullptr;
    unsigned char* image0_jpeg = nullptr;
    sLock->mState = MS_BUSY;

    uint8_t sensor = 0;
    int snap_image_counts = 0;
    int save_image_sub_path = 0;
    int frame_id = 0;
    int jpeg_len = 0;

    int status;
    char save_path[32] = { 0 };

    s_msg_snap_image_data* snap_image_data = (s_msg_snap_image_data*)msg->data;
    sensor = snap_image_data->sensor_id;
    if (sensor != 0 && sensor != 1) {
        sLock->reply(MID_SNAPIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }
    snap_image_counts = snap_image_data->image_counts;
    save_image_sub_path = snap_image_data->start_number;
    if (snap_image_counts < 1 || snap_image_counts > SINGLE_MAX_SNAP_IMAGE_COUNTS || save_image_sub_path < PICTURES_BEGIN_INDEX_NUMBER ||
        save_image_sub_path > PICTURES_MAX_INDEX_NUMBER) {
        sLock->reply(MID_SNAPIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    while (!Senselock::mAbort) {
        if (KOKS_RET_OK != sLock->GetCameraFrame()) {
            LOG_ERROR("open camera failed\n");
            sLock->reply(MID_VERIFY, MR_FAILED4_CAMERA, NULL, 0);
            sLock->mState = MS_STANDBY;
            return;
        }
        std::unique_lock<std::mutex> lock(sLock->mFrameQLock);
        sLock->mImageState = C_STATE_NO_FRAME;
        lock.unlock();

        frame_id++;
        if (frame_id > IGNORED_FRAME_COUNTS + snap_image_counts) {
            LOG_DEBUG("snap %d images success\n", snap_image_counts);
            sLock->reply(MID_SNAPIMAGE, MR_SUCCESS, NULL, 0);
            break;
        }

        // @TODO: setCameraROI, Getimageinfo infterface isn't ready

        if (frame_id > IGNORED_FRAME_COUNTS) {
            if (sensor == 0)
                ret = st::sense_koks::SoftCodec::ConvertYUV400ToRGB(sLock->mImage.first_image, &rgb0, sLock->mImage.width, sLock->mImage.height);
            else
                ret = st::sense_koks::SoftCodec::ConvertYUV400ToRGB(sLock->mImage.second_image, &rgb0, sLock->mImage.width, sLock->mImage.height);
            if (ret != ST_OK) {
                sLock->reply(MID_SNAPIMAGE, MR_FAILED4_UNKNOWNREASON, NULL, 0);
                LOG_ERROR("Failed to convert YUV400 to RGB\n");
                goto cleanup;
            }

            // Encode first RGB image to JPEG
            ret = st::sense_koks::SoftCodec::EncodeRGBToJPEG(rgb0, sLock->mImage.width, sLock->mImage.height, &image0_jpeg, &image0_len, 40);
            if (ST_OK != ret) {
                sLock->reply(MID_SNAPIMAGE, MR_FAILED4_UNKNOWNREASON, NULL, 0);
                LOG_ERROR("Failed to encode RGB to JPEG\n");
                goto cleanup;
            } else {
                // save image
                if (save_image_sub_path > PICTURES_MAX_INDEX_NUMBER) {
                    save_image_sub_path = PICTURES_BEGIN_INDEX_NUMBER;
                }

                snprintf(save_path, sizeof(save_path), "%s/%d.jpg", data_save_path, save_image_sub_path);
                LOG_DEBUG("save image path: %s\n", save_path);
                ret = WriteFile(std::string(save_path), reinterpret_cast<void*>(image0_jpeg), image0_len);
                if (ST_OK != ret) {
                    sLock->reply(MID_SNAPIMAGE, MR_FAILED4_WRITE_FILE, NULL, 0);
                }
                save_image_sub_path++;
                sLock->sendMessage(NID_SNAP_STATE, nullptr, 0);
            }
        }
    }
cleanup:
    if (rgb0 != nullptr) {
        free(rgb0);
        rgb0 = nullptr;
    }

    if (image0_jpeg != nullptr) {
        free(image0_jpeg);
        image0_jpeg = nullptr;
    }
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgGetSavedImage(s_msg* msg, unsigned char* upload_image_buf, int& upload_image_len)
{
    LOG_DEBUG("get saved image size... \n");
    mount_pictures_partition();
    sLock->mState = MS_BUSY;

    char saved_image_path[32] = { 0 };

    // data preprocessing
    s_msg_get_saved_image_data* saved_image_data = (s_msg_get_saved_image_data*)msg->data;
    if (saved_image_data->image_number < PICTURES_BEGIN_INDEX_NUMBER || saved_image_data->image_number > PICTURES_MAX_INDEX_NUMBER) {
        sLock->reply(MID_GETSAVEDIMAGE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        return;
    }

    // read image file and send it to host
    snprintf(saved_image_path, sizeof(saved_image_path), "%s/%d.jpg", data_save_path, saved_image_data->image_number);
    char* content = nullptr;
    int ret = ReadFile(std::string(saved_image_path), (void**)&content, upload_image_len);
    LOG_DEBUG("saved image path = %s, saved image size = %d\n", saved_image_path, upload_image_len);
    if (ST_OK != ret) {
        LOG_ERROR("read file failed\n");
        sLock->reply(MID_GETSAVEDIMAGE, MR_FAILED4_READ_FILE, NULL, 0);
    } else {
        memset(upload_image_buf, 0, 2.5 * sLock->mImage.width * sLock->mImage.height);
        memcpy(upload_image_buf, content, upload_image_len);
        s_msg_reply_get_saved_image_data reply_get_saved_image_data;
        reply_get_saved_image_data.image_size[0] = (upload_image_len >> 24) & 0xFF;
        reply_get_saved_image_data.image_size[1] = (upload_image_len >> 16) & 0xFF;
        reply_get_saved_image_data.image_size[2] = (upload_image_len >> 8) & 0xFF;
        reply_get_saved_image_data.image_size[3] = upload_image_len & 0xFF;
        sLock->reply(MID_GETSAVEDIMAGE, MR_SUCCESS, &reply_get_saved_image_data, sizeof(s_msg_reply_get_saved_image_data));
    }
    releaseFileContent(content);
    sLock->mState = MS_STANDBY;
}

#if defined(KOKS_PROJECT_M31) || defined(KOKS_PROJECT_M32) || defined(KOKS_PROJECT_M31A) || defined(KOKS_PROJECT_M33) || defined(KOKS_PROJECT_M35)
#define THRESHOLD_LEVEL 4
#elif defined(KOKS_FEATURE_KDS)
#define THRESHOLD_LEVEL 4
#else
#define THRESHOLD_LEVEL 6
#endif
void SenselockHandler::handleMsgSetThreLevel(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_SET_THRESHOLD_LEVEL\n");
    sLock->mState = MS_BUSY;
    s_msg_algo_threshold_level* level = (s_msg_algo_threshold_level*)msg->data;
    if (level->liveness_threshold_level < 0 || level->liveness_threshold_level > THRESHOLD_LEVEL
		|| level->verify_threshold_level < 0 || level->verify_threshold_level > THRESHOLD_LEVEL) {
        sLock->reply(MID_SET_THRESHOLD_LEVEL, MR_FAILED4_INVALIDPARAM, NULL, 0);
    } else {
        LOG_WARNING("MID_SET_THRESHOLD_LEVEL verify: %d liveness: %d\n", level->verify_threshold_level, level->liveness_threshold_level);
        FaceUnlockWrapper::getInstance()->updateParamConfig(level);
        sLock->reply(MID_SET_THRESHOLD_LEVEL, MR_SUCCESS, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgGetThreLevel(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_GET_THRESHOLD_LEVEL\n");
    sLock->mState = MS_BUSY;
    s_msg_algo_threshold_level level;
    FaceUnlockWrapper::getInstance()->getLevelParamConfig(&level);
    sLock->reply(MID_GET_THRESHOLD_LEVEL, MR_SUCCESS, &level, sizeof(s_msg_algo_threshold_level));
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgSetThreshold(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_SET_THRESHOLD\n");
    sLock->mState = MS_BUSY;
    s_msg_algo_threshold threshold;
    memcpy(&threshold, &msg->data, sizeof(s_msg_algo_threshold));
    if (threshold.liveness_threshold < 0.0f || threshold.liveness_threshold >= 10.0f || threshold.verify_threshold < 0.0f || threshold.verify_threshold >= 10.0f) {
        LOG_ERROR("threshold set failed with verify_threshold %f, liveness_threshold %f\n", threshold.verify_threshold, threshold.liveness_threshold);
        sLock->reply(MID_SET_THRESHOLD, MR_FAILED4_INVALIDPARAM, NULL, 0);
    } else {
        LOG_WARNING("MID_SET_THRESHOLD verify: %f liveness: %f\n", threshold.verify_threshold, threshold.liveness_threshold);
        FaceUnlockWrapper::getInstance()->updateAlgoThreshold(&threshold);
        sLock->reply(MID_SET_THRESHOLD, MR_SUCCESS, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgGetThreshold(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_GET_THRESHOLD\n");
    sLock->mState = MS_BUSY;
    s_msg_algo_threshold threshold;
    FaceUnlockWrapper::getInstance()->getAlgoThreshold(&threshold);
    sLock->reply(MID_GET_THRESHOLD, MR_SUCCESS, &threshold, sizeof(s_msg_algo_threshold));
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgMiscAiConfig(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_MISC_AI_CONFIG\n");
    sLock->mState = MS_BUSY;
    uint8_t rw_status = msg->data[0]; // 0 读 1 写
    if (rw_status == 0)               //读
    {
        s_msg_misc_ai_config config;
        FaceUnlockWrapper::getInstance()->get_misc_ai_config(&config);
        sLock->reply(MID_MISC_AI_CONFIG, MR_SUCCESS, &config, sizeof(s_msg_misc_ai_config));
        LOG_ERROR("read misc ai config\n");
    } else if (rw_status == 1) //写
    {
        s_msg_misc_ai_config misc_ai_config;
        memcpy(&misc_ai_config, &msg->data[1], sizeof(s_msg_misc_ai_config));
        if (misc_ai_config.beard_enable > 1 || misc_ai_config.device_type > 1) {
            LOG_ERROR("write date invalid misc_ai_config.beard_enable %d,misc_ai_config.device_type %d\n", misc_ai_config.beard_enable, misc_ai_config.device_type);
            sLock->reply(MID_MISC_AI_CONFIG, MR_FAILED4_INVALIDPARAM, NULL, 0);
            sLock->mState = MS_STANDBY;
            return;
        }
        FaceUnlockWrapper::getInstance()->set_misc_ai_config(misc_ai_config);
        LOG_ERROR("write misc ai config\n"); // misc_ai_config.beard_enable %d, misc_ai_config.device_type %d", misc_ai_config.beard_enable, misc_ai_config.device_type);
        sLock->reply(MID_MISC_AI_CONFIG, MR_SUCCESS, NULL, 0);
    } else {
        LOG_ERROR("not support cmd\n");
        sLock->reply(MID_MISC_AI_CONFIG, MR_FAILED4_INVALIDPARAM, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgBlurConfig(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is MID_BLUR_CONFIG\n");
    sLock->mState = MS_BUSY;
    uint8_t rw_status = msg->data[0]; // 0 读 1 写
    union data_type {
        uint8_t a[4];
        float b;
    };
    s_msg_blur_config blur;
    if (rw_status == 0) //读
    {
        FaceUnlockWrapper::getInstance()->get_blur_config(&blur);
        sLock->reply(MID_BLUR_CONFIG, MR_SUCCESS, &blur, sizeof(s_msg_blur_config));
        LOG_DEBUG("read misc ai config\n");
    } else if (rw_status == 1) //写
    {
        blur.enable = msg->data[1];
        if (blur.enable > 1) {
            LOG_ERROR("blur.enable %d failed\n", blur.enable);
            sLock->reply(MID_BLUR_CONFIG, MR_FAILED4_INVALIDPARAM, NULL, 0);
            sLock->mState = MS_STANDBY;
            return;
        }
        // union data_type *_blur = (union data_type *)&msg->data[2];
        union data_type _blur;
        memcpy(&_blur.a, &msg->data[2], 4);
        blur.blur_threshold = _blur.b;
        LOG_DEBUG("blur enable %d, blur threshold %f\n", blur.enable, blur.blur_threshold);
        FaceUnlockWrapper::getInstance()->set_blur_config(blur);
        sLock->reply(MID_BLUR_CONFIG, MR_SUCCESS, NULL, 0);
    } else {
        LOG_ERROR("not support cmd\n");
        sLock->reply(MID_BLUR_CONFIG, MR_FAILED4_INVALIDPARAM, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgGetLogFile(s_msg* msg, unsigned char** upload_log_buf, int& upload_log_len)
{
    LOG_DEBUG("get log file ...");
    sLock->mState = MS_BUSY;
    s_msg_reply_get_log_data reply_get_log_data;
    upload_log_len = 0;

    // free malloced log buf previously
    if (*upload_log_buf) {
        free(*upload_log_buf);
        *upload_log_buf = 0;
    }

    // sync_logfile(sLock->mLogInfo, FALSE);
    uint32_t log_addr;
    SysLogger::GetInstance()->GetFixedLogBufInfo(log_addr, upload_log_len);
    LOG_DEBUG("get log size %d", upload_log_len);
    *upload_log_buf = (unsigned char*)malloc(upload_log_len);
    if (*upload_log_buf == 0) {
        sLock->mState = MS_STANDBY;
        sLock->reply(MID_GET_LOGFILE, MR_FAILED4_NOMEMORY, NULL, 0);
        return;
    }
    memcpy(*upload_log_buf, reinterpret_cast<void*>(log_addr), upload_log_len);

    reply_get_log_data.log_size[0] = (upload_log_len >> 24) & 0xFF;
    reply_get_log_data.log_size[1] = (upload_log_len >> 16) & 0xFF;
    reply_get_log_data.log_size[2] = (upload_log_len >> 8) & 0xFF;
    reply_get_log_data.log_size[3] = upload_log_len & 0xFF;
    sLock->reply(MID_GET_LOGFILE, MR_SUCCESS, &reply_get_log_data, sizeof(s_msg_reply_get_log_data));
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgUploadLogFile(s_msg* msg, unsigned char* upload_log_buf, int upload_log_len)
{
    LOG_DEBUG("start upload logfile ... ");
    sLock->mState = MS_BUSY;

    if (!upload_log_buf) {
        sLock->reply(MID_UPLOAD_LOGFILE, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        sLock->mState = MS_STANDBY;
        LOG_ERROR("LOGBUFFER is nullptr ... ");
        return;
    }

    s_msg_upload_logfile_data* upload_logfile_data = (s_msg_upload_logfile_data*)msg->data;
    int logfile_offset = (upload_logfile_data->upload_logfile_offset[0] << 24) + (upload_logfile_data->upload_logfile_offset[1] << 16) +
                         (upload_logfile_data->upload_logfile_offset[2] << 8) + upload_logfile_data->upload_logfile_offset[3];
    int logfile_size = (upload_logfile_data->upload_logfile_size[0] << 24) + (upload_logfile_data->upload_logfile_size[1] << 16) +
                       (upload_logfile_data->upload_logfile_size[2] << 8) + upload_logfile_data->upload_logfile_size[3];
    LOG_DEBUG("upload image offset = %d, upload image size = %d", logfile_offset, logfile_size);
    if (logfile_offset > upload_log_len || (logfile_offset + logfile_size) > upload_log_len) {
        sLock->reply(MID_UPLOAD_LOGFILE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        LOG_ERROR("upload logfile size exseeds the actual size");
        return;
    }

    s_status ret = sLock->sendImage(upload_log_buf + logfile_offset, logfile_size);
    if (OK != ret) {
        sLock->reply(MID_UPLOAD_LOGFILE, MR_FAILED4_INVALIDPARAM, NULL, 0);
        sLock->mState = MS_STANDBY;
        LOG_ERROR("upload logfile failed :%d", ret);
        return;
    }
    sLock->mState = MS_STANDBY;
}

#ifdef KOKS_FEATURE_LCD_EMOTION
#include "LCDVedioPlayer.h"

void SenselockHandler::handleMsgEmotion(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    s_msg_eomtion_data* emotion_data = (s_msg_eomtion_data*)msg->data;
    LOG_INFO("handle emotion %u, voice %u, set default emotion %u\n", emotion_data->emotion_id, emotion_data->voice_id, emotion_data->default_emotion_id);
    if (emotion_data->emotion_id) {
        LCDVedioPlayer::getInstance()->play(emotion_data->emotion_id);
    }

    if (emotion_data->voice_id) {
        // TODO: play voice
    }

    if (emotion_data->default_emotion_id) {
        st::sense_koks::LcdEmotionStrategy EmotionStrategy;
        st::sense_koks::koks_db::GetInstance().writeData(&EmotionStrategy, &emotion_data->default_emotion_id);
    }

    sLock->replyNoEncrypt(MID_EMOTION, MR_SUCCESS, NULL, 0);
    sLock->mState = MS_STANDBY;
}
#endif

void SenselockHandler::format_fs(s_msg* msg)
{
    int ret = 0;
    LOG_DEBUG("The command of protocal is MID_FORMAT_FATFS\n");
    sLock->mState = MS_BUSY;
    // formatRawPartition();
    // ret = FormatFatfs();
    // if (ret == 0) {
    //     sLock->reply(MID_FORMAT_FATFS, MR_SUCCESS, NULL, 0);
    // } else {
    //     LOG_ERROR("format_fs failed %d", ret);
    sLock->reply(MID_FORMAT_FATFS, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    // }
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::restore_factory_settings(s_msg* msg)
{
    int ret = 0;
    std::string sourceDir = "/etc/koks/";
    std::string destinationDir = "/config/";
    std::string filesToCopy[] = { "config.json" };
    LOG_INFO("MID_RESTORE_FACTORY_SETTINGS\n");
    sLock->mState = MS_BUSY;

    // // backup sn
    // raw_file_t* file_sn;
    // int read_ret;
    // read_ret = readRawFile("sn.bin", &file_sn);

    // // format raw partition
    // formatRawPartition();

    // // restore sn
    // if (!read_ret) {
    //     writeRawFile("sn.bin", (char*)file_sn->body->data, file_sn->body->file_len);
    //     releaseRawFileContent(file_sn);
    // }

    // // format fatfs
    // ret = FormatFatfs();
    auto copyFile = [](const std::string& source, const std::string& destination) -> bool {
        std::ifstream src(source, std::ios::binary);
        if (!src) {
            LOG_ERROR("can not open: %s", source.c_str());
            return false;
        }

        std::ofstream dst(destination, std::ios::binary);
        if (!dst) {
            LOG_ERROR("can not open: %s", destination.c_str());
            return false;
        }

        dst << src.rdbuf();
        if (!dst) {
            LOG_ERROR("copy file failed: %s", destination.c_str());
            return false;
        }
        dst.flush();
        if (!dst) {
            LOG_ERROR("flush file failed: %s", destination.c_str());
            return false;
        }
        src.close();
        dst.close();
        LOG_DEBUG("copy file from %s to %s success", source.c_str(), destination.c_str());
        std::ifstream checkDst(destination, std::ios::binary);
        if (!checkDst) {
            LOG_ERROR("check copied file failed: %s", destination.c_str());
            return false;
        }
        checkDst.seekg(0, std::ios::end);
        std::streamsize dstSize = checkDst.tellg();
        checkDst.close();
        std::ifstream checkSrc(source, std::ios::binary);
        if (!checkSrc) {
            LOG_ERROR("check source file failed: %s", source.c_str());
            return false;
        }
        checkSrc.seekg(0, std::ios::end);
        std::streamsize srcSize = checkSrc.tellg();
        checkSrc.close();
        if (dstSize != srcSize) {
            LOG_ERROR("file size mismatch: %s (%lld) vs %s (%lld)", source.c_str(), srcSize, destination.c_str(), dstSize);
            return false;
        }
        return true;
    };
    PartitionGuard configGuard(CONFIG_FILE_ALL);
    for (const auto& filename : filesToCopy) {
        std::string src = sourceDir + filename;
        std::string dst = destinationDir + filename;

        if (!copyFile(src, dst)) {
            LOGE("Failed to copy %s to %s", src.c_str(), dst.c_str());
            sLock->reply(msg->mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        }
    }
    ret = st_face_module_delete_all_user();
#ifdef KOKS_FEATURE_PALM
    ret |= st_palm_module_delete_all_user();
#endif
    configGuard.SyncAndLock();
    if (ret == ST_OK) {
        LOG_DEBUG("delete all user success");
        sLock->reply(MID_RESTORE_FACTORY_SETTINGS, MR_SUCCESS, NULL, 0);
    } else {
        LOG_ERROR("delete all user failed %d", ret);
        sLock->reply(MID_RESTORE_FACTORY_SETTINGS, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    }

#if 0
    // 写入factory_setting_flag文件
    if (!WriteFlagFile("factory_setting_flag")) {
        sLock->reply(MID_RESTORE_FACTORY_SETTINGS, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        return;
    }
    if (ret == 0) {
        sLock->reply(MID_RESTORE_FACTORY_SETTINGS, MR_SUCCESS, NULL, 0);
    } else {
        LOG_ERROR("format_fs failed %d\n", ret);
        sLock->reply(MID_RESTORE_FACTORY_SETTINGS, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    }
#endif

    sLock->mState = MS_STANDBY;
    // boot_reset_immediately(BOOT_REASON_SOFT_APP, 0);
}

void SenselockHandler::handlePowerDown(s_msg* msg)
{
    LOG_DEBUG("The command of protocal is POWERDOWN\n");

    if (sLock->mState == MS_STANDBY) {
        sLock->mState = MS_BUSY;
        sync();
#ifdef ENABLE_SAVE_LOG
        sync_logfile(sLock->mLogInfo, FALSE);
#endif
        LOG_DEBUG("ready for powerdown\n");
        sLock->reply(MID_POWERDOWN, MR_SUCCESS, NULL, 0);
        // stop uart rx
        sLock->mHost->deinit();
    } else {
        LOG_ERROR("module busy\n");
        sLock->reply(MID_POWERDOWN, MR_REJECTED, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgSetRgbImageSize(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    // JpgImageReady = false;
    // s_msg_set_rgbimage_size_data* rgb_size = (s_msg_set_rgbimage_size_data*)msg->data;
    // sLock->mRGBImageBufferSize = rgb_size->rgbimage_size[3] << 24 | rgb_size->rgbimage_size[2] << 16 | rgb_size->rgbimage_size[1] << 8 | rgb_size->rgbimage_size[0];

    // if (sLock->mRGBImageBuffer) {
    //     free(sLock->mRGBImageBuffer);
    //     sLock->mRGBImageBuffer = NULL;
    // }

    // sLock->mRGBImageBuffer = (uint8_t*)malloc(WQ_APP_DEMO_MID, sLock->mRGBImageBufferSize);
    // if (NULL != sLock->mRGBImageBuffer) {
    //     sLock->reply(MID_SET_RGBIMAGE_SIZE, MR_SUCCESS, NULL, 0);
    //     LOG_DEBUG("rgbimage malloc memory %dsuccess!", sLock->mRGBImageBufferSize);
    // } else {
    //     LOG_ERROR("rgbimage malloc memory %d failed!", sLock->mRGBImageBufferSize);
    //     sLock->reply(MID_SET_RGBIMAGE_SIZE, MR_FAILED4_NOMEMORY, NULL, 0);
    // }
    sLock->reply(MID_SET_RGBIMAGE_SIZE, MR_SUCCESS, NULL, 0);
    LOG_DEBUG("rgbimage malloc memory %dsuccess!\n", sLock->mRGBImageBufferSize);
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgTransferRGBImage(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    sLock->reply(MID_TRANSFER_RGBIMAGE, MR_SUCCESS, NULL, 0);
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgEnrollRGB(s_msg* msg, uint8_t dup_status)
{
    mEarlyVerifyResult.ret = -1;

    sLock->mState = MS_BUSY;
    st_face_module_set_enable_liveness(0);
    st_face_module_set_enroll_mode(ST_MODULE_ENROLL_MODE_NORMAL);
    sLock->reply(msg->mid, MR_FAILED4_TIMEOUT, NULL, 0);
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleSetFaceInfo(st_module_face_info_t* face_info)
{
    face_data.left = (int16_t)face_info->left;
    face_data.top = (int16_t)face_info->top;
    face_data.right = (int16_t)face_info->right;
    face_data.bottom = (int16_t)face_info->bottom;
    face_data.yaw = (int16_t)face_info->yaw;
    face_data.pitch = (int16_t)face_info->pitch;
    face_data.roll = (int16_t)face_info->roll;
}

void SenselockHandler::handleReset(void)
{
    LOG_DEBUG("The command of protocal is RESET\n");
    sLock->mState = MS_STANDBY;
    // wait for the reply
    Senselock::mAbort = false;
    sLock->reply(MID_RESET, MR_SUCCESS, NULL, 0);
}

/**
 * @name: handleHardwareCheck
 * @brief: 处理硬件自检业务
 * @return {*}
 */
void SenselockHandler::handleHardwareCheck(void)
{
    sLock->reply(MID_HW_CHECK, MR_SUCCESS, NULL, 0);
}

void SenselockHandler::handleFactoryTest(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    g_FactoryTest = true;
    LOG_INFO("factory test request\n");
    char *platform_name = getenv("PLATFORM");
    char *project_name = getenv("PROJECT_MODE");
    LOG_INFO("platform name: %s\n", platform_name);
    auto version = rota_get_version();
    s_msg_reply_factroy_data factory_data = { 0 };
    strncpy((char*)&factory_data.version_info, version, sizeof(factory_data.version_info) - 1);
    strncpy((char*)&factory_data.platform_name, platform_name ? platform_name : "t32", sizeof(factory_data.platform_name) - 1);
    strncpy((char*)&factory_data.project_name, project_name ? project_name : "other", sizeof(factory_data.project_name) - 1);
    factory_data.support_factory_test = 1;
    sLock->reply(msg->mid, MR_SUCCESS, &factory_data, sizeof(s_msg_reply_factroy_data));
    sLock->mState = MS_STANDBY;
}


void SenselockHandler::StartMessageNotifyThread()
{
#ifdef KOKS_FEATURE_DSM  
    memset(&face_data, 0, sizeof(s_note_data_face));
    face_data.state = FACE_STATE_NOFACE; //默认状态是没有检测到人脸
    mMessageNotifyData.nid = NID_FACE_STATE;
    memset(mMessageNotifyData.data, 0, sizeof(mMessageNotifyData.data));
    this->SetNotifyMessagedata(NID_FACE_STATE, &face_data);
    bool expected = false;

    if (!mMessageNotifyThreadRunning.compare_exchange_strong(expected, true)) {
        return; // 已经启动
    }
    mMessageNotifyThread = std::thread(&SenselockHandler::MessageNotifyThreadFunc, this);
#endif //KOKS_FEATURE_DSM
}

void SenselockHandler::SetNotifyMessagedata(uint8_t nid, void *data)
{
#ifdef KOKS_FEATURE_DSM  
    std::lock_guard<std::mutex> lk(mMessageNotifyMutex);
    int size = nid == NID_FACE_STATE ? sizeof(s_note_data_face) : sizeof(s_note_data_palm);
    if(mMessageNotifyData.nid != nid || memcmp(mMessageNotifyData.data, data, size) != 0) {
        mMessageNotifyData.nid = nid;
        memcpy(mMessageNotifyData.data, data, size);
        mMessageNotifyHasNewData = true;
        mMessageNotifyCv.notify_one();
    }
#endif //KOKS_FEATURE_DSM
}

void SenselockHandler::StopMessageNotifyThread()
{
#ifdef KOKS_FEATURE_DSM    
    bool expected = true;
    if (!mMessageNotifyThreadRunning.compare_exchange_strong(expected, false)) {
        return; // 未运行
    }

    mMessageNotifyCv.notify_one();
    if (mMessageNotifyThread.joinable()) {
        mMessageNotifyThread.join();
    }
#endif //KOKS_FEATURE_DSM
}

void SenselockHandler::MessageNotifyThreadFunc()
{
#ifdef KOKS_FEATURE_DSM
    while (mMessageNotifyThreadRunning.load()) {
        std::unique_lock<std::mutex> lk(mMessageNotifyMutex);
        LOG_DEBUG("MessageNotifyThreadFunc waiting for new data...\n");
        if(SysLogger::GetInstance()->isNeed(ST_LEVEL_DEBUG))
        {
            //如果是debug级别日志，不需要定时唤醒线程，可以一直等到有新数据到来
            mMessageNotifyCv.wait(lk, [this] {
                return !mMessageNotifyThreadRunning.load() || mMessageNotifyHasNewData;
            });
        }
        else
        {
            mMessageNotifyCv.wait_for(
                    lk,
                    std::chrono::milliseconds(100),
                    [this] {
                        return !mMessageNotifyThreadRunning.load() || mMessageNotifyHasNewData;
                    });
        }

        if (!mMessageNotifyThreadRunning.load()) {
            break;
        }
        auto nid = mMessageNotifyData.nid;
        auto data = mMessageNotifyData.data;
        int size = nid == NID_FACE_STATE ? sizeof(s_note_data_face) : sizeof(s_note_data_palm);
        LOG_INFO("Send Note nid = %d, status = %d\n", nid, ((s_note_data_palm*)data)->state);
        sLock->sendMessage(nid, data, size);
        mMessageNotifyHasNewData = false;
    }
#endif //KOKS_FEATURE_DSM 
}

#ifdef KOKS_SDK_VERSION_V2
void SenselockHandler::sendNote(uint8_t note_id, int16_t state)
{
    if (note_id == NID_FACE_STATE) {
        face_data.state = state;
    #ifdef KOKS_FEATURE_DSM
        this->SetNotifyMessagedata(NID_FACE_STATE, &face_data);
    #else
        sLock->sendMessage(note_id, &face_data, sizeof(face_data));
    #endif
    } else {
        s_note_data_palm palm_data = { 0 };
        palm_data.state = state;
    #ifdef KOKS_FEATURE_DSM
        if(state == PALM_STATE_PALM_MOVE) {
            // DSM 协议没有PALM_STATE_PALM_MOVE字段，替换成PALM_STATE_NORMAL
            palm_data.state = PALM_STATE_NORMAL;
        }
        this->SetNotifyMessagedata(NID_PALM_STATUS, &palm_data);
    #else
        sLock->sendMessage(NID_PALM_STATUS, &palm_data, sizeof(palm_data));
    #endif
    }
}

int SenselockHandler::handleEnrollUnlockStatus(s_msg* msg, int uid, int orientation, int status)
{
    uint8_t mid = msg->mid;
    if (status >= ST_PALM_MODULE_STATUS_NORMAL) {
        if (status == ST_PALM_MODULE_STATUS_UNLOCK_OK) {
            s_msg_reply_verify_data reply;
            st_module_user_t user_info = { 0 };
            st_palm_module_get_user(uid, &user_info);
            reply.user_id_heb = (uid >> 8) & 0xFF;
            reply.user_id_leb = uid & 0xFF;
            memcpy(reply.user_name, user_info.name, sizeof(reply.user_name));
            reply.admin = user_info.admin;
            LOG_INFO("unlock uid=%d, admin=%d, name=%s, status = %d\n", uid, user_info.admin, user_info.name, status);
        #ifdef KOKS_FEATURE_KDS
            if(mid == MID_PALM_VERIFY) {
                sLock->reply(mid, MR_SUCCESS, &reply, sizeof(reply));
            }
            else {
                sLock->reply(MID_PALM_VERIFY_ONLY, MR_SUCCESS, &reply, sizeof(reply));
            }
        #else
            sLock->reply(MID_PALM_VERIFY_ONLY, MR_SUCCESS, &reply, sizeof(reply));
        #endif
            return 1;
        } else if (status == ST_PALM_MODULE_STATUS_PALM_ADDED) {
            uint8_t enroll_id[2] = { (uint8_t)(uid >> 8), (uint8_t)uid };
            uint8_t admin = ((s_msg_enroll_data*)msg->data)->admin;
            char *name = (char *)((s_msg_enroll_data*)msg->data)->user_name;
            LOG_INFO("palm enroll uid=%d, admin=%d, name=%s\n", uid, admin, name);
            sLock->reply(mid, MR_SUCCESS, &enroll_id, sizeof(enroll_id));
            return 1;
        } else if (status >= ST_PALM_MODULE_STATUS_NORMAL && status <= ST_PALM_MODULE_STATUS_OUT_RB) {
            uint8_t stat = status - ST_PALM_MODULE_STATUS_NORMAL + PALM_STATE_NORMAL;
            if (stat > PALM_STATE_PALM_WATER)
                stat++;
            sendNote(NID_PALM_STATUS, stat);
        } else if (status == ST_PALM_MODULE_STATUS_UNFINISHED) {
            sendNote(NID_PALM_STATUS, PALM_STATE_PALM_MOVE);
        } else if(status == ST_PALM_MODULE_STATUS_MORE_HACKNESS_FAILED) {
            LOG_WARNING("Reached limited hackness count, enroll failed...\n");
            sLock->reply(mid, MR_FAILED4_LIVENESSCHECK, nullptr, 0);
            return 1;
        } else {
            const int16_t ERR_ARR[] = {
                ST_PALM_MODULE_STATUS_NO_TEMPLATE,
                MR_FAILED4_PALM_NONE,
                ST_PALM_MODULE_STATUS_MAXIMUM_USER_NUMBER,
                MR_FAILED4_PALM_EXCEED_MAX,
                ST_PALM_MODULE_STATUS_TIMEOUT,
                MR_FAILED4_PALM_TIMEOUT,
                ST_PALM_MODULE_STATUS_ABORT,
                MR_FAILED4_PALM_ABORTED,
                ST_PALM_MODULE_STATUS_MODEL_MISSMATCH,
                MR_FAILED4_PALM_MISMATCH,
                ST_PALM_MODULE_STATUS_MODEL_TOO_LARGE,
                MR_FAILED4_PALM_MODEL_TOO_LARGE,
                ST_PALM_MODULE_STATUS_INTERFACE_EXCEPTION,
                MR_FAILED4_PALM_INTERFACE_ABNORMAL,
                ST_PALM_MODULE_STATUS_INTERFACE_ERROR,
                MR_FAILED4_PALM_INTERFACE_ERR,
            #ifndef KOKS_FEATURE_KDS
                ST_PALM_MODULE_STATUS_SWITCH_TO_PALM,
                MR_FAILED4_PALM_MODLE_SWITCH,
            #endif
                ST_PALM_MODULE_STATUS_SYS_ERROR,
                MR_FAILED4_UNKNOWNREASON,
                ST_PALM_MODULE_STATUS_UNKNOWN_ERROR,
                MR_FAILED4_UNKNOWNREASON,
                // ST_PALM_MODULE_STATUS_UNFINISHED, ?,
                ST_PALM_MODULE_STATUS_UNLOCK_FAILED,
                MR_FAILED4_PALM_MISMATCH,
                0,
            };
            const int16_t* err = ERR_ARR;
            for (; *err; err += 2) {
                if (err[0] == status) {
                    sLock->reply(mid, err[1], nullptr, 0);
                    return 1;
                }
            }
        }
    } else {
        const int8_t ERR_ARR[] = { ST_FACE_MODULE_STATUS_DETECT_FAILED,
                                   FACE_STATE_NOFACE,
                                   ST_FACE_MODULE_STATUS_FACE_IOU_ERROR,
                                   FACE_STATE_NOFACE,
                                   ST_FACE_MODULE_STATUS_ALIGN_FAILED,
                                   FACE_STATE_NOFACE,
                                   ST_FACE_MODULE_STATUS_HACKNESS_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_FAKE_HACKNESS_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_VERIFY_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_QUALITY_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_GET_FEATURE_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_FACE_UP,
                                   FACE_STATE_TOOUP,
                                   ST_FACE_MODULE_STATUS_FACE_DOWN,
                                   FACE_STATE_TOODOWN,
                                   ST_FACE_MODULE_STATUS_FACE_LEFT,
                                   FACE_STATE_TOOLEFT,
                                   ST_FACE_MODULE_STATUS_FACE_RIGHT,
                                   FACE_STATE_TOORIGHT,
                                   ST_FACE_MODULE_STATUS_FACE_TOO_CLOSE,
                                   FACE_STATE_TOOCLOSE,
                                   ST_FACE_MODULE_STATUS_FACE_TOO_FAR,
                                   FACE_STATE_TOOFAR,
                                   ST_FACE_MODULE_STATUS_LEFT_EYEBROW_OCCLUSION,
                                   FACE_STATE_EYEBROW_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_RIGHT_EYEBROW_OCCLUSION,
                                   FACE_STATE_EYEBROW_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_LEFT_EYE_OCCLUSION,
                                   FACE_STATE_EYE_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_RIGHT_EYE_OCCLUSION,
                                   FACE_STATE_EYE_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_BOTH_EYE_OCCLUSION,
                                   FACE_STATE_EYE_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_FACE_OCCLUSION,
                                   FACE_STATE_FACE_OCCLUSION,
                                   ST_FACE_MODULE_STATUS_POSE_ERROR,
                                   FACE_STATE_DIRECTION_ERROR,
                                   ST_FACE_MODULE_STATUS_BLUR_FAILED,
                                   FACE_STATE_NORMAL,
                                   ST_FACE_MODULE_STATUS_DIRECTION_ERROR,
                                   FACE_STATE_DIRECTION_ERROR,
                                   0 };
        const int8_t* err = ERR_ARR;
        for (; *err; err += 2) {
            if (err[0] == status) {
                sendNote(NID_FACE_STATE, err[1]);
                break;
            }
        }

        switch (status) {
        case ST_FACE_MODULE_STATUS_CREATE_FOLDER_FAILED:
        case ST_FACE_MODULE_STATUS_SAVE_TEMPLATE_FAILED:
            LOG_ERROR("Enroll file opperation failed\n");
            sLock->reply(mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_FACE_ENROLLED:
            sLock->reply(mid, MR_FAILED4_FACEENROLLED, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_MAXIMUM_USER_NUMBER:
            sLock->reply(mid, MR_FAILED4_MAXUSER, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_MORE_HACKNESS_FAILED:
            LOG_WARNING("Reached limited hackness count, enroll failed...\n");
            sLock->reply(mid, MR_FAILED4_LIVENESSCHECK, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_UP:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_DOWN:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_LEFT:
        case ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH:
        case ST_FACE_MODULE_STATUS_ENROLL_OK: {
            s_msg_reply_enroll_data reply;
            reply.user_id_heb = (uid >> 8) & 0xFF;
            reply.user_id_leb = uid & 0xFF;
            reply.face_direction = 31;
            if (status >= ST_FACE_MODULE_STATUS_FACE_DIRECTION_MIDDLE && status <= ST_FACE_MODULE_STATUS_FACE_DIRECTION_RIGTH ||
                mid >= MID_ENROLL_FACE_UP && mid <= MID_ENROLL_FACE_RIGHT) {
                // only for testing(face up/down/left/right), not save
                int enroll_status = 0;
                st_face_module_get_enroll_status(&enroll_status);
                reply.face_direction = enroll_status;
            } else {
                memset(&face_data, 0, sizeof(face_data));
            }
        #ifdef KOKS_FEATURE_KDS
            if(mid == MID_PALM_ENROLL_ITG) {
                sLock->reply(MID_ENROLL_ITG, MR_SUCCESS, &reply, sizeof(reply));
            }
            else {
        #endif
            sLock->reply(mid, MR_SUCCESS, &reply, sizeof(reply));
        #ifdef KOKS_FEATURE_KDS
            }
        #endif
            return 1;
        }
        case ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE:
        case ST_FACE_MODULE_STATUS_UNLOCK_OK: {
            st_module_user_t user_info = { 0 };
            int ret = st_face_module_get_user(uid, &user_info);
            if (ret != ST_OK) {
                LOG_DEBUG("get user informaiton failed, ret = %d\n", ret);
            }
            s_msg_reply_verify_data reply;
            reply.user_id_heb = (uid >> 8) & 0xFF;
            reply.user_id_leb = uid & 0xFF;
            memcpy(reply.user_name, user_info.name, sizeof(reply.user_name));
            reply.admin = user_info.admin;
            reply.unlockStatus = status;
        #ifdef KOKS_FEATURE_KDS
            sLock->reply(MID_VERIFY, MR_SUCCESS, &reply, sizeof(reply));
        #else
            sLock->reply(mid, MR_SUCCESS, &reply, sizeof(reply));
        #endif
            return 1;
        }
        case ST_FACE_MODULE_STATUS_UNLOCK_FAILED:
        case ST_FACE_MODULE_STATUS_TEMPLDATE_FILE_DAMAGED:
            sLock->reply(mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_NO_TEMPLATE:
            sLock->reply(mid, MR_FAILED4_UNKNOWNUSER, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_VERIFY_FAILED:
            sLock->reply(mid, MR_FAILED4_UNLOCK_FAIL, NULL, 0);
            return 1;
        case ST_FACE_MODULE_STATUS_ABORTED:
            sLock->reply(mid, MR_ABORTED, NULL, 0);
            return 1;
        default:
            break;
        }
    }

    return 0;
}

int SenselockHandler::getUsrCnt(EnrollUnlockFlag type) {
    int cnt = 0;
    st_module_user_t* usrs = nullptr;
    if (type == EnrollUnlockFlag::FLG_FACE) {
        if (ST_OK == st_face_module_get_all_users(&usrs, &cnt) && usrs) {
            st_face_module_delete_users_buffer(usrs, cnt);
        }
    } else if(type == EnrollUnlockFlag::FLG_PALM) {
        if (ST_OK == st_palm_module_get_all_users(&usrs, &cnt) && usrs) {
            st_palm_module_delete_users_buffer(usrs, cnt);
        }
    }
    return cnt;
}

int SenselockHandler::checkEarlyVerifyResult(s_msg* msg, uint8_t flg)
{
    if(mEarlyVerifyResult.ret != ST_OK) return -1;

    mEarlyVerifyResult.ret = -1;
    bool isFaceUnlockOk = mEarlyVerifyResult.sdk_status == ST_FACE_MODULE_STATUS_UNLOCK_OK || mEarlyVerifyResult.sdk_status == ST_FACE_MODULE_STATUS_UNLOCK_WITH_EYES_CLOSE;
    bool isPalmUnlockOk = mEarlyVerifyResult.sdk_status == ST_PALM_MODULE_STATUS_UNLOCK_OK;
    if(isPalmUnlockOk && msg->mid == MID_VERIFY) {
    #ifdef KOKS_FEATURE_DSM
        s_note_data_palm palm_data = { 0 };
        palm_data.state = PALM_STATE_NO_PALM;
        this->SetNotifyMessagedata(NID_PALM_STATUS, &palm_data);
    #endif
    #ifndef KOKS_FEATURE_KDS
        sLock->reply(msg->mid, MR_FAILED4_PALM_MODLE_SWITCH, NULL, 0);
    #endif
    }

    if((flg == (EnrollUnlockFlag::FLG_PALM | EnrollUnlockFlag::FLG_FACE) && (isFaceUnlockOk || isPalmUnlockOk))
        || (flg == EnrollUnlockFlag::FLG_FACE && isFaceUnlockOk)
        || (flg == EnrollUnlockFlag::FLG_PALM && isPalmUnlockOk)) {
        handleEnrollUnlockStatus(msg, mEarlyVerifyResult.sdk_user_id, -1, mEarlyVerifyResult.sdk_status);
        return 0;
    }

    if(sLock->mDemoMode) return -1;

    if((flg == (EnrollUnlockFlag::FLG_PALM | EnrollUnlockFlag::FLG_FACE) && (mEarlyVerifyResult.sdk_status == ST_FACE_MODULE_STATUS_NO_TEMPLATE || mEarlyVerifyResult.sdk_status == ST_PALM_MODULE_STATUS_NO_TEMPLATE))
        || (flg == EnrollUnlockFlag::FLG_FACE && mEarlyVerifyResult.sdk_status == ST_FACE_MODULE_STATUS_NO_TEMPLATE)
        || (flg == EnrollUnlockFlag::FLG_PALM && mEarlyVerifyResult.sdk_status == ST_PALM_MODULE_STATUS_NO_TEMPLATE)) {
        handleEnrollUnlockStatus(msg, mEarlyVerifyResult.sdk_user_id, -1, mEarlyVerifyResult.sdk_status);
        return 0;
    }

    return -1;
}

void SenselockHandler::handleEnrollUnlock(s_msg* msg, st_module_detect_mode mode, uint8_t flg)
{
    const char MODULE_PALM[] = "palm";
    const char MODULE_FACE[] = "face";
    bool first_enroll = true;
    bool first_unlock = true;
    uint8_t mid = msg->mid, _flg, db_flg = flg, rightaway = 0, err_code = 0, failed_cnt = 0, liveness_failed_cnt = 0;
    uint64_t exp = GetCurTimeMs();
    int ret, status = 0, uid = 0, cnt_palm = 0, len = (msg->size_heb << 8) | msg->size_leb, orientation{ -1 };
    st_image_t img_rgb, img_ir;
    s_face_dir direction = FACE_DIRECTION_MIDDLE;
    st_module_enroll_id_mode enroll_id_mode = ST_MODULE_ENROLL_ID_MODE_INTERNAL;

    // 手背检测后需要连续三次手掌检测的控制变量
    bool palm_back_mode = false;
    int palm_consecutive_detect_cnt = 0;
    bool enroll = (mode == ST_MODULE_DETECT_MODE_ENROLL);

    do {
        if (enroll) {
            mEarlyVerifyResult.ret = -1;
        #ifndef KOKS_FEATURE_KDS
            if (mid == MID_FACE_ENROLL_BYID || mid == MID_FACE_ENROLL_BYID1 || mid == MID_FACE_ENROLL_BYID_NO_OVERWRITE
                || mid == MID_PALM_ENROLL_BYID || mid == MID_PALM_ENROLL_BYID1 || mid == MID_PALM_ENROLL_BYID_NO_OVERWRITE) {
                st_module_user_t usr;
                uid = msg->data[len - 1];
                enroll_id_mode = ST_MODULE_ENROLL_ID_MODE_SPECIFY_WITH_OVERWRITE;
                if (mid == MID_FACE_ENROLL_BYID_NO_OVERWRITE && st_face_module_get_user(uid, &usr) == ST_OK
                    || mid == MID_PALM_ENROLL_BYID_NO_OVERWRITE && st_palm_module_get_user(uid, &usr) == ST_OK) {
                    err_code = (flg & EnrollUnlockFlag::FLG_FACE) ? MR_FAILED4_FACEENROLLED : MR_FAILED4_PALM_EXIST;
                    LOGE("user already exist!");
                    break;
                }
            }
        #endif
            if (flg == EnrollUnlockFlag::FLG_FACE) {
                int enroll_duplicate_enable = 0;
                st_module_enroll_mode enroll_mode = ST_MODULE_ENROLL_MODE_NORMAL;
            #ifdef KOKS_FEATURE_KDS
                if(mid == MID_PALM_ENROLL_ITG) {
                    s_msg_enroll_itg *enroll_data = (s_msg_enroll_itg*)msg->data;
                    enroll_mode = enroll_data->enroll_type == 0 ? ST_MODULE_ENROLL_MODE_INTERACTIVE : ST_MODULE_ENROLL_MODE_NORMAL;
                    enroll_duplicate_enable = enroll_data->enable_duplicate == 2 ? 1 : 0;
                }
                else {
            #endif
                    direction = ((s_msg_enroll_data*)msg->data)->face_direction;
                    if (direction == FACE_DIRECTION_UNDEFINE) {
                        if (mid == MID_ENROLL_FACE_UP) {
                            direction = FACE_DIRECTION_UP;
                        } else if (mid == MID_ENROLL_FACE_DN) {
                            direction = FACE_DIRECTION_DOWN;
                        } else if (mid == MID_ENROLL_FACE_LEFT) {
                            direction = FACE_DIRECTION_LEFT;
                        } else if (mid == MID_ENROLL_FACE_RIGHT) {
                            direction = FACE_DIRECTION_RIGHT;
                        } else {
                            direction = FACE_DIRECTION_MIDDLE;
                        }
                    }
                    enroll_duplicate_enable = (mid == MID_ENROLL_FACE_SINGLE) ? 0 : 1;
                    enroll_mode = (mid == MID_ENROLL || direction != FACE_DIRECTION_MIDDLE) ? ST_MODULE_ENROLL_MODE_INTERACTIVE : ST_MODULE_ENROLL_MODE_NORMAL;
            #ifdef KOKS_FEATURE_KDS
                }
            #endif
                st_face_module_set_enroll_mode(enroll_mode);
                st_face_module_set_enable_dup_enroll(enroll_duplicate_enable);
            } else {
            #ifdef KOKS_FEATURE_KDS
                st_palm_module_set_enable_dup_enroll(((s_msg_enroll_data*)msg->data)->face_direction);
            #else
                st_palm_module_set_enable_dup_enroll(mid != MID_PALM_ENROLL_SINGLE);
            #endif
            }
        } else {
            rightaway = msg->data[0];
            if (!sLock->mDemoMode) {
                db_flg = 0;
                if ((flg & EnrollUnlockFlag::FLG_FACE) && getUsrCnt(EnrollUnlockFlag::FLG_FACE)) {
                    db_flg |= EnrollUnlockFlag::FLG_FACE;
                }
                if ((flg & EnrollUnlockFlag::FLG_PALM) && getUsrCnt(EnrollUnlockFlag::FLG_PALM)) {
                    db_flg |= EnrollUnlockFlag::FLG_PALM;
                }
                if ((db_flg == 0 || flg != (EnrollUnlockFlag::FLG_FACE | EnrollUnlockFlag::FLG_PALM) && !(db_flg & flg))) {
                    err_code = (flg & EnrollUnlockFlag::FLG_FACE) ? MR_FAILED4_UNKNOWNUSER : MR_FAILED4_PALM_NONE;
                    LOGE("no record!");
                    break;
                }
                if (db_flg && flg == (EnrollUnlockFlag::FLG_FACE | EnrollUnlockFlag::FLG_PALM)) {
                    flg &= db_flg; // only verify if record not empty
                    if (flg == EnrollUnlockFlag::FLG_PALM) {
                        #ifdef KOKS_FEATURE_DSM
                            s_note_data_palm palm_data = { 0 };
                            palm_data.state = PALM_STATE_NO_PALM;
                            this->SetNotifyMessagedata(NID_PALM_STATUS, &palm_data);
                        #endif
                        #ifndef KOKS_FEATURE_KDS
                        sLock->reply(mid, MR_FAILED4_PALM_MODLE_SWITCH, NULL, 0);
                        #endif
                    }
                }
            }
        }
    } while (0);

#ifdef KOKS_FEATURE_KDS
    if(mid == MID_PALM_ENROLL_ITG) {
        ret = ((s_msg_enroll_itg*)msg->data)->timeout;
    }
    else {
        ret = msg->data[len - (1 + (uid > 0))]; // timeout, last byte
    }
#else
    ret = msg->data[len - (1 + (uid > 0))]; // timeout, last byte
#endif
    LOGI("timeout=%ds, direction=0x%x, rightaway=%d, uid=%d", ret, direction, rightaway, uid);
    exp += ret ? ret * (1000 - 15) - 700 : 10 * 1000;
    if (err_code) {
        sLock->reply(mid, err_code, NULL, 0);
        return;
    }

    st_palm_module_set_enable_verify((flg & EnrollUnlockFlag::FLG_PALM) != 0);

    double brightness = 0;
    this->StartMessageNotifyThread();
    while (!Senselock::mAbort) {
        int user_id = uid;
        if (GetCurTimeMs() >= exp) {
            uint8_t stat = (flg & EnrollUnlockFlag::FLG_FACE) ? MR_FAILED4_TIMEOUT : MR_FAILED4_PALM_TIMEOUT;
            if (!db_flg) {
                stat = (flg & EnrollUnlockFlag::FLG_FACE) ? MR_FAILED4_DUAL_FACE_VERIFY_FAIL : MR_FAILED4_DUAL_PALM_VERIFY_FAIL;
            }
            if (status == ST_PALM_MODULE_STATUS_UNLOCK_FAILED) {
                stat = MR_FAILED4_PALM_NONE;
            }
            if(status == ST_FACE_MODULE_STATUS_VERIFY_FAILED) {
                stat = MR_FAILED4_UNKNOWNUSER;
            }
            sLock->reply(mid, stat, NULL, 0);
            LOGW("timeout, %s(%d) failed!", enroll ? "enroll" : "unlock", stat);
            break;
        }
        uint8_t face = flg & EnrollUnlockFlag::FLG_FACE ? 1 : 0;
        if (sLock->GetCameraFrame(img_rgb, img_ir, face) != KOKS_RET_OK) {
            LOGE("open camera failed");
            sLock->reply(mid, MR_FAILED4_CAMERA, NULL, 0);
            break;
        }
        sLock->setImageState(C_STATE_NO_FRAME);

        bool rc_is_scaled = false;
        const char* module = NULL;
        //三个变量分别记录检测结果中是否有人脸 掌 和手背
        bool ir_has_face = false, ir_has_palm = false, rgb_has_face = false;
        st_rect_t rc_face_ir = { 0 }, rc_face_rgb{ 0 }, rc_palm_ir = { 0 }, rc_palm_rgb{ 0 }, opisthenar_rect{ 0 }, *rc_ir = NULL, *rc_rgb = nullptr;
        st_rect_t rc_faceae_ir = { 0 }, rc_faceae_rgb = { 0 };
        ret = st_face_plam_detect(&img_ir, &rc_face_ir, &rc_palm_ir, &opisthenar_rect, &status, mode);
        if(rc_face_ir.right > 0) ir_has_face = true;
        if(rc_palm_ir.right > 0) ir_has_palm = true;

        // 手掌注册场景：若检测为手背，进入“手背模式”，后续需连续三次检测到手掌才允许注册
        if (enroll && (flg & EnrollUnlockFlag::FLG_PALM) && ret == ST_OK) {
            if (status == ST_PALM_MODULE_STATUS_PALM_BACK_OR_GLOVE) {
                palm_back_mode = true;
                palm_consecutive_detect_cnt = 0;
                LOGW("palm back detected, require 3 consecutive palm detections");
                if(opisthenar_rect.right > 0) {
                    int cross = 0;
                    st_rect_t rc = { 0 };
                    st_palm_module_get_align_rect(&img_ir, opisthenar_rect, &rc, &cross);
                    if(rc.right > 0) {
                        sLock->setCameraROI(&rc, &rc, 0, EnrollUnlockFlag::FLG_PALM);
                    }
                }
                sendNote(NID_PALM_STATUS, PALM_STATE_PALM_BACK);
                // 不继续本轮流程，直接下一帧
                continue;
            }
            if (palm_back_mode) {
                if (ir_has_palm > 0) {
                    palm_consecutive_detect_cnt++;
                } else {
                    palm_consecutive_detect_cnt = 0;
                }
                if (palm_consecutive_detect_cnt < 3) {
                    // 未达到3次连续手掌，继续等待
                    LOGW("waiting consecutive palm detections: %d/3", palm_consecutive_detect_cnt);
                    if(opisthenar_rect.right> 0) {
                        int cross = 0;
                        st_rect_t rc = { 0 };
                        st_palm_module_get_align_rect(&img_ir, opisthenar_rect, &rc, &cross);
                        if(rc.right > 0) {
                            sLock->setCameraROI(&rc, &rc, 0, EnrollUnlockFlag::FLG_PALM);
                        }
                    }
                    sendNote(NID_PALM_STATUS, PALM_STATE_PALM_MOVE);
                    continue;
                } else {
                    // 达到三次，退出“手背模式”，允许后续注册
                    palm_back_mode = false;
                    LOGW("palm consecutive detections satisfied, continue enroll");
                }
            }
        }

        if (ret == ST_OK && status == ST_PALM_MODULE_STATUS_PALM_BACK_OR_GLOVE
                && (flg & EnrollUnlockFlag::FLG_PALM) == EnrollUnlockFlag::FLG_PALM) {
            LOGW("palm back detect");
            if(opisthenar_rect.right > 0) {
                int cross = 0;
                st_rect_t rc = { 0 };
                st_palm_module_get_align_rect(&img_ir, opisthenar_rect, &rc, &cross);
                if(rc.right > 0) {
                    sLock->setCameraROI(&rc, &rc, 0, EnrollUnlockFlag::FLG_PALM);
                }
            }
            sendNote(NID_PALM_STATUS, PALM_STATE_PALM_BACK);
            continue;
        }

        if (ret == ST_OK && (flg & EnrollUnlockFlag::FLG_FACE)) {
            LOGDD("before rgb detect");
            ret = st_face_plam_detect(&img_rgb, &rc_face_rgb, &rc_palm_rgb, &opisthenar_rect, &status, mode);
            if(rc_face_rgb.right > 0) rgb_has_face = true;
        }
        if (ret != ST_OK) {
            LOGE("st_face_plam_detect %d", ret);
            sLock->reply(mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }

        _flg = ir_has_palm ? EnrollUnlockFlag::FLG_PALM : 0;
        _flg |= (ir_has_face && rgb_has_face) ? EnrollUnlockFlag::FLG_FACE : 0;
        _flg &= flg;

        // 在“手背模式”下，即使检测到手掌，也要满足三次连续检测条件后才允许进入注册流程
        if (enroll && (flg & EnrollUnlockFlag::FLG_PALM) && palm_back_mode) {
            // 由于上面已在达到3次前continue，这里理论上不会触发；再次保护
            _flg &= ~EnrollUnlockFlag::FLG_PALM;
        }

        if (flg == (EnrollUnlockFlag::FLG_FACE | EnrollUnlockFlag::FLG_PALM)) {
            cnt_palm = (_flg & EnrollUnlockFlag::FLG_PALM) ? cnt_palm + 1 : 0;
            if ((_flg & EnrollUnlockFlag::FLG_PALM) || cnt_palm == 3) {
            #ifdef KOKS_FEATURE_DSM
                s_note_data_palm palm_data = { 0 };
                palm_data.state = PALM_STATE_NO_PALM;
                this->SetNotifyMessagedata(NID_PALM_STATUS, &palm_data);
            #endif
            #ifndef KOKS_FEATURE_KDS
                sLock->reply(mid, MR_FAILED4_PALM_MODLE_SWITCH, NULL, 0);
            #endif
                flg = EnrollUnlockFlag::FLG_PALM; // only palm later
                sLock->switchEffectBin(true);
            }
        }
        if (!_flg) {
            if ((flg & EnrollUnlockFlag::FLG_FACE) && (ir_has_face || rgb_has_face)) {
                rc_ir = ir_has_face ? &rc_face_ir : &rc_face_rgb;
                sLock->setCameraROI(rc_ir, rc_ir, 0, EnrollUnlockFlag::FLG_FACE);
            }
            handleEnrollUnlockStatus(msg, user_id, orientation, (flg & EnrollUnlockFlag::FLG_FACE) ? ST_FACE_MODULE_STATUS_DETECT_FAILED : ST_PALM_MODULE_STATUS_NO_PALM);
            continue;
        } else if (!db_flg) {
            sLock->reply(mid, (_flg & EnrollUnlockFlag::FLG_FACE) ? MR_FAILED4_DUAL_FACE_VERIFY_OK : MR_FAILED4_DUAL_PALM_VERIFY_OK, NULL, 0);
            break;
        }
        LOGI(" --- begin");
        if (_flg & EnrollUnlockFlag::FLG_FACE) {
            rc_ir = &rc_face_ir;
            rc_rgb = &rc_face_rgb;
            double brightness_ir = 0, brightness_rgb = 0;
            module = MODULE_FACE;
            ret = st_face_quality(&img_ir, rc_face_ir, enroll, &status, &brightness_ir, &rc_faceae_ir);
            LOGI("ir face quality brightness=%f, rc_faceae_ir(%d,%d,%d,%d)", brightness_ir, rc_faceae_ir.left, rc_faceae_ir.top, rc_faceae_ir.right, rc_faceae_ir.bottom);
            if (ret == ST_OK && status == ST_FACE_MODULE_STATUS_NORMAL) {
                ret = st_face_quality(&img_rgb, rc_face_rgb, enroll, &status, &brightness_rgb, &rc_faceae_rgb);
                LOGI("rgb face quality brightness=%f, rc_faceae_rgb(%d,%d,%d,%d)", brightness_rgb, rc_faceae_rgb.left, rc_faceae_rgb.top, rc_faceae_rgb.right, rc_faceae_rgb.bottom);
            }
            brightness = brightness_ir > brightness_rgb ? brightness_ir : brightness_rgb;
            if (ret == ST_OK && status == ST_FACE_MODULE_STATUS_NORMAL) {
                if (enroll) {
                    st_module_user_t user_info = { 0 };
                    #ifdef KOKS_FEATURE_KDS
                    user_info.admin = ((s_msg_enroll_itg*)msg->data)->admin;
                    memcpy(user_info.name, ((s_msg_enroll_itg*)msg->data)->user_name, sizeof(user_info.name));
                    #else
                    user_info.admin = ((s_msg_enroll_data*)msg->data)->admin;
                    memcpy(user_info.name, ((s_msg_enroll_data*)msg->data)->user_name, sizeof(user_info.name));
                    #endif
                    st_face_module_set_enroll_direction(direction);
#ifdef KOKS_FEATURE_NOAESYNC
                    ret = st_face_module_enroll(&img_ir, rc_face_ir, &img_rgb, rc_face_rgb, &status, enroll_id_mode, &user_id, user_info, first_enroll);
                    first_enroll = false;
#else
                    auto brightness_diff = brightness_ir > brightness_rgb ? brightness_ir - brightness_rgb : brightness_rgb - brightness_ir;
                    LOGI("brightness diff = %f", brightness_diff);
                    if(brightness_diff < 0.2) {
                        ret = st_face_module_enroll(&img_ir, rc_face_ir, &img_rgb, rc_face_rgb, &status, enroll_id_mode, &user_id, user_info, first_enroll);
                        first_enroll = false;
                    }
#endif
                } else {
                    ret = st_face_module_unlock(&img_ir, rc_face_ir, &img_rgb, rc_face_rgb, &status, &user_id, first_unlock);
                    first_unlock = false;
                }
            }
        }
        if (!module && (_flg & EnrollUnlockFlag::FLG_PALM)) {
            st_rect_t rc { 0 };
            rc_ir = &rc_palm_ir;
            rc_rgb = &rc_palm_rgb;
            module = MODULE_PALM;
            if(enroll) {
                st_module_user_t user_info = { 0 };
                user_info.admin = ((s_msg_enroll_data*)msg->data)->admin;
                memcpy(user_info.name, ((s_msg_enroll_data*)msg->data)->user_name, sizeof(user_info.name));
                ret = st_palm_module_enroll(&img_ir, rc_palm_ir, &img_rgb, rc_palm_rgb, &rc, &status, enroll_id_mode, &user_id, &orientation, &brightness, user_info, first_enroll);
                first_enroll = false;
            } else {
                ret = st_palm_module_unlock(&img_ir, rc_palm_ir, &img_rgb, rc_palm_rgb, &rc, &status, &user_id, &brightness, first_unlock);
                first_unlock = false;
            }
            if (rc.right) {
                rc_is_scaled = true;
                rc_palm_ir = rc;
            }
        }
        LOGI("%s, ret=%d, uid=%d, status=%d, brightness = %f, ir rect(%d,%d,%d,%d), rgb rect(%d,%d,%d,%d), %d", module ? module : "null", ret, user_id, status, brightness, rc_ir->left, rc_ir->top, rc_ir->right,
             rc_ir->bottom, rc_rgb->left, rc_rgb->top, rc_rgb->right, rc_rgb->bottom, (int)(img_ir.timestamp - img_rgb.timestamp));
        if (rc_ir == &rc_palm_ir) {
            if (!rc_is_scaled) {
                const float SCALE_SIDE = 0.35f;
                rc_scalar(img_ir, rc_palm_ir, SCALE_SIDE, -SCALE_SIDE, SCALE_SIDE, -SCALE_SIDE);
            }
        } else {
            rc_ir = &rc_faceae_ir;
            rc_rgb = &rc_faceae_rgb;
        }
        if(rc_ir->right > 0) {
            sLock->setCameraROI(rc_ir, rc_rgb, brightness, (rc_ir == &rc_faceae_ir) ? EnrollUnlockFlag::FLG_FACE : EnrollUnlockFlag::FLG_PALM);
            if (sLock->mDebugMode) {
                draw_rc(img_ir.data, img_ir.width, img_ir.height, rc_ir->left, rc_ir->top, rc_ir->right - rc_ir->left, rc_ir->bottom - rc_ir->top, 1);
            }
        }
        if (ret != ST_OK) {
            sLock->reply(mid, MR_FAILED4_UNKNOWNREASON, NULL, 0);
            break;
        }
        if ((status == ST_PALM_MODULE_STATUS_IR_NOT_LIVE || status == ST_FACE_MODULE_STATUS_HACKNESS_FAILED) && (++liveness_failed_cnt >= HACKNESS_LIMITED_FRAME_COUNT)) {
            sLock->reply(mid, MR_FAILED4_LIVENESSCHECK, NULL, 0);
            LOGW("liveness check failed!");
            break;
        }

        if (status == ST_FACE_MODULE_STATUS_VERIFY_FAILED && !rightaway && (++failed_cnt < VERIFY_LIMITED_FRAME_COUNT)) {
            sendNote(NID_FACE_STATE, FACE_STATE_NORMAL);
        } else if (status == ST_PALM_MODULE_STATUS_UNLOCK_FAILED && !rightaway && (++failed_cnt < VERIFY_LIMITED_FRAME_COUNT)) {
            sendNote(NID_PALM_STATUS, PALM_STATE_COMPARE_FAIL);
        } else if (handleEnrollUnlockStatus(msg, user_id, orientation, status) && module) {
            break;
        }
    }
    this->StopMessageNotifyThread();
    st_face_module_set_enable_liveness(0);
}

void SenselockHandler::handleCommon(s_msg* msg)
{
    uint16_t len = 0;
    uint8_t err_code = MR_SUCCESS;
    uint8_t buf[256] = {0};
    uint8_t mid = msg->mid;
    if (mid == MID_GET_VERSION) {
        s_msg_reply_version_data *reply = (s_msg_reply_version_data*)buf;
        auto version = rota_get_version();
        memcpy(reply->version_info, version, strlen(version));
        len = sizeof(s_msg_reply_version_data);
    }
    else if (mid == MID_GET_ALL_USERID) {
        int cnt;
        st_module_user_t* usrs = NULL;
        int ret = st_face_module_get_all_users(&usrs, &cnt);
        if (ST_OK != ret) {
            err_code = MR_FAILED4_UNKNOWNREASON;
        } else {
            st_result_t (*delete_users_buffer)(st_module_user_t*, int) = st_face_module_delete_users_buffer;
            s_msg_reply_all_userid_data *reply = (s_msg_reply_all_userid_data*)buf;
            reply->user_counts = cnt;
            for (int i = 0; i < cnt; i++) {
                reply->users_id[2 * i] = (usrs[i].id >> 8) & 0xFF;
                reply->users_id[2 * i + 1] = usrs[i].id & 0xFF;
            }
            if (usrs) delete_users_buffer(usrs, cnt);
            len = sizeof(s_msg_reply_all_userid_data);
        }
    }
    else if (mid == MID_DELUSER) {
        int uid = BYTE2WORD(msg->data, 0);
        PartitionGuard configGuard(CONFIG_FACE_DB);
        int ret = st_face_module_delete_user(uid);
        if (ret == ST_OK) err_code = MR_SUCCESS;
        else if (ret == ST_FACE_MODULE_STATUS_NO_SUCH_USER) err_code = MR_FAILED4_UNKNOWNUSER;
        else err_code = MR_FAILED4_UNKNOWNREASON;
    } else if (mid == MID_DELALL) {
        PartitionGuard configGuard(CONFIG_FACE_DB);
        st_result_t (*delete_all_user)(void) = st_face_module_delete_all_user;
        err_code = delete_all_user() == ST_OK ? MR_SUCCESS : MR_FAILED4_UNKNOWNREASON;
    }
    else if (mid == MID_FACE_DEL_MULTI) {
        PartitionGuard configGuard(CONFIG_FACE_DB);
        st_result_t (*delete_user)(int) = st_face_module_delete_user;
        int ret = 0;
        uint16_t length = (msg->size_heb << 8) | msg->size_leb;
        uint16_t idCount = length / 2;
        for (uint16_t i = 0; i < idCount; i++) {
            int uid = (msg->data[2 * i] << 8) | msg->data[2 * i + 1];
            ret |= delete_user(uid);
        }
        if (ret) err_code = MR_FAILED4_UNKNOWNREASON;
    }
#ifdef KOKS_FEATURE_PALM
    else if (mid == MID_PALM_QUERY) {
        int cnt;
        st_module_user_t* usrs = NULL;
        int ret = st_palm_module_get_all_users(&usrs, &cnt);
        if (ST_OK != ret) {
            err_code = MR_FAILED4_UNKNOWNREASON;
        } else {
            st_result_t (*delete_users_buffer)(st_module_user_t*, int) = st_palm_module_delete_users_buffer;
            s_msg_reply_all_userid_data *reply = (s_msg_reply_all_userid_data*)buf;
            reply->user_counts = cnt;
            for (int i = 0; i < cnt; i++) {
                reply->users_id[2 * i] = (usrs[i].id >> 8) & 0xFF;
                reply->users_id[2 * i + 1] = usrs[i].id & 0xFF;
            }
            if (usrs) delete_users_buffer(usrs, cnt);
            len = sizeof(s_msg_reply_all_userid_data);
        }
    } else if (mid == MID_PALM_DELETE) {
        int uid = BYTE2WORD(msg->data, 0);
        PartitionGuard configGuard(CONFIG_PALM_DB);
        int ret = st_palm_module_delete_user(uid);
        if (ret == ST_OK) err_code = MR_SUCCESS;
        else if (ret == ST_PALM_MODULE_STATUS_NO_SUCH_USER) err_code = MR_FAILED4_PALM_NONE;
        else err_code = MR_FAILED4_UNKNOWNREASON;
    } else if (mid == MID_PALM_DELALL) {
        PartitionGuard configGuard(CONFIG_PALM_DB);
        st_result_t (*delete_all_user)(void) = st_palm_module_delete_all_user;
        err_code = delete_all_user() == ST_OK ? MR_SUCCESS : MR_FAILED4_UNKNOWNREASON;
    } else if (mid == MID_PALM_LEVEL) {
        bool set = msg->data[0] != 0;
        if (set) {
            memcpy(buf, msg->data + 1, 4);
        } else {
            len = 4;
        }
        setPalmLevel(buf, set);
    }
    else if (mid == MID_PALM_DEL_MULTI) {
        PartitionGuard configGuard(CONFIG_PALM_DB);
        st_result_t (*delete_user)(int) = st_palm_module_delete_user;
        int ret = 0;
        uint16_t length = (msg->size_heb << 8) | msg->size_leb;
        uint16_t idCount = length / 2;
        for (uint16_t i = 0; i < idCount; i++) {
            int uid = (msg->data[2 * i] << 8) | msg->data[2 * i + 1];
            ret |= delete_user(uid);
        }
        if (ret) err_code = MR_FAILED4_UNKNOWNREASON;
    }
#if 0
    else if (mid == MID_PALM_SWITCH) {
        // TODO:
    } else if (mid == MID_MEMBERSHIP) { // 0x70
        uint16_t cnt = BYTE2WORD(msg->data, 0);
        for (uint16_t i = 0; i < cnt; i++) {
            uint16_t k = 2 + i * 3;
            uint16_t mid = BYTE2WORD(msg->data, k);     // member id
            uint16_t fid = BYTE2WORD(msg->data, k + 2); // face id
            uint16_t pid = BYTE2WORD(msg->data, k + 4); // palm id
            // TODO:
        }
    }
#endif
#endif
    else if (mid == MID_DEBUG_MODE) {
        sLock->mDebugMode = msg->data[0];
        if (sLock->mDebugMode == 0) {
            if (LOG_VDEBUG_LEVEL_TMP != 0xF) {
                // disable debug mode, change log level, delete debug pictures
                SET_LOG_LEVEL(ST_LEVEL_INFO);
            }
        } else {
            // enable debug mode, change log level, creat directory
            LOG_VDEBUG_LEVEL_TMP = ST_LEVEL_DEBUG;
            SET_LOG_LEVEL(ST_LEVEL_DEBUG);
        }
    } else if (mid == MID_DEMOMODE) {
        sLock->mDemoMode = msg->data[0];
        st_face_module_set_unlock_mode(msg->data[0] ? ST_MODULE_UNLOCK_MODE_DEMO : ST_MODULE_UNLOCK_MODE_NORMAL);
    } else {
        err_code = MR_REJECTED;
    }
    sLock->reply(mid, err_code, buf, len);
    LOGI("mid=0x%x, err_code=%d", mid, err_code);
}

void SenselockHandler::handleMsgGetModelMD5(s_msg* msg)
{
    //核心使用方法 三级指针 注意释放
    int model_count = 0;
    char **md5_outputs = nullptr;
    char buffer[MAX_REPLY_DATA_SIZE] = { 0 };
    char *pbuffer = buffer;;

    st_result_t result =
        st_palm_module_get_models_md5(&model_count, &md5_outputs);
    if (result != ST_OK) {
        printf("st_palm_module_get_models_md5 failed with error code: %d\n",
            result);
    }

    printf("Retrieved %d model MD5 hashes:\n", model_count);
    for (int i = 0; i < model_count; ++i) {
        printf("Model %d MD5: %s\n", i, md5_outputs[i]);
        pbuffer += sprintf(pbuffer, "%s\n", md5_outputs[i]);
    }
    for (int i = 0; i < model_count; ++i) {
        delete[] md5_outputs[i];
    }
    delete[] md5_outputs;

    sLock->reply(MID_GET_MODELS_MD5, MR_SUCCESS, buffer, strlen(buffer));
    sLock->mState = MS_STANDBY;
}

#ifdef KOKS_FEATURE_PALM
void SenselockHandler::setPalmLevel(uint8_t lvl[4], bool set)
{
    auto& db = st::sense_koks::koks_db::GetInstance();
    st::sense_koks::PalmStrategy palm;
    if (set) {
        st_palm_module_config_640_480.palm_levle.detection_levle = (enum PalmConfidenceLevel)lvl[0];
        st_palm_module_config_640_480.palm_levle.brightness_levle = (enum PalmConfidenceLevel)lvl[1];
        st_palm_module_config_640_480.palm_levle.match_levle = (enum PalmConfidenceLevel)lvl[2];
        st_palm_module_config_640_480.palm_levle.liveness_levle = (enum PalmConfidenceLevel)lvl[3];
        db.writeData(&palm, &st_palm_module_config_640_480.palm_levle);
    } else {
        lvl[0] = st_palm_module_config_640_480.palm_levle.detection_levle;
        lvl[1] = st_palm_module_config_640_480.palm_levle.brightness_levle;
        lvl[2] = st_palm_module_config_640_480.palm_levle.match_levle;
        lvl[3] = st_palm_module_config_640_480.palm_levle.liveness_levle;
    }
}

int SenselockHandler::palmInit(void)
{
    if (rota_has_flag()) {
        LOG_ERROR("Last OTA incomplete, disable palm\n");
        return -1;
    }

    if (palm_inited == false) {
        int ret = 0;
        auto& db = st::sense_koks::koks_db::GetInstance();
        st::sense_koks::PalmStrategy palm;
        PartitionGuard configGuard(CONFIG_PALM_DB);
        db.readData(&palm, &st_palm_module_config_640_480.palm_levle);
#ifdef KOKS_LOAD_MODEL_FROM_SFC
        ret = st_palm_sfc_init(model_offset, sfc_device);
        LOGI("st_palm_sfc_init ret = %d", ret);
        if (ret != ST_OK) {
            LOGE("st_palm_sfc_init failed %d", ret);
            return -1;
        }
#endif
        ret = st_palm_module_init(&st_palm_module_config_640_480);
        if (ret != ST_OK) {
            LOGE("st_palm_module_init failed %d", ret);
            return -1;
        }
        ret = st_palm_module_set_max_user_count(MAX_PALM_USER_COUNTS);
        if (ret != ST_OK) {
            LOGE("st_palm_module_set_max_user_count failed %d", ret);
            return -1;
        }
        default_palm_param_config.config_verify_threshold = palm_verify_threshold_level[st_palm_module_config_640_480.palm_levle.match_levle];
        default_palm_param_config.config_enroll_verify_threshold = palm_verify_rgb_threshold_level[st_palm_module_config_640_480.palm_levle.brightness_levle];
        default_palm_param_config.config_hackness_threshold = palm_liveness_threshold_level[st_palm_module_config_640_480.palm_levle.liveness_levle];
        ret = st_palm_module_set_param_config(&default_palm_param_config);
        if (ret != ST_OK) {
            LOGE("st_palm_module_set_param_config failed %d", ret);
            return -1;
        }
        sLock->mPalmBrightnessLimit.brightness_max = default_palm_param_config.config_quality_brightness_max;
        sLock->mPalmBrightnessLimit.brightness_min = default_palm_param_config.config_quality_brightness_min;
        palm_inited = true;
    }
    return 0;
}

void SenselockHandler::setPalmDualEnroll(bool enable)
{
    if (default_palm_param_config.enable_dup_enroll_ != enable) {
        default_palm_param_config.enable_dup_enroll_ = enable;
        st_palm_module_set_param_config(&default_palm_param_config);
    }
}
#endif

#else
void SenselockHandler::handleGetAllUserId()
{
    LOG_DEBUG("get all users ID\n");
    sLock->mState = MS_BUSY;
    int user_counts;
    st_module_user_t* users_info = nullptr;
    int ret = st_face_module_get_all_users(&users_info, &user_counts);
    LOG_DEBUG("st_face_module_get_all_users end\n");
    if (ST_OK != ret) {
        sLock->reply(MID_GET_ALL_USERID, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    } else {
        s_msg_reply_all_userid_data reply_userid_data = { 0 };
        reply_userid_data.user_counts = (uint8_t)user_counts;
        for (int i = 0; i < user_counts; i++) {
            reply_userid_data.users_id[2 * i] = ((users_info + i)->id >> 8) & 0xFF;
            reply_userid_data.users_id[2 * i + 1] = (users_info + i)->id & 0xFF;
        }
        sLock->reply(MID_GET_ALL_USERID, MR_SUCCESS, &reply_userid_data, sizeof(s_msg_reply_all_userid_data));
    }

    if (users_info != nullptr) {
        ret = st_face_module_delete_users_buffer(users_info, user_counts);
        if (ST_OK != ret) {
            sLock->reply(MID_GET_ALL_USERID, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        }
        users_info = nullptr;
    }
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleMsgDelUser(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    s_msg_deluser_data* deluser_data = (s_msg_deluser_data*)(msg->data);
    uint16_t user_id = (deluser_data->user_id_heb << 8) | deluser_data->user_id_leb;
    LOG_INFO("Deleting user_id %u\n", user_id);
    PartitionGuard configGuard(CONFIG_FACE_DB);
    int ret = st_face_module_delete_user(user_id);
    configGuard.SyncAndLock();
    if (ret != ST_OK) {
        if (ret == ST_FACE_MODULE_STATUS_NO_SUCH_USER) {
            sLock->reply(MID_DELUSER, MR_FAILED4_UNKNOWNUSER, NULL, 0);
        } else {
            sLock->reply(MID_DELUSER, MR_FAILED4_UNKNOWNREASON, NULL, 0);
        }
    } else {
        sLock->reply(MID_DELUSER, MR_SUCCESS, NULL, 0);
    }
    sLock->mState = MS_STANDBY;
    return;
}

int SenselockHandler::backup_file(char* name)
{
    int ret = 0;
    LOG_DEBUG("in\n");
    // if (isFileExisted(name)) {
    //     strcpy(_back_group.files[_back_group.count].name, name);
    //     ret = readFile(name, &(_back_group.files[_back_group.count].payload), &(_back_group.files[_back_group.count].size));
    //     _back_group.count++;
    // }
    LOG_DEBUG("exit\n");
    return ret;
}

void SenselockHandler::handleMsgDelAll()
{
    int ret = 0;
    sLock->mState = MS_BUSY;
    LOG_DEBUG("in\n");

    PartitionGuard configGuard(CONFIG_FACE_DB);
    st_face_module_delete_all_user();
    configGuard.SyncAndLock();

    // ret = FormatFatfs();
    if (ST_OK != ret) {
        sLock->reply(MID_DELALL, MR_FAILED4_UNKNOWNREASON, NULL, 0);
    } else {
        sLock->reply(MID_DELALL, MR_SUCCESS, NULL, 0);
    }
    LOG_DEBUG("exit\n");
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgDelOVDConfig()
{
    LOG_DEBUG("The command of protocal is delate OVD config info\n");
#ifdef KOKS_PROJECT_M53D
    system("rm -rf /cmcc_fs/configure.bin");
    system("sync");
#endif
    return;
}

void SenselockHandler::getSWVersion()
{
    LOG_DEBUG("The command of protocal is GET VERSION INFO\n");
    auto version = rota_get_version();
    sLock->mState = MS_BUSY;
    s_msg_reply_version_data version_data = { 0 };
    strncpy((char*)&version_data.version_info, version, sizeof(version_data.version_info) - 1);
    sLock->reply(MID_GET_VERSION, MR_SUCCESS, &version_data, sizeof(s_msg_reply_version_data));
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleDemoMode(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    sLock->mDemoMode = msg->data[0];
    LOG_DEBUG("DEMOMODE enabled:%d\n", sLock->mDemoMode);
    st_module_unlock_mode unlock_mode;
    if (sLock->mDemoMode == 0) {
        unlock_mode = ST_MODULE_UNLOCK_MODE_NORMAL;
    } else {
        unlock_mode = ST_MODULE_UNLOCK_MODE_DEMO;
        mEarlyVerifyResult.ret = -1;
    }
    st_face_module_set_unlock_mode(unlock_mode);
    sLock->reply(MID_DEMOMODE, MR_SUCCESS, NULL, 0);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleMsgDebugMode(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    sLock->mDebugMode = msg->data[0];
    LOG_DEBUG("debug mode enabled:%d", sLock->mDebugMode);
    if (sLock->mDebugMode == 0) {
        if (LOG_VDEBUG_LEVEL_TMP != 0xF) {
            // disable debug mode, change log level, delete debug pictures
            SET_LOG_LEVEL(ST_LEVEL_INFO);
        }
    } else {
        // enable debug mode, change log level, creat directory
        LOG_VDEBUG_LEVEL_TMP = ST_LEVEL_DEBUG;
        SET_LOG_LEVEL(ST_LEVEL_DEBUG);
    }
    sLock->reply(MID_DEBUG_MODE, MR_SUCCESS, NULL, 0);
    sLock->mState = MS_STANDBY;
    return;
}
#endif

#ifdef KOKS_FEATURE_VIDEO
void SenselockHandler::handleSetVideoParam(s_msg* msg, s_type type)
{
    sLock->mState = MS_BUSY;
    cam_msg_video_param_data* video_param = (cam_msg_video_param_data*)msg->data;
    LOG_INFO("set video param: camera_type = %d, stream_type = %d, encode = %d, resolution_width = %d, resolution_height = %d, framerate = %d, bitrate = %d, save_flag = %d\n",
             video_param->camera_type, video_param->stream_type, video_param->encode, video_param->resolution_width, video_param->resolution_height, video_param->framerate,
             video_param->bitrate, video_param->save_flag);
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0, type);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleSetAudioParam(s_msg* msg)
{
    auto* audio_input = MediaManager::GetInstance().GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
    auto* audio_output = MediaManager::GetInstance().GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);
    sLock->mState = MS_BUSY;
    cam_msg_audio_param_data* audio_param = (cam_msg_audio_param_data*)msg->data;
    LOG_INFO("set audio param: mic_samplerate = %d, mic_payload_type = %d, mic_volume = %d, speaker_samplerate = %d, speaker_payload_type = %d, speaker_volume = %d, "
             "audio_denoisel_level = %d, save_flag = %d\n",
             audio_param->mic_samplerate, audio_param->mic_payload_type, audio_param->mic_volume, audio_param->speaker_samplerate, audio_param->speaker_payload_type,
             audio_param->speaker_volume, audio_param->audio_denoisel_level, audio_param->save_flag);
    if (audio_input->SetVol(audio_param->mic_volume) != 0) {
        LOG_ERROR("set audio param failed\n");
        sLock->reply(msg->mid, MR_FAILED4_UNKNOWNREASON, nullptr, 0);
        sLock->mState = MS_STANDBY;
        return;
    }
    if (audio_output->SetVol(audio_param->speaker_volume) != 0) {
        LOG_ERROR("set audio param failed\n");
        sLock->reply(msg->mid, MR_FAILED4_UNKNOWNREASON, nullptr, 0);
        sLock->mState = MS_STANDBY;
        return;
    }
    if (audio_param->audio_denoisel_level > 0) {
        st::sense_koks::Audio::AudioSetAec(true);
    } else {
        st::sense_koks::Audio::AudioSetAec(false);
    }
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleSetUVCParam(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    uint8_t enable_stream = msg->data[0];
    uint8_t type = msg->data[1];
    LOG_INFO("set uvc param: enable_stream = %d, type = %d\n", enable_stream, type);
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleSetUVCStreamType(s_msg* msg, s_type type)
{
    int ret = ST_OK;
    sLock->mState = MS_BUSY;
    cam_msg_req_video_data* video_param = (cam_msg_req_video_data*)msg->data;
    LOG_INFO("set uvc stream type: stream_type = %d, encode = %d, video_frame_type = %d\n", video_param->stream_type, video_param->encode, video_param->video_frame_type);
    if ((video_param->encode == 0 || video_param->encode == 1) && video_param->video_frame_type == 0) { // h264/h265, I frame.
        ret = MediaUvc::GetInstance().SetFrameIDR();
    }
    if (ret != ST_OK) {
        LOG_ERROR("SetFrameIDR failed\n");
        sLock->reply(msg->mid, MR_FAILED4_UNKNOWNREASON, nullptr, 0, type);
        sLock->mState = MS_STANDBY;
        return;
    }
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0, type);
    sLock->mState = MS_STANDBY;
    return;
}

void SenselockHandler::handleUVCSwitch(s_msg* msg, s_type type)
{
    sLock->mState = MS_BUSY;
    cam_msg_uvc_switch_param_data* video_param = (cam_msg_uvc_switch_param_data*)msg->data;
    LOG_INFO("switch uvc: camera_type = %d, uvc_flow = %d\n", video_param->camera_type, video_param->uvc_flow);
    MediaUvc::GetInstance().SetCamType(video_param->uvc_flow, video_param->camera_type);
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0, type);
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleUVCSwitch2(s_msg* msg)
{
    sLock->mState = MS_BUSY;
    cam_msg_uvc_switch2_param_data* video_param = (cam_msg_uvc_switch2_param_data*)msg->data;
    LOG_INFO("handleUVCSwitch2: camera_type = %d\n", video_param->camera_type);
    MediaUvc::GetInstance().SetCamType2(video_param->camera_type);
    sLock->reply(msg->mid, MR_SUCCESS, nullptr, 0, s_type::TYPE_CDC);
    sLock->mState = MS_STANDBY;
}

void SenselockHandler::handleGetVideoMode(s_msg* msg, s_type type)
{
    sLock->mState = MS_BUSY;

    EncodeVideoMode encode_mode = static_cast<EncodeVideoMode>(msg->data[0]);
    auto video_mode = MediaUvc::GetInstance().GetVideoMode(encode_mode);
    LOG_INFO("handleGetVideoMode: encode mode %u, video mode %u, type %d\n", encode_mode, video_mode, type);

    if (video_mode == VIDEO_FRAME_MODE_INVALID) {
        sLock->reply(msg->mid, MR_FAILED4_INVALIDPARAM, NULL, 0, type);
        sLock->mState = MS_STANDBY;
        return;
    }

    sLock->reply(msg->mid, MR_SUCCESS, &video_mode, 1, type);
    sLock->mState = MS_STANDBY;
}
#endif

void SenselockHandler::earlyVerify()
{
    auto& result = mEarlyVerifyResult;


#ifdef KOKS_SDK_VERSION_V2
    int status = 0;
    double brightness = 0, rgb_brightness = 0;
    st_rect_t rc_face_ir = { 0 }, rc_face_rgb{ 0 }, rc_palm_ir = { 0 }, rc_palm_rgb{ 0 }, opisthenar_rect{ 0 };
    st_module_detect_mode detect_mode = ST_MODULE_DETECT_MODE_UNLOCK;
    st_image_t img_ir = { 0 };
    st_image_t img_rgb = { 0 };
    
    uint8_t licence_buffer[64] = { 0 };
    if (!app_flash_read_licence(licence_buffer)) {
        result.ret = -1;
        LOG_ERROR("earlyVerify licence invalid\n");
        return;
    }


    img_ir.data = sLock->mImage.first_image;
    img_ir.width = sLock->mImage.width;
    img_ir.height = sLock->mImage.height;
    img_ir.pixel_format = sLock->mImage.first_pixel_format;
    img_ir.stride = sLock->mImage.first_stride;

    result.ret = st_face_plam_detect(&img_ir, &rc_face_ir, &rc_palm_ir, &opisthenar_rect, &result.sdk_status, detect_mode);
    if (result.ret != ST_OK || result.sdk_status == ST_PALM_MODULE_STATUS_NO_PALM) {
        LOG_ERROR("%s ir st_face_plam_detect ret %d, sdk_status %d\n", __func__, result.ret, result.sdk_status);
        return;
    }

    if (rc_face_ir.right > 0 && (rc_palm_ir.right <= 0 || rc_palm_ir.right - rc_palm_ir.left < 300)) {
        LOG_INFO("ir detect face, start detect face in rgb\n");
        img_rgb.data = sLock->mImage.second_image;
        img_rgb.width = sLock->mImage.width;
        img_rgb.height = sLock->mImage.height;
        img_rgb.pixel_format = sLock->mImage.second_pixel_format;
        img_rgb.stride = sLock->mImage.second_stride;
        result.ret = st_face_plam_detect(&img_rgb, &rc_face_rgb, &rc_palm_rgb, &opisthenar_rect, &result.sdk_status, detect_mode);
        if (result.ret != ST_OK || result.sdk_status != ST_PALM_MODULE_STATUS_NORMAL) {
            LOG_ERROR("%s rgb st_face_plam_detect ret %d, sdk_status %d\n", __func__, result.ret, result.sdk_status);
            return;
        }
        if (rc_face_ir.right > 0 && rc_face_rgb.right > 0) {
            LOG_INFO("%s face detected\n", __func__);
            st_rect_t rc_faceae_ir, rc_faceae_rgb;
            result.ret = st_face_quality(&img_ir, rc_face_ir, false, &result.sdk_status, &brightness, &rc_faceae_ir);
            if (result.ret != ST_OK || result.sdk_status != ST_FACE_MODULE_STATUS_NORMAL) {
                LOG_ERROR("%s ir st_face_quality ret %d, sdk_status %d\n", __func__, result.ret, result.sdk_status);
                return;
            }

            result.ret = st_face_quality(&img_rgb, rc_face_rgb, false, &result.sdk_status, &rgb_brightness, &rc_faceae_rgb);
            if (result.ret != ST_OK || result.sdk_status != ST_FACE_MODULE_STATUS_NORMAL) {
                LOG_ERROR("%s rgb st_face_quality ret %d, sdk_status %d\n", __func__, result.ret, result.sdk_status);
                return;
            }
            st_face_module_set_enable_liveness(1);
            result.ret = st_face_module_unlock(&img_ir, rc_face_ir, &img_rgb, rc_face_rgb, &result.sdk_status, &result.sdk_user_id, true);
            if(rc_faceae_ir.right > 0 && rc_faceae_rgb.right > 0) {
                sLock->setCameraROI(&rc_faceae_ir, &rc_faceae_rgb, brightness, EnrollUnlockFlag::FLG_FACE);
            }
        }
    }
#ifdef KOKS_FEATURE_PALM
    else {
        do {
            st::sense_koks::LedGuard ledGuard;
            auto& media_manager = MediaManager::GetInstance();
            IMPISPAEExprInfo exprinfo;
            if(sLock->ev_info.irAeIntegrationTime0 >= 1000) {
                extern int sensor_stream_on(int sernsor_id);
                extern int sensor_stream_off(int sernsor_id);
                extern int sensor_load_ev(int sernsor_id, int ev);
                IMP_ISP_Tuning_GetAeExprInfo((IMPVI_NUM)0, &exprinfo);
                // LOG_INFO("%s get AeIntegrationTime: %d, AeAGain = %d, AeIspDGain = %d from sensor\n", __func__,
                //                             exprinfo.AeIntegrationTime, exprinfo.AeAGain, exprinfo.AeIspDGain);
                exprinfo.AeIntegrationTime = sLock->ev_info.irAeIntegrationTime1;
                exprinfo.AeIspDGain = sLock->ev_info.irAeAGain1;
                exprinfo.AeAGain = sLock->ev_info.irAeIspDGain1;
                IMP_ISP_Tuning_SetAeExprInfo((IMPVI_NUM)0, &exprinfo);
                // LOG_INFO("%s load AeIntegrationTime: %d, AeAGain = %d, AeIspDGain = %d to sensor\n", __func__,
                //                 sLock->ev_info.AeIntegrationTime1, sLock->ev_info.AeAGain, sLock->ev_info.AeIspDGain);
                int ev = (sLock->ev_info.irAeAGain1 << 10) + sLock->ev_info.irAeIntegrationTime1;
                sensor_load_ev(0, ev);
                sLock->switchEffectBin(true);
                usleep(2 * 1000 * 1000 / SENSOR_FRAME_RATE_NUM);
                sensor_stream_off(0);
                sensor_stream_off(1);
                sensor_stream_on(0);
                sensor_stream_on(1);
                usleep(1000 * 1000 / SENSOR_FRAME_RATE_NUM);
            }
            else {
                sLock->switchEffectBin(true);
                usleep(2 * 1000 * 1000 / SENSOR_FRAME_RATE_NUM);
            }

            LOG_INFO("GetCameraFrame start call GetCameraFrame \n");
            sLock->GetCameraFrame(img_rgb, img_ir, 0);
        } while (0);

        LOG_INFO("GetCameraFrame end call GetCameraFrame --- vertify begin\n");
        result.ret = st_face_plam_detect(&img_ir, &rc_face_ir, &rc_palm_ir, &opisthenar_rect, &result.sdk_status, detect_mode);
        if (result.ret != ST_OK || result.sdk_status != ST_PALM_MODULE_STATUS_NORMAL) {
            LOG_ERROR("%s ir st_face_plam_detect ret %d, sdk_status %d\n", __func__, result.ret, result.sdk_status);
            return;
        }

        st_rect_t rc{ 0 };
        result.ret = st_palm_module_unlock(&img_ir, rc_palm_ir, &img_rgb, rc_palm_rgb, &rc, &result.sdk_status, &result.sdk_user_id, &brightness, true);
        if(rc.right > 0) {
            sLock->setCameraROI(&rc, &rc, brightness, EnrollUnlockFlag::FLG_PALM);
        }
        LOG_INFO("%s ret %d, sdk_status %d, user_id %d, brightness %f\n", __func__, result.ret, result.sdk_status, result.sdk_user_id, brightness);
        return;
    }
#endif //KOKS_FEATURE_PALM
#else  // KOKS_SDK_VERSION_V2
    st_face_module_set_enable_liveness(1);
    result.ret = st_face_module_unlock(&sLock->mImage, &result.sdk_status, &result.sdk_user_id, &result.occlusion_percent);
#endif // KOKS_SDK_VERSION_V2
    LOG_INFO("%s ret %d, sdk_status %d, user_id %d\n", __func__, result.ret, result.sdk_status, result.sdk_user_id);
}
