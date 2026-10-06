/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 19:30:41
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-25 12:01:29
 * @FilePath: /sense-koks/application/media_core/src/st_koks_isp.cpp
 */
#include "st_koks_isp.h"
#include "factory.h"
#include "imp_encoder.h"
#include "imp_ldc.h"
#include "imp_system.h"
#include "koks_project_def.h"
#include "os_utils.h"
#include "st_common.h"
#include "st_koks_const.h"
#include "st_koks_led.h"
#include "st_koks_uvc.h"
#include "st_koks_video.h"
#include "sys_logger.h"
#include "imp_isp.h"
#include <atomic>
#include <thread>
#include <list>

uint8_t g_uvc_init_video = 0;

#if defined(KOKS_PROJECT_M53B) || defined(KOKS_PROJECT_M55)
#define SENSOR1_NAME        "sc520css1"
#define SENSOR1_IC2_ADDR    0x36
#else
#define SENSOR1_NAME        "ov02fs1"
#define SENSOR1_IC2_ADDR    0x3d
#endif

#ifdef KOKS_PROJECT_M53D
int g_normal_nightvision_mode = 0;
#endif

namespace st
{
namespace sense_koks
{

#if KOKS_FEATURE_MAX_SENSOR_NUM == 2
const IMPSensorInfo DEFAULT_SENSOR_INFO[MAX_SENSOR_NUM] = { {
                                                                "ov02f", // name
                                                                .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
                                                                {
                                                                    "ov02f", // type
                                                                    0x3c,    // addr
                                                                    0,       // i2c_adapter_id
                                                                },
                                                                .rst_gpio = GPIO_PA(20),
                                                                .pwdn_gpio = GPIO_PA(21),
                                                                .power_gpio = -1,
                                                                .switch_gpio = 0,
                                                                .switch_gpio_state = 0,
                                                                .sensor_id = 0,
                                                                .video_interface = IMPISP_SENSOR_VI_MIPI_CSI0,
                                                                .mclk = IMPISP_SENSOR_MCLK0,
                                                                .default_boot = 0,
                                                                { SENSOR_FRAME_RATE_NUM, 1 } // fps
                                                            },
                                                            {
                                                                SENSOR1_NAME, // name
                                                                .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
                                                                {
                                                                    SENSOR1_NAME,     // type
                                                                    SENSOR1_IC2_ADDR, // addr
                                                                    0,                // i2c_adapter_id
                                                                },
                                                                .rst_gpio = GPIO_PA(19),
                                                                .pwdn_gpio = GPIO_PA(22),
                                                                .power_gpio = -1,
                                                                .switch_gpio = 0,
                                                                .switch_gpio_state = 0,
                                                                .sensor_id = 1,
                                                                .video_interface = IMPISP_SENSOR_VI_MIPI_CSI1,
                                                                .mclk = IMPISP_SENSOR_MCLK1,
                                                                .default_boot = 0,
                                                                { SENSOR_FRAME_RATE_NUM, 1 } // fps
                                                            } };
#elif KOKS_FEATURE_MAX_SENSOR_NUM == 3
const IMPSensorInfo DEFAULT_SENSOR_INFO[MAX_SENSOR_NUM] = { {
                                                                "ov02f", // name
                                                                .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
                                                                {
                                                                    "ov02f", // type
                                                                    0x3c,    // addr
                                                                    0,       // i2c_adapter_id
                                                                },
                                                                .rst_gpio = GPIO_PA(20),
                                                                .pwdn_gpio = GPIO_PA(21),
                                                                .power_gpio = -1,
                                                                .switch_gpio = GPIO_PB(28),
                                                                .switch_gpio_state = 0,
                                                                .sensor_id = 0,
                                                                .video_interface = IMPISP_SENSOR_VI_MIPI_CSI0,
                                                                .mclk = IMPISP_SENSOR_MCLK0,
                                                                .default_boot = 0,
                                                                { SENSOR_FRAME_RATE_NUM, 1 } // fps
                                                            },
                                                            {
                                                                SENSOR1_NAME, // name
                                                                .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
                                                                {
                                                                    SENSOR1_NAME,     // type
                                                                    SENSOR1_IC2_ADDR, // addr
                                                                    1,                // i2c_adapter_id
                                                                },
                                                                .rst_gpio = GPIO_PA(18),
                                                                .pwdn_gpio = GPIO_PA(22),
                                                                .power_gpio = -1,
                                                                .switch_gpio = 0,
                                                                .switch_gpio_state = 0,
                                                                .sensor_id = 1,
                                                                .video_interface = IMPISP_SENSOR_VI_MIPI_CSI1,
                                                                .mclk = IMPISP_SENSOR_MCLK1,
                                                                .default_boot = 0,
                                                                { SENSOR_FRAME_RATE_NUM, 1 } // fps
                                                            },
                                                            {
                                                                "ov02fs2", // name
                                                                .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
                                                                {
                                                                    "ov02fs2", // type
                                                                    0x3d,      // addr
                                                                    1,         // i2c_adapter_id
                                                                },
                                                                .rst_gpio = GPIO_PA(20),
                                                                .pwdn_gpio = GPIO_PA(21),
                                                                .power_gpio = -1,
                                                                .switch_gpio = GPIO_PB(28),
                                                                .switch_gpio_state = 0,
                                                                .sensor_id = 1,
                                                                .video_interface = IMPISP_SENSOR_VI_MIPI_CSI0,
                                                                .mclk = IMPISP_SENSOR_MCLK0,
                                                                .default_boot = 0,
                                                                { SENSOR_FRAME_RATE_NUM, 1 } // fps
                                                            } };
#else
#error "KOKS_FEATURE_MAX_SENSOR_NUM must be 2 or 3"
#endif

struct fast_frame_ev_info Isp::ev_info_ = { 0 };

Isp& Isp::GetInstance()
{
    static Isp isp;
    return isp;
}


ST_ERR Isp::SetSensorDayNightMode(IMPVI_NUM sensor_id, IMPISPRunningMode sensor_mode)
{
    IMPISPRunningMode pmode;
    IMP_ISP_Tuning_GetISPRunningMode(sensor_id, &pmode);
    if (pmode != sensor_mode) {
        if (IMP_ISP_Tuning_SetISPRunningMode(sensor_id, &sensor_mode) == 0) {
            return ST_ERRNO_SUCCESS;
        } else {
            LOG_ERROR("Set sensor %d to mode %d failed\n", sensor_id, sensor_mode);
            return ST_ERRNO_FAIL;
        }
    }
    return ST_ERRNO_SUCCESS;
}

ST_ERR Isp::SetIspByPass(IMPVI_NUM id)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPISPModuleCtl ctl;

