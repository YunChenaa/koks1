/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-03-28 15:34:42
 * @LastEditors: 洪照
 * @LastEditTime: 2025-06-09 11:17:38
 * @FilePath: /sense-koks/application/common/include/st_koks_led.h
 */
#pragma once
#include "st_koks_errno.h"
#include <mutex>
#include <thread>
namespace st
{
namespace sense_koks
{
class Led
{
    public:
    enum class LedState {
        OFF = 0,
        ON = 1,
    };

    enum Mode {
        MODE_AI = (1 << 0),    // 算法开灯
        MODE_NIGHT = (1 << 1), // 夜视开灯
    };

    public:
    static Led& GetInstance(int id);
    Led() = default;
    Led(const Led&) = delete;
    void operator=(const Led&) = delete;
    ST_ERR Init();
    ST_ERR Init(uint8_t chn, uint32_t freq, float duty_cycle, float night_duty_cycle);
    void SetMode(Mode mode, bool enable);
    bool IsNightMode();

    private:
    ST_ERR turnOn();
    ST_ERR turnOff();
    ST_ERR setDutyCycle(float duty_cycle);

    private:
    int id_{0};
    uint8_t pwm_chn{ 1 };
    uint32_t pwm_period{ 0 };
    float pwm_duty_cycle{ 0.0f };
    float night_pwm_duty_cycle{ 0.0f };
    float current_duty_cycle{ 0.0f };
    bool is_init{ false };

    std::mutex mutex_;
    int mode_flag_{ 0 };
    LedState status{ LedState::OFF };
    ~Led() = default;

    friend class LedProxy;
};

class LedGuard
{
    public:
    LedGuard()
    {
        auto &led = Led::GetInstance(0);
        led.SetMode(Led::MODE_AI, true);
    }
    ~LedGuard()
    {
        auto &led = Led::GetInstance(0);
        led.SetMode(Led::MODE_AI, false);
    }
};

class LedProxy {
public:
    static void Init();;
    // 算法开关灯控制
    static void SetAiMode(bool enable);
    // 夜视开关灯控制
    static void SetNightMode(bool enable);

    LedProxy() = delete;
    LedProxy(const LedProxy&) = delete;
    LedProxy& operator=(const LedProxy&) = delete;
};

} // namespace sense_koks

} // namespace st
