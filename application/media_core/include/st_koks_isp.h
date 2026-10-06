/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 19:30:36
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-25 12:01:35
 * @FilePath: /sense-koks/application/media_core/include/st_koks_isp.h
 */
#pragma once
#include "imp_isp.h"
#include "koks_project_def.h"
#include "sys_logger.h"
#include "koks1_config.h"
#include "st_koks_errno.h"
#include "fast_frame_info.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace st
{
namespace sense_koks
{
#define CUSTOM_GPIO GPIO_PB(5)
#define MAX_SENSOR_NUM KOKS_FEATURE_MAX_SENSOR_NUM

typedef struct {
    IMPISPFaceAeWeiget weight;
    uint16_t face_ae_target;
#ifdef KOKS_SDK_VERSION_V2
    uint16_t palm_ae_target;
#endif
} FaceAeConfig;
class Isp
{
    public:
    static Isp& GetInstance();
    ST_ERR Init(const std::vector<ISPConfig>& config);
    ST_ERR Deinit(void);
    ST_ERR SetFaceAE(uint8_t id, IMPISPFaceAttr& face_attr);
    ST_ERR GetFaceAE(uint8_t id, IMPISPFaceAttr* face_attr);
    ST_ERR CloseFaceAe(void);
    ST_ERR CloseFaceAe(uint8_t id);
    ST_ERR setFaceAe(bool en);
    ST_ERR GPIO_Init(void);
    ST_ERR GPIO_Deinit(void);
    ST_ERR GPIO_Set(int value);
    ST_ERR SetSensorFrameRate(int frameRate);
    ST_ERR SetSensorDayNightMode(IMPVI_NUM sensor_id, IMPISPRunningMode sensor_mode);
    ST_ERR SetIspByPass(IMPVI_NUM id);
    int GetSensorFrameRate() const { return sensor_frame_rate_; }
    bool IsAutoAEMode() const { return autoAE_.load(std::memory_order_relaxed); }

    const FaceAeConfig& GetAEConfig(uint8_t id) const
    {
        return face_ae_config[id];
    }

    ST_ERR setAEMode(int id)
    {
#ifdef AE_ONLY_MAIN_SENSOR
        if (id != IMPVI_MAIN)
            return ST_ERRNO_SUCCESS;
#endif
        IMPISPAEExprInfo exprinfo;
        ST_ERR ret = IMP_ISP_Tuning_GetAeExprInfo(static_cast<IMPVI_NUM>(id), &exprinfo);
        // MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_GetAeExprInfo failed with %#x\n", ret);
        LOG_INFO("sensor %d IMP_ISP_Tuning_GetAeExprInfo success, AeMode %d\n", id, exprinfo.AeMode);

        if (exprinfo.AeMode == IMPISP_TUNING_OPS_TYPE_MANUAL) {
            exprinfo.AeMode = IMPISP_TUNING_OPS_TYPE_AUTO;

            ret = IMP_ISP_Tuning_SetAeExprInfo(static_cast<IMPVI_NUM>(id), &exprinfo);
            // MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetAeExprInfo failed with %#x\n", ret);
            LOG_INFO("sensor %d IMP_ISP_Tuning_SetAeExprInfo success, AeMode %d\n", id, exprinfo.AeMode);
            if (ret == ST_ERRNO_SUCCESS && !autoAE_.load(std::memory_order_relaxed)) {
                autoAE_.store(true, std::memory_order_relaxed);
            }
        }
        return ret;
    }

    void setVideoStatus(bool active);
#ifdef KOKS_PROJECT_M24B3
    void checkBrightness(uint8_t id);
    bool isBright(uint8_t id) { return (id < MAX_SENSOR_NUM) ? sensor_bright[id] : true; }
#endif

    static struct fast_frame_ev_info ev_info_;

    //uvc初始化锁函数; // 该函数会阻塞调用线程，直到BootJudgefinish为true，放行线程；
    void WaitForBootDayNightJudge(){
        std::unique_lock<std::mutex> lock(uvc_init_mutex_);
        // LOG_DEBUG("Waiting for Boot day/night judge to finish before initializing UVC...\n");
        uvc_init_cv_.wait(lock, [this]{ return BootJudgefinish; });
        // LOG_DEBUG("Boot day/night judge finished, proceeding with UVC initialization.\n");
    }

    private:
    std::mutex uvc_init_mutex_;
    std::condition_variable uvc_init_cv_;

    bool BootJudgefinish = false;

    uint16_t sensor_num{ 0 };
    IMPSensorInfo sensor_info[MAX_SENSOR_NUM];
    FaceAeConfig face_ae_config[MAX_SENSOR_NUM];
    int sensor_frame_rate_{30};
    std::atomic<bool> video_active_{ true };
    std::atomic<bool> initialized_{ false };
    std::atomic<bool> autoAE_{ false };

#ifdef KOKS_PROJECT_M24B3
    bool sensor_bright[MAX_SENSOR_NUM] {true, true};
#endif
    static void SetSensorInfo(IMPSensorInfo& info, FaceAeConfig& ae_config, const struct ISPConfig& config);
    Isp() = default;
    ~Isp() = default;
};
} // namespace sense_koks

} // namespace st