    ret = IMP_ISP_Tuning_GetModuleControl(id, &ctl);
    if(ret != ST_ERRNO_SUCCESS) {
        return ret;
    }

    ctl.bitBypassTMO = 1;
    ctl.bitBypassLSC = 1;
    ctl.bitBypassGAMMA = 0;
    ctl.bitBypassLCE = 1;
    ctl.bitBypassDEFOG = 1;
    ctl.bitBypassBCSH = 1;
    return IMP_ISP_Tuning_SetModuleControl(id, &ctl);
}

/**
 * @name: SetSensorInfo
 * @brief: 设置传感器信息
 * @param {Json&} config 配置文件
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::Init(const std::vector<ISPConfig>& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    LOG_DEBUG("Init\n");

    sensor_frame_rate_ = SENSOR_FRAME_RATE_NUM;

    ret = GPIO_Init();
    MISC_CHK_STATUS_REPORT(ret, ret, "GPIO_Init failed with %#x\n", ret);

    sensor_num = config.size();
    LOG_DEBUG("sensor num : %d\n", sensor_num);

#if defined(KOKS_PROJECT_M53)
    const bool useDefaultJpegBsSize = g_FactoryTest ? true :  false;
#else
    const bool useDefaultJpegBsSize = true;
#endif

    if (useDefaultJpegBsSize) {
        ret = IMP_Encoder_SetJpegBsSize(500 * 1024);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_Encoder_SetJpegBsSize failed with %#x!\n", ret);
    } else {
        ret = IMP_Encoder_SetJpegMultiSectionMode(0, 0, 2);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_Encoder_SetJpegMultiSectionMode failed with %#x!\n", ret);

        ret = IMP_Encoder_SetJpegBsSize(256);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_Encoder_SetJpegBsSize failed with %#x!\n", ret);
    }

    // ret = IMP_ISP_Tuning_SetOsdPoolSize(512 * 1024);
    // MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetOsdPoolSize failed with %d!\n", ret);

    ret = IMP_Encoder_SetMultiSectionMode(1, 250, 2);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_Encoder_SetMultiSectionMode failed with %#x\n", ret);

    ret = IMP_ISP_Open();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Open failed with %#x\n", ret);

    IMPISPCameraInputMode isp_input_mode;
    memset(&isp_input_mode, 0, sizeof(isp_input_mode));
    isp_input_mode.sensor_num = static_cast<IMPISPSensorNum>(sensor_num);

    if (isp_input_mode.sensor_num > IMPISP_TOTAL_ONE) {
        ret = IMP_ISP_SetCameraInputMode(&isp_input_mode);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_SetCameraInputMode failed with %#x\n", ret);
    }

    for (auto& iter : config) {
        auto camera_id = iter.cam_id;
        if (camera_id > MAX_SENSOR_NUM - 1) {
            LOG_ERROR("camera id %d out of limit\n", camera_id);
            break;
        }
        memcpy(&sensor_info[camera_id], &DEFAULT_SENSOR_INFO[camera_id], sizeof(sensor_info[0]));
        SetSensorInfo(sensor_info[camera_id], face_ae_config[camera_id], iter);
    }

	IMPISPLDCMode mode2 = IMPISP_OFFLINE_MODE;
	IMPISPLDCOfflineOpsMode offline_mode = IMPISP_Offline_ENABLE;
	IMP_ISP_LDC_SetMode(mode2);
	IMP_ISP_LDC_OfflineInit(offline_mode);

    for (int i = 0; i < MAX_SENSOR_NUM; ++i) {
        ret = IMP_ISP_AddSensor((IMPVI_NUM)i, &sensor_info[i]);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_AddSensor failed with %#x, id %d\n", ret, i);
        ret = IMP_ISP_EnableSensor((IMPVI_NUM)i, &sensor_info[i]);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_EnableSensor failed with %#x, id %d\n", ret, i);
    }

    ret = IMP_System_Init();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_System_Init failed with %#x\n", ret);

    ret = IMP_ISP_EnableTuning();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_EnableTuning failed with %#x\n", ret);

    unsigned char value = 128;
    for (int i = 0; i < MAX_SENSOR_NUM; ++i) {
        ret = IMP_ISP_Tuning_SetContrast((IMPVI_NUM)i, &value);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetContrast failed with %#x, id %d\n", ret, i);
        ret = IMP_ISP_Tuning_SetSharpness((IMPVI_NUM)i, &value);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetSharpness failed with %#x, id %d\n", ret, i);
        ret = IMP_ISP_Tuning_SetSaturation((IMPVI_NUM)i, &value);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetSaturation failed with %#x, id %d\n", ret, i);
        ret = IMP_ISP_Tuning_SetBrightness((IMPVI_NUM)i, &value);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetBrightness failed with %#x, id %d\n", ret, i);
#ifdef KOKS_ISP_BYPASS
        ret = SetIspByPass((IMPVI_NUM)i);
        MISC_CHK_STATUS_REPORT(ret, ret, "SetIspByPass failed with %#x, id %d\n", ret, i);
#endif
    }

    if (g_uvc_init_video) {
        LOG_WARNING("g_uvc_init_video=1\n");
        Isp::GetInstance().setAEMode(0);
        Isp::GetInstance().setAEMode(1);
    }

#ifdef KOKS_FEATURE_VIDEO
    auto isp_day_switch = std::thread([this]() {
        IMPVI_NUM vinum = IMPVI_SEC;
        IMPVI_NUM vinum_main = IMPVI_MAIN;
        IMPVI_NUM vinnum_thr = IMPVI_THR;
   
        IMPISPAeOnlyReadAttr aeOrAttr;
        memset(&aeOrAttr, 0, sizeof(aeOrAttr));
        uint64_t iso_buf;
        uint32_t luma = 0, luma_scence = 0;
        int night_count = 0;
        int day_count = 0;
        int total_count = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(400)); // 启动后等待400ms让AE稳定
        const NightDaySwitchConfig nd_config = koks_config.night_day_switch;

        std::list<float> iso_queue;  // 用于存储最近的iso值
        const int queue_size = 5;    // 队列大小
        int discard_after_count = 0; // Iso跳变判断，切换后丢弃帧计数

#ifdef KOKS_FEATURE_KDS
        const uint64_t LED_ON_MAX_DURATION = 5 * 60 * 1000; // 5 min
#else
        const uint64_t LED_ON_MAX_DURATION = UINT64_MAX;
#endif
        uint64_t lastLedOnTime = 0;
        auto& led = Led::GetInstance(0);
        auto turnOnLed = [&led](bool on) -> bool {
            if (on) {
                if (!led.IsNightMode()) {
                    LedProxy::SetNightMode(true);
                    return true;
                }
            } else { // off
                if (led.IsNightMode()) {
                    LedProxy::SetNightMode(false);
                    return true;
                }
            }
            return false;
        };




        auto switchtonight = [&]()->int{
                    IMPISPRunningMode pmode;
                    IMP_ISP_Tuning_GetISPRunningMode(vinum, &pmode);
                    if (pmode != IMPISP_RUNNING_MODE_NIGHT) {
                        if (video_active_) {
                            if (turnOnLed(true)) {
                                lastLedOnTime = GetCurTimeMs();
                            }
                        }
                        GPIO_Set(1);
                        MediaUvc::GetInstance().SetNightMode(true);
             
                        this->SetSensorDayNightMode(vinum, IMPISP_RUNNING_MODE_NIGHT);
                        this->SetSensorDayNightMode(vinum_main, IMPISP_RUNNING_MODE_NIGHT);
                        if(KOKS_FEATURE_MAX_SENSOR_NUM == 3)
                            this->SetSensorDayNightMode(vinnum_thr, IMPISP_RUNNING_MODE_NIGHT);  


                        IMPISPModuleCtl ctl;
                        auto ret = IMP_ISP_Tuning_GetModuleControl(vinum, &ctl);
                        if (ret) {
                            LOG_DEBUG("IMP_ISP_Tuning_GetModuleControl error !\n");
                            return -1;
                        }
                        LOG_DEBUG(" ------------------- ISP module control: begin 0x%x DPC :%d MDNS :%d ------------------- \n", ctl.key, ctl.bitBypassDPC, ctl.bitBypassMDNS);
                        ctl.bitBypassDPC = 0;
                        ctl.bitBypassMDNS = 0;
                        ret = IMP_ISP_Tuning_SetModuleControl(vinum, &ctl);
                        if (ret) {
                            LOG_DEBUG("IMP_ISP_Tuning_SetModuleControl error !\n");
                            return -1;
                        }
                        LOG_INFO("### entry night mode ###\n");
                        return 1;
                    }
                    return 0;
        };

        auto switchtoday = [&]()->int{
                    IMPISPRunningMode pmode;
                    IMP_ISP_Tuning_GetISPRunningMode(vinum, &pmode);
                    if (pmode != IMPISP_RUNNING_MODE_DAY) {
                        turnOnLed(false);
                        lastLedOnTime = 0;
                        GPIO_Set(0);
                        MediaUvc::GetInstance().SetNightMode(false);

                        this->SetSensorDayNightMode(vinum, IMPISP_RUNNING_MODE_DAY);

                        this->SetSensorDayNightMode(vinum_main, IMPISP_RUNNING_MODE_DAY);
                        if (KOKS_FEATURE_MAX_SENSOR_NUM == 3)
                            this->SetSensorDayNightMode(vinnum_thr, IMPISP_RUNNING_MODE_DAY);

                        LOG_INFO("### entry day mode ###\n");
                        return 1;
                    }
                    return 0;
        };

        int BootMode = 0; // 0 auto, 1 day, 2 night 
        
        while (1) {
            if (g_FactoryTest) {
                if (!this->BootJudgefinish) {
                    this->BootJudgefinish = true;
                    this->uvc_init_cv_.notify_all();
                }
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }
        #ifdef KOKS_PROJECT_M53D
            if(g_normal_nightvision_mode == 0) { //auto
        #endif
            auto ret = IMP_ISP_Tuning_GetAeOnlyReadAttr(vinum, &aeOrAttr);
            MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, continue, "IMP_ISP_Tuning_GetAeOnlyReadAttr failed with %#x\n", ret);
            iso_buf = aeOrAttr.ExposureValue;
            luma = aeOrAttr.luma;
            luma_scence = aeOrAttr.luma_scence;

            // 前3帧判断是否要进入夜视
                   
            IMPISPAWBGlobalStatisInfo awb_stats;
            IMP_ISP_Tuning_GetAwbGlobalStatistics(vinum, &awb_stats);
            IMPISPAWBGain statis_gol_gain = awb_stats.statis_gol_gain;


            ++total_count;
            bool isNight = (iso_buf > nd_config.RGB_ISOThreshold_DaytoNight) 
                        && (luma <= nd_config.RGB_LumaThreshold_DaytoNight) || luma == 0;
                        
            if (total_count <= 3) {
                IMPISPAEExprInfo ei1;
                IMP_ISP_Tuning_GetAeExprInfo(static_cast<IMPVI_NUM>(1), &ei1);
                LOG_INFO("RGB AeIntegrationTime=%d, AeAGain=%d, AeIspDGain=%d, luma = %d,total_count = %d, awb = %d , iso = %d\n", ei1.AeIntegrationTime, ei1.AeAGain,
                         ei1.AeIspDGain, luma, total_count, (statis_gol_gain.rgain + statis_gol_gain.bgain) / 2, iso_buf);

                if (isNight) {
                    night_count++;
                }

                if (night_count >= 2 || (total_count == 3 && ev_info_.isReady == 0x3 && ev_info_.rgbAeAGain >= 12 * 1024 && (statis_gol_gain.rgain+statis_gol_gain.bgain)/2 > 240) ) {
                    // 夜晚，设置night_count=100进入夜视模式
                    LOG_INFO("Boot with night mode\n");
                    night_count = 100;
                    day_count = 0;
                    total_count = 3;
                    BootMode = 2;
                } else if (total_count == 3) {
                    // 白天
                    LOG_INFO("Boot with day mode\n");
                    night_count = 0;
                    day_count = 0;
                    BootMode = 1;
                    MediaUvc::GetInstance().SetNightMode(false);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(65));
                    continue;
                }
            }


            
            
            //通过awb判断白天黑夜
            bool Awb_IsDay =  ((statis_gol_gain.rgain+statis_gol_gain.bgain)/2 
                                < nd_config.RGB_AWBThreshold_NighttoDay);
            
            
            bool is_night_to_day_jump = false;
            bool is_day_to_night_jump = false;
            //差值
            float max_to_min_diff = 0.0f;

            float max_iso = 0.0f;
            int max_iso_index = 0;
            float min_iso = 99999999999.0f; 
            int min_iso_index = 0;
            if (discard_after_count) {
                discard_after_count--;
                iso_queue.clear();
     
            } else {
                iso_queue.push_back(iso_buf);

                if (iso_queue.size() > queue_size) {
                    iso_queue.pop_front();

                }
                {
                    // 判读晚上到白天的跳变
                    auto iso_it = iso_queue.begin();
                    for (int i = 0; i < iso_queue.size(); i++) {
                        if (*iso_it > max_iso) {
                            max_iso = *iso_it;
                            max_iso_index = i;
                        }

                        if (*iso_it < min_iso) {
                            min_iso = *iso_it;
                            min_iso_index = i;
                        }
                        iso_it++;
                    }
                    if (max_iso_index < min_iso_index) {
                        max_to_min_diff = (max_iso - min_iso) / max_iso;
                        if (max_to_min_diff > nd_config.RGB_ISOJumpThreshold_NighttoDay && Awb_IsDay) {
                            is_night_to_day_jump = true;
                        }
                    }
                }   
            }

            // LOG_INFO("iso_buf: %llu, luma: %u, max_to_min_diff: %f, Awb %d\n", iso_buf,luma,max_to_min_diff,(statis_gol_gain.rgain + statis_gol_gain.bgain) / 2);
            /***********************阈值判断切换********************************* */

