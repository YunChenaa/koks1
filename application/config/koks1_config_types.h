/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-07-25 11:02:16
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-25 14:42:43
 * @FilePath: /sense-koks/application/config/koks1_config.h
 */

#pragma once
#include "koks_project_def.h"
#include <map>
#include <string>
#include <vector>

// Serial 配置
struct SerialConfig {
    std::string path;
    int BaudRate;
};

// 数据库配置
struct DBConfig {
    std::string path;
};

// LED 配置
struct LEDConfig {
    int channel;
    int freq;
    double duty;
    double night_duty;
};

// 日志配置
struct LogLevel {
    int level;
    std::map<std::string, int> type_list;
};

struct LogConfig {
    std::string path;
    int max_size;
    int max_num;
    LogLevel level;
};

// 权重配置
struct WeightConfig {
    int global;
    int roi;
    int smooth;
};

// ISP 配置
struct ISPConfig {
    int cam_id;
    std::string sensor_name;
    int face_ae_target;
#ifdef KOKS_SDK_VERSION_V2
    int palm_ae_target;
#endif
    WeightConfig weight;
};

// 输入配置
struct InputConfig {
    uint16_t width;
    uint16_t height;
    struct {
        bool enable;
        bool flip;
        bool mirror;
        struct {
            bool enable;
            int angle;
        } rotate;
    } _2d;
};

// 裁剪配置
struct CropConfig {
    bool enable;
    uint16_t left;
    uint16_t top;
    uint16_t width;
    uint16_t height;
};

// 缩放配置
struct ScalerConfig {
    bool enable;
    uint16_t width;
    uint16_t height;
};

// FrameSource 配置
struct FrameSourceConfig {
    int dev;
    int group;
    bool enable;
    InputConfig input;
    ScalerConfig scaler;
    CropConfig crop;
};

// 输出配置
struct OutputConfig {
    int chn;
    bool enable;
    std::string type;
    uint16_t width;
    uint16_t height;
    int quality;
    CropConfig crop;
};

// 视频编码配置
struct VencConfig {
    int dev;
    int group;
    bool enable;
    std::vector<OutputConfig> output;
};

// 配置FrameSource或者Venc的拉流参数
struct RestartAttr {
    // 对于FrameSource，配置缩放；对于Venc配置OutputConfig的宽和高
    bool enable;
    uint16_t width; 
    uint16_t height;
    // 裁剪
    bool crop_enable;
    uint16_t crop_left;
    uint16_t crop_top;
    uint16_t crop_width;
    uint16_t crop_height;
    // 修改FrameSource的旋转，0: 不修改, 1: 旋转90度, 2: 旋转270度, 3: 旋转180度
    uint16_t i2d;
};

struct ReStartLayout {
    uint8_t joint;  // 拼接方式。0：上下拼接 1：左右拼接。仅对拼接有效。
    uint8_t center; // 图像居中。0：不处理 1：水平居中 2：垂直居中(暂不支持)
    uint16_t line_x; // 垂直线位置。0：忽略。仅对拼接有效。
    uint16_t line_y; // 水平线位置。0：忽略。仅对拼接有效。暂不支持。
};

typedef struct ReStartParam {
    uint16_t v; // Version. 0: default, 1: also has ch and venc, 2: also has ch2 and fs2
    uint16_t out_width;
    uint16_t out_height;
    RestartAttr fs;
    int ch;
    int ch2;
    RestartAttr fs2;
    RestartAttr venc;
    ReStartLayout layout;
} ReStartParam;

// 视频配置
struct VideoConfig {
    std::vector<ISPConfig> isp;
    std::vector<FrameSourceConfig> frame_source;
    std::vector<VencConfig> venc;
};

struct AudioInputConfig {
    int dev;
    int dev_id;
    bool enable;
    int sample_rate;
    int sample_width;
    int channels;
    int volume;
    bool howling;
    struct {
        bool enable;
        int level;
        std::map<std::string, int> level_list;
    } ans;
    int analog_gain;
};

struct AudioOutputConfig {
    int dev;
    int dev_id;
    bool enable;
    int sample_rate;
    int sample_width;
    int channels;
    int volume;
    int analog_gain;
};

struct AudioEncodeConfig {
    int dev;
    bool enable;
    std::string type;
    int bit_rate;
    int sample_rate;
    int sample_width;
    int channels;
    struct {
        bool enable;
        std::string name;
    } customer_encoder;
};

struct AudioDecodeConfig {
    int dev;
    bool enable;
    std::string type;
    int sample_rate;
    int sample_width;
    int channels;
    struct {
        bool enable;
        std::string name;
    } customer_decoder;
};

// 音频配置
struct AudioConfig {
    bool aec;
    std::vector<AudioInputConfig> input;
    std::vector<AudioOutputConfig> output;
    std::vector<AudioEncodeConfig> encode;
    std::vector<AudioDecodeConfig> decode;
};

// 媒体配置
struct MediaConfig {
    VideoConfig video;
    AudioConfig audio;
};
// 日夜切换配置
struct NightDaySwitchConfig {
    uint64_t RGB_ISOThreshold_DaytoNight;
    uint32_t RGB_LumaThreshold_DaytoNight;

    uint64_t RGB_ISOThreshold_NighttoDay;
    uint32_t RGB_LumaThreshold_NighttoDay;
    uint32_t RGB_AWBThreshold_NighttoDay;

    float RGB_ISOJumpThreshold_NighttoDay;
    int RGB_ISOJumpDiscardAfterSwitch_NighttoDay;
};

// 总配置
struct Koks_Config {
    SerialConfig serial;
    DBConfig db;
    std::vector<LEDConfig> led;
    NightDaySwitchConfig night_day_switch;
    LogConfig log;
    MediaConfig media;
    std::vector<ReStartParam> mj_restart_params;
    std::vector<ReStartParam> h264_restart_params;
};
