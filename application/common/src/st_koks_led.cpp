/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-03-28 15:35:25
 * @LastEditors: 洪照
 * @LastEditTime: 2025-10-28 17:55:57
 * @FilePath: /sense-koks/application/common/src/st_koks_led.cpp
 */
#include "st_koks_led.h"
#include "os_utils.h"
#include "sys_logger.h"
#include "koks_project_def.h"
#include "koks1_config.h"

namespace st
{
namespace sense_koks
{
Led& Led::GetInstance(int id)
{
    static std::vector<Led> instance(koks_config.led.size());
    return instance[id];
}

ST_ERR Led::Init()
{
    const auto& ledConfig = koks_config.led[id_];
    return Init(ledConfig.channel, ledConfig.freq, ledConfig.duty, ledConfig.night_duty);
}

ST_ERR Led::Init(uint8_t chn, uint32_t freq, float duty_cycle, float night_duty_cycle)
{
    uint32_t cycle = 0;
    pwm_chn = chn;
    pwm_duty_cycle = duty_cycle;
    night_pwm_duty_cycle = night_duty_cycle;
    pwm_period = static_cast<uint32_t>(1000000000.0f / freq);
    cycle = pwm_period * pwm_duty_cycle;
    std::string pwmChannelPath = "/sys/class/pwm/pwmchip0/pwm" + std::to_string(pwm_chn);

    bool success = WriteSysFile("/sys/class/pwm/pwmchip0/export", std::to_string(pwm_chn).c_str());

#ifdef KOKS_FEATURE_LED_GPIO
    // success &= WriteSysFile(pwmChannelPath + "/polarity", "inversed"); // 设置极性反向
#endif

    success &= WriteSysFile(pwmChannelPath + "/period", pwm_period); // 设置周期
    success &= WriteSysFile(pwmChannelPath + "/duty_cycle", cycle); // 设置占空比
    success &= WriteSysFile(pwmChannelPath + "/enable", "1"); // 使能 PWM

#ifndef KOKS_FEATURE_LED_GPIO
    success &= WriteSysFile(pwmChannelPath + "/enable", "0"); // 禁用 PWM
#endif

#ifdef KOKS_FEATURE_LED_GPIO
    // 初始化GPIO PB27 GPIO编号为 1* 32 + 27 = 59
    success &= WriteSysFile("/sys/class/gpio/export", "59"); // 导出 GPIO59
    success &= WriteSysFile("/sys/class/gpio/gpio59/direction", "out"); // 设置为输出
#endif

    is_init = success;
    return ST_ERRNO_SUCCESS;
}

ST_ERR Led::turnOn()
{
    if (is_init == false) {
        Init();
    }
    if (is_init == false) {
        LOG_ERROR("turnOn failed\n");
        return ST_ERRNO_FAIL;
    }
    if (status == LedState::ON) {
        LOG_DEBUG("turnOn already\n");
        return ST_ERRNO_SUCCESS;
    }

#ifdef KOKS_FEATURE_LED_GPIO
    // 使能GPIO59 输出高电平
    WriteSysFile("/sys/class/gpio/gpio59/value", "1"); // 设置为高电平
#else
    WriteSysFile("/sys/class/pwm/pwmchip0/pwm" + std::to_string(pwm_chn) + "/enable", "1");
#endif

    status = LedState::ON;
    LOG_DEBUG("turnOn\n");
    return ST_ERRNO_SUCCESS;
}

ST_ERR Led::turnOff()
{
    if (is_init == false) {
        LOG_ERROR("turnOff failed\n");
        return ST_ERRNO_FAIL;
    }

#ifdef KOKS_FEATURE_LED_GPIO
    // 使能GPIO59 输出低电平
    WriteSysFile("/sys/class/gpio/gpio59/value", "0"); // 设置为低电平
#else
    WriteSysFile("/sys/class/pwm/pwmchip0/pwm" + std::to_string(pwm_chn) + "/enable", "0"); // 禁用 PWM
#endif

    status = LedState::OFF;
    LOG_DEBUG("turnOff\n");
    return ST_ERRNO_SUCCESS;
}

ST_ERR Led::setDutyCycle(float duty_cycle)
{
    uint32_t cycle;
    if (is_init == false) {
        LOG_ERROR("setDutyCycle failed\n");
        return ST_ERRNO_FAIL;
    }
    cycle = pwm_period * duty_cycle;

    WriteSysFile("/sys/class/pwm/pwmchip0/pwm" + std::to_string(pwm_chn) + "/duty_cycle", cycle); // 设置占空比

    LOG_DEBUG("setDutyCycle %f\n", duty_cycle);
    current_duty_cycle = duty_cycle;
    return ST_ERRNO_SUCCESS;
}

bool Led::IsNightMode()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return static_cast<bool>(mode_flag_ & MODE_NIGHT);
}

void Led::SetMode(Mode mode, bool enable)
{
    std::unique_lock<std::mutex> lock(mutex_);
    int new_mode = enable ? (mode_flag_ | mode) : (mode_flag_ & (~mode));
    if (new_mode == mode_flag_) {
        return;
    }
    LOG_INFO("Led %d, mode %d -> %d\n", id_, mode_flag_, new_mode);
    mode_flag_ = new_mode;

    if (is_init == false) {
        Init();
    }

    if (new_mode == 0) {
        turnOff();
    } else {
        float duty = (new_mode & MODE_AI) ? pwm_duty_cycle : night_pwm_duty_cycle;
        if (duty != current_duty_cycle) {
            setDutyCycle(duty);
        }
        turnOn();
    }
}

void LedProxy::Init()
{
    for (size_t i = 0; i < koks_config.led.size(); ++i) {
        Led::GetInstance(i).id_ = i;
    }

    Led::GetInstance(0).Init();
}

void LedProxy::SetAiMode(bool enable)
{
    LOG_DEBUG("Led AI mode %s\n", (enable ? "on" : "off"));
    Led::GetInstance(0).SetMode(Led::Mode::MODE_AI, enable);
}

void LedProxy::SetNightMode(bool enable)
{
    LOG_DEBUG("Led night mode %s\n", (enable ? "on" : "off"));
    for (size_t i = 0; i < koks_config.led.size(); ++i) {
        auto& led = Led::GetInstance(i);
        led.SetMode(Led::Mode::MODE_NIGHT, enable);
    }
}

} // namespace sense_koks

} // namespace st