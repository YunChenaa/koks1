/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-25 17:27:55
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:32:09
 * @FilePath: /sense-koks/application/media_core/include/st_koks_audio.h
 */
#pragma once
#include "imp_audio.h"
#include "json11.hpp"
#include "koks1_config.h"
#include "st_koks_errno.h"
#include "st_koks_video.h"
#include "sys_logger.h"

#define GPIO_PB(n) (1 * 32 + (n))
#define SPK_GPIO GPIO_PB(31)

namespace st
{
namespace sense_koks
{
using EncoderFrameCb = std::function<int(void* encoder, IMPAudioFrame* data, unsigned char* outbuf, int* outLen)>;
typedef std::shared_ptr<IMPAudioFrame> SpInputAudioFrameInfo;
typedef std::shared_ptr<IMPAudioStream> SpCodecsAudioStreamInfo;
using AudioEncFrameProcCallback = std::function<ST_ERR(SpCodecsAudioStreamInfo& frame_info)>;
using AudioDecFrameProcCallback = std::function<SpCodecsAudioStreamInfo(void)>;
namespace Audio
{
inline ST_ERR AudioSetAec(bool enable)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (enable == true) {
        ret = IMP_AI_EnableAec(0, 0, 0, 0);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableAec failed with %#x\n", ret);
    } else {
        ret = IMP_AI_DisableAec(0, 0);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableAec failed with %#x\n", ret);
    }
    return ST_ERRNO_SUCCESS;
}
} // namespace Audio
class DeviceAudioInput final : public Device
{
    public:
    explicit DeviceAudioInput(int32_t dev_id = 0);
    ~DeviceAudioInput();