            bool ThresholdSwitchtoNight = false;
            if (night_count == 100 || isNight) {
                night_count++;
                // printf("night_count == %d\n", night_count);
                if (night_count >= 2) {
                    // printf("### entry night mode ###\n");
                  
                    ThresholdSwitchtoNight = true;
                    night_count = 0;
                }
            } else {
                night_count = 0;
            }

            bool ThresholdSwitchtoDay = false;
            // if (((int)iso_buf < 4900) && ((luma >= 22) && (luma_scence < 52000 && luma_scence != 0))) {
            if ((iso_buf < nd_config.RGB_ISOThreshold_NighttoDay) 
                && ((luma >= nd_config.RGB_LumaThreshold_NighttoDay))&& Awb_IsDay) {
                day_count++;
                // printf("day_count == %d\n", day_count);
                if (day_count >= 2) {
                    // LOG_INFO("### entry day mode ###\n");
                    // switchtoday();
                    ThresholdSwitchtoDay = true;
                    day_count = 0;
                }
            } else {
                day_count = 0;
            }

            /***********************阈值判断切换********************************* */
            if (ThresholdSwitchtoNight) {
                auto res = switchtonight();
                if (res == -1) {
                    return -1;
                } else if (res == 1) {
                    discard_after_count = nd_config.RGB_ISOJumpDiscardAfterSwitch_NighttoDay;
                    LOG_INFO("jump: %d , Threshold %d\n", is_day_to_night_jump, ThresholdSwitchtoNight);
                }
            } else if (ThresholdSwitchtoDay || is_night_to_day_jump) {
                auto res = switchtoday();
                if (res == -1) {
                    return -1;
                } else if (res == 1) {
                    discard_after_count = nd_config.RGB_ISOJumpDiscardAfterSwitch_NighttoDay;
                    LOG_INFO("jump: %d , Threshold %d\n", is_night_to_day_jump, ThresholdSwitchtoDay);
                }
            }

