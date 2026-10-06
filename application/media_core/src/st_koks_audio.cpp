/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-25 17:27:47
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:33:51
 * @FilePath: /sense-koks/application/media_core/src/st_koks_audio.cpp
 */
#include "st_koks_audio.h"
#include "koks_project_def.h"
#include "audio_customer_encoder.h"
#include "os_utils.h"
#include "st_koks_mediamanager.h"
#include "sys_logger.h"
#include <chrono>
#ifdef __cplusplus
extern "C" {
#endif
#include "usbcamera.h"
#ifdef __cplusplus
}
#endif
namespace st
{
namespace sense_koks
{
/**
 * @name: DeviceAudioInput
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
DeviceAudioInput::DeviceAudioInput(int32_t dev_id) : Device(dev_id)
{
    memset(&attr, 0, sizeof(attr));
}

/**
 * @name: DeviceAudioInput
 * @brief: destructor
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
DeviceAudioInput::~DeviceAudioInput()
{
}

/**
 * @name: Init
 * @brief: 音频输入初始化
 * @param {Json&} config 配置文件
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceAudioInput::Init(const struct AudioInputConfig& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;

    dev_id = config.dev_id;
    AudioInputconfig = config;
    attr.samplerate = (IMPAudioSampleRate)KOKS_FEATURE_AUDIO_SAMPLE_RATE;
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 2;
#ifdef KOKS_PROJECT_M53D
    attr.numPerFrm = KOKS_FEATURE_AUDIO_SAMPLE_RATE * 10 / 1000;
#else
    attr.numPerFrm = KOKS_FEATURE_AUDIO_SAMPLE_RATE * 40 / 1000;
#endif
    attr.chnCnt = 1;
    ret = IMP_AI_SetPubAttr(dev_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetPubAttr failed with %#x\n", ret);

    ret = IMP_AI_Enable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_Enable failed with %#x\n", ret);

    IMPAudioIChnParam chnParam;
    chnParam.usrFrmDepth = 2;
    chn_id = config.channels;
    chnParam.aecChn = static_cast<IMPAudioAecChn>(chn_id);
    ret = IMP_AI_SetChnParam(dev_id, chn_id, &chnParam);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetChnParam failed with %#x\n", ret);
  
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 音频输入启动
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceAudioInput::Start()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_EnableChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableChn failed with %#x\n", ret);

    // ret = SetHs(AudioInputconfig.howling);
    // MISC_CHK_STATUS_REPORT(ret, ret, "SetHs failed with %#x\n", ret);

    AudioSetAec(true);

    ret = SetVol(AudioInputconfig.volume);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetVol failed with %#x\n", ret);
    ret = SetAnalogGain(AudioInputconfig.analog_gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetAnalogGain failed with %#x\n", ret);

    // ret = SetAns(AudioInputconfig.ans.enable, attr, static_cast<Level_ns>(AudioInputconfig.ans.level));
    // MISC_CHK_STATUS_REPORT(ret, ret, "SetAns failed with %#x\n", ret);
    int gain = 0;
    ret = GetAnalogGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetAnalogGain failed with %#x\n", ret);
    LOG_DEBUG("GetAnalogGain %d success!\n", gain);
    // ret = GetDigitalGain(gain);
    // MISC_CHK_STATUS_REPORT(ret, ret, "GetDigitalGain failed with %#x\n", ret);
    // LOG_DEBUG("GetDigitalGain %d success!\n", gain);
    LOG_DEBUG("DeviceAudioInput %d Init success!\n", dev_id);
    
    LOG_DEBUG("Start success!\n");
    on_off = true;
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 音频输入停止
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceAudioInput::Stop()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    AudioSetAec(false);
    ret = IMP_AI_DisableChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_DisableChn failed with %#x\n", ret);
    LOG_DEBUG("Stop success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 音频输入反初始化
 * @return {*}
 */
ST_ERR DeviceAudioInput::DeInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_Disable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_Disable failed with %#x\n", ret);
    LOG_DEBUG("DeInit success!\n");
    on_off = false; // 设置音频输入状态为关闭
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReInit
 * @brief: 音频输入重新初始化
 * @return {*}
 */
ST_ERR DeviceAudioInput::ReInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_SetPubAttr(dev_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetPubAttr failed with %#x\n", ret);

    ret = IMP_AI_Enable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_Enable failed with %#x\n", ret);

    IMPAudioIChnParam chnParam;
    chnParam.usrFrmDepth = 2;
    chnParam.aecChn = static_cast<IMPAudioAecChn>(chn_id);
    ret = IMP_AI_SetChnParam(dev_id, chn_id, &chnParam);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetChnParam failed with %#x\n", ret);
    ret = SetVol(volume);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetVol failed with %#x\n", ret);
    ret = SetHs(howling_enable);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetHs failed with %#x\n", ret);
    ret = SetAns(ans_enable, attr, static_cast<Level_ns>(ans_level));
    MISC_CHK_STATUS_REPORT(ret, ret, "SetAns failed with %#x\n", ret);
    ret = SetAnalogGain(AnalogGain);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetAnalogGain failed with %#x\n", ret);
    int gain = 0;
    ret = GetAnalogGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetAnalogGain failed with %#x\n", ret);
    LOG_DEBUG("GetAnalogGain %d success!\n", gain);
    ret = GetDigitalGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetDigitalGain failed with %#x\n", ret);
    LOG_DEBUG("GetDigitalGain %d success!\n", gain);
    LOG_DEBUG("DeviceAudioInput %d Init success!\n", dev_id);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetVol
 * @brief: 设置音量
 * @param {int} vol 音量
 * @return {*}
 */
ST_ERR DeviceAudioInput::SetVol(int vol)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (vol < 0)
        vol = 0;
    if (vol > 100)
        vol = 100;
    // Map [0, 100] to [-30, 120]
    vol = (vol * 1.5) - 30;
    MISC_CHK_CONDITION_REPORT(vol >= -30 && vol <= 120, ST_ERRNO_FAIL, "vol value out of range\n");
    if (vol == -30) {
        ret = IMP_AI_SetVolMute(dev_id, chn_id, 1);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetVolMute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AI_SetVolMute success!\n");
    } else {
        ret = IMP_AI_SetVolMute(dev_id, chn_id, 0);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetVolMute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AI_SetVolMute success!\n");
    }
    ret = IMP_AI_SetVol(dev_id, chn_id, vol);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetVol failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetVol
 * @brief: 获取音量
 * @param {int&} vol 音量
 * @return {*}
 */
ST_ERR DeviceAudioInput::GetVol(int& vol)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_GetVol(dev_id, chn_id, &vol);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_GetVol failed with %#x\n", ret);
    vol = (2 * vol + 60) / 3;
    LOG_DEBUG("GetVol %d success!\n", vol);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetFrame
 * @brief: 获取原始音频数据
 * @param {IMPAudioFrame&} frame 音频帧
 * @return {*}
 */
ST_ERR DeviceAudioInput::GetFrame(IMPAudioFrame& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_PollingFrame(dev_id, chn_id, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_PollingFrame failed with %#x\n", ret);
    ret = IMP_AI_GetFrame(dev_id, chn_id, &frame, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_GetFrame failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReleaseFrame
 * @brief: 释放音频帧
 * @param {IMPAudioFrame&} frame 音频帧
 * @return {*}
 */
ST_ERR DeviceAudioInput::ReleaseFrame(IMPAudioFrame& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AI_ReleaseFrame(dev_id, chn_id, &frame);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_ReleaseFrame failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetAns
 * @brief：设置ANS
 * @param {bool} enable
 * @param {IMPAudioIOAttr&} attr
 * @param {enum Level_ns} lv
 * @return {*}
 */
ST_ERR DeviceAudioInput::SetAns(bool enable, IMPAudioIOAttr& attr, enum Level_ns lv)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (enable == true) {
        ret = IMP_AI_EnableNs(&attr, lv);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableNs failed with %#x\n", ret);
    } else {
        ret = IMP_AI_DisableNs();
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_DisableNs failed with %#x\n", ret);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeviceAudioOutput
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*}
 */
DeviceAudioOutput::DeviceAudioOutput(int32_t dev_id) : Device(dev_id)
{
}

/**
 * @name: DeviceAudioOutput
 * @brief: destructor
 * @return {*}
 */
DeviceAudioOutput::~DeviceAudioOutput()
{
}

/**
 * @name: Init
 * @brief: 音频输出初始化
 * @param {Json&} config 配置文件
 * @return {*}
 */

ST_ERR DeviceAudioOutput::Init(const struct AudioOutputConfig& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = GPIO_Init();
    MISC_CHK_STATUS_REPORT(ret, ret, "GPIO_Init failed with %#x\n", ret);
    config_ = config;
    dev_id = config.dev_id;
    
    memset(&attr, 0, sizeof(attr));
    attr.samplerate = static_cast<IMPAudioSampleRate>(config.sample_rate);
    attr.bitwidth = AUDIO_BIT_WIDTH_16;
    attr.soundmode = AUDIO_SOUND_MODE_MONO;
    attr.frmNum = 2;
    attr.numPerFrm = attr.samplerate / 1000 * 40;
    attr.chnCnt = 1;
    ret = IMP_AO_SetPubAttr(dev_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetPubAttr failed with %#x\n", ret);
    ret = IMP_AO_Enable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_Enable failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

void DeviceAudioOutput::SetSampleRate(int sample_rate)
{
    attr.samplerate = static_cast<IMPAudioSampleRate>(sample_rate);
    attr.numPerFrm = attr.samplerate / 1000 * 40;
}

/**
 * @name: SendFrame
 * @brief: 发送音频帧
 * @param {IMPAudioFrame} frame 音频帧
 * @return {*}
 */
void* DeviceAudioOutput::AudioOutProcess(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    struct Ucamera_Audio_Frame u_frame;
    while (sp_audio_out_thread.first == true) {
        ret = IF_Ucam_Audio_Get_Frame(&u_frame);
        if (ret || u_frame.len <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        IMPAudioFrame frm;
        frm.virAddr = (uint32_t*)u_frame.data;
        frm.len = u_frame.len;
        ret = SendFrame(frm);
        if (ret != 0) {
            LOG_ERROR("SendFrame failed with %#x\n", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        IF_Ucam_Audio_Release_Frame(&u_frame);
    }
    return nullptr;
}

/**
 * @name: Start
 * @brief: 音频输出启动
 * @return {*}
 */
ST_ERR DeviceAudioOutput::Start()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_EnableChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_EnableChn failed with %#x\n", ret);

    ret = SetVol(config_.volume);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetVol failed with %#x\n", ret);
    ret = SetAnalogGain(config_.analog_gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetAnalogGain failed with %#x\n", ret);
    int gain = 0;
    ret = GetAnalogGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetAnalogGain failed with %#x\n", ret);
    LOG_DEBUG("GetAnalogGain %d success!\n", gain);

	ret = IMP_AO_EnableAlgo(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_EnableAlgo failed with %#x\n", ret);

    sp_audio_out_thread.first = true;
    sp_audio_out_thread.second = std::make_shared<std::thread>(&DeviceAudioOutput::AudioOutProcess, this);
    LOG_DEBUG("Start success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 音频输出停止
 * @return {*}
 */
ST_ERR DeviceAudioOutput::Stop()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    sp_audio_out_thread.first = false;
    sp_audio_out_thread.second->join();
    sp_audio_out_thread.second.reset();
    ret = IMP_AO_DisableAlgo(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_DisableAlgo failed with %#x\n", ret);
    ret = IMP_AO_DisableChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_DisableChn failed with %#x\n", ret);
    LOG_DEBUG("Stop success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Resume
 * @brief: 音频输出恢复
 * @return {*}
 */
ST_ERR DeviceAudioOutput::Resume()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_ResumeChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_ResumeChn failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Pause
 * @brief: 音频输出暂停
 * @return {*}
 */
ST_ERR DeviceAudioOutput::Pause()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_PauseChn(dev_id, chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_PauseChn failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 音频输出反初始化
 * @return {*}
 */
ST_ERR DeviceAudioOutput::DeInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_Disable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_Disable failed with %#x\n", ret);
    GPIO_Deinit();
    LOG_DEBUG("DeInit success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReInit
 * @brief: 音频输出重新初始化
 * @return {*}
 */
ST_ERR DeviceAudioOutput::ReInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_SetPubAttr(dev_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetPubAttr failed with %#x\n", ret);
    ret = IMP_AO_Enable(dev_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_Enable failed with %#x\n", ret);

    ret = SetVol(volume);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetVol failed with %#x\n", ret);

    ret = SetAnalogGain(analog_gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "SetAnalogGain failed with %#x\n", ret);
    int gain = 0;
    ret = GetAnalogGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetAnalogGain failed with %#x\n", ret);
    LOG_DEBUG("GetAnalogGain %d success!\n", gain);
    ret = GetDigitalGain(gain);
    MISC_CHK_STATUS_REPORT(ret, ret, "GetDigitalGain failed with %#x\n", ret);
    LOG_DEBUG("GetDigitalGain %d success!\n", gain);
    LOG_DEBUG("DeviceAudioOutput %d Init success!\n", dev_id);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetVol
 * @brief: 设置音量
 * @param {int} vol 音量
 * @return {*}
 */
ST_ERR DeviceAudioOutput::SetVol(int vol)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (vol < 0)
        vol = 0;
    if (vol > 100)
        vol = 100;
    // Map [0, 100] to [-30, 120]
    vol = (vol * 1.5) - 30;
    MISC_CHK_CONDITION_REPORT(vol >= -30 && vol <= 120, ST_ERRNO_FAIL, "vol value out of range\n");
    if (vol == -30) {
        ret = IMP_AO_SetVolMute(dev_id, chn_id, 1);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetVolMute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AO_SetVolMute success!\n");
    } else {
        ret = IMP_AO_SetVolMute(dev_id, chn_id, 0);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetVolMute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AO_SetVolMute success!\n");
    }
    ret = IMP_AO_SetVol(dev_id, chn_id, vol);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetVol failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetVol
 * @brief: 获取音量
 * @param {int&} vol 音量
 * @return {*}
 */
ST_ERR DeviceAudioOutput::GetVol(int& vol)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_GetVol(dev_id, chn_id, &vol);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_GetVol failed with %#x\n", ret);
    vol = (2 * vol + 60) / 3;
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetSoftMute
 * @brief: 设置软静音
 * @param {bool} mute 静音标志
 * @return {*}
 */
ST_ERR DeviceAudioOutput::SetSoftMute(bool mute)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (mute == true) {
        ret = IMP_AO_Soft_Mute(dev_id, chn_id);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_Soft_Mute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AO_Soft_Mute success!\n");
    } else {
        ret = IMP_AO_Soft_UNMute(dev_id, chn_id);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_Soft_UNMute failed with %#x\n", ret);
        LOG_DEBUG("IMP_AO_Soft_UNMute success!\n");
    }
    LOG_DEBUG("SetSoftMute %d success!\n", mute);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SendFrame
 * @brief: 发送音频帧
 * @param {IMPAudioFrame&} frame 音频帧
 * @return {*}
 */
ST_ERR DeviceAudioOutput::SendFrame(IMPAudioFrame& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AO_SendFrame(dev_id, chn_id, &frame, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SendFrame failed with %#x\n", ret);
    IMPAudioOChnState play_status;
    ret = IMP_AO_QueryChnStat(dev_id, chn_id, &play_status);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_QueryChnStat failed with %#x\n", ret);
    LOG_VERBOSE("IMP_AO_QueryChnStat chnTotalNum %d, chnFreeNum %d, chnBusyNum %d\n", play_status.chnTotalNum, play_status.chnFreeNum, play_status.chnBusyNum);
    // ret = IMP_AO_ClearChnBuf(dev_id, chn_id);
    // MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_ClearChnBuf failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceAudioOutput::GPIO_Init(void)
{
#ifdef KOKS_FEATURE_SPK_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/export", SPK_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
    snprintf(command, sizeof(command), "echo out > /sys/class/gpio/gpio%d/direction", SPK_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
    snprintf(command, sizeof(command), "echo 0 > /sys/class/gpio/gpio%d/active_low", SPK_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceAudioOutput::GPIO_Deinit(void)
{
#ifdef KOKS_FEATURE_SPK_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/unexport", SPK_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceAudioOutput::GPIO_Set(int value)
{
#ifdef KOKS_FEATURE_SPK_GPIO
    char command[256] = { 0 };
    snprintf(command, sizeof(command), "echo %d > /sys/class/gpio/gpio%d/value", value, SPK_GPIO);
    if (utils_shell(command, nullptr, 0) != 0) {
        LOG_ERROR("failed\n");
        return -1;
    }
#endif
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceAudioOutput::SetSpkEnable(bool enable)
{
    if(enable) {
        return GPIO_Set(1);
    } else {
        return GPIO_Set(0);
    }
}

#ifdef AUDIO_CODEC
/**
 * @name: DeviceAudioEncode
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*}
 */
DeviceAudioEncode::DeviceAudioEncode(int32_t dev_id) : Device(dev_id)
{
}

/**
 * @name: DeviceAudioEncode
 * @brief: destructor
 * @return {*}
 */
DeviceAudioEncode::~DeviceAudioEncode()
{
}

/**
 * @name: Init
 * @brief: 音频编码初始化
 * @param {Json&} config 配置文件
 * @return {*}
 */
ST_ERR DeviceAudioEncode::Init(const json11::Json& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    memset(&attr, 0, sizeof(attr));
    dev_id = config["dev"].int_value();
    if (config["customer_encoder"]["enable"].bool_value() == true) {
        customer_encoder = true;
        memset(&encoder, 0, sizeof(encoder));
        encoder.maxFrmLen = 1024;
        sprintf(encoder.name, "%s", config["customer_encoder"]["name"].string_value().c_str());
        encoder.openEncoder = nullptr;
        encoder.encoderFrm = AudioUtils::G711A_Encode_Frm;
        encoder.closeEncoder = nullptr;
        ret = IMP_AENC_RegisterEncoder(&enc_handle, &encoder);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_RegisterEncoder failed with %#x\n", ret);
        LOG_DEBUG("IMP_AENC_RegisterEncoder enc_handle %d success!\n", enc_handle);
        type = static_cast<IMPAudioPalyloadType>(enc_handle);
    }
    attr.type = type;
    attr.bufSize = 20;
    ret = IMP_AENC_CreateChn(chn_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_CreateChn failed with %#x\n", ret);
    LOG_DEBUG("DeviceAudioEncode %d Init success!\n", dev_id);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 音频编码启动
 * @return {*}
 */
ST_ERR DeviceAudioEncode::Start()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    sp_audio_thread.first = true;
    sp_audio_thread.second = std::make_shared<std::thread>(&DeviceAudioEncode::AudioEncProcessStream, this);
    LOG_DEBUG("Start success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 音频编码停止
 * @return {*}
 */
ST_ERR DeviceAudioEncode::Stop()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    sp_audio_thread.first = false;
    sp_audio_thread.second->join();
    sp_audio_thread.second.reset();
    LOG_DEBUG("Stop success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 音频编码反初始化
 * @return {*}
 */
ST_ERR DeviceAudioEncode::DeInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AENC_DestroyChn(chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_DestroyChn failed with %#x\n", ret);
    if (customer_encoder == true) {
        ret = IMP_AENC_UnRegisterEncoder(&enc_handle);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_UnRegisterEncoder failed with %#x\n", ret);
    }
    LOG_DEBUG("DeInit success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetStream
 * @brief: 获取音频流
 * @param {IMPAudioStream&} stream 音频流
 * @return {*}
 */
ST_ERR DeviceAudioEncode::GetStream(IMPAudioStream& stream)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AENC_PollingStream(chn_id, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_PollingStream failed with %#x\n", ret);
    ret = IMP_AENC_GetStream(chn_id, &stream, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_GetStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SendGetStream
 * @brief: 发送并获取音频流
 * @param {IMPAudioFrame&} frame 音频帧
 * @param {IMPAudioStream&} stream 音频流
 * @return {*}
 */
ST_ERR DeviceAudioEncode::SendGetStream(IMPAudioFrame& frame, IMPAudioStream& stream)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AENC_SendFrame(chn_id, &frame);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_SendFrame failed with %#x\n", ret);
    ret = IMP_AENC_PollingStream(chn_id, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_PollingStream failed with %#x\n", ret);
    ret = IMP_AENC_GetStream(chn_id, &stream, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_GetStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReleaseStream
 * @brief: 释放音频流
 * @param {IMPAudioStream&} stream 音频流
 * @return {*}
 */
ST_ERR DeviceAudioEncode::ReleaseStream(IMPAudioStream& stream)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_AENC_ReleaseStream(chn_id, &stream);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AENC_ReleaseStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: AudioEncProcessStream
 * @brief: 音频编码处理流
 * @return {*}
 */
void* DeviceAudioEncode::AudioEncProcessStream(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto* audio_input = MediaManager::GetInstance().GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
    while (sp_audio_thread.first == true) {
        SpInputAudioFrameInfo sp_in_frame(new IMPAudioFrame, [=](IMPAudioFrame* stream) -> void {
            ST_ERR ret = audio_input->ReleaseFrame(*stream);
            delete stream;
            MISC_CHK_STATUS_REPORT_NO_RET(ret, "ReleaseFrame failed with %#x\n", ret);
            LOG_DEBUG("ReleaseStream success!\n");
        });

        ret = audio_input->GetFrame(*sp_in_frame);
        MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, continue, "GetStream failed with %#x\n", ret);

        ret = IMP_AENC_SendFrame(chn_id, sp_in_frame.get());
        MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, continue, "chn %d IMP_AENC_SendFrame failed with %#x\n", chn_id, ret);

        IMPAudioStream* _stream = new IMPAudioStream;
        ret = GetStream(*_stream);
        MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, delete _stream; continue, "chn %d GetStream failed with %#x\n", chn_id, ret);

        SpCodecsAudioStreamInfo sp_enc_stream(_stream, [=](IMPAudioStream* stream) -> void {
            ST_ERR ret = ReleaseStream(*stream);
            delete stream;
            MISC_CHK_STATUS_REPORT_NO_RET(ret, "ReleaseStream failed with %#x\n", ret);
            LOG_DEBUG("ReleaseStream success!\n");
        });

        if (enc_frame_proc_cb != nullptr) {
            enc_frame_proc_cb(sp_enc_stream);
        }
        LOG_DEBUG("IMP_AENC_GetStream success!\n");
    }
    return nullptr;
}

/**
 * @name: DeviceAudioDecode
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*}
 */
DeviceAudioDecode::DeviceAudioDecode(int32_t dev_id) : Device(dev_id)
{
}

/**
 * @name: DeviceAudioDecode
 * @brief: destructor
 * @return {*}
 */
DeviceAudioDecode::~DeviceAudioDecode()
{
}

/**
 * @name: Init
 * @brief: 音频解码初始化
 * @param {Json&} config 配置文件
 * @return {*}
 */
ST_ERR DeviceAudioDecode::Init(const json11::Json& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    memset(&attr, 0, sizeof(attr));
    dev_id = config["dev"].int_value();
    if (config["customer_decoder"]["enable"].bool_value() == true) {
        customer_decoder = true;
        memset(&decoder, 0, sizeof(decoder));
        sprintf(decoder.name, "%s", config["customer_decoder"]["name"].string_value().c_str());
        decoder.openDecoder = nullptr;
        decoder.decodeFrm = AudioUtils::G711A_Decode_Frm;
        decoder.getFrmInfo = nullptr;
        decoder.closeDecoder = nullptr;
        ret = IMP_ADEC_RegisterDecoder(&dec_handle, &decoder);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_RegisterDecoder failed with %#x\n", ret);
        LOG_DEBUG("IMP_ADEC_RegisterDecoder dec_handle %d success!\n", dec_handle);
        type = static_cast<IMPAudioPalyloadType>(dec_handle);
    }
    attr.type = type;
    attr.bufSize = 20;
    attr.mode = ADEC_MODE_PACK;
    ret = IMP_ADEC_CreateChn(chn_id, &attr);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_CreateChn failed with %#x\n", ret);
    ret - IMP_ADEC_ClearChnBuf(chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_ClearChnBuf failed with %#x\n", ret);

    LOG_DEBUG("DeviceAudioDecode %d Init success!\n", dev_id);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 音频解码启动
 * @return {*}
 */
ST_ERR DeviceAudioDecode::Start()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    sp_audio_dec_thread.first = true;
    sp_audio_dec_thread.second = std::make_shared<std::thread>(&DeviceAudioDecode::AudioDecProcessStream, this);
    LOG_DEBUG("Start success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 音频解码停止
 * @return {*}
 */
ST_ERR DeviceAudioDecode::Stop()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    sp_audio_dec_thread.first = false;
    sp_audio_dec_thread.second->join();
    sp_audio_dec_thread.second.reset();
    LOG_DEBUG("Stop success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 音频解码反初始化
 * @return {*}
 */
ST_ERR DeviceAudioDecode::DeInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_ADEC_DestroyChn(chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_DestroyChn failed with %#x\n", ret);
    if (customer_decoder == true) {
        ret = IMP_ADEC_UnRegisterDecoder(&dec_handle);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_UnRegisterDecoder failed with %#x\n", ret);
    }
    LOG_DEBUG("DeInit success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetStream
 * @brief: 获取音频流
 * @param {IMPAudioStream&} stream 音频流
 * @return {*}
 */
ST_ERR DeviceAudioDecode::GetStream(IMPAudioStream& stream)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_ADEC_PollingStream(chn_id, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_PollingStream failed with %#x\n", ret);
    ret = IMP_ADEC_GetStream(chn_id, &stream, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_GetStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SendGetStream
 * @brief: 发送并获取音频流
 * @param {IMPAudioStream&} in 输入音频流
 * @param {IMPAudioStream&} out 输出音频流
 * @return {*}
 */
ST_ERR DeviceAudioDecode::SendGetStream(IMPAudioStream& in, IMPAudioStream& out)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_ADEC_SendStream(chn_id, &in, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_SendStream failed with %#x\n", ret);
    ret = IMP_ADEC_PollingStream(chn_id, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_PollingStream failed with %#x\n", ret);
    ret = IMP_ADEC_GetStream(chn_id, &out, BLOCK);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_GetStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReleaseStream
 * @brief: 释放音频流
 * @param {IMPAudioStream&} stream 音频流
 * @return {*}
 */
ST_ERR DeviceAudioDecode::ReleaseStream(IMPAudioStream& stream)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_ADEC_ReleaseStream(chn_id, &stream);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_ADEC_ReleaseStream failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: AudioDecProcessStream
 * @brief: 音频解码处理流
 * @return {*}
 */
void* DeviceAudioDecode::AudioDecProcessStream(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPAudioFrame frm;
    memset(&frm, 0, sizeof(frm));
    auto* audio_output = MediaManager::GetInstance().GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);
    while (sp_audio_dec_thread.first == true) {
        if (dec_frame_get_cb != nullptr) {
            IMPAudioStream* _stream = new IMPAudioStream;
            memset(_stream, 0, sizeof(IMPAudioStream));
            SpCodecsAudioStreamInfo sp_dec_stream(_stream, [this](IMPAudioStream* stream) -> void {
                if (stream->stream != nullptr) {
                    ST_ERR ret = ReleaseStream(*stream);
                    MISC_CHK_STATUS_REPORT_NO_RET(ret, "ReleaseStream failed with %#x\n", ret);
                }
                delete stream;
                LOG_VERBOSE("ReleaseStream success!\n");
            });

            auto frame = dec_frame_get_cb();
            MISC_CHK_UN_NULL_REPORT_EXEC_NO_RET(frame, continue, "frame is nullptr\n");
            ret = SendGetStream(*frame, *sp_dec_stream);
            MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, continue, "SendGetStream failed with %#x\n", ret);
            LOG_VERBOSE("SendGetStream seq %d, ts %lld success!\n", sp_dec_stream->seq, sp_dec_stream->timeStamp);

            frm.virAddr = reinterpret_cast<uint32_t*>(sp_dec_stream->stream);
            frm.len = sp_dec_stream->len;
            frm.timeStamp = sp_dec_stream->timeStamp; // GetCurTimeUs();
            ret = audio_output->SendFrame(frm);
            MISC_CHK_STATUS_REPORT_NO_RET(ret, "SendFrame failed with %#x\n", ret);
        }
    }
    return nullptr;
}
#endif
} // namespace sense_koks

} // namespace st