    ST_ERR Init(void) override
    {
    }
    ST_ERR Init(const struct AudioInputConfig& config);
    ST_ERR Start();
    ST_ERR Stop();
    ST_ERR DeInit();
    ST_ERR ReInit();
    ST_ERR SetVol(int vol);
    ST_ERR GetVol(int& vol);
    ST_ERR SetAnalogGain(int gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        MISC_CHK_CONDITION_REPORT(gain >= 0 && gain <= 31, ST_ERRNO_FAIL, "gain value out of range\n");
        ret = IMP_AI_SetGain(dev_id, chn_id, gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR GetAnalogGain(int& gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_AI_GetGain(dev_id, chn_id, &gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_GetGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR SetDigitalGain(int gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        MISC_CHK_CONDITION_REPORT(gain >= 0 && gain <= 255, ST_ERRNO_FAIL, "digital gain value out of range\n");
        ret = IMP_AI_SetDigitalGain(dev_id, chn_id, gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_SetDigitalGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR GetDigitalGain(int& gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_AI_GetDigitalGain(dev_id, chn_id, &gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_GetDigitalGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR SetHs(bool enable)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        if (enable == true) {
            ret = IMP_AI_EnableHs();
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableHs failed with %#x\n", ret);
        } else {
            ret = IMP_AI_DisableHs();
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_DisableHs failed with %#x\n", ret);
        }
        return ST_ERRNO_SUCCESS;
    }
    bool GetAeC(void) const
    {
        return enable_aec;
    }
    bool Getstaus(void) const
    {
        return on_off;
    }
    ST_ERR AudioSetAec(bool enable)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        if (enable == true) {
            ret = IMP_AI_EnableAec(dev_id, chn_id, 0, 0);
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_EnableAec failed with %#x\n", ret);
        } else {
            ret = IMP_AI_DisableAec(dev_id, chn_id);
            MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AI_DisableAec failed with %#x\n", ret);
        }
        enable_aec = true;
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR SetAns(bool enable, IMPAudioIOAttr& attr, enum Level_ns lv);
    ST_ERR GetFrame(IMPAudioFrame& frame);
    ST_ERR ReleaseFrame(IMPAudioFrame& frame);

    private:
    AudioInputConfig AudioInputconfig;
    IMPAudioIOAttr attr;
    bool enable_aec{ false };
    int dev_id;
    int volume{ 0 };
    bool howling_enable{ false };
    bool ans_enable{ false };
    int ans_level{ 0 };
    int AnalogGain{ 0 };
    int chn_id{ 0 };
    bool on_off{ false }; // 是否开启音频输入
};

class DeviceAudioOutput final : public Device
{
    public:
    explicit DeviceAudioOutput(int32_t dev_id = 0);
    ~DeviceAudioOutput();

    ST_ERR Init(void) override
    {
    }
    ST_ERR Init(const struct AudioOutputConfig& config);
    ST_ERR Start();
    ST_ERR Stop();
    ST_ERR Resume();
    ST_ERR Pause();
    ST_ERR DeInit();
    ST_ERR ReInit();
    ST_ERR SetVol(int vol);
    ST_ERR GetVol(int& vol);
    ST_ERR SetSoftMute(bool mute);
    ST_ERR SetSpkEnable(bool enable);

    ST_ERR SetAnalogGain(int gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        MISC_CHK_CONDITION_REPORT(gain >= 0 && gain <= 31, ST_ERRNO_FAIL, "gain value out of range\n");
        ret = IMP_AO_SetGain(dev_id, chn_id, gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR GetAnalogGain(int& gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_AO_GetGain(dev_id, chn_id, &gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_GetGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR SetDigitalGain(int gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        MISC_CHK_CONDITION_REPORT(gain >= 0 && gain <= 255, ST_ERRNO_FAIL, "digital gain value out of range\n");
        ret = IMP_AO_SetDigitalGain(dev_id, chn_id, gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_SetDigitalGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }
    ST_ERR GetDigitalGain(int& gain)
    {
        ST_ERR ret = ST_ERRNO_SUCCESS;
        ret = IMP_AO_GetDigitalGain(dev_id, chn_id, &gain);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_AO_GetDigitalGain failed with %#x\n", ret);
        return ST_ERRNO_SUCCESS;
    }

    void SetSampleRate(int sample_rate);
    ST_ERR SendFrame(IMPAudioFrame& frame);
    void* AudioOutProcess(void);
    std::pair<bool, std::shared_ptr<std::thread>> sp_audio_out_thread{ false, nullptr };

    private:
    ST_ERR GPIO_Init(void);
    ST_ERR GPIO_Deinit(void);
    ST_ERR GPIO_Set(int value);

    private:
    AudioOutputConfig config_;
    int dev_id;
    int chn_id{ 0 };
    int volume{ 0 };
    int analog_gain{ 0 };
    IMPAudioIOAttr attr;
};
#ifdef AUDIO_CODEC
class DeviceAudioEncode final : public Device
{
    public:
    explicit DeviceAudioEncode(int32_t dev_id = 0);
    ~DeviceAudioEncode();

    ST_ERR Init(void) override
    {
    }
    ST_ERR Init(const json11::Json& config);
    ST_ERR Start();
    ST_ERR Stop();
    ST_ERR DeInit();
    ST_ERR SendGetStream(IMPAudioFrame& frame, IMPAudioStream& stream);
    ST_ERR GetStream(IMPAudioStream& stream);
    ST_ERR ReleaseStream(IMPAudioStream& stream);
    ST_ERR SetFrameProcCb(AudioEncFrameProcCallback Cb)
    {
        enc_frame_proc_cb = Cb;
        return ST_ERRNO_SUCCESS;
    }

    private:
    int dev_id;
    int chn_id{ 0 };
    bool customer_encoder{ false };
    IMPAudioPalyloadType type{ PT_G711A };
    IMPAudioEncEncoder encoder;
    IMPAudioEncChnAttr attr;
    EncoderFrameCb cb{ nullptr };

    void* AudioEncProcessStream(void);
    std::pair<bool, std::shared_ptr<std::thread>> sp_audio_thread{ false, nullptr };
    AudioEncFrameProcCallback enc_frame_proc_cb{ nullptr };
    int enc_handle{ 0 };
};

class DeviceAudioDecode final : public Device
{
    public:
    explicit DeviceAudioDecode(int32_t dev_id = 0);
    ~DeviceAudioDecode();

    ST_ERR Init(void) override
    {
    }
    ST_ERR Init(const json11::Json& config);
    ST_ERR Start();
    ST_ERR Stop();
    ST_ERR DeInit();
    ST_ERR GetStream(IMPAudioStream& stream);
    ST_ERR SendGetStream(IMPAudioStream& in, IMPAudioStream& out);
    ST_ERR ReleaseStream(IMPAudioStream& stream);
    ST_ERR SetFrameProcCb(AudioDecFrameProcCallback Cb)
    {
        dec_frame_get_cb = Cb;
        return ST_ERRNO_SUCCESS;
    }
    // 其他成员函数和变量
    private:
    int dev_id;
    int chn_id{ 0 };
    bool customer_decoder{ false };
    IMPAudioPalyloadType type{ PT_G711A };
    IMPAudioDecDecoder decoder;
    IMPAudioDecChnAttr attr;
    int dec_handle{ 0 };
    void* AudioDecProcessStream(void);
    std::pair<bool, std::shared_ptr<std::thread>> sp_audio_dec_thread{ false, nullptr };
    AudioDecFrameProcCallback dec_frame_get_cb{ nullptr };
};
#endif
} // namespace sense_koks

} // namespace st