/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 23:54:51
 * @LastEditors: 洪照
 * @LastEditTime: 2026-01-29 15:27:31
 * @FilePath: /sense-koks/application/communication/include/senselock_handler.h
 */
#pragma once
#include "faceunlock_wrapper.h"
#include "koks_project_def.h"
#include "senselock.h"
#include <cstdint>
#include <vector>

#define HACKNESS_LIMITED_FRAME_COUNT 15
#if defined(KOKS_FEATURE_KDS)
#define VERIFY_LIMITED_FRAME_COUNT 2
#else
#define VERIFY_LIMITED_FRAME_COUNT 5
#endif
#define VERIFY_LIMITED_NOTSMILE_COUNT 5

#define BANDRATE_9600 (9600)
#define BANDRATE_115200 (115200)
#define BANDRATE_230400 (230400)
#define BANDRATE_460800 (460800)
#define BANDRATE_1500000 (1500000)

class SenselockHandler
{
    public:
    static SenselockHandler* getInstance();
    static void nv12_to_I420(const st_image_t* image_src, st_image_t* image_dst);
    void handleMsgFaceReset(s_msg* msg);
    void handleMsgEnroll(s_msg* msg, uint8_t enroll_type);
    void handleMsgEnrollFeature(s_msg* msg);
    void handleMsgSetRgbImageSize(s_msg* msg);
    void handleMsgTransferRGBImage(s_msg* msg);
    void handleMsgEnrollRGB(s_msg* msg, uint8_t dup_status);
    void handleMsgEnrollItg(s_msg* msg);
    void handleMsgVerify(s_msg* msg);
    void handleGetAllUserId();
    void handleConfigBaudrate(s_msg* msg);
    int mount_pictures_partition();
    void handleMsgUploadImage(s_msg* msg, unsigned char* image_buf, int image_len);
    void handleMsgGetRawImage(unsigned char* image_buf, int& image_len);
    void handleMsgGetImage(unsigned char* image_buf, int& image_len);
    void handleReboot(void);
    void handleMsgSnapDualImage(s_msg* msg);
    void handleMsgSnapImage(s_msg* msg);
    void handleMsgGetSavedImage(s_msg* msg, unsigned char* upload_image_buf, int& upload_image_len);
    void format_fs(s_msg* msg);
    void handleMsgGetDebugInfo(s_msg* msg, int& upload_debug_info_total_len);
    void handleMsgUploadDebugInfo(s_msg* msg, int upload_debug_info_total_len);
    void restore_factory_settings(s_msg* msg);

    void handleMsgGetLogFile(s_msg* msg, unsigned char** upload_log_buf, int& upload_log_len);
    void handleMsgUploadLogFile(s_msg* msg, unsigned char* upload_log_buf, int upload_log_len);
#ifdef KOKS_FEATURE_LCD_EMOTION
    void handleMsgEmotion(s_msg* msg);
#endif
    void handlePowerDown(s_msg* msg);
    void handleMsgDelAll(void);
    void handleMsgDelOVDConfig(void);
    void handleMsgDelUser(s_msg* msg);
    void handleMsgGetUserInfo(s_msg* msg);
    void handleMsgwriteSN(s_msg* msg);
    void handleChangeReleaseKey(s_msg* msg);
    void handleMsgSetThreLevel(s_msg* msg);
    void handleMsgGetThreLevel(s_msg* msg);
    void handleMsgSetThreshold(s_msg* msg);
    void handleMsgGetThreshold(s_msg* msg);
    void handleMsgMiscAiConfig(s_msg* msg);
    void handleMsgBlurConfig(s_msg* msg);
    void handleSetFaceInfo(st_module_face_info_t* face_info);
    void handleReset(void);
    void handleSaveFeature(s_msg* msg);
    void handleHardwareCheck(void);
    void handleFactoryTest(s_msg* msg);
#ifdef KOKS_SDK_VERSION_V2
    void handleEnrollUnlock(s_msg* msg, st_module_detect_mode mode, uint8_t flg);
    void handleCommon(s_msg* msg);
    //获取模型MD5
    void handleMsgGetModelMD5(s_msg* msg);
#ifdef KOKS_FEATURE_PALM
    int palmInit(void);
#endif
    int checkEarlyVerifyResult(s_msg* msg, uint8_t flg);
#else
    void handleMsgDebugMode(s_msg* msg);
    void handleDemoMode(s_msg* msg);
#endif
    void earlyVerify();

    private:
    Senselock* sLock;
    SenselockHandler();
    uint8_t LOG_VDEBUG_LEVEL_TMP{ 0xF };
    int saveDebugImage(st_module_image_t* raw_image);
    int getSavingImageId(uint8_t* image_id);
#ifdef KOKS_SDK_VERSION_V2
    int handleEnrollUnlockStatus(s_msg* msg, int uid, int orientation, int status);
#ifdef KOKS_FEATURE_PALM
    void setPalmDualEnroll(bool enable);
    std::atomic<bool> palm_inited{ false };
    void setPalmLevel(uint8_t lvl[4], bool set = true);
#endif
    void sendNote(uint8_t note_id, int16_t state);
    int getUsrCnt(EnrollUnlockFlag type);

#else
    int backup_file(char* name);
#endif
    std::string DEBUG_INFO_PATH{ "/debug" };
    constexpr static int IGNORED_FRAME_COUNTS{ 5 };
    constexpr static int SINGLE_MAX_SNAP_IMAGE_COUNTS{ 5 };
    constexpr static int PICTURES_BEGIN_INDEX_NUMBER{ 1 };
    constexpr static int PICTURES_MAX_INDEX_NUMBER{ 15 };

    public:
    void getSWVersion();
    void getLibraryVersion();
    void handleSetUTCTime(s_msg* msg);
#ifdef KOKS_FEATURE_VIDEO
    void handleSetVideoParam(s_msg* msg, s_type type);
    void handleSetAudioParam(s_msg* msg);
    void handleSetUVCParam(s_msg* msg);
    void handleSetUVCStreamType(s_msg* msg, s_type type);
    void handleUVCSwitch(s_msg* msg, s_type type);
    void handleUVCSwitch2(s_msg* msg);
    void handleGetVideoMode(s_msg* msg, s_type type);
#endif
    private:
    struct EarlyVerifyResult {
        int sdk_status = 0;
        int sdk_user_id = 0;
        uint8_t occlusion_percent = 0;
        int ret = -1;
    };

    EarlyVerifyResult mEarlyVerifyResult;

    struct MessageNotifyData
    {
        uint8_t nid;
        uint8_t data[16];
    };

    void StartMessageNotifyThread();
    void SetNotifyMessagedata(uint8_t nid, void *data);
    void StopMessageNotifyThread();
    void MessageNotifyThreadFunc();//100ms通知一次 , 如果有新数据就立即发送 ，没有新数据就超时发送上次的数据

    std::thread mMessageNotifyThread;
    std::atomic<bool> mMessageNotifyThreadRunning{ false };
    std::mutex mMessageNotifyMutex;
    std::condition_variable mMessageNotifyCv;
    bool mMessageNotifyHasNewData{ false };
    MessageNotifyData mMessageNotifyData;
    
};