            IMPISPRunningMode NowMode;
            IMP_ISP_Tuning_GetISPRunningMode(vinum, &NowMode);
            if (NowMode == IMPISP_RUNNING_MODE_NIGHT) {
                if (video_active_ && (lastLedOnTime == 0 || GetCurTimeMs() - lastLedOnTime < LED_ON_MAX_DURATION)) {
                    if (turnOnLed(true)) {
                        lastLedOnTime = GetCurTimeMs();
                    }
                } else {
                    turnOnLed(false);
                    if (!video_active_) {
                        lastLedOnTime = 0;
                    }
                }
            }

            if(BootMode == 2 && this->BootJudgefinish == false)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200)); //夜晚模式，增加200ms等待AE稳定
                this->BootJudgefinish = true;
                this->uvc_init_cv_.notify_all();
            }else if(BootMode == 1 && this->BootJudgefinish == false)
            {
                this->BootJudgefinish = true;
                this->uvc_init_cv_.notify_all();
            }
        #ifdef KOKS_PROJECT_M53D
            }
            else if(g_normal_nightvision_mode == 1) {
                // 强制夜视
                switchtonight();
            }
            else if(g_normal_nightvision_mode == 2) {
                // 强制白天
                switchtoday();
            }
        #endif

            std::this_thread::sleep_for(std::chrono::milliseconds(65));
        }
    });
    isp_day_switch.detach();
