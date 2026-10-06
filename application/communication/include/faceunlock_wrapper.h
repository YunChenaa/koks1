/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-07 14:29:21
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-14 17:50:58
 * @FilePath: /sense-koks/application/communication/include/faceunlock_wrapper.h
 */
#pragma once
#include "koks_project_def.h"
#include "senselock.h"

#define SAVE_THRESHOLD_LEVEL_PATH "thres.dat"
#define LIVENESS_LOGIC_METHOD 2

typedef struct {
    float min;
    float max;
} Value;

typedef struct {
    Value roll;
    Value pitch;
    Value yaw;
} Axis;

typedef struct {
    Axis enroll_angle;
    Axis unlock_angle;
} PoseAngle;

enum DEVICE_TYPE { DEVICE_TYPE_LOCK = 0, DEVICE_TYPE_BELL = 1, DEVICE_TYPE_MAX };
enum verify_model_status {
    VERIFY_MODEL_STANDBY = 0,
    VERIFY_MODEL_LOADING,
    VERIFY_MODEL_LOADED,
    VERIFY_MODEL_ERROR,
};

class FaceUnlockWrapper
{
    public:
    static FaceUnlockWrapper* getInstance();
    int init(void);
#ifndef KOKS_SDK_VERSION_V2
    int initFaceModule(void);
    bool checkLicence(void);
    void getParamConfig(st_module_param_config_t& paramConfig);
    void getAttriStateInfo(uint8_t& attriState);
    void setAttriStateInfo(uint8_t attriState);
    void outputFaceRect(st_image_t* image, std::vector<st_rect_t>* rect);
    void loadVerifyModel(void);
    void updateVerifyModel(void);
    verify_model_status waitVerifyModelLoaded(void);
#endif
    void updateParamConfig(s_msg_algo_threshold_level* threLevel);
    void getLevelParamConfig(s_msg_algo_threshold_level* threLevel);
    void updateAlgoThreshold(s_msg_algo_threshold* threshold);
    void getAlgoThreshold(s_msg_algo_threshold* threshold);
    void set_misc_ai_config(s_msg_misc_ai_config config);
    void get_misc_ai_config(s_msg_misc_ai_config* config);
    void set_blur_config(s_msg_blur_config blur);
    void get_blur_config(s_msg_blur_config* config);

    private:
    static Senselock* sLock;
    FaceUnlockWrapper();

    private:
    uint8_t m_attribute_state_info;
    verify_model_status mVerifyModelStatus;
    st_module_config_t* m_moduleConfig;
#ifndef KOKS_SDK_VERSION_V2
    st_face_model_info mVerifyModelInfo;
#endif
};