#endif // KOKS_FEATURE_VIDEO
    initialized_.store(true);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Deinit
 * @brief: 反初始化
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::Deinit(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_System_Exit();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_System_Exit failed with %#x\n", ret);

    ret = IMP_ISP_DisableSensor(IMPVI_MAIN);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_DisableSensor 0 failed with %#x\n", ret);
    ret = IMP_ISP_DelSensor(IMPVI_MAIN, &sensor_info[0]);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_DelSensor 0 failed with %#x\n", ret);

    if (sensor_num == 2) {
        ret = IMP_ISP_DisableSensor(IMPVI_SEC);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_DisableSensor 1 failed with %#x\n", ret);
        ret = IMP_ISP_DelSensor(IMPVI_SEC, &sensor_info[1]);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_DelSensor 1 failed with %#x\n", ret);
    }

    ret = IMP_ISP_DisableTuning();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_DisableTuning failed with %#x\n", ret);

    ret = IMP_ISP_Close();
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Close failed with %#x\n", ret);
    GPIO_Deinit();
    LOG_DEBUG("Deinit success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetFaceAE
 * @brief: 设置人脸AE
 * @param {uint8_t} id 通道ID
 * @param {IMPISPFaceAttr} face_attr 人脸属性
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::SetFaceAE(uint8_t id, IMPISPFaceAttr& face_attr)
{
#ifdef AE_ONLY_MAIN_SENSOR
    if (id != IMPVI_MAIN)
        return ST_ERRNO_SUCCESS;
#endif
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPISPFaceAeWeiget _weight;
    IMPISPFaceAeLuma luma;
    const IMPISPFaceAeWeiget& weight = GetAEConfig(id).weight;
    memset(&luma, 0, sizeof(luma));
    memset(&_weight, 0, sizeof(_weight));
    ret = IMP_ISP_Tuning_SetFaceAe(static_cast<IMPVI_NUM>(id), &face_attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetFaceAe failed with %#x\n", ret);

    ret = IMP_ISP_Tuning_GetFaceAeWeiget(static_cast<IMPVI_NUM>(id), &_weight);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_GetFaceAeWeiget failed with %#x\n", ret);
    // LOG_INFO("sensor %d IMP_ISP_Tuning_GetFaceAeWeiget success,global_weight %d, roi_weight %d,smoot %d\n", id, _weight.global_weight, _weight.roi_weight, _weight.smooth);
    if (_weight.global_weight != weight.global_weight || _weight.roi_weight != weight.roi_weight || _weight.smooth != weight.smooth) {
        ret = IMP_ISP_Tuning_SetFaceAeWeiget(static_cast<IMPVI_NUM>(id), (IMPISPFaceAeWeiget*)&weight);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetFaceAeWeiget failed with %#x\n", ret);
        // LOG_INFO("sensor %d IMP_ISP_Tuning_SetFaceAeWeiget success,global_weight %d, roi_weight %d,smoot %d\n", id, weight.global_weight, weight.roi_weight, weight.smooth);
    }

    ret = IMP_ISP_Tuning_GetFaceAeLuma(static_cast<IMPVI_NUM>(id), &luma);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_GetFaceAeLuma failed with %#x\n", ret);
    // LOG_INFO("sensor %d IMP_ISP_Tuning_GetFaceAeLuma success, luma global %d, luma roi %d\n", id, luma.global, luma.roi);
    LOG_DEBUG("IMP_ISP_Tuning_SetFaceAe success\n");
}

/**
 * @name: GetFaceAE
 * @brief: 获取人脸AE
 * @param {uint8_t} id 通道ID
 * @param {IMPISPFaceAttr*} face_attr 人脸属性
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::GetFaceAE(uint8_t id, IMPISPFaceAttr* face_attr)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_ISP_Tuning_GetFaceAe(static_cast<IMPVI_NUM>(id), face_attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_GetFaceAe failed with %#x\n", ret);
    LOG_INFO("IMP_ISP_Tuning_GetFaceAe success\n");
}

ST_ERR Isp::setFaceAe(bool en)
{
    if (!initialized_.load()) {
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR ret;
    IMPISPFaceAttr attr;
    IMPISPTuningOpsMode enable = en ? IMPISP_TUNING_OPS_MODE_ENABLE : IMPISP_TUNING_OPS_MODE_DISABLE;
#ifdef AE_ONLY_MAIN_SENSOR
    uint8_t e = IMPVI_MAIN + 1;
#else
    uint8_t e = IMPVI_SEC + 1;
#endif
    for (uint8_t i = IMPVI_MAIN; i < e; i++) {
        IMP_ISP_Tuning_GetFaceAe(static_cast<IMPVI_NUM>(i), &attr);
        if (enable != attr.enable) {
            attr.enable = enable;
            ret = IMP_ISP_Tuning_SetFaceAe(static_cast<IMPVI_NUM>(i), &attr);
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetFaceAe failed with %#x\n", ret);
        }
    }
    return ST_ERRNO_SUCCESS;
}

ST_ERR Isp::CloseFaceAe(void)
{
    auto ret0 = CloseFaceAe(0);
    auto ret1 = CloseFaceAe(1);
    return (ret0 == ST_ERRNO_SUCCESS && ret1 == ST_ERRNO_SUCCESS) ? ST_ERRNO_SUCCESS : ST_ERRNO_FAIL;
}

ST_ERR Isp::CloseFaceAe(uint8_t id)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPISPFaceAttr face_attr;
    memset(&face_attr, 0, sizeof(face_attr));
    ret = GetFaceAE(id, &face_attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_GetFaceAe failed with %#x, id %u\n", ret, id);
    if (face_attr.enable == IMPISP_TUNING_OPS_MODE_ENABLE) {
        face_attr.enable = IMPISP_TUNING_OPS_MODE_DISABLE;
        ret = IMP_ISP_Tuning_SetFaceAe(static_cast<IMPVI_NUM>(id), &face_attr);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetFaceAe failed with %#x, id %u\n", ret, id);
        // LOG_DEBUG("CloseFaceAe %u done\n", id);
    }
    return ST_ERRNO_SUCCESS;
}

#ifdef KOKS_PROJECT_M24B3
void Isp::checkBrightness(uint8_t id)
{
    if (id < MAX_SENSOR_NUM) {
        IMPISPAeOnlyReadAttr aeOrAttr;
        auto ret = IMP_ISP_Tuning_GetAeOnlyReadAttr(static_cast<IMPVI_NUM>(id), &aeOrAttr);
        if (ret != 0) {
            LOG_ERROR("IMP_ISP_Tuning_GetAeOnlyReadAttr failed, id %u ret %#x\n", id, ret);
        }
        else {
            auto iso_buf = aeOrAttr.ExposureValue;
            auto luma = aeOrAttr.luma;
            auto luma_scence = aeOrAttr.luma_scence;
            bool bright = (iso_buf < 189642) && (luma >= 29) && (luma_scence < 1675479);
            LOG_DEBUG("%s, id %u, iso_buf %llu luma %u luma_scence %u, bright %d\n", __func__, id, iso_buf, luma, luma_scence, bright);
            sensor_bright[id] = bright;
        }
    }
}
#endif

/**
 * @name: SetSensorInfo
 * @brief: 设置传感器信息
 * @param {IMPSensorInfo&} info 传感器信息
 * @param {Json&} config 配置文件
 * @return {*} void
 */
void Isp::SetSensorInfo(IMPSensorInfo& info, FaceAeConfig& ae_config, const struct ISPConfig& config)
{
    auto camera_id = config.cam_id;
    ae_config.face_ae_target = static_cast<uint16_t>(config.face_ae_target);
#ifdef KOKS_SDK_VERSION_V2
    ae_config.palm_ae_target = static_cast<uint16_t>(config.palm_ae_target);
#endif
    ae_config.weight.global_weight = config.weight.global;
    ae_config.weight.roi_weight = config.weight.roi;
    ae_config.weight.smooth = config.weight.smooth;

    LOG_DEBUG("sensor[%d] name:%s,addr:%#x,i2c_adapter_id %d,framerate:%d rst gpio %d\n", camera_id, info.name, info.i2c.addr, info.i2c.i2c_adapter_id, info.fps.num,
              info.rst_gpio);
}

ST_ERR Isp::SetSensorFrameRate(int frame_rate)
{
    if (sensor_frame_rate_ != frame_rate) {
        LOG_INFO("Set sensor frame rate %d\n", frame_rate);
        IMPISPSensorFps setFps = { (uint32_t)frame_rate, 1 };
        for (int i = 0; i < MAX_SENSOR_NUM; ++i) {
            auto ret = IMP_ISP_Tuning_SetSensorFPS((IMPVI_NUM)i, &setFps);
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ISP_Tuning_SetSensorFPS failed with %#x\n, id %d", ret, i);
        }
        sensor_frame_rate_ = frame_rate;
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GPIO_Init
 * @brief: 初始化GPIO
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::GPIO_Init(void)
{
#ifdef KOKS_FEATURE_CUSTOM_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/export", CUSTOM_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
    snprintf(command, sizeof(command), "echo out > /sys/class/gpio/gpio%d/direction", CUSTOM_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
    snprintf(command, sizeof(command), "echo 0 > /sys/class/gpio/gpio%d/active_low", CUSTOM_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GPIO_Deinit
 * @brief: 反初始化GPIO
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::GPIO_Deinit(void)
{
#ifdef KOKS_FEATURE_CUSTOM_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/unexport", CUSTOM_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GPIO_Set
 * @brief: 设置GPIO电平
 * @param {gpio} value 电平值
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR Isp::GPIO_Set(int value)
{
#ifdef KOKS_FEATURE_CUSTOM_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/gpio%d/value", value, CUSTOM_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

void Isp::setVideoStatus(bool active)
{
    video_active_ = true;
}

} // namespace sense_koks

} // namespace st
