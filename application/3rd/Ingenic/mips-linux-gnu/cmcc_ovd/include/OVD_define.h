/**
 * @file OVD_define.h
 * @brief   OVD sdk 数据类型定义
 * @author 中移杭州信息技术有限公司
 * @copyright 2021 OVD All rights reserved.
 *
 */

  /**
 * @defgroup SDK主要功能 SDK主要功能
 * @brief SDK主要功能接口包括初始化，登录服务器2等,基础业务流程参考 3.1章节 @ref ovd_doc_manual
 *
 */

  /**
 * @defgroup SDK流媒体 SDK流媒体
 * @brief SDK流媒体业务主要功能接口
 */

/**
 * @defgroup 设备状态变更 设备状态变更
 * @brief 设备同步sdk状态通知结构，例如中继设备低功耗离线休眠状态等,参考设备逻辑关流程 @ref ovd_doc_manual 里的3.5章节
 */

 /**
 * @defgroup 日志 日志
 * @brief 日志业务相关接口等
 */

  /**
 * @defgroup 能力集 能力集
 * @brief 能力集结构体介绍,设备与安防平台协议请参考 @ref ovd_doc_config_and_caps
 */

 /**
 * @defgroup 告警 告警
 * @brief 告警业务相关接口等,告警流程参考 @ref ovd_doc_manual 3.3章节
 */

 /**
 * @defgroup 低功耗 低功耗
 * @brief 告警业务相关接口等,详细业务参考 @ref ovd_doc_low
 */

  /**
 * @defgroup 软探针 软探针
 * @brief 软探针业务相关接口等
 */


/**
 * @defgroup 对讲 对讲
 * @brief 对讲业务相关接口
 */

 /**
 * @defgroup 安防业务状态通知 安防业务状态通知
 * @brief 安防业务状态通知相关接口
 */

/**
 * @defgroup 智能语音交互 智能语音交互
 * @brief 智能语音交互相关接口,业务参考 @ref ovd_doc_aispeech
 */

 /**
 * @defgroup 安防配网 安防配网
 * @brief 设备配网相关接口,配网流程参考 @ref ovd_doc_manual 3.2章节
 */

  /**
 * @defgroup 调试 调试
 * @brief 调试功能相关接口
 */

   /**
 * @defgroup 多通道 多通道
 * @brief NVR,多目等业务相关接口
 */

 /**
 * @defgroup OTA OTA
 * @brief 固件升级业务相关接口及结构体
 *
 */

 #ifdef CONFIG_USING_CLOUD_AI
 /**
 * @defgroup 动态算法 动态算法
 * @brief 动态算法,云化AI相关业务
 *
 */
#endif

/**
 * @defgroup 设备信息查询 设备信息查询
 * @brief 设备信息查询相关业务接口及结构体
 *
 */
/**
 * @defgroup 设备配置 设备配置
 * @brief 设备配置相关业务接口及结构体
 *
 */

 /**
 * @defgroup 设备控制 设备控制
 * @brief 设备控制相关业务接口及结构体
 *
 */

/**
 * @defgroup 机器人业务 机器人业务
 * @brief 机器人相关业务接口及结构体
 *
 */

 /**
 * @defgroup 卡回放 卡回放
 * @brief 卡回放相关业务接口及结构体 流程参考 @ref ovd_doc_manual 3.4章节
 *
 */

 /**
 * @defgroup 白盒摄像头 白盒摄像头
 * @brief 白盒业务接口及结构体
 *
 */

 /**
 * @defgroup AOV AOV
 * @brief AOV业务 参考@ref ovd_doc_manual 3.9章节
 */

  /**
 * @defgroup 枪球联动 枪球联动
 * @brief 枪球联动业务
 *
 */


#ifndef __OVD_DEFINE_H__
#define __OVD_DEFINE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "menuconfig.h"
#include <string.h>

#define MAX_LEN_8  8  /**< 基础类型定义 */
#define MAX_LEN_24  24  /**< 基础类型定义 */
#define MAX_LEN_32  32  /**< 基础类型定义 */
#define MAX_LEN_64  64  /**< 基础类型定义 */
#define MAX_LEN_128  128 /**< 基础类型定义 */
#define MAX_LEN_256  256 /**< 基础类型定义 */
#define MAX_LEN_512  512 /**< 基础类型定义 */
#define MAX_LEN_1024  1024 /**< 基础类型定义 */

#define WIFI_SSID_LEN  33 /**< 基础类型定义 */
#define WIFI_PWD_LEN  80 /**< 基础类型定义 */

#define MAX_STREAM_COUNT   4   /**< 单个视频通道最大码流个数*/

// 网卡名称长度
#ifndef IFNAMSIZ                /* defined in <net/if.h> in linux */
#define IFNAMSIZ 16
#endif /* IFNAMSIZ */

//#define ALARM_TIME_MIN_INTERVAL 10
//#define ALARM_TIME_MAX_INTERVAL 3600
#define OVD_MAX_LOCAL_STORAGE_PATH_LEN 256 /**< sdk卡存储路径最大字符*/

#ifndef IN
#define IN ///< 函数参数修饰符 标识入参
#endif

#ifndef OUT
#define OUT ///< 函数参数修饰符 标识入参
#endif

#ifdef __GNUC__
    #define OVD_EXPORT __attribute__((visibility("default"))) ///< 函数参数修饰符
#else
    #define OVD_EXPORT ///< 函数参数修饰符
#endif

#ifdef __GNUC__
#    define GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ///< GCC版本要求
#else
#    define GCC_VERSION_AT_LEAST(x,y) 0 ///< GCC版本要求
#endif

#if GCC_VERSION_AT_LEAST(3,1)
#    define attribute_deprecated __attribute__((deprecated))
#elif defined(_MSC_VER)
#    define attribute_deprecated __declspec(deprecated)
#else
#    define attribute_deprecated    ///< 函数参数修饰符
#endif



/**
 * @brief 重启原因
 */
 typedef enum
 {
    OVD_REBOOT_REASON_DUT_COMMON            =1000,             ///< 设备侧通用原因重启
    OVD_REBOOT_REASON_DUT_POWER_DOWN        =1001,             ///< 设备侧断电重启
    OVD_REBOOT_REASON_DUT_CRASH             =1002,             ///< 设备侧崩溃触发的重启
    OVD_REBOOT_REASON_DUT_HWATCHDOG         =1003,             ///< 设备侧硬件看门狗触发的重启
    OVD_REBOOT_REASON_DUT_FACTORY_RESET     =1004,             ///< 设备侧本地恢复出厂设置引发的重启
    OVD_REBOOT_REASON_DUT_ABNORMAL_REBOOT   =1005,             ///< 设备侧异常重启，可能规避一些异常逻辑而导致的重启

    OVD_REBOOT_REASON_SDK_COMMON            =2000,             ///< sdk逻辑触发的通用原因重启
    OVD_REBOOT_REASON_SDK_SCHEDULED_MAINTENANCE=2001,          ///< sdk自动维护触发的重启
    OVD_REBOOT_REASON_SDK_SWATCHDOG         =2002,             ///< sdk软看门狗触发的重启

    OVD_REBOOT_REASON_CLOUD_COMMON          =3000,             ///< 平台触发的通用重启原因
    OVD_REBOOT_REASON_CLOUD_REBOOT          =3001,             ///< 平台触发的远程重启
    OVD_REBOOT_REASON_CLOUD_UPGRADE         =3002,             ///< 平台触发的升级重启
    OVD_REBOOT_REASON_CLOUD_FACTORY_RESET   =3003,             ///< 平台触发的恢复出厂设置
 }ovd_reboot_reason_e;

/**
 * @brief 配网模式类型(OVD Distribution Network Mode)
 */
typedef enum{
    QR_CONFIG,///< 二维码配网
    WAVE_CONFIG,///< 声波配网
    WIRED_CONFIG,///< 有线配网
    AP_CONFIG,///< AP配网
    IGRS_CONFIG,///< 移动闪联配网
    BLE_CONFIG = 5,///< 蓝牙配网
    G4_CONFIG,///< 4G配网
    G5_CONFIG,///< 5G配网
}OVD_DNM_e;

/**
 * @brief 错误码类型
 */
typedef enum
{
    OVD_RET_STREAM_CONNECT_CHANNEL_BROKEN = -5,     ///< 底层流媒体通道出现断开
    OVD_RET_COMMON_ERROR                  = -1,     ///< 通用错误码
    OVD_RET_SUCCESS                       =  0,     ///< 通用成功
    OVD_RET_BADPARAMETER                  =  1,     ///< 入参错误
    OVD_RET_NOMEM                         =  2,     ///< 内存错误

    //配网相关错误码
    OVD_NETCONF_SUCCESS                   =  7,     ///< 配网识别数据成功
    OVD_NETCONF_LESS_DATA                 =  9,     ///< 配网缺失数据，数据太长，二维码分页，需要厂商再次输入
    OVD_RET_RPC_UNIMPLEMENT               = 101,    ///< 该功能模拟未使能或未支持
    DME_RPC_SUBSTREAM_NOTSUPPORT          = 107,    ///< 性能不足，子码流开启失败

    // 3d光传感器环境采集错误码
    OVD_RPC_3DSENSOR_IS_COLLECTING          = 109,    ///< 3D光传感器，正在环境采集
    OVD_RPC_3DSENSOR_NO_DETECT              = 110,    ///< 3D光传感器，未开启检测功能
    OVD_RPC_3DSENSOR_RECOGNIZE_PLANES_FAIL  = 111,    ///< 3D光传感器，识别平面失败
    OVD_RPC_3DSENSOR_COLLECT_GROUND_FAIL    = 112,    ///< 3D光传感器，收集地面系数和检测区域失败
    OVD_RPC_3DSENSOR_RECOGNIZE_BED_FAIL     = 113,    ///< 3D光传感器，识别床错误
    OVD_RPC_3DSENSOR_SAVE_FAIL              = 114,    ///< 3D光传感器，保存背景环境失败
    
    //srt卡回放机制错误码
    OVD_RET_READ_FRAME_RETRY=201, ///< 读帧重试错误码
    OVD_RET_READ_FRAME_EOF=202,   ///< 卡录像读取完毕，厂商反馈EOF
    OVD_RET_SEEK_NODATA=203,      ///< seek点无音视频数据

}OVD_ERRNO;




typedef char OVD_bool;  /**< 基础类型定义 */
typedef unsigned int OVD_uint32; /**< 基础类型定义 */
typedef int OVD_int32; /**< 基础类型定义 */
typedef unsigned long long  OVD_uint64;/**< 基础类型定义 */
typedef long long OVD_int64;/**< 基础类型定义 */
typedef char  OVD_char;/**< 基础类型定义 */
typedef unsigned char  OVD_uchar;/**< 基础类型定义 */
typedef void OVD_void;/**< 基础类型定义 */
typedef float  OVD_float;/**< 基础类型定义 */
typedef unsigned short OVD_ushort;/**< 基础类型定义 */
typedef unsigned char  OVD_byte;/**< 基础类型定义 */
typedef unsigned short OVD_word;/**< 基础类型定义 */
typedef unsigned int OVD_dword;/**< 基础类型定义 */

#define OVD_FALSE   0/**< 基础类型定义 */
#define OVD_TRUE    1/**< 基础类型定义 */

/**
 * @brief 设备类型定义
 *
 */
typedef enum
{
    DEVICE_TYPE_IPC = 0,    ///< 普通IPC设备，单个物理视频通道
    DEVICE_TYPE_NVR = 3,    ///< NVR设备
    DEVICE_TYPE_MIPC,       ///< 多通道IPC设备
    DEVICE_TYPE_IPD,        ///< 普通云台摄像机
    DEVICE_TYPE_DOORLOCK,   ///< 门锁
    DEVICE_TYPE_DOORBELL,   ///< 门铃
    DEVICE_TYPE_DASH_CAM,   ///< 行车记录仪
    DEVICE_TYPE_YARD_LIGHT, ///< 庭院灯
    DEVICE_TYPE_ROBOT = 10, ///< 机器人(作废)
    DEVICE_TYPE_TV_BOX,     ///< 机顶盒
    DEVICE_TYPE_VOICE_BOX,  ///< 可视音箱
    DEVICE_TYPE_ENTRANCE_GUARD,  ///< 可视门禁
    DEVICE_TYPE_SPC,             ///< 太阳能摄像头
    DEVICE_TYPE_LPCC,            ///< 低功耗摄像头
    DEVICE_TYPE_MOBOCAM,         ///< 可移动摄像头
    DEVICE_TYPE_EDL,     ///< 可视教育台灯
    DEVICE_TYPE_THSC,    /// 温湿度传感器
    DEVICE_TYPE_3DSENSOR, /// 3D光感知传感器
    DEVICE_TYPE_MAX
}ovd_device_type_e;


/**
 * @brief 视频码流加密映射表
 *
 */
typedef enum
{
    OVD_MEDIA_NO_ENCRYPT        = 0,                ///< 无加密 该设备不支持码流加密功能
    OVD_MEDIA_BASE_ENCRYPT      = 1,                ///< 基础加密 只支持基础加密方案，适用于所有设备
    OVD_MEDIA_ADVANCED_ENCRYPT  = 2,                ///< 钥匙链加密 用于设备性能富裕的设备，说明设备能够支持aes_cb_256加密及基础加密方案
    OVD_MEDIA_ENCRPYT_TYPE_OTHER,
}ovd_media_encrypt_type_e;


#define OVD_CARD_PLAYBACK_SPEED_MASK_QUARTER   0x0001   ///< 卡回看倍速0.25倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_HALF      0x0002   ///< 卡回看倍速0.5倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_1TIMES    0x0004   ///< 卡回看倍速1倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_2TIMES    0x0008   ///< 卡回看倍速2倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_4TIMES    0x0010   ///< 卡回看倍速4倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_8TIMES    0x0020   ///< 卡回看倍速8倍
#define OVD_CARD_PLAYBACK_SPEED_MASK_16TIMES   0x0040   ///< 卡回看倍速16倍

#define OVD_CAPTURE_MODE_QUALITY    0x0001  ///< 人脸抓拍模式 质量抓拍
#define OVD_CAPTURE_MODE_QUICK      0x0002  ///< 人脸抓拍模式 快速抓拍


#define OVD_CLOTHES_COLOR_WHITE 0x0001  ///< 设备所支持工服颜色列表 白 white
#define OVD_CLOTHES_COLOR_BLACK 0x0002  ///< 设备所支持工服颜色列表 黑 black
#define OVD_CLOTHES_COLOR_RED   0x0004  ///< 设备所支持工服颜色列表 红 red
#define OVD_CLOTHES_COLOR_BLUE  0x0008  ///< 设备所支持工服颜色列表 blue
#define OVD_CLOTHES_COLOR_GREEN 0x0010  ///< 设备所支持工服颜色列表 绿 green

#define OVD_KITCHEN_CAPTURE_MASK    0x0001  ///<阳光厨房抓拍模式掩码 口罩
#define OVD_KITCHEN_CAPTURE_CAP     0x0002  ///<阳光厨房抓拍模式掩码 帽子
#define OVD_KITCHEN_CAPTURE_CLOTHES 0x0004  ///<阳光厨房抓拍模式掩码 衣服


#define LINKAGE_MODE_SPEECH 0x0001      ///< 联动声音 speech
#define LINKAGE_MODE_LIGHT  0x0002      ///< 联动灯光 light
#define LINKAGE_MODE_BUZ    0x0004      ///< 联动蜂鸣器 buz

/**
 * @brief 夜视模式
 *
 */
typedef enum
{
    OVD_NIGHTVISION_MODE_NONE = 0,  ///< 不支持夜视模式
    OVD_NIGHTVISION_MODE_NORMAL,    ///< 黑白夜视
    OVD_NIGHTVISION_MODE_COLOR,     ///< 全彩夜视
    OVD_NIGHTVISION_MODE_ULTRAVIOLET, ///< 黑光夜视
}ovd_night_vison_mode_e;

#define OVD_AIDIY_MASK_BABY_MASK        0x0001  ///< 婴儿遮面

#define OVD_NIGHTVISION_DETECT_HUMAN   0x0001   ///< 智能夜视之人形侦测触发
#define OVD_NIGHTVISION_DETECT_MOTION  0x0002   ///< 智能夜视之移动侦测触发


#define OVD_CAMERA_LENS_TYPE_SINGLE             0x01 ///<摄像机形态 单目
#define OVD_CAMERA_LENS_TYPE_SPLICE             0x02 ///<摄像机形态 双目拼接
#define OVD_CAMERA_LENS_TYPE_DOUBLE_IPC         0x04 ///<摄像机形态 单目枪球联动
#define OVD_CAMERA_LENS_TYPE_DOUBLE_SPLICE_IPC  0x08 ///<摄像机形态 双目拼接枪球联动
#define OVD_CAMERA_LENS_TYPE_DOULE_FULL_IPC     0x10 ///<摄像机形态 全景拼接枪球联动


#define OVD_ROBOT_HOUSE_KEEPER_NONE      0x00   ///< 机器人管家功能 功能比特位 空
#define OVD_ROBOT_HOUSE_KEEPER_WEATHER   0x01   ///< 机器人管家功能 功能比特位 支持天气预报
#define OVD_ROBOT_HOUSE_KEEPER_MEDICINE  0x02   ///< 机器人管家功能 功能比特位 支持吃药提醒


#define OVD_VIDEO_QUALITY_LD    0x0001      ///< 功能比特位：支持低清
#define OVD_VIDEO_QUALITY_SD    0x0002      ///< 功能比特位：支持标清
#define OVD_VIDEO_QUALITY_HD    0x0004      ///< 功能比特位：支持高清
#define OVD_VIDEO_QUALITY_FHD   0x0008      ///< 功能比特位：支持全高清
#define OVD_VIDEO_QUALITY_DEFAULT   (OVD_VIDEO_QUALITY_SD | OVD_VIDEO_QUALITY_HD) ///< 默认要求支持标签和高清


#define OVD_VIDEO_CODEC_H264    0x0001 ///< 功能比特位：支持H264
#define OVD_VIDEO_CODEC_H265    0x0002 ///< 功能比特位：支持H265


#define OVD_VIDEO_RESOLUTION_W640_H360      0x0001      ///< 视频分辨率支持640*360
#define OVD_VIDEO_RESOLUTION_W960_H540      0x0002      ///< 视频分辨率支持960*540
#define OVD_VIDEO_RESOLUTION_W1280_H720     0x0004      ///< 视频分辨率支持1280*720
#define OVD_VIDEO_RESOLUTION_W1920_H1080    0x0008      ///< 视频分辨率支持1920*1080
#define OVD_VIDEO_RESOLUTION_W2304_H1296    0x0010      ///< 视频分辨率支持2304*1296
#define OVD_VIDEO_RESOLUTION_W2560_H1440    0x0020      ///< 视频分辨率支持2560*1440
#define OVD_VIDEO_RESOLUTION_W2560_H1920    0x0040      ///< 视频分辨率支持2560*1920
#define OVD_VIDEO_RESOLUTION_W3840_H2160    0x0080      ///< 视频分辨率支持3840*2160


#define OVD_VIDEO_FRAMERATE_15    0x0001     ///< 视频帧率支持15帧
#define OVD_VIDEO_FRAMERATE_20    0x0002     ///< 视频帧率支持20帧
#define OVD_VIDEO_FRAMERATE_25    0x0004     ///< 视频帧率支持25帧
#define OVD_VIDEO_FRAMERATE_30    0x0008     ///< 视频帧率支持30帧


#define OVD_VIDEO_GOP_TIME_2S    0x0001      ///< I帧间隔支持2秒
#define OVD_VIDEO_GOP_TIME_3S    0x0002      ///< I帧间隔支持3秒
#define OVD_VIDEO_GOP_TIME_4S    0x0004      ///< I帧间隔支持4秒
#define OVD_VIDEO_GOP_TIME_5S    0x0008      ///< I帧间隔支持5秒
#define OVD_VIDEO_GOP_TIME_6S    0x0010      ///< I帧间隔支持6秒


#define OVD_AUDIO_SAMPLE_RATE_8K    0x0000    ///< 音频采样率8000
#define OVD_AUDIO_SAMPLE_RATE_16K   0x0001    ///< 音频采样率16000

/**
 * @brief 人脸检测能力集定义
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_face;                 ///< 是否支持人脸检测
    OVD_bool support_faceExposureAdjust;   ///< 设备是否支持人脸曝光度调节,默认值为false
    OVD_bool support_mask_detection;       ///< 口罩检测,默认值为false
    OVD_dword capture_mode_mask;           ///< 抓拍模式掩码，一个比特位代表一种模式，可同时支持多种模式，参考 OVD_CAPTURE_MODE_XXX
    OVD_int32 alarm_zone;                  ///< 规则区域个数，0表示不支持
    OVD_bool support_speech_keep_alarm;    ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;  ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;     ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false
}OVDAIFaceCapInfo;

/**
 * @brief 客流统计能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_passenger;                           ///< 是否支持客流统计
    OVD_bool support_passenger_exposure_adjust;           ///< 设备是否支持人脸曝光度调节,默认值为false
    OVD_bool support_osd_status;                ///< 设备是否支持osd显示客流统计信息
    OVD_dword capture_mode_mask;                ///< 抓拍模式掩码，一个比特位代表一种模式，可同时支持多种模式，参考 OVD_CAPTURE_MODE_XXX
    OVD_int32 alarm_zone;                       ///< 规则区域个数，0表示不支持
    OVD_bool support_welcome_promotion;         ///< 是否支持迎宾促销
    OVD_bool support_alert_time;                ///< 是否支持设置检测时间(迎宾促销)， 0表示不支持，1表示支持，默认不支持
}OVDAIPassengerCapInfo;

/**
 * @brief 阳光厨房能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_kitchen_detection;         ///< 是否支持阳光厨房
    OVD_dword capture_mode_mask;                ///< 抓拍模式掩码，一个比特位代表一种模式，可同时支持多种模式，参考 OVD_KITCHEN_CAPTURE_XXX
    OVD_dword clothes_color_mask;               ///< 工服颜色 参考 OVD_CLOTHES_COLOR_XXX
    OVD_int32 alarm_zone;                       ///< 规则区域个数，0表示不支持
}OVDAIKitchenCapInfo;

/**
 * @brief 机动车检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_vehicle_detection;         ///< 是否支持机动车检测
    OVD_bool support_vehicle_detect_site;       ///< 设备是否支持车辆识别的安装场景
    OVD_int32 alarm_zone;                       ///< 规则区域个数，0表示不支持
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false
}OVDAIVehicleCapInfo;

#define OVD_VEHICLE_CAPTURE_MODE_ELECTROMOBILE  0x0001 ///<梯控能力 功能比特位 支持电瓶车
#define OVD_VEHICLE_CAPTURE_MODE_BIKE           0x0002 ///<梯控能力 功能比特位 支持自行车

/**
 * @brief 非机动车检测能力集定义
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_nonvehicle_detection;      ///< 是否支持非机动车检测
    OVD_dword capture_mode_mask;                ///< 抓拍模式掩码，一个比特位代表一种模式，可同时支持多种模式，参考 OVD_VEHICLE_CAPTURE_MODE_XXX
    OVD_int32 alarm_zone;                       ///< 规则区域个数，0表示不支持
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false
}OVDAINonmotorvehicleCapInfo;

/**
 * @brief 区域人数统计能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_regional_people_stat;            ///< 是否支持区域人数统计，默认关闭
    OVD_bool support_alert_time;                      ///< 是否支持设置检测时间， 0表示不支持，1表示支持，默认不支持
    OVD_bool support_osd_status;                      ///< 是否支持设置OSD叠加人数， 0表示不支持，1表示支持，默认不支持
    OVD_int32 regional_people_count;                  ///< 个数，0表示不支持，其他值表示对应配置项“触发预警的人数”的最大值限制，默认不支持
    OVD_int32 alarm_report_duration;                  ///< 分钟，0表示不支持，其他值表示对应配置项“告警上报间隔时间”的最大值限制，默认不支持
    OVD_int32 detect_result_report_duration;          ///< 分钟，0表示不支持，其他值表示对应配置项“区域人数定时上传”的最大值限制，默认不支持
    OVD_int32 detect_plans_num;                       ///< 检测计划的个数，0不支持，最大值10个，默认不支持
    OVD_int32 alarm_area_num;                         ///< 检测区域的个数，0不支持，最大值10个，默认不支持
    OVD_int32 alarm_zone;                             ///< 0不支持，1：只支持矩形，n(n>=3):支持不规则n边形，目前只支持4边形。最大支持10边形，默认不支持
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false
} OVDAIRegionalPeopleStatCapInfo;

/**
 * @brief 高空抛物能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_parabolicaerial;    ///< 设备是否支持上报高空抛物,默认值为false
    OVD_int32 detect_zone_num;           ///< 检测区域最大个数
    OVD_int32 detect_zone;               ///< 检测区域绘制多边形最大边数, 0不支持，1：只支持矩形，n(n>=3):支持不规则n边形
    OVD_int32 shield_zone_num;           ///< 屏蔽区域最大个数
    OVD_int32 shield_zone;               ///< 屏蔽区域绘制多边形最大边数, 0不支持，1：只支持矩形，n(n>=3):支持不规则n边形
    OVD_int32 floor_conf_num;            ///< 楼层设置最大个数
} OVDAIParabolicAerialCapInfo;

/**
 * @brief 车道线检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool  support_lane_line;                ///< 设备是否支持上报AI,默认值为false
    OVD_int32 lane_line_num;                    ///< 支持车道线数量，1：单个车道线， 2：双车道线， 目前只支持配置1和2，默认值为0
    OVD_int32 lane_line_alarm_zone;             ///< 是否支持检测区域，0不支持， 1：只支持矩形， n(n>=3)，支持不规则n边形，目前只支持矩形，默认值为0
} OVDAILaneLineCapInfo;

/**
 * @brief 离岗检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_off_duty_detection;        ///< 设备是否支持离岗检测,默认值为false
    OVD_bool support_alert_time;                ///< 是否支持设置检测时间
    OVD_int32 on_duty_count;                    ///< 0表示不支持，其他值表示对应配置项"在岗人数"的最大值限制，默认值为0
    OVD_int32 off_duty_durtion;                 ///< 分钟，0表示不支持，其他值表示对应配置项“设置离岗时长”的最大值限制，默认值为0
    OVD_int32 detect_plan_num;                  ///< 检测计划的个数，0不支持，最大值5个，默认值为0
    OVD_int32 alarm_area_num;                   ///< 检测区域的个数，0不支持，最大值10个，默认值为0
    OVD_int32 alarm_zone;                       ///< 0不支持，1：只支持矩形，n(n>=3):支持不规则n边形，目前只支持4边形。最大支持10边形，默认值为0

} OVDAIOffDutyCapInfo;

/**
 * @brief 盔佩戴检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_helmet_wearing_detection;          ///< 设备是否支持头盔佩戴检测,默认值为false
    OVD_int32 detect_zone;                              ///< 整型,检测区域绘制多边形最大边数, 0不支持，1:只支持矩形，n(n>=3):支持不规则n边形，默认值为0
    OVD_bool support_detect_time;                       ///< 是否支持设置检测时间， 0表示不支持，1表示支持，默认值为false
    OVD_bool support_speech_keep_alarm;                 ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;               ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;                  ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false

} OVDAIHelmetWearingCapInfo;

/**
 * @brief 手势识别能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_gesture_recognition;          ///< 设备是否支持手势识别,默认值为false
    OVD_bool support_gesture_call;                 ///< 设备是否支持手势呼叫，默认值为false
    OVD_bool support_set_duration;                 ///< 设备是否支持设置单次通话最长时间，默认值为false
    OVD_bool support_set_calltime;                 ///< 设备是否支持设置允许通话时段，默认值为false
    OVD_bool support_gesture_help;                 ///< 设备是否支持设置允许通话时段，默认值为false

} OVDAIGestureRecognitionCapInfo;

/**
 * @brief 烟火检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_smoke_fire_detection;         ///< 设备是否支持烟火检测,默认值为false
    OVD_int32 detect_zone;                         ///< 整型,检测区域绘制多边形最大边数, 0不支持，1:只支持矩形，n(n>=3):支持不规则n边形，默认值为0
    OVD_bool support_detect_time;                  ///< 是否支持设置检测时间， false表示不支持，true表示支持，默认值为false
    OVD_int32 alertarea_num;                       ///< 整型,设备支持警戒区域数,默认值为1, 厂商上报

    OVD_bool support_linkage_speech;               ///< 是否支持<语音输出>联动策略，厂商上报，默认值为false
    OVD_bool support_speech_keep_alarm;            ///< 是否支持<语音输出>联动策略的持续告警, 默认值为false
    OVD_bool support_speech_set_interval;          ///< 是否支持<语音输出>联动策略的告警间隔设置, 默认值为false

    OVD_bool support_linkage_light;                ///< 是否支持<联动警灯>联动策略，厂商上报，默认值为false
    OVD_bool support_light_keep_alarm;             ///< 是否支持<联动警灯>联动策略的持续告警，默认值为false

    OVD_bool support_linkage_buz;                  ///< 是否支持<蜂鸣器>联动策略，厂商上报，默认值为false
    OVD_bool support_buz_keep_alarm;               ///< 是否支持<蜂鸣器>联动策略的持续告警，默认值为false
} OVDAISmokeFireCapInfo;

/**
 * @brief 家人识别能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_family_recognition;          ///< 布尔型，是否支持家人识别，默认值为false
    OVD_bool support_face_management;             ///< 布尔型，是否支持人脸管理，默认值为false
    OVD_bool support_set_schedule_time;           ///< 布尔型，是否支持设置检测时间段，默认值为false
    OVD_bool support_speech_keep_alarm;           ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;         ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;            ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false

} OVDAIFamilyRecognitionCapInfo;

/**
 * @brief 隐私遮蔽能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_privacy_masking;          ///< 布尔型，设备是否支持隐私遮蔽，默认值为false
    OVD_int32 zone;                            ///< 整型,隐私遮蔽区域绘制多边形最大边数, 0不支持，1:只支持矩形，n(n>=3):支持不规则n边形，默认值为0
    OVD_int32 zone_mode;                        ///< 整型，是否支持选择区域 遮掩模式自由调节 0代表选择区域被遮掩， 1代表非选择区域被遮掩, 2代表可由用户自由选择两种模式
    OVD_bool support_set_schedule_time;        ///< 布尔型，是否支持设置时间段，默认值为false

} OVDAIPrivacyMaskingCapInfo;

/**
 * @brief 跌倒检测能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_fall_detection;           ///< 布尔型，设备是否支持隐私遮蔽，默认值为false
    OVD_int32 zone;                            ///< 整型, 区域绘制多边形最大边数, 0不支持，1:只支持矩形，n(n>=3):支持不规则n边形，默认值为0
    OVD_bool support_set_schedule_time;        ///< 布尔型，是否支持设置检测时间段，默认值为false
    OVD_bool support_speech_keep_alarm;        ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;      ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;         ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false

} OVDAIFallDetectionCapInfo;

/**
 * @brief 睡眠检测sleep_monitor能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_sleep_monitor;     ///< 布尔型：是否支持睡眠检测开关功能，默认值为false
    OVD_bool support_sleep_detection;   ///< 布尔型：是否支持入睡检测开关功能，默认值为false
    OVD_bool support_sleep_detection_sens;///< 布尔型：是否支持入睡检测灵敏度功能，默认值为false
    OVD_bool support_face_cover;        ///< 布尔型：是否支持遮脸检测开关功能，默认值为false
    OVD_bool support_face_cover_sens;  ///< 布尔型：是否支持遮脸检测灵敏度功能，默认值为false
    OVD_bool support_face_cover_mode;  ///< 布尔型：是否支持遮脸检测模式选择功能，默认值为false
    OVD_bool support_wakeup_detection;  ///< 布尔型：是否支持觉醒检测开关功能，默认值为false
    OVD_bool support_wakeup_detection_sens;///< 布尔型：是否支持觉醒检测灵敏度功能，默认值为false
    OVD_bool support_sleep_move_detection;  ///< 布尔型：是否支持睡眠移动检测开关功能，默认值为false
    OVD_bool support_sleep_move_detection_sens;///< 布尔型：是否支持睡眠移动检测灵敏度功能，默认值为false
    OVD_bool support_face_cover_area_mode;  ///< 布尔型：是否支持遮脸检测区域模式选择功能，默认值为false, 选择区域为口鼻或面部或全部
} OVDAISleepMonitorCapInfo;

/**
 * @brief 表情识别expression_recognition能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_expression_recognition;     ///< 布尔型,是否支持AI表情识别能力
    OVD_bool support_expression_recognition_onoff; ///< 布尔型,是否支持AI表情识别开关功能
} OVDAIExpressionRecognitionCapInfo;

/**
 * @brief AI能力集定义
 * @ingroup 能力集
 */
typedef struct
{
    OVDAIFaceCapInfo AIface;    ///< 人脸检测
    OVDAIKitchenCapInfo AIkitchen;  ///< 阳光厨房
    OVDAIVehicleCapInfo AIvehicle;  ///< 机动车检测
    OVDAINonmotorvehicleCapInfo AInonmotorvehicle;  ///< 非机动车检测
    OVDAIPassengerCapInfo AIpassenger;  ///< 客流统计
    OVDAIRegionalPeopleStatCapInfo AIregionalPeopleStat;    ///< 区域人数统计
    OVDAIParabolicAerialCapInfo AIparabolicAerial;  ///< 高空抛物
    OVDAILaneLineCapInfo AILaneLine;    ///< 车道线检测
    OVDAIOffDutyCapInfo AIOffDuty;  ///< 离岗检测
    OVDAIHelmetWearingCapInfo AIHelmetWearing;  ///< 头盔检测
    OVDAIGestureRecognitionCapInfo AIGestureRecognition;  ///< 手势识别
    OVDAISmokeFireCapInfo AISmokeFire;  ///< 烟火检测
    OVDAIFamilyRecognitionCapInfo AIFamilyRecognition;  ///< 家人识别
}OVDAICapInfo;

/**
 * @brief CloudAI能力集定义
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_CloudAI;                   ///< 是否支持云化算法，能够通过算法商城订购算法，实现算法的动态应用
    OVD_int32 cloudai_version;                  ///< 云化算法版本，非系统白盒不填写或者填写1, 系统白盒填写2
    OVD_bool support_report_package_status;     ///< 是否支持算法包状态主动上报
    OVD_bool support_caps_manager;              ///< 是否支持能力集动态管理
}OVDCloudAICapInfo;

/**
 * @brief 智能语音交互业务能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_AISpeech;  ///<是否支持AI语音交互

}OVDAISpeechCapInfo;  ///<AI语音交互相关能力集

/**
 * @brief 童谣业务能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_lullby;  ///<是否支持童谣业务版本

}OVDLullbyCapInfo;  ///<AI语音交互相关能力集

/**
 * @brief AI DIY疑似告警业务能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_AIDIY;         ///< 是否支持AI DIY疑似告警业务
    OVD_dword support_AIDIY_mask;   ///< 支持的AI DIY模型列表掩码，比特位为1表示支持，共64位。参考OVD_AIDIY_MASK_XXX，默认值0x0000000000000000
}OVDAIDIYCapInfo;


/**
 * @brief   联动模式定义
 *          联动模式描述，比特位为1表示支持该类型联动，参考 LINKAGE_MODE_XXX
 * @ingroup 能力集
 * @note    当前和家亲app声光联动只有体现在警戒功能上
 *
 */
typedef struct
{
    OVD_dword alertarea;                        ///< 区域入侵联动方式，0x07 表示支持speech light buz三种联动
    OVD_dword vehicle_detection;                ///< 机动车检测联动方式
    OVD_dword nonmotorvehicle_detection;        ///< 非机动车检测联动方式
    OVD_dword mask_detection;                   ///< 口罩检测联动方式
    OVD_dword regionalPeopleStat_detection;     ///< 区域人数统计联动方式
    OVD_dword transgression;                    ///< 越界侦测联动方式
    OVD_dword helmet_wearing_detection;         ///< 头盔佩戴联动方式
    // OVD_dword smoke_fire_detection;          ///< 烟火告警检测联动方式
    OVD_dword family_recognition;               ///< 家人识别联动方式
    OVD_dword fall_detection;                   ///< 跌倒检测联动方式
}OVDlinkage_mode;

typedef OVDlinkage_mode OVD_CapInfoLinkage; ///<联动模式定义》的别名

/**
 * @brief 设备上对和家看护功能的功能支持配置
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_preference_call;         ///< 是否支持呼叫偏好功能；呼叫偏好：可设置在发起呼叫时，普通电话优先或者APP通话优先
    OVD_bool support_set_calling_number;      ///< 是否支持呼叫号码设置的功能；呼叫号码：可设置点击呼叫按键后，摄像头呼叫的号码（被叫号码）
    OVD_bool support_not_disturb_mode;        ///< 是否支持手机勿扰模式；手机勿扰是控制设备端主叫手机端。可设置设备勿扰开关是否打开，如果打开，可设置勿扰时间
    OVD_bool support_not_disturb_device_mode; ///< 是否支持设备勿扰模式，设备勿扰是控制手机端主叫设备端
    OVD_bool support_auto_hangup;             ///< 是否支持自动挂断功能；自动挂断：可设置检测到访客离开/进门后自行停止呼叫
    OVD_bool support_voice_calling_number;    ///< 是否支持语音呼叫号码功能；如果设备上报支持，则APP新增“语音呼叫号码”配置项，配置完成后，可实现通过语音给指定的号码打电话
}OVDHJKH_mode;

/**
 * @brief EBO机器人工作模式定义
 * @ingroup 能力集
*/
typedef struct
{
    OVD_bool support_move;    ///< 是否支持左右前后移动
    OVD_bool support_nofall;  ///< 是否支持防跌落
    OVD_bool support_laser;   ///< 是否支持激光
    OVD_bool support_onrush;  ///< 是否支持向前猛冲
    OVD_bool support_recharge;///< 是否支持回充
    OVD_bool support_shake;   ///< 是否支持抖动
    OVD_bool support_spin;    ///< 是否支持转圈
    OVD_bool support_track;   ///< 是否支持移动追踪，开启后  默认值是： 人形追踪 1.27新增
    OVD_bool support_body_track;    ///< 是否支持人形追踪追踪，开启后支持人形追踪 1.27新增
    OVD_bool support_pets_track;    ///< 是否支持宠物追踪追踪，开启后支持宠物追踪 1.27新增
    OVD_bool support_cruise;        ///< 是否支持巡航功能，开启后支持巡航 1.29新增
    OVD_bool support_collision;     ///< 是否支持防碰撞功能，开启后支持防碰撞 1.29新增
    OVD_bool support_reminder;      ///< 是否支持提示音功能，开启后支持提示音 1.34新增
    OVD_bool support_speed;  ///< 是否支持速度调节，开启后速度调节 1.40新增
} OVDEBO_mode;

/**
 * @brief 门锁开始方式能力集定义
 * @ingroup 能力集
 */
typedef enum
{
    OVD_UNLOCKWAYCAP_FP         = (1<<0),       ///< 指纹
    OVD_UNLOCKWAYCAP_PWD        = (1<<1),       ///< 密码
    OVD_UNLOCKWAYCAP_CARD       = (1<<2),       ///< 门卡
    OVD_UNLOCKWAYCAP_WHITE_SIM  = (1<<3),       ///< 超级SIM卡(白卡)
    OVD_UNLOCKWAYCAP_ENCRY_SIM  = (1<<4),       ///< 超级SIM卡(加密)
    OVD_UNLOCKWAYCAP_FINGER_VEIN= (1<<5),       ///< 指静脉
    OVD_UNLOCKWAYCAP_IRIS       = (1<<6),       ///< 虹膜
    OVD_UNLOCKWAYCAP_TMPPWD     = (1<<7),       ///< 临时密码
    OVD_UNLOCKWAYCAP_REMOTECALL = (1<<8),       ///< 固话按键远程开锁
    OVD_UNLOCKWAYCAP_FACE       = (1<<9),       ///< 人脸
    OVD_UNLOCKWAYCAP_OTHER,
}ovd_unlockway_cap_t;

/**
 * @brief 门锁供电方式能力集定义
 * @ingroup 能力集
 */
typedef enum
{
    OVD_LOCKBATTERYCAP_SINGLE           =0,     ///< 单电池方案
    OVD_LOCKBATTERYCAP_PRIMARY_BACKUP   =1,     ///< 主备电
    OVD_LOCKBATTERYCAP_DUAL_POWER       =2,     ///< 双供电
    OVD_LOCKBATTERYCAP_OTHER,
}ovd_lockbattery_cap_t;

/**
 * @brief 门锁品类
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_dword unlockway;                ///< 门锁支持的开锁方式，用比特位掩码表示支持方式，掩码映射表参考 OVD_UNLOCKWAYCAP_XXX
    ovd_lockbattery_cap_t battery;      ///< 门锁电池方案
    OVD_bool  usermanage;               ///< 是否支持成员管理
    OVD_bool  tmppwd;                   ///< 是否支持临时密码
} ovd_lock_cap_params_t;

/**
 * @brief 区域入侵参数
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_int32 support_set_target;   ///< 位运算0x0F。设备是否支持设置检测目标。
                                    ///< 最低位表示"人形" 0x01，
                                    ///< 第二位表示"机动车" 0x02，
                                    ///< 第三位表示"非机动车" 0x04，
                                    ///< 第四位表示"其他" 0x08，默认0表示不支持
    OVD_bool support_staymode_time; ///< 是否支持逗留时长设置
    OVD_bool support_set_alerttime; ///< 是否支持警戒时间段设置
    OVD_int32 alarms_zone;    // 0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
    OVD_int32 alertarea_num; // <1.41.0新增,整形,设备支持警戒区域数,默认值为1
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false
} ovd_alertarea_params_t;

/**
 * @brief 越界侦测参数
 * @ingroup 能力集
 */
typedef struct
{
    OVD_int32 support_set_target;   ///< 位运算0x0F。设备是否支持设置检测目标。
                                    ///< 最低位表示"人形" 0x01，
                                    ///< 第二位表示"机动车" 0x02，
                                    ///< 第三位表示"非机动车" 0x04，
                                    ///< 第四位表示"其他" 0x08，默认0表示不支持
    OVD_bool support_set_alerttime;
    OVD_bool support_set_statistics_line;
    OVD_int32 statistics_line_another_num;  ///< 1.49.0新增,整形,设备支持设置额外越界规则线（第二条及以上规则线）数量,默认值为0
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，是否支持<联动警灯>联动策略的持续告警，默认值为false, 
} ovd_transgression_params_t;

/**
 * @brief wifi感知参数
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_set_schedule_time;     ///< 布尔型：是否支持设置时间段,默认值为false
    OVD_bool support_set_sensing_mode;      ///< 布尔型：是否支持设置感知模式,默认值为false
} ovd_wifi_sensing_params_t;

/**
 * @brief 告警相关能力集
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool have_alarms_io;   				 ///< 布尔型：是否支持外包报警,默认值为false,
    OVD_bool have_alarms_face; 				 ///< 布尔型：是否支持人脸侦测,默认值为false,
    OVD_int32 support_alarms_face_zone; 	 ///< 整型:0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可

    OVD_bool have_alarms_cry;                             ///< 布尔型：是否支持哭声侦测,默认值为false,
    OVD_bool support_crying_pacify;                       ///< 布尔型：是否啼哭安抚功能,
    OVD_int32 crying_pacify_audio_playing_count;          ///< 整型：啼哭安抚音频播放次数的最大值限制,
    OVD_bool support_crying_pacify_audio_playing_volumn;  ///< 布尔型：是否支持设置啼哭安抚音频播放音量,
    OVD_int32 crying_pacify_audio_playing_type;           ///< 整型：支持设置啼哭安抚音频的方式,
    OVD_bool support_crying_pacify_detect_on;             ///< 是否支持啼哭安抚检测开关是否可配置
    OVD_bool support_crying_pacify_sense;                 ///< 是否支持啼哭安抚灵敏度调节
    OVD_bool support_cry_appease_music_selected;          ///< 是否支持安防歌曲可自由选择
    OVD_bool support_cry_appease_music_schedule;          ///< 是否支持安抚时间段设置

    OVD_bool have_alarms_voice; 			 ///< 布尔型：是否支持声音侦测,默认值为false
    OVD_int32 alarm_voice_detect_stand;                   ///声音侦测标准，1代表厂商根据档位自行调整，2代表按照杭研档位要求
    OVD_bool have_alarms_motion; 			 ///< 布尔型：是否支持移动侦测,默认值为false
    OVD_int32 alarm_motion_detect_stand;         ///移动侦测标准，1代表厂商根据档位自行调整，2代表厂商严格按照杭研档位要求设置
    OVD_int32 support_alarms_motion_zone;    ///< 整型:0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
    OVD_bool have_alarms_body;  			 ///< 布尔型：是否支持人形侦测,默认值为false
    OVD_int32 support_alarms_body_zone; 	 ///< 整型:0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
    OVD_bool have_alarms_cross;				 ///< 布尔型：是否支持拌网侦测
    OVD_bool have_alarms_pir; 				 ///< 布尔型，是否支持pir侦测
    OVD_bool have_alarms_pir_staymode;		 ///< 布尔型，是否支持pir逗留模式,门铃
    OVD_bool have_alarms_lossLock; 			 ///< 布尔型，是否支持撬锁侦测
    OVD_bool have_alarms_alertarea;          ///< 布尔型，是否支持警戒功能
    OVD_bool have_alertarea_expel;           ///< 布尔型，是否支持智能驱离功能（针对带云台的警戒设备）,1.34新增
    OVD_int32 have_alarms_alertarea_zone;    ///< 整型:0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
    ovd_alertarea_params_t alertarea_params; ///< 1.33新增，区域入侵参数
    OVD_bool have_alarms_transgression;      ///< 1.33 新增 布尔型：是否支持越界侦测功能，默认值为false
    ovd_transgression_params_t transgression_params; ///< 1.33新增，越界侦测参数
    OVD_bool have_alarms_pet;                ///< 1.57  布尔型，是否支持宠物侦测,默认值为false,
    OVD_int32 support_alarms_pet_zone;                  ///< 1.57  整形  0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
}OVDAlarmsInfo;

/**
 * @brief V2 版本告警相关能力集
 * @note 通过 \ref OVD_CapGetV2 接口获取到该能力集，然后根据实际情况修改
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_stop_alarm;                          ///< 是否支持消控能力，默认值为false

    OVD_bool support_alarms_io;                           ///< 是否支持外包报警,默认值为false,

    OVD_bool support_alarms_face;                         ///< 是否支持人脸侦测,默认值为false,
    OVD_int32 alarms_face_zone;                           ///< 0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可

    OVD_bool support_alarms_voice;                        ///< 是否支持声音侦测,默认值为false,
    OVD_int32 alarm_voice_detect_stand;                   ///< 声音侦测标准，1代表厂商根据档位自行调整，2代表按照杭研档位要求
    OVD_bool support_alarms_cry;                          ///< 是否支持哭声侦测,默认值为false,
    OVD_bool support_crying_pacify;                       ///< 是否啼哭安抚功能,
    OVD_bool support_crying_pacify_audio_playing_volumn;  ///< 是否支持设置啼哭安抚音频播放音量,
    OVD_int32 crying_pacify_audio_playing_count;          ///< 啼哭安抚音频播放次数的最大值限制,
    OVD_int32 crying_pacify_audio_playing_type;           ///< 支持设置啼哭安抚音频的方式,
    OVD_bool support_crying_pacify_detect_on;             ///< 是否支持啼哭安抚检测开关是否可配置
    OVD_bool support_crying_pacify_sense;                 ///< 是否支持啼哭安抚灵敏度调节
    OVD_bool support_cry_appease_music_selected;          ///< 是否支持安防歌曲可自由选择
    OVD_bool support_cry_appease_music_schedule;          ///< 是否支持安抚时间段设置   

    OVD_bool support_alarms_motion;              ///< 是否支持移动侦测,默认值为false,
    OVD_int32 alarm_motion_detect_stand;         ///< 移动侦测标准，1代表厂商根据档位自行调整，2代表厂商严格按照杭研档位要求设置
    OVD_int32 alarms_motion_zone;                ///< 0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可

    OVD_bool support_alarms_body;                ///< 是否支持人形侦测,默认值为false,
    OVD_int32 alarms_body_zone;                  ///< 0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可

    OVD_bool support_alarms_cross;               ///< 是否支持拌网侦测,
    OVD_bool support_alarms_pir;                 ///< 是否支持pir侦测,
    OVD_bool support_alarms_pir_staymode;        ///< 是否支持pir逗留模式,门铃
    OVD_bool support_alarms_lossLock;            ///< 是否支持撬锁侦测
    OVD_bool support_alertarea_expel;            ///< 是否支持智能驱离功能（针对带云台的警戒设备）,1.34新增

    OVD_bool support_alarms_alertarea;           ///< 布尔型，是否支持警戒功能
    OVD_int32 support_alarms_alertarea_zone;    ///< 整型:0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
    OVD_bool support_speech_keep_alarm;         ///< 布尔型，警戒功能是否支持<语音输出>联动策略的持续告警，默认值为false
    OVD_bool support_speech_set_interval;       ///< 布尔型，警戒功能是否支持<语音输出>联动策略的告警间隔设置，默认值为false
    OVD_bool support_light_keep_alarm;          ///< 布尔型，警戒功能是否支持<联动警灯>联动策略的持续告警，默认值为false

    ovd_alertarea_params_t alertarea_params;     ///< 1.33 新增，区域入侵参数

    OVD_bool support_alarms_transgression;       ///< 1.33 新增 是否支持越界侦测功能，默认值为false
    ovd_transgression_params_t transgression_params; ///< 1.33新增，越界侦测参数

    OVD_bool support_alarms_wifi_sensing;          ///< 1.55 是否支持wifi感知功能，默认值为false
    ovd_wifi_sensing_params_t wifi_sensing_params; ///< 1.55 新增，wifi感知参数

    OVD_bool support_3d_light_sensor;             ///< 1.55.1 是否支持3D光传感器，默认值为false

    OVD_bool support_alarms_pet;                ///< 1.57  布尔型，是否支持宠物侦测,默认值为false,
    OVD_int32 alarms_pet_zone;                  ///< 1.57  整形  0不支持，1：只支持矩形，n(n>=3):支持不规则n边型, 注：目前app限制，只需要设备支持矩形即可
}OVD_CapInfoAlarm;



/**
 * @brief 卡回看倍速播放枚举值
*/
typedef enum
{
    OVD_PLAYBACK_SPEED_INVALID = 0,     ///< 无效倍速播放
    OVD_PLAYBACK_SPEED_QUARTER = 1,     ///< 1/4倍速播放
    OVD_PLAYBACK_SPEED_HALF = 2,        ///< 1/2倍速播放
    OVD_PLAYBACK_SPEED_THREEQUART = 3,  ///< 3/4倍速播放
    OVD_PLAYBACK_SPEED_1TIMES = 4,      ///< 1倍速播放
    OVD_PLAYBACK_SPEED_2TIMES = 5,      ///< 2倍速播放
    OVD_PLAYBACK_SPEED_4TIMES = 6,      ///< 4倍速播放
    OVD_PLAYBACK_SPEED_8TIMES = 7,      ///< 8倍速播放
    OVD_PLAYBACK_SPEED_16TIMES = 8,     ///< 16倍速播放
    OVD_PLAYBACK_SPEED_MAX_TIMES,       ///< 最大倍速播放
}ovd_playback_speed_times_e;

/**
 * @brief 基础能力集定义 BASE
 * @see OVD_CAP_TYPE_BASE
 * @ingroup 能力集
 * @note 如果厂家不使用杭研提供的firmware_encrypt来加密固件，那么support_ota_without_verify_header应当填 OVD_TRUE
 */
typedef struct
{
    OVD_bool support_auto_reboot;   ///< 是否支持自动重启 \ref OVD_CAP_TYPE_BASE
    ovd_media_encrypt_type_e stream_encryption_mode;    ///< 码流加密模式
    OVD_bool support_ota_without_verify_header;///<固件升级时，是否支持不带128字节校验头的固件包
}OVD_CapInfoBase;

/**
 * @brief 低功耗能力集 Low power
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_lowpower_mode;     ///< 是否支持低功耗相关功能,1.25版本新增
    OVD_bool support_awaked;            ///< 是否支持低功耗唤醒，默认值为false,不支持低功耗唤醒 1.21版本新增
    OVD_bool support_awaked_switch;     ///< 是否支持低功耗唤醒开关,1.25版本新增
    OVD_bool support_smart_mode;        ///< 是否智能模式,1.25版本新增
    OVD_bool support_constant_power;    ///< 是否支持常电工作模式,1.56版本新增
}OVD_CapInfoLowPower;

/**
 * @brief 通用摄像头能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_switch;               ///< 视频通道是否支持休眠，默认值为false
    OVD_bool support_timed_dormancy;       ///< 是否支持定时休眠开关，默认值为false
    ovd_night_vison_mode_e nightvision_mode; ///< 支持切换夜视模式，默认值 OVD_NIGHTVISION_MODE_NONE
    OVD_dword ai_nightvision_mask;      ///< 智能夜视模式可支持的侦测类型掩码，比特位为1表示支持,共64位。参考 OVD_NIGHTVISION_DETECT_XXX，默认值0x0000000000000000
    OVD_bool support_set_fill_light;             ///< 1.55-是否支持补光灯控制，默认值为false
    OVD_bool support_set_fill_light_brightness;    ///< 1.55-是否支持补光灯亮度控制，默认值为false
    OVD_bool support_set_fill_light_schedule_time; ///< 1.55-是否支持补光灯计划时间，默认值为false
    OVD_bool support_set_fill_light_auto;         ///< 1.55-是否支持补光灯自动模式，默认值为false
    OVD_dword card_playback_speed;      ///< 卡回看播放速度能力掩码，参考 OVD_CARD_PLAYBACK_SPEED_MASK_XXX
    OVD_bool AIGC_cloud_record;         ///< 是否支持AI云存

    OVD_bool support_sd;                   ///< 是否支持SD卡/TF卡，默认值为true
    OVD_bool support_led;                  ///< 是否支持led灯开关，默认值为false
    OVD_bool support_sdcapacity;           ///< 是否支持存储卡容量信息获取，默认值为false
    OVD_bool support_DN_mode;              ///< 是否支持配网模式上报，默认不支持，默认值为false，配网模式详见OVD_DNM_e
}OVD_CapInfoCamera;

/**
 * @brief 云台能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_ptz;               ///< 是否支持云台控制能力,默认值为false
    OVD_bool support_ptz_preset;        ///< 是否支持云台预置位,默认值为false
    OVD_bool support_trace;             ///< 是否支持移动跟踪,默认值为false
    OVD_int32 support_set_target;       ///< 位运算0x0F。设备是否支持设置检测目标。
                                        ///< 最低位表示"人形" 0x01，
                                        ///< 第二位表示"宠物" 0x04，默认0表示不支持设置追踪目标
    OVD_bool support_zoom_control;      ///< 是否支持用户手动调节倍率，默认值为false
    OVD_bool support_focus_control;     ///< 是否支持用户手动调节焦距，默认值为false
    OVD_bool support_doorbell_preset;   ///< 是否支持设置门铃机位，默认值为false
}OVD_CapInfoPtz;

/**
 * @brief 枪球联动能力集
 * @ingroup 能力集
 *
 */
typedef struct {
    OVD_bool support_smart;   ///< 是否支持智能枪球联动，默认值为false
    OVD_bool support_manual;  ///< 是否支持手动枪球联动，默认值为false
    OVD_bool support_fix;     ///< 是否支持枪球联动矫正，默认值为false
} OVD_CapInfoLinkageTracking;

/**
 * @brief 特殊业务能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_4G;             ///< 是否支持4G,1.25版本新增
    OVD_bool support_screen;         ///< 是否支持带屏,默认值为false
    OVD_bool support_smoke_sensor;           ///< 是否支持四合一烟雾传感器功能，默认不支持
    OVD_bool support_battery;           ///< 是否电池供电,主要用于门铃设备,默认值为false,
    OVD_bool support_light_supplement_lamp;  ///< 是否支持补光灯进行照明功能用,默认值为false
    OVD_dword camera_lens_type;      ///< 1.34.0新增，默认 OVD_CAMERA_LENS_TYPE_SINGLE
    OVD_dword robot_house_keeper;    ///< 1.33新增，设备是否支持机器人管家相关功能，默认 OVD_ROBOT_HOUSE_KEEPER_NONE
    OVD_bool support_hjkh;               ///< 设备是否集成了和家固话，默认值为false
    OVDHJKH_mode hjkh_mode;             ///< 设备和家看护相关的功能
}OVD_CapInfoSpecial;

/**
 * @brief 行车记录仪
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_dashcam;        ///< 是否支持行车记录仪功能，默认不支持
    OVD_bool support_dashcam_findcar;        ///< 1.45.0-布尔型： 是否支持智能寻车，默认值为false>
    OVD_bool support_dashcam_rescue;        ///< 1.45.0-布尔型： 是否支持智能救援，默认值为false>

}OVD_CapInfoDashCam;

/**
 * @brief 庭院灯
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_yard_light_brightness;         ///< 是否支持庭院灯灯光控制,默认false
    OVD_bool support_yard_light_manual_control;     ///< 是否支持手动控制,默认false
    OVD_int32 yard_light_timed_schedule;            ///< 是否支持计划控制，0为不支持，非0数字表示支持几个计划控制时间段，默认为2，最大值为5
}OVD_CapInfoYardLight;

/**
  * @brief 门铃能力集
 * @ingroup 能力集
*/
typedef struct
{
    OVD_bool have_doorbell_volume;///<是否支持门铃音量调节
}OVD_CapInfoDoorbell;

/**
 * @brief 视频能力集 V2
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_multi_stream;       ///< 是否支持主子码流功能，默认值为false 1.30版本新增
    OVD_bool support_set_video_formats;  ///< 是否支持平台切换视频编码格式
    OVD_bool support_static_scence_ABR;  ///< 静态场景码率自适应，详细要求参考入库规范相关功能说明，默认值为不支持
    OVD_bool support_set_bitrate;        ///< 是否支持码率设置，默认值为false
    OVD_bool support_set_framerate;      ///< 是否支持帧率设置，默认值为false
    OVD_bool support_set_gop;            ///< 是否支持设置gop，默认值为false
    OVD_dword video_quality;             ///< 支持的视频清晰度 默认 OVD_VIDEO_QUALITY_DEFAULT
    OVD_dword video_codec;               ///< 默认支持 OVD_VIDEO_CODEC_H264

    OVD_bool support_osd_text;           ///< 是否支持持文本水印
    OVD_bool support_osd_logo;           ///< 是否支持水印图片
    OVD_bool support_set_AI_rules_mode;  ///< 是否支持叠加智能规则信息显示,默认值false 1.40.0新增
    OVD_bool support_aov;                ///< 是否支持AOV功能,默认值为false 1.48新增

    OVD_dword resolution_supportlists;  ///< 1.53新增-支持的视频分辨率，可选值为：640*360、960*540、1280*720、1920*1080、2304*1296、2560*1440、2560*1920、3840*2160，至少要支持标清和高清中的任意一种分辨率
    OVD_dword framerate_supportlists;   ///< 1.53新增-支持的视频帧率，可选值为：15、20、25、30，默认15帧
    OVD_dword GOP_supportlists;         ///< 1.53新增-支持的GOP时长，可选值为：2~6秒，默认4秒

}OVD_CapInfoVideo;

/**
 * @brief 音频能力集 V2
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_audio_out;                 ///< 是否支持音乐输出,默认值为false
    OVD_bool support_voice_out;                 ///< 是否支持对讲输出,默认值为false
    OVD_bool support_voiceout_volume;           ///< 是否支持对讲音量调节，默认值为false
    OVD_dword audiosamplerate;                  ///< 音频采样率，默认 OVD_AUDIO_SAMPLE_RATE_8K
}OVD_CapInfoAudio;

/**
 * @brief 监控音视频传输协议类型枚举
 *
 */
enum DME_media_protocol_type
{
    DME_MEDIA_PROTOCOL_RTMP = 0,    ///< RTMP
    DME_MEDIA_PROTOCOL_SRT = 1      ///< SRT
};

/**
 * @brief 对讲输协议类型枚举
 *
 */
enum DME_talk_protocol_type
{
    DME_TALK_PROTOCOL_P2P = 0,  ///< P2P
    DME_TALK_PROTOCOL_SRT = 1,  ///< SRT
    DME_TALK_PROTOCOL_CCRTC = 2 ///< CCRCT
};

/**
 * @brief 卡回看传输协议类型枚举
 *
 */
enum DME_replay_protocol_type
{
    DME_REPLAY_PROTOCOL_P2P = 0,    ///< P2P协议
    DME_REPLAY_PROTOCOL_SRT = 1,    ///< SRT
    DME_REPLAY_PROTOCOL_CCRTC = 2   ///< CCRCT
};

#define OVD_EXTENSION_SUPPORT_ECHO_TEST  0x0001 ///< echotest
#define OVD_EXTENSION_SUPPORT_ECHO_CCRTC 0x0002 ///< ccrtc

#define OVD_AI_FUNCTION_FACE                0x00000001      ///< 人脸检测
#define OVD_AI_FUNCTION_KITCHEN             0x00000002      ///< 阳光厨房
#define OVD_AI_FUNCTION_VEHICLE             0x00000004      ///< 机动车检测
#define OVD_AI_FUNCTION_NON_MOTOR           0x00000008      ///< 非机动车检测
#define OVD_AI_FUNCTION_PASSENGER           0x00000010      ///< 客流统计
#define OVD_AI_FUNCTION_REGIONAL_PEOPLE     0x00000020      ///< 区域人数统计
#define OVD_AI_FUNCTION_PARABOLIC_AERIAL    0x00000040      ///< 高空抛物
#define OVD_AI_FUNCTION_LANE_LINE           0x00000080      ///< 车道线检测
#define OVD_AI_FUNCTION_OFF_DUTY            0x00000100      ///< 离岗检测
#define OVD_AI_FUNCTION_HELMET_WEARING      0x00000200      ///< 头盔佩戴
#define OVD_AI_FUNCTION_GESTURE_RECOGNITION 0x00000400      ///< 手势识别
#define OVD_AI_FUNCTION_SMOKE_FIRE          0x00000800      ///< 烟火检测
#define OVD_AI_FUNCTION_FAMILY_RECOGNITION  0x00001000      ///< 家人识别
#define OVD_AI_FUNCTION_PRIVACY_MASKING     0x00002000      ///< 隐私遮蔽
#define OVD_AI_FUNCTION_FALL_DETECTION      0x00004000      ///< 跌倒检测
#define OVD_AI_FUNCTION_SLEEP_MONITOR       0x00008000      ///< 睡眠检测 sleep_monitor
#define OVD_AI_FUNCTION_EXPRESSION_RECOGNITION 0x00010000      ///< 表情识别

#define OVD_PRODUCT_FUNCTION_LOWPOWER       0x00000001      ///< 低功耗功能
#define OVD_PRODUCT_FUNCTION_DASHCAM        0x00000002      ///< 行车记录仪功能
#define OVD_PRODUCT_FUNCTION_ROBOT          0x00000004      ///< 机器人功能
#define OVD_PRODUCT_FUNCTION_MOBOCAM        0x00000008      ///< 可移动摄像头功能

/**
 * @brief 获取SDK能力集时需要设置的参数
 * @ingroup 能力集
 */
typedef struct ovd_cap_params_s
{
    OVD_int32 max_channel;      ///< 设备最大支持的视频通道个数
    OVD_dword product_function; ///< 通用功能掩码 \ref OVD_PRODUCT_FUNCTION_LOWPOWER
    OVD_dword ai_function;      ///< 设备支持的智能功能掩码, \ref OVD_AI_FUNCTION_FACE
}OVD_cap_params_t;

/**
 * @brief SDK内部能力集
 * @ingroup 能力集
 */
typedef struct
{
    OVD_bool support_cloud_record;                  ///< 是否支持云存
    OVD_int32  support_netprobe_cap;                 ///< 是否支持网络探测能力，默认值为0
    OVD_int32 support_auto_logupload;               ///< 是否支持自动上传日志，默认值为0
    OVD_bool support_p2p_jckd;                      ///< 是否启用p2p jckd方案,默认值为true
    enum DME_media_protocol_type media_protocol;    ///< 流媒体传输协议，目前可选值为：rtmp、srt,非厂商填
    enum DME_talk_protocol_type talk_protocol;      ///< 语音对讲使用协议，目前可选值为：p2p,srt,ccrpc，默认值为p2p;ccrpc为固话方案
    enum DME_replay_protocol_type replay_protocol;  ///< 前端回放所使用协议，目前可选值为：p2p,srt,ccrtc,默认值为p2p;当前前端回放为卡回放，ccrtc为固话方案

    OVD_bool support_ivrs;                          ///< 是否支持ivrs协议，默认值为false，表示云存只支持ivr协议,
    OVD_bool support_pic_upload_https;              ///< 截图上传是否支持HTTPS，若不支持，则OVC下发的截图指令中的http URL为http协议，默认值为false,
    OVD_bool support_log_upload_https;              ///< 日志文件上传是否支持HTTPS，若不支持，则OVC下发的日志上传指令中的http URL为http协议，默认值为false,
    OVD_bool support_firmware_download_https;       ///< 固件下载是否支持HTTPS，若不支持，则OVC下发的升级指令中的http URL为http协议，默认值为false,
    OVD_bool support_support_softprobe;             ///< 是否支持软探针模块，默认值为false
    OVD_bool support_video_encrypt;                 ///< 支持视频加密
    OVD_dword support_extension;                    ///< 支持的extension方法列表，参考 OVD_EXTENSION_SUPPORT_ECHO_XXX
    OVD_bool support_change_alarm_max_duration;     ///<是否支持调整状态告警的最长时长>
    OVD_bool support_softflow;      ///<是否支持网络安全监测功能>
}OVD_CapInfoSdk;

/**
 * @brief 门锁能力集
 *
 */
typedef ovd_lock_cap_params_t OVD_CapInfoDoorLock;

/**
 * @brief 机器人能力集
 * @ingroup 能力集
 *
 */
typedef struct
{
    OVD_bool support_EBO;       ///< 设备是否支持机器人指令，默认值为false
    OVD_bool support_move;      ///< 是否支持左右前后移动
    OVD_bool support_nofall;    ///< 是否支持防跌落
    OVD_bool support_laser;     ///< 是否支持激光
    OVD_bool support_onrush;    ///< 是否支持向前猛冲
    OVD_bool support_recharge;  ///< 是否支持回充
    OVD_bool support_shake;     ///< 是否支持抖动
    OVD_bool support_spin;      ///< 是否支持转圈
    OVD_bool support_track;     ///< 是否支持移动追踪，开启后  默认值是： 人形追踪 1.27新增
    OVD_bool support_body_track;    ///< 是否支持人形追踪追踪，开启后支持人形追踪 1.27新增
    OVD_bool support_pets_track;    ///< 是否支持宠物追踪追踪，开启后支持宠物追踪 1.27新增
    OVD_bool support_cruise;        ///< 是否支持巡航功能，开启后支持巡航 1.29新增
    OVD_bool support_collision;     ///< 是否支持防碰撞功能，开启后支持防碰撞 1.29新增
    OVD_bool support_reminder;      ///< 是否支持提示音功能，开启后支持提示音 1.34新增
    OVD_bool support_speed;         ///< 是否支持速度调节，开启后速度调节 1.40新增
    OVD_bool support_petfeeder;     ///< 是否支持众鼎宠物喂食器1.29版本新增
    OVD_char robot_ctrl[MAX_LEN_32];///< 字符串类型，支持的机器人控制类型
    OVD_int32 petfeeder_plan_num;   ///< int型，数量为支持的最大计划条数，最大值为10
    OVD_int32 petfeeder_feed_num;   ///< int型，数量为支持的喂食份数，值必须大于0

}OVD_CapInfoRobot;

typedef struct
{
    OVD_bool support_interaction;    ///< 是否支持互动类指令，默认值为false
    OVD_bool support_onrush;         ///< 布尔型，true支持像前猛冲；false不支持, 默认false
    OVD_bool support_shake;          ///< 布尔型，true支持抖动；false不支持, 默认false
    OVD_bool support_nod;            ///布尔型，true支持点头；false不支持, 默认false
    OVD_bool support_turnACW;        ///布尔型，true支持逆时针转圈；false不支持, 默认false
    OVD_bool support_turnCW;      ///布尔型，true支持顺时针转圈；false不支持, 默认false
    OVD_bool support_figure_8;      ///布尔型，true支持8字绕行；false不支持, 默认false
    OVD_bool support_figure_Z;      ///布尔型，true支持Z字移动；false不支持, 默认false
    OVD_bool support_figure_S;      ///布尔型，true支持S型移动；false不支持, 默认false
    OVD_bool support_spin;      ///布尔型，true支持原地旋转；false不支持, 默认false
    OVD_bool support_abrupt;      ///布尔型，true支持急转；false不支持, 默认false
    OVD_bool support_waddle;      ///布尔型，true支持摇摆前进；false不支持, 默认false
    OVD_bool support_backpedal;      ///布尔型，true支持摇摆后退；false不支持, 默认false

    OVD_bool support_move;      ///< 是否支持左右前后移动， 默认true
    OVD_bool support_motioncal;  ///< 是否支持运动校准，默认false
    OVD_int32 support_speedswitch;         ///< 支持速度调节档位
    OVD_bool support_petTrack;  ///< 是否支持自动寻宠，默认false
    OVD_bool support_recharge;  ///< 是否支持回充，默认false
    OVD_bool support_escape;  ///< 是否支持脱困，默认false
    OVD_bool support_laser_on;  ///< 是否支持激光开关，默认false
    OVD_bool support_nofall_on;  ///< 是否支持防跌落开关，默认false
    OVD_bool support_collision_on;  ///< 是否支持防碰撞开关，默认false
    OVD_bool support_reminder_on;  ///< 是否支持提示音开关，默认false


    OVD_bool support_cruise;       ///< 设备是否支持自动巡航，true支持/false不支持, 默认false
    OVD_bool support_cruiseroute;  ///< 是否支持巡航路径保存,true支持/false不支持, 默认false
    OVD_bool support_cruisemanual;  ///< 是否支持手动巡航,true支持/false不支持, 默认false
    OVD_bool support_cruisetime;  ///< 是否支持巡航轨迹时间设置,true支持/false不支持, 默认false
    OVD_bool support_cruiserepeat;  ///< 是否支持巡航轨迹周期设置,true支持/false不支持, 默认false

    OVD_bool support_track;    ///< 设备是否支持自动追踪功能， 默认false
    OVD_bool support_track_on;   ///< 设备是否支持自动追踪开关， 默认false
    OVD_int32 support_track_type; ///< 设备支持自动追踪的类型,位运算表示,0x01表示人形,0x02表示宠物追踪

}OVD_CapInfoMobocam;

/**
 * @brief 能力集类型
 * @ingroup 能力集
 *
 */
typedef enum
{
    OVD_CAP_TYPE_CAMERA = 1,        ///< \ref OVD_CapInfoCamera
    OVD_CAP_TYPE_ROBOT,             ///< \ref OVD_CapInfoRobot
    OVD_CAP_TYPE_DASH_CAM,          ///< \ref OVD_CapInfoDashCam
    OVD_CAP_TYPE_DOORLOCK,          ///< \ref OVD_CapInfoDoorLock
    OVD_CAP_TYPE_DOORBELL,          ///< \ref OVD_CapInfoDoorbell
    OVD_CAP_TYPE_YARDLIGHT,         ///< \ref OVD_CapInfoYardLight
    OVD_CAP_TYPE_MOBOCAM,             ///< \ref OVD_CapInfoMobocam

    OVD_CAP_TYPE_BASE = 1000,       ///< \ref OVD_CapInfoBase
    OVD_CAP_TYPE_SDK,               ///< \ref OVD_CapInfoSdk
    OVD_CAP_TYPE_PTZ,               ///< \ref OVD_CapInfoPtz
    OVD_CAP_TYPE_LINKAGE_TRACKING,  ///< \ref OVD_CapInfoLinkageTracking
    OVD_CAP_TYPE_SPECIAL,           ///< \ref OVD_CapInfoSpecial
    OVD_CAP_TYPE_LOW_POWER,         ///< \ref OVD_CapInfoLowPower
    OVD_CAP_TYPE_IPC_VIDEO,         ///< \ref OVD_CapInfoVideo
    OVD_CAP_TYPE_IPC_AUDIO,         ///< \ref OVD_CapInfoAudio
    OVD_CAP_TYPE_IPC_ALARM,         ///< \ref OVD_CapInfoAlarm
    OVD_CAP_TYPE_IPC_LINKAGE,       ///< \ref OVD_CapInfoLinkage

    OVD_CAP_TYPE_IPC_AI_FACE = 2000,            ///< \ref OVDAIFaceCapInfo
    OVD_CAP_TYPE_IPC_AI_KITCHEN,                ///< \ref OVDAIKitchenCapInfo
    OVD_CAP_TYPE_IPC_AI_VEHICLE,                ///< \ref OVDAIVehicleCapInfo
    OVD_CAP_TYPE_IPC_AI_NON_MOTOR,              ///< \ref OVDAINonmotorvehicleCapInfo
    OVD_CAP_TYPE_IPC_AI_PASSENGER,              ///< \ref OVDAIPassengerCapInfo
    OVD_CAP_TYPE_IPC_AI_REGIONAL_PEOPLE,        ///< \ref OVDAIRegionalPeopleStatCapInfo
    OVD_CAP_TYPE_IPC_AI_PARABOLIC_AERIAL,       ///< \ref OVDAIParabolicAerialCapInfo
    OVD_CAP_TYPE_IPC_AI_LANE_LINE,              ///< \ref OVDAILaneLineCapInfo
    OVD_CAP_TYPE_IPC_AI_OFF_DUTY,               ///< \ref OVDAIOffDutyCapInfo
    OVD_CAP_TYPE_IPC_AI_HELMET_WEARING,         ///< \ref OVDAIHelmetWearningCapInfo
    OVD_CAP_TYPE_IPC_AI_GESTURE_RECOGNITION,    ///< \ref OVDAIGestureRecognitionCapInfo
    OVD_CAP_TYPE_IPC_AI_SMOKE_FIRE,             ///< \ref OVDAISmokeFireCapInfo
    OVD_CAP_TYPE_IPC_AI_FAMILY_RECOGNITION,     ///< \ref OVDAIFamilyRecognitionCapInfo
    OVD_CAP_TYPE_IPC_AI_PRIVACY_MASKING,        ///< \ref OVDAIPrivacyMaskingCapInfo
    OVD_CAP_TYPE_IPC_AI_FALL_DETECTION,         ///< \ref OVDAIFallDetectionCapInfo
    OVD_CAP_TYPE_IPC_AI_SLEEP_MONITOR,          ///< \ref OVDAISleepMonitorCapInfo
    OVD_CAP_TYPE_IPC_CLOUDAI,                   ///< \ref OVDCloudAICapInfo

    OVD_CAP_TYPE_IPC_AI_SPEECH,                 ///< \ref OVDAISpeechCapInfo
    OVD_CAP_TYPE_IPC_LULLBY,                    ///< \ref OVDLullbyCapInfo 

    OVD_CAP_TYPE_IPC_AI_DIY,                    ///< \ref OVDAIDIYCapInfo
    OVD_CAP_TYPE_IPC_AI_EXPRESSION_RECOGNITION,  ///< \ref OVDExpressionRecognitionCapInfo

}ovd_capability_type_e;

/**
 * @brief 能力集抽象定义
 * @ingroup 能力集
 */
typedef struct OVDCapItem_s
{
    ovd_capability_type_e cap_type;     ///< 能力集类型
    OVD_int32 channel;                  ///< 视频通道号
    OVD_int32 size;                     ///< cap_data 缓冲区的大小
    void* cap_data;                     ///< cap_type 为不同的能力集类型时，该指针指向不同的数据类型
}OVDCapItem_t;

/**
 * @brief V2版本能力集返回结果定义
 * @ingroup 能力集
 */
typedef struct OVDCapInfoV2_s
{
    OVDCapItem_t **cap; ///< 能力集数组
    OVD_uint32 count;   ///< 能力集数量
}OVDCapInfoV2_t;

/**
 * @brief V1版本能力集定义
 * @ingroup 能力集
 * @deprecated 建议用V2版本
 */
typedef struct OVDCapInfo_s
{
    ovd_device_type_e device_type;
    OVD_int32 audiosamplerate; ///< 枚举整型，0（默认值）：代表8K， 1：代表16K, 1.27版本新增
    OVD_bool enable_low;/// <是否支持低功耗相关功能,1.25版本新增
    OVD_bool support_awaked;///< 是否支持低功耗唤醒，默认值为false,不支持低功耗唤醒 1.21版本新增
    OVD_bool support_awaked_switch;///< 是否支持低功耗唤醒开关,1.25版本新增
    OVD_bool support_constant_power;///< 是否支持常电模式,1.56版本新增
    OVD_bool support_4G;///< 是否支持4G,1.25版本新增
    OVD_bool smart_mode;///< 是否智能模式,1.25版本新增
    OVD_bool have_ptz;          ///< 是否支持云台控制能力,默认值为false
    OVD_bool support_ptz_preset;///< 是否支持云台预置位,默认值为false
    OVD_bool stopalarm; ///< 是否支持消控能力，默认值为false
    OVD_bool zoomcontrol; ///< 是否支持用户手动调节倍率，默认值为false
    OVD_bool focuscontrol; ///< 是否支持用户手动调节焦距，默认值为false

    OVD_bool multi_stream; ///< 是否支持主子码流功能，默认值为false 1.30版本新增

    OVD_bool have_battery;      ///< 是否电池供电,主要用于门铃设备,默认值为false,
    OVD_bool have_audio_out;    ///< 是否支持音乐输出,默认值为false
    OVD_bool have_voice_out;    ///< 是否支持对讲输出,默认值为false
    OVD_bool have_ivrs;         ///< 是否支持ivrs协议，默认值为false，表示云存只支持ivr协议,
    OVD_bool have_trace;        ///< 是否支持移动跟踪,默认值为false

    OVDAICapInfo  ovdAICapInfo;      ///< AI相关
    OVDCloudAICapInfo ovdCloudAICapInfo;    ///< CloudAI相关
    OVDAISpeechCapInfo ovdAISpeechCapinfo;  ///< AI语音对讲相关
    OVDLullbyCapInfo ovdLullbyCapInfo; ///< 童谣相关

    OVDAlarmsInfo ovdCapInfo_alarms; ///< alarms 告警上报相关能力
    OVDlinkage_mode ovdlinkage_mode; ///< 联动方式的支持

    OVD_bool log_upload_https;       ///< 日志文件上传是否支持HTTPS，若不支持，则OVC下发的日志上传指令中的http URL为http协议，默认值为false,
    OVD_bool firmware_download_https;///< 固件下载是否支持HTTPS，若不支持，则OVC下发的升级指令中的http URL为http协议，默认值为false,
    OVD_bool video_encrypt;          ///< 是否支持视频加密，默认值为false

    OVD_bool have_switch;            ///< 是否支持通道使能开关，默认值为false
    OVD_bool have_sd;                ///< 是否支持SD卡/TF卡，默认值为true
    OVD_int32 card_playback_speed;   ///< 卡回看播放速度能力掩码，参考 OVD_CARD_PLAYBACK_SPEED_MASK
    OVD_bool have_sdcapacity;        ///< 是否支持存储卡容量信息获取，默认值为false
    OVD_bool have_led;               ///< 是否支持led灯开关，默认值为false
    OVD_bool light_supplement_lamp;  ///< 是否支持补光灯进行照明功能用,默认值为false
    OVD_bool have_screen;            ///< 是否支持带屏,默认值为false
    OVD_bool have_support_softprobe; ///< 是否支持软探针模块，默认值为false
    OVD_bool have_auto_reboot;       ///< 是否支持设备自动维护，默认值为false
    OVD_bool have_timed_dormancy;    ///< 是否支持定时休眠开关，默认值为false
    OVD_bool yard_light_brightness;  ///< 是否支持庭院灯灯光控制,默认false
    OVD_bool yard_light_manual_control;///< 是否支持手动控制,默认false
    OVD_int32 yard_light_timed_schedule;///< 整型，是否支持计划控制，0为不支持，非0数字表示支持几个计划控制时间段，默认为2，最大值为5
    OVD_bool dashcam;                   ///< 是否支持行车记录仪功能，默认不支持
    OVD_bool dashcam_findcar;           ///< 1.45.0-布尔型： 是否支持智能寻车，默认值为false
    OVD_bool dashcam_rescue;            ///< 1.45.0-布尔型： 是否支持智能救援，默认值为false
    OVD_bool smoke_sensor;              ///< 是否支持四合一烟雾传感器功能，默认不支持
    OVD_bool AIGC_cloud_record;         ///< 是否支持AI云存
    OVD_bool support_DN_mode;///<是否支持配网能力集上报，默认不支持


    OVD_char video_quality[MAX_LEN_24]; ///< 字符串列表(中间用,隔开)：支持的视频清晰度选项列表，列表元素可选值为：ld/sd/hd/fhd，分别代表低清/标清/高清/全高清，默认支持sd和hd

    OVD_char video_formats_supportlists[MAX_LEN_24]; ///< 字符串列表(中间用,隔开)：支持的视频编码格式列表，列表元素可选值为：h264/h265,默认支持h264

    OVD_bool support_set_video_formats; ///< 是否支持平台切换视频h264,h265格式

    OVD_bool support_static_scence_ABR; ///< bool型，静态场景码率自适应，详细要求参考入库规范相关功能说明，默认值为不支持

    OVD_bool support_set_bitrate;   ///< 是否支持码率设置，默认值为false

    OVD_bool support_set_framerate; ///< 是否支持帧率设置，默认值为false

    OVD_bool support_set_gop;   ///< 是否支持gop设置，默认值为false


    ovd_media_encrypt_type_e stream_encryption_mode; ///<码流加密能力集

    ovd_lock_cap_params_t lock_params;  ///< 1.34新增，门锁品类相关能力集

    OVD_bool have_voiceout_volume; ///< 设备是否支持对讲音量调节，默认值为false
    OVD_bool have_doorbell_volume;
    OVD_bool support_set_normal_nightvision_mode;///< 布尔型: 普通摄像头是否支持设备切换夜视模式，夜视模式分为开启/关闭/自动
    OVD_bool support_set_color_nightvision_mode; ///< 全彩摄像头是否支持设备切换夜视模式，夜视模式分为黑白夜视/全彩夜视/智能夜视
    OVD_int64 support_detect_nightvision_mask;   ///< 整形：智能夜视模式可支持的侦测类型掩码，比特位为1表示支持,共64位。\ref OVD_NIGHTVISION_DETECT_HUMAN,默认值0x0000000000000000

    OVD_bool support_set_AI_rules_mode;          ///< 是否支持叠加智能规则信息显示,默认值false1.40.0新增
    OVD_bool osd_text; ///< 是否支持持文本水印
    OVD_bool osd_logo; ///< 是否支持水印图片
    OVD_bool enable_hjkh;  ///< 设备是否集成了和家固话，默认值为false
    OVDHJKH_mode hjkh_mode;///< 设备和家看护相关的功能
    OVD_bool enable_EBO; ///< 设备是否支持机器人指令，默认值为false
    OVDEBO_mode EBO_mode;///< 机器人业务细节能力集
    OVD_char robot_ctrl[MAX_LEN_32];///< 字符串类型，支持的机器人控制类型

    OVD_char have_support_extension[MAX_LEN_256]; ///< 字符串列表(中间用,隔开)：支持的extension方法列表，列表元素可选值为：echotest/ccrtc


    OVD_bool enable_petfeeder; ///< 是否支持众鼎宠物喂食器相关能力集，1.29版本新增
    OVD_int32 petfeeder_plan_num; ///< int型，数量为支持的最大计划条数，最大值为10
    OVD_int32 petfeeder_feed_num; ///< int型，数量为支持的喂食份数，值必须大于0

    OVD_int32 camera_channel_max;  ///< 多通道设备最大支持的通道个数，单通道设备不用填写该字段
    OVD_int32 robot_housekeeper; ///< 1.33新增，整型，位运算0x0F。设备是否支持机器人管家相关功能。最低位表示"天气预报" 0x01，第二位表示"吃药提醒" 0x02，默认0表示不支持
    OVD_int32 camera_lens_type;  ///< 1.34.0新增，整型，位运算 0x0F。设备支持的摄像头镜头形态。最低位表示"单目" 0x01，第二位表示"双目拼接" 0x02，第三位表示"单目枪球联动" 0x04，第四位表示"双目拼接枪球联动" 0x08，第五位表示"全景拼接枪球联动" ，默认 0x01

    OVD_bool support_aov; ///< 1.48新增，是否支持aov功能
} OVDCapInfo;

/**
 * @brief 动态添加视频通道时，需要上报附加信息时，使用该数据结构
 *
 */
typedef struct OVD_ChannelExtension_s
{
    OVD_char name[MAX_LEN_256]; ///< 扩展内容
}OVD_ChannelExtension_t;

/**
 * @brief 通道层次的能力集
 * @ingroup 能力集
 */
typedef struct OVD_ChannelCap_s
{
    OVD_bool support_switch;            ///< 是否支持休眠


    OVD_dword audiosamplerate;          ///< 音频采样率，默认 OVD_AUDIO_SAMPLE_RATE_8K
    OVD_bool support_audio_out;         ///< 是否支持音乐输出,默认值为false
    OVD_bool support_voice_out;         ///< 是否支持对讲输出,默认值为false
    OVD_bool support_voiceout_volume;   ///< 设备是否支持对讲音量调节，默认值为false


    OVD_bool multi_stream;              ///< 是否支持主子码流功能，默认值为false 1.30版本新增
    OVD_dword video_quality;            ///< 支持的视频清晰度 默认 OVD_VIDEO_QUALITY_DEFAULT
    OVD_dword video_codec;              ///< 默认支持 OVD_VIDEO_CODEC_H264
    OVD_bool support_set_video_formats; ///< 是否支持平台切换视频h264,h265格式
    OVD_bool support_static_scence_ABR; ///< 静态场景码率自适应，详细要求参考入库规范相关功能说明，默认值为false
    OVD_bool support_set_bitrate;       ///< 是否支持码率设置，默认值为false
    OVD_bool support_set_framerate;     ///< 是否支持帧率设置，默认值为false
    OVD_bool support_set_gop;           ///< 是否支持gop设置，默认值为false

    ovd_media_encrypt_type_e stream_encryption_mode;///< 设备最强性能支持的加密方式
    OVD_bool support_set_AI_rules_mode; ///< 是否支持叠加智能规则信息显示,默认值false1.40.0新增


    ovd_night_vison_mode_e nightvision_mode; ///< 支持切换夜视模式，默认值 OVD_NIGHTVISION_MODE_NONE
    OVD_dword ai_nightvision_mask;      ///< 智能夜视模式可支持的侦测类型掩码，比特位为1表示支持,共64位。参考 OVD_NIGHTVISION_DETECT_XXX，默认值0x0000000000000000


    OVD_bool osd_text;                  ///< 是否支持持文本水印
    OVD_bool osd_logo;                  ///< 是否支持水印图片


    OVD_bool support_sd;                ///< 是否支持本地录像,默认值为false
    OVD_dword card_playback_speed;      ///< 卡回看播放速度能力掩码，参考 OVD_CARD_PLAYBACK_SPEED_MASK
    OVD_bool support_sdcapacity;        ///< 是否支持存储卡容量信息获取，默认值为false



    OVD_bool support_ptz;               ///< 是否支持云台控制能力,默认值为false
    OVD_bool support_ptz_preset;        ///< 是否支持云台预置位,默认值为false
    OVD_bool zoomcontrol;               ///< 是否支持用户手动调节倍率，默认值为false
    OVD_bool focuscontrol;              ///< 是否支持用户手动调节焦距，默认值为false
    OVD_bool support_doorbell_preset;   ///< 是否支持设置门铃机位，默认值为false


    OVDAlarmsInfo ovdCapInfo_alarms;    ///< 告警相关能力集
    OVDlinkage_mode ovdlinkage_mode;    ///< 联动方式的支持

    OVDAICapInfo ovdAICapInfo;  ///< AI能力集

    OVDCloudAICapInfo ovdCloudAICapInfo;    ///< CloudAI能力集

    OVDAISpeechCapInfo ovdAISpeechCapInfo;  ///< AI语音交互
    OVDLullbyCapInfo ovdLullbyCapInfo;      ///<童谣业务

    OVD_ChannelExtension_t *extension; ///< 扩展信息，内存由外部指定，如果是动态分配的内存，由厂商自行释放
}OVD_ChannelCap_t;

/**
 * @brief 媒体类型
 *
 */
typedef enum
{
    OVD_CONTENT_NONE    =    0,
    OVD_Video           =    1,          ///< 视频
    OVD_Audio           =    2,          ///< 音频
    OVD_Private         =    3,          ///< 私有数据：该类型只在SDK内部使用
    OVD_AOV             =    4,          ///< AOV视频数据，帧率很低1秒1帧
    OVD_3D_Point_Cloud  =    5,          ///< 3D点云数据
}OVDContentType;

/**
 * @brief 主子码流枚举
 *
 */
typedef enum
{
    OVD_HIGH_STREAM  =   1,   ///< 主码流
    OVD_LOW_STREAM   =   2    ///< 子码流
} OVDCodeStream;

/**
 * @brief 音视频编码格式枚举
 *
 */
typedef enum
{
    source_STREAM_NO_CODEC = 0,
    source_STREAM_CODEC_H264,            ///< h264码流格式
    source_STREAM_CODEC_H265,            ///< h265码流格式
    source_STREAM_VIDEO_CODEC_MAX = 15,
    source_STREAM_CODEC_AAC_WITH_ADTS = 16,  ///< 带adts头的AAC格式
    source_STREAM_CODEC_AAC,                ///< 当前sdk音频格式只支持aac with adts
    source_STREAM_AUDIO_CODEC_MAX = 30,
    source_STREAM_CODEC_PRIVATE = 31,      ///< 私有格式
    source_STREAM_CODEC_POINTCLOUD = 32,   ///< 3D点云格式
    source_STREAM_CODEC_MAX
}OVDAVCodec;

/**
 * @brief 视频清晰度枚举
 *
 */
typedef enum
{
    OVD_1DMODE  =   0, ///< 低清
    OVD_SDMODE  =   1, ///< 标清
    OVD_HDMODE  =   2, ///< 高清
    OVD_FHDMODE  =  3, ///< 超高清
}OVDEncodeQuality;

/**
 * @brief 视频码流格式定义
*/
typedef struct
{
    OVDAVCodec codec;     ///< 编解码类型，例如h264/h265/aac
    OVD_int32 quality;    ///< 暂未使用，编码质量CQS
    OVD_uint32 bitrate;   ///< 暂未使用，码率
    OVD_ushort width;     ///< 暂未使用，分辨率宽
    OVD_ushort height;    ///< 暂未使用，分辨率高
    OVD_uchar framerate;  ///< 帧率
    OVD_uchar framerate_aov;  ///< 1.48以上，aov帧率，对于支持aov功能设备需要填写
    OVD_uchar colorDepth; ///< 暂未使用，像素位数，例如8位像素为256色
    OVD_uchar frameInterval;  ///< 1.44.1以上，gop值
    OVD_uchar reserve;    ///< 暂未使用，保留字段

}OVDVideoDataFormat;

/**
 * @brief 音频码流格式定义
*/
typedef struct
{
    OVDAVCodec codec;         ///< 编解码类型，例如h264/h265/aac
    OVD_uint32 samplesRate;   ///< 采样率，例如8000/16000
    OVD_uint32 bitrate;       ///< 暂未使用，码率
    OVD_ushort waveFormat;    ///< 暂未使用，音频文件格式
    OVD_ushort channelNumber; ///< 声道数
    OVD_ushort blockAlign;    ///< 暂未使用，每帧采样的字节数
    OVD_ushort bitsPerSample; ///< 暂未使用，采样位数
    OVD_int32 sampleperframe; ///< 暂未使用，每帧采样点个数
    OVD_ushort frameInterval; ///< 暂未使用，帧间隔
    OVD_ushort reserve;       ///< 暂未使用，保留字段
}OVDAudioDataFormat;

/**
 * @brief 点云码流格式定义
*/
typedef struct
{
    OVDAVCodec codec;         ///< 编解码类型
    OVD_uchar framerate;      ///< 帧率
    OVD_ushort reserve;       ///< 暂未使用，保留字段
}OVDPointCloudDataFormat;


/**
 * @brief 设备焦距信息定义
 *
 */

typedef struct
{
    OVD_int32 channel_id;      ///< 通道号，从0开始
    OVD_char  focal_length_mm[32]; ///<某一通道上的焦距长度，单位为mm，用字符串格式来兼容小数例如3.6mm
}OVD_CameraLensesInfo;


/**
 * @brief 白盒摄像头特有信息
 * @ingroup 白盒摄像头
 */
typedef struct
{
    OVD_bool is_white_box;                          ///< 可选，布尔型，是否为白盒摄像头，1 是白盒，0 不是白盒
    OVD_bool is_authorized;                         ///< 可选，布尔型，设备是否授权
    OVD_char app_version[MAX_LEN_32];               ///< 可选，字符串，应用程序版本
    OVD_char hal_version[MAX_LEN_32];               ///< 可选，字符串，硬件抽象层版本号
    OVD_char soc_sdk_version[MAX_LEN_32];           ///< 可选，字符串，芯片SDK版本
    OVD_char isp_version[MAX_LEN_32];               ///< 可选，字符串，isp版本信息
}OVD_WhiteBoxInfo;


/**
 * @brief 定义许可证状态结构体
 * @ingroup 白盒摄像头
 */
typedef struct {
    OVD_bool is_valid;    ///< 是否有效
    OVD_bool is_expired;  ///< 是否过期
    OVD_bool is_revoked;  ///< 是否撤销
} OVD_LicenseStatus;

/**
 * @brief 定义许可证有效期结构体
 * @ingroup 白盒摄像头
 */
typedef struct {
    OVD_char start_time[32];  ///< 开始时间，格式为时间戳，单位秒
    OVD_char end_time[32];    ///< 结束时间，格式为时间戳，单位秒
} OVD_Validity;

/**
 * @brief 定义元数据结构体
 * @ingroup 白盒摄像头
 */
typedef struct {
    OVD_char license_source[32];  ///< 许可证来源
    OVD_char version[16];         ///< 版本号
} OVD_Metadata;

/**
 * @brief 许可证类型定义
 * @ingroup 白盒摄像头
 */
enum OVD_LicenseType {
    OVD_LICENSE_TYPE_TRIAL = 0,
    OVD_LICENSE_TYPE_FULL = 1,
    OVD_LICENSE_TYPE_FEATURE = 2,
};

/**
 * @brief 定义主许可证结构体
 * @ingroup 白盒摄像头
 */
typedef struct {
    OVD_LicenseStatus license_status;  ///< 许可证状态
    enum OVD_LicenseType license_type; ///< 许可证的类型
    OVD_char *feature_list[16];        ///< 功能列表
    OVD_int32 feature_count;             ///< 功能列表数量
    OVD_Validity validity;             ///< 许可证有效期
    OVD_Metadata metadata;             ///< 元数据
} OVD_License;


/**
 * @brief 设备信息
 */
typedef struct
{
    OVD_bool OVDDeviceID[MAX_LEN_32];               ///< 设备ID号，必填
    OVD_char OVDChipModel[MAX_LEN_32];              ///< 芯片型号，必填
    OVD_char OVDHardWareModel[MAX_LEN_32];          ///< 设备型号，必填
    OVD_char OVDSystemVersion[MAX_LEN_32];          ///< 设备固件版本号，必填
    OVD_int32 networkType;                          ///< 设备当前网络连接类型， 1: 无线网络   2：有线网络   3：有线和无线都配置   4:4G供网   5:5G供网   6:4G供网和有线都配置   7:5G供网和有线都配置,必填
    OVD_DNM_e DN_mode;                              ///< 设备配网类型，类型详见OVD_DNM_e,必填
    OVD_char wifi_ssid[WIFI_SSID_LEN];              ///< 设备当前连接的wifi的ssid, 该字段空串表示未连接wifi，必填
    OVD_int32  wifi_signal;                         ///< 设备当前wifi的信号强度, 0-100, 当wifi_ssid不为空时有效，可选
    OVD_int32 upBandwidth;                          ///< 设备探测到的上行最大带宽，单位bps，不存在则表示上行带宽未知，负值表示未知，可选
    OVD_int32 downBandwidth;                        ///< 设备探测到的下行最大带宽，单位bps，不存在则表示下行带宽未知，负值表示未知，可选
    OVD_bool ipAddr[MAX_LEN_128];                   ///< IP地址    <局域网IP，支持IPV6，空值表示未知，必填
    OVD_bool macAddr[MAX_LEN_32];                   ///< MAC地址，空值表示未知，可选,格式大写，以:号相隔
    OVD_int32 battery;                              ///< 可选，整数： 设备当前设备电池电量, 0-100, 该字段不存在表示设备不支持电池供电
    OVD_int32 cpuLoad;                              ///< 可选，整数，cpu负载率0~100
    OVD_int32 memoryTotal;                          ///< 可选，整数，总内存数，单位为KB
    OVD_int32 memoryAvailable;                      ///< 可选，整数，剩余可用内存，单位为KB
    OVD_int32 channel_count;                        ///< 可选，整数，通道个数，应该不高于之前初始化时传入的最大通道数
#ifdef CONFIG_WHITE_BOX
    OVD_WhiteBoxInfo whibox_info;                   ///< 可选，白盒摄像头特有信息，所有生态设备厂家都不需要填写该字段
#endif
    OVD_CameraLensesInfo *camera_lenses_info;       ///< 可选，摄像头 lens 信息，预分配的数组长度为初始化时输入的最大通道数
}OVDDeviceInfo;



/**
 * @brief 初始化参数定义
 *
 */
typedef struct
{
    OVD_char OVDDeviceID[MAX_LEN_64]; ///<物料清单里的设备序列号,需要烧录固化到设备里,16位，1-2位代表device vendor，3-6位代表device model，后10位代表生产流水号
    OVD_char OVDDeviceCMEI[MAX_LEN_64]; ///< cmei号
    OVD_char OVDLoginPassword[MAX_LEN_128];///<设备接入密钥，用于登录鉴权
    OVD_char OVDMediaEncPassword[MAX_LEN_128];///<视频加密秘钥，用于码流加密
    OVD_char OVDHardWareModel[MAX_LEN_32];///<设备硬件型号
    /**
     * @brief 设备固件版本号
     * @details 说明：固件版本号命名规则:需满足 3 段及 3 段以上，且总长不要超过 10 位纯数字，不可使用中划线等其他符号。
     */
    OVD_char OVDSystemVersion[MAX_LEN_32];
    /**
     * @brief 设备芯片ID，必填
     * @details 说明：杭研分配，主要区分同一设备型号不同芯片方案的情况。
     */
    OVD_char OVDModelId[MAX_LEN_64];
    /**
     * @brief 设备mac地址，必填
     * @details 说明：
     * (1)设备首次连接平台，平台会记录mac地址，区分大小写。用英文:分隔符分开。
     * (2)设备侧上报的mac地址格式需与包材的mac地址格式保持一致
     */
    OVD_char OVDmacaddress[MAX_LEN_32];

    OVD_char servicescheduleurl[MAX_LEN_1024];  ///< 服务调度的URL

    OVD_char local_storage_path[MAX_LEN_256];   ///< 卡录像挂载路径

    OVD_char ovd_data_path[MAX_LEN_256];    ///< SDK专用分区挂载路径，需要给SDK分配一个单独的flash分区
    OVD_char ovd_log_path[MAX_LEN_256]; ///< ovd sdk默认保存路径，需可读写，eg:/tmp,如不设置，默认写在/tmp目录下
    OVD_char ovd_speaker_path[MAX_LEN_256];///< 扬声器设备文件路径，用于记录扬声器发声打点，eg:路径如：“/dev/snd/pcmC0D0p”

    OVD_int32 max_channel;  ///< 设备最大支持的视频通道个数

    OVD_char chip_id[MAX_LEN_128];          ///< 可选，CPU出厂时的全球唯一ID，该ID一般是调用芯片SDK接口获取得到
}OVDClientParam;

/**
 * @brief OVD 日志相关，注意要做到日志隔离
 *        日志输出级别依次增高
 */
typedef enum
{
    OVD_LOGLEVEL_TRACE = 0, ///<TRACE级别
    OVD_LOGLEVEL_DEBUG = 1, ///<DEBUG级别
    OVD_LOGLEVEL_INFO  = 2, ///<INFO级别
    OVD_LOGLEVEL_WARN  = 3, ///<WARN级别
    OVD_LOGLEVEL_ERROR = 4, ///<ERROR级别
    OVD_LOGLEVEL_FATAL = 5, ///<FATLA级别
}OVDLogLevel;

/**
 * @brief 日志输出位置
 *
 */
typedef enum
{

    OVD_LOGSTD_NO = 0,    ///< 不输出
    OVD_LOGSTD_OUT = 1,   ///< 输出到标准输出
    OVD_LOGSTD_ERR  = 2,  ///< 输出到标准出错
}OVDLogSTD;

/**
 * @brief 日志相关的初始化参数
 *
 */
typedef struct
{

    OVDLogSTD   logSTD;             ///< 日志输出位置，可选，详细见枚举值LogSTD，可选
    OVD_int32   max_size;           ///最大本地日志存储空间，单位为MB，默认为1MB
    OVD_void(*pOVDLogOutCallBack)(const char *buff);   ///< 设备提供的日志输出回调，SDK的输出日志可以保存到device的存储文件中，必选
#ifdef CONFIG_LOG_SUPPORT_LOGCALLBACK_V2
    OVD_void (*pOVDLogOutCallBackV2)(OVDLogLevel level, const char *tag, const char *buff); ///< 可选，设备提供的日志输出格式化输出回调，如果该回调函数不为空，则优先使用该回调。
#endif /* CONFIG_WHITE_BOX */
} OVDLogParam;

/**
 * @brief 重启原因相关的初始化参数
 *
 */
typedef struct
{
    ovd_reboot_reason_e reason;     ///< 重启原因
    OVD_uint64 ts;                  ///< 重启的时间戳（绝对时间戳）
    OVD_char message[256];          ///< 辅助描述性字符串，厂商可在此简要描述重启原因
}OvdRebootParam, ovd_reboot_param_t;                ///< 厂商传入上次重启的具体信息


/**
 * @brief 固件加密方式
 *
 */
enum OVD_OTA_EncryptType
{
    OTA_CRYPT_TYPE_NONE = 0,        ///< 未加密
    OTA_CRYPT_TYPE_AES128_CBC = 1,      ///< aes-128-cbc加密, iv向量长度为16，内容全部为0x00
    OTA_CRYPT_TYPE_AES256_CBC = 2,      ///< aes-256-cbc加密, iv向量长度为16，内容全部为0x00
};

/**
 * @brief 固件升级包信息
 * @details 正常升级不需要用到此结构体；升级失败进入备份分区后使用
 */
typedef struct {
    OVD_int64                 pkg_size;           /**< 固件升级包大小，包括128字节头部信息 */
    OVD_uint32                crc;                /**< 固件升级包文件crc32值，用于一致性校验，校验范围不包括128字节头部信息 */
    OVD_char                  firmware_model[64]; /**< 目标版本号 */
    enum OVD_OTA_EncryptType  encrypt_type;       /**< 加密类型 */
    OVD_uchar                 aes_key[32];        /**< aes解密密钥 */
    OVD_int32                 key_size;           /**< 密钥字节数 */
    OVD_char                  url[256];           /**< 升级包的URL地址，正常升级流程中厂商不要使用此URL下载固件。 SDK中会自动下载升级包，厂商可以在设备升级失败无法启动的时候使用此链接下载固件（通过小系统启动）, 需务必支持http和https*/
} OVD_ota_upgrade_package_info_t;

/**
 * @brief OTA升级指令
 *
 */
typedef enum
{
    /// 固件升级包信息，SDK开始下载升级包时调用，用于下载完毕后做完整性校验
    /// OVD_ota_upgrade_package_info_t
    OVD_CMD_UPGRADE_PKG_INFO,

    /// 下载固件升级包数据
    /// 回调指针类型：unsigned char
    OVD_CMD_UPGRADE_DOWNLOAD,

    /// 下载固件升级包完毕，设备需对升级包做md5校验
    /// 回调指针类型：NULL
    OVD_CMD_UPGRADE_DOWNLOAD_FINISH,

    /// 下载固件升级包失败
    /// 回调指针类型：NULL
    OVD_CMD_UPGRADE_DOWNLOAD_FAIL,

    ///  开始安装固件升级包
    /// 回调指针类型：NULL
    OVD_CMD_UPGRADE_INSTALL,

}OVD_Upgrade_cmd_e;

/**
 * @brief 升级结果确认类型
 *
 */
typedef enum
{
    OVD_UPGRADE_CONFIRM_NORMAL = 0,     ///< 正常重启，无升级操作发生
    OVD_UPGRADE_CONFIRM_FAIL = 1,       ///< 升级重启，上次升级失败
    OVD_UPGRADE_CONFIRM_SUCCUSS = 2,    ///< 升级重启，上次升级成功
}OVD_Upgrade_Confirm_e;

/**
 * @brief 启动设备后，确认升级状态的数据结构
 *
 */
typedef struct
{
    OVD_Upgrade_Confirm_e confirm;            ///< 升级结果确认
    OVD_char cur_version[MAX_LEN_64];         ///< 当前系统的版本号，如果升级成功，则是新的版本号，如果失败，则是老版本号
    OVD_char last_upgrade_time[MAX_LEN_64];   ///< 最近一次处理升级指令的时间, 格式 YY-MM-DDTHH:MM:SS
}OVD_UpgradeConfirmInfo_t;




/**
 * @brief p2p 卡录像机制
 *
 */
typedef struct
{
    OVD_int32 channel;                  ///< 通道号
    OVD_char FileName[MAX_LEN_256];     ///< 文件名
    OVD_int32  FileType;                ///< 文件类型(0 视频文件， 1 告警文件)
    OVD_uint64 FileStartStamp;          ///< 录像开始时间（单位为秒）
    OVD_uint64 FileEndStamp;            ///< 录像接收时间（单位为秒）
    OVD_int32  RecordDuration;          ///< 时长（单位为秒）
    OVD_int32  FileSize;                ///< 文件大小
}OVDRecordFileInfo;

/**
 * @brief 录像文件列表信息
 *
 */
typedef struct
{
    OVD_int32               fileCount;            ///< 文件数量
    OVDRecordFileInfo fileinfo[MAX_LEN_24];       ///< 文件列表
}OVDRecordFileListPerPage;

/**
 * @brief 卡回看操作指令
 *
 */
typedef enum
{
    OVD_CONTINUE    =   0,      ///< 继续播放
    OVD_PAUSE       =   1,      ///< 暂停
    OVD_STOP        =   2,      ///< 停止
    OVD_FAST        =   3,      ///< 快进
    OVD_SLOW        =   4,      ///< 慢放
    OVD_JUMP        =   5,      ///< 拖动  ms
}OVDCONTROLTYPE;

/**
 * @brief 存储卡信息
 *
 */
typedef struct
{
    OVD_int32  SDExist;                 ///< 0 not, 1 yes, 2 error
    OVD_int32     SDTotalSize;          ///< 总容量(M)
    OVD_int32     SDFreeSize;           ///< 空闲量
    OVD_char EarlyFileName[MAX_LEN_24]; ///< 当前SD卡最早一个录像文件
}OVDSDInfo;



/**
 * @brief PTZ控制指令
 *
 */
typedef enum
{
    OVC_PTZ_MV_UP        = 0,           ///< 向上
    OVC_PTZ_MV_DOWN      = 1,           ///< 向下
    OVC_PTZ_MV_LEFT      = 2,           ///< 向左
    OVC_PTZ_MV_RIGHT     = 3,           ///< 向右
    OVC_PTZ_MV_UPLEFT    = 4,           ///< 左上
    OVC_PTZ_MV_UPRIGHT   = 5,           ///< 右上
    OVC_PTZ_MV_DOWNLEFT  = 6,           ///< 左下
    OVC_PTZ_MV_DOWNRIGHT = 7,           ///< 右下
    OVC_PTZ_ZOOM_IN      = 8,           ///< 拉近
    OVC_PTZ_ZOOM_OUT     = 9,           ///< 拉远
    OVC_PTZ_MV_STOP      = 10,          ///< 停止运动
    OVC_PTZ_GOTO_PRESET  = 11,          ///< 跳转预置位
    OVC_PTZ_SET_PRESET   = 12,          ///< 设置预置位点
    OVC_PTZ_CLEAR_PRESET = 13,          ///< 清除预置位点
    OVC_PTZ_MV_UP_STEP   = 14,          ///< 单步上
    OVC_PTZ_MV_DOWN_STEP   = 15,        ///< 单步下
    OVC_PTZ_MV_LEFT_STEP   = 16,        ///< 单步左
    OVC_PTZ_MV_RIGHT_STEP  = 17,        ///< 单步右
    OVC_PTZ_MV_UPLEFT_STEP  = 18,       ///< 单步左上
    OVC_PTZ_MV_UPRIGHT_STEP  = 19,      ///< 单步右上
    OVC_PTZ_MV_DOWNLEFT_STEP  = 20,     ///< 单步左下
    OVC_PTZ_MV_DOWNRIGHT_STEP  = 21,    ///< 单步右下
    OVC_PTZ_ZOOM_IN_STEP       = 22,    ///< 单步拉近
    OVC_PTZ_ZOOM_OUT_STEP      = 23,    ///< 单步拉远
    OVC_PTZ_FOCUS_DOWN         = 24,    ///< 焦距调小
    OVC_PTZ_FOCUS_UP           = 25,    ///< 焦距调大
    OVC_PTZ_ZOOM_RESET         = 26,    ///< 恢复默认倍率
    OVC_PTZ_FOCUS_DOWN_STEP    = 27,    ///< 焦距单步调小
    OVC_PTZ_FOCUS_UP_STEP      = 28,    ///< 焦距单步调大
    OVC_PTZ_SET_433_PRESET     = 29,   ///< 设置433预置位点
    OVC_PTZ_MAX ,
}OVCPTZControlCmd;


typedef enum {
    OVD_PTZ_OK                  = 0,           ///< 成功
    OVD_PTZ_CTRL_FAIL           = -1001,       ///< PTZ控制失败

    OVD_PTZ_PRESET_SET_EXSIT    = -1002,       ///< PTZ预置位已存在
    OVD_PTZ_PRESET_SET_FAIL     = -1003,       ///< PTZ预置位设置失败

    OVD_PTZ_PRESET_CLEAR_FAIL   = -1004,       ///< PTZ预置位清除失败
}ovd_ptz_cmd_code;


/**
 * @brief 云台转动达到限制的方向
 *
 */
typedef enum {
    OVD_POS_LIMIT_NULL = 0,             ///< 未达限制（默认）
    OVD_POS_LIMIT_UP = 1,               ///< 向上转动达到限制
    OVD_POS_LIMIT_DOWN = 2,             ///< 向下转动达到限制
    OVD_POS_LIMIT_LEFT = 3,             ///< 向左转动达到限制
    OVD_POS_LIMIT_RIGHT = 4,            ///< 向右转动达到限制
    OVD_POS_LIMIT_UPLEFT = 5,           ///< 左上转动达到限制
    OVD_POS_LIMIT_UPRIGHT = 6,          ///< 右上转动达到限制
    OVD_POS_LIMIT_DOWNLEFT = 7,         ///< 左下转动达到限制
    OVD_POS_LIMIT_DOWNRIGHT = 8,        ///< 右下转动达到限制
    OVD_POS_LIMIT_ZOOMIN = 9,           ///< 拉近达到限制
    OVD_POS_LIMIT_ZOOMOUT = 10,         ///< 拉远达到限制
    OVD_POS_LIMIT_FOCUSUP = 11,         ///< 焦距调大达到限制
    OVD_POS_LIMIT_FOCUSDOWN = 12,       ///< 焦距调小达到限制
}OVD_pos_limit_type;

/**
 * @brief 云台PTZ状态信息
 *
 */
typedef struct
{
    int pan;                  ///< 左右位置
    int tilt;                 ///< 上下位置
    int zoom;                 ///< 缩放
    int focus;                ///< 焦距
    OVD_pos_limit_type limit; ///< 云台转动达到限制的方向
}OVD_ptz_pos;


/**
 * @brief 设备云台状态信息
 *
 */
typedef struct
{
    int channel;                    ///< 通道号
    OVD_ptz_pos pos_info;           ///< 云台PTZ信息
}OVD_ptz_state_t;

/**
 * @brief EBO机器人控制指令
 *
 */
typedef enum
{
    OVC_EBO_MV_STEP      = 1,   ///< 步进指令
    OVC_EBO_MV_CONT      = 2,   ///< 持续移动指令
    OVC_EBO_NOFALL_TEST  = 3,   ///< 防跌落
    OVC_EBO_LED_TEST   = 4,     ///< 补光灯
    OVC_EBO_MV_FORWARD   = 5,   ///< 向前猛冲
    OVC_EBO_RECHARGE     = 6,   ///< 回充
    OVC_EBO_CIRCLE       = 7,   ///< 转圈
    OVC_EBO_SHAKE        = 8,   ///< 抖动
    OVC_EBO_SELF_TEST      = 9, ///< 启动自检
    OVC_EBO_REPORT      = 10,   ///< 报告查看
    OVC_EBO_REPORT_LEN      = 11,   ///< 报告返回长度
    OVC_EBO_MV_START     = 12,      ///< 移动开始指令
    OVC_EBO_RESERVE   =13,          ///< 保留指令

}OVCEBOControlCmd;


/**
 * @brief EBO转动位置坐标信息
 *
 */
typedef struct
{
    OVD_int32 pan;        /**< 水平方向坐标，整型，范围是[-100,100],只有步进指令或持续移动指令带有这个字段，左为负，右为正 */
    OVD_int32 tilt;       /**< 垂直方向坐标，整型，范围是[-100,100],只有步进指令或持续移动指令带有这个字段，下为负，上为正 */
    OVD_int32 seq;       /**< 指令顺序 */
} ovd_ebo_space_t;

/**
 * @brief 缓冲区定义
 *
 */
typedef struct
{
    OVD_char *buf;                  ///< 数据buf
    OVD_int32  size;                ///< 数据长度
}ovd_common_info_t;

/**
 * @brief EBO 控制内容
 *
 */
typedef struct
{
    OVCEBOControlCmd  ebo_cmd;
    union
    {
        ovd_ebo_space_t  ebo_space;
        ovd_common_info_t ebo_data;
    };
}ovd_ebo_content_t;

/**
 * @brief 机器人控制内容
 *
 */
typedef struct
{
    OVD_char type[MAX_LEN_64]; ///< 类型
    OVD_char action[MAX_LEN_64]; ///< 动作
    OVD_char *data;///< 数据
}OvdRobotContent, ovd_robot_content_t;

/**
 * @brief 机器人类型
 *
 */
typedef enum
{
    OVC_ROBOT_OWL      = 0,   ///< 猫头鹰
    OVC_ROBOT_TGP      = 1,
}ovd_robot_cmd;

/**
 * @brief GPS 信息
 *
 */
typedef struct{
    OVD_float longitude; ///< 经度信息
    OVD_float latitude;  ///< 纬度信息
}OvdGpsInfo, ovd_gps_info_t;

/**
 * @brief 对讲输出的音频格式
 *
 */
typedef struct
{
    OVD_uint32 codec;         ///< 音频格式 OVDAUDIOPLY_TYPE
    OVD_uint32 samplesRate;   ///<采样率
    OVD_uint32 bitWidth;      ///<位宽
    OVD_uint32 volume;        ///<音量大小
    OVD_uint32 priority;      ///< 优先级the biger ,the higher
}OVDAudioOutDataFormat;

/**
 * @brief MP3音频文件播放控制
 *
 */
typedef enum
{
    MP3_CLOSE    =    0,       ///< 停止播放
    MP3_PAUSE    =    1,       ///< 暂停播放
    MP3_RESUME   =    2,       ///< 恢复播放
    MP3_OTHER,
}OVDMp3PlayCtrl;

/**
 * @brief 音频播放状态
 *
 */
typedef enum
{
    OVD_PLAY_STOPPED        =    0,  ///< 停止播放状态
    OVD_PLAY_PAUSING        =    1,  ///< 暂停状态
    OVD_PLAY_PLAYING        =    2,  ///< 播放中
}OVDAudioPlayStatus;

/**
 * @brief 图像缓存定义
 *
 */
typedef struct
{
    OVD_char *buf;                  ///< 数据buf
    OVD_int32  size;                ///< 数据长度
    //char          ImageUrl[1024]; ///< 目前没用到,可填空
}OVDImageInfo;

/**
 * @brief 手势识别告警类型定义，用于扩展字段填写
 *
 */
typedef enum
{
    OVD_OBJ_TYPE_GESTURE        =    42,  ///< 手势识别
    OVD_OBJ_TYPE_GESTURE_CALL   =    43,  ///< 手势呼叫
    OVD_OBJ_TYPE_GESTURE_HELP   =    44,  ///< 手势求助
}ovd_gesture_obj_type;

// ====================== 宏生成核心引擎 ======================
// 定义枚举项生成器宏
#define OVD_AI_GEN_ENUM_ITEM(TYPE, IDX, VALUE) \
    OVD_AI_##TYPE##_##IDX = VALUE,  ///< AI ##TYPE## 告警类型 ##IDX

// 定义类型特定的生成器宏
#define OVD_AI_SUSPECTED_GEN(IDX, VALUE) OVD_AI_GEN_ENUM_ITEM(SUSPECTED, IDX, VALUE)
#define OVD_AI_RESERVED_GEN(IDX, VALUE)  OVD_AI_GEN_ENUM_ITEM(RESERVED, IDX, VALUE)

// ====================== 枚举项迭代器 (支持不同数量) ======================
// 生成20个项的迭代器
#define OVD_AI_ITERATE_20(GEN, START) \
    GEN( 1, START +  0) \
    GEN( 2, START +  1) \
    GEN( 3, START +  2) \
    GEN( 4, START +  3) \
    GEN( 5, START +  4) \
    GEN( 6, START +  5) \
    GEN( 7, START +  6) \
    GEN( 8, START +  7) \
    GEN( 9, START +  8) \
    GEN(10, START +  9) \
    GEN(11, START + 10) \
    GEN(12, START + 11) \
    GEN(13, START + 12) \
    GEN(14, START + 13) \
    GEN(15, START + 14) \
    GEN(16, START + 15) \
    GEN(17, START + 16) \
    GEN(18, START + 17) \
    GEN(19, START + 18) \
    GEN(20, START + 19)

// 生成24个项的迭代器
#define OVD_AI_ITERATE_24(GEN, START) \
    OVD_AI_ITERATE_20(GEN, START)     \
    GEN(21, START + 20) \
    GEN(22, START + 21) \
    GEN(23, START + 22) \
    GEN(24, START + 23)

// ====================== 类型特定的枚举生成器 ======================
#define OVD_AI_GENERATE_SUSPECTED(START) \
    OVD_AI_ITERATE_20(OVD_AI_SUSPECTED_GEN, START)

#define OVD_AI_GENERATE_RESERVED(START) \
    OVD_AI_ITERATE_24(OVD_AI_RESERVED_GEN, START)
/**
 * @brief 告警类型定义
 *
 */
typedef enum
{
    OVD_ALARM_BROADCAST         = 0,        ///< 所有告警 关闭所有告警时用

    OVD_OUTTER                  = 2,        ///< 外部告警
    OVD_MOTIOM                  = 3,        ///< 移动侦测
    OVD_CROSS                   = 4,        ///< 拌网侦测
    OVD_CRY                     = 5,        ///< 哭声侦测
    OVD_FACE                    = 6,        ///< 脸部识别
    OVD_VOICE                   = 7,        ///< 声音侦测
    OVD_LOW_BATTERY             = 8,        ///< 低电告警
    OVD_LOSS_LOCK               = 9,        ///< 撬锁告警
    OVD_BELL                    = 10,       ///< 按铃事件
    OVD_MAN                     = 11,       ///< 人形事件
    OVD_PIR                     = 12,       ///< pir移动侦测
    OVD_ALERTAREA               = 13,       ///< 警戒功能告警
    OVD_KITCHEN_MASK            = 14,       ///< 厨房行为检测，检测到口罩异常
    OVD_KITCHEN_CAP             = 15,       ///< 厨房行为检测，检测到帽子异常
    OVD_KITCHEN_CLOTHES         = 16,       ///< 厨房行为检测，检测到工服异常
    OVD_VEHICLE_GEN             = 17,       ///< 车型侦测，通用车辆侦测
    OVD_VEHICLE_MOTOR           = 18,       ///< 车型侦测，检测到电瓶车
    OVD_VEHICLE_BIKE            = 19,       ///< 车型侦测，检测到自行车
    OVD_FACE_MASK               = 20,       ///< 口罩检测，检测到人脸未戴口罩
    OVD_REGIONAL_PEOPLE_STAT    = 21,       ///< 区域人数统计
    OVD_PARABOLIC_AERIAL        = 22,       ///< 高空抛物
    OVD_BABY_CRYING             = 23,       ///< 婴儿啼哭，哭声侦测因安防平台原因，合并到婴儿啼哭
    OVD_OFF_DUTY                = 24,       ///< 离岗检测
    OVD_FEEDER                  = 25,       ///< 喂食检测
    OVD_PET                     = 26,       ///< 宠物出现
    OVD_PARKING_NOTICE          = 27,       ///< 行车记录仪：停车通知
    OVD_DRIVING_COLLISION       = 28,       ///< 行车记录仪：行车碰撞告警
    OVD_PARKING_COLLISION       = 29,       ///< 行车记录仪：停车碰撞告警
    OVD_VOICE_CAPTURE           = 30,       ///< 行车记录仪：语音抓拍
    OVD_TRANSGRESSION           = 31,       ///< 越界侦测
    OVD_AUTOCHECK               = 32,       ///< 四合一烟雾传感器：设备自检
    OVD_ANTI_DISMANTLING        = 33,       ///< 四合一烟雾传感器：防拆告警
    OVD_SMOKE                   = 34,       ///< 四合一烟雾传感器：烟雾告警
    OVD_TEMPERATURE             = 35,       ///< 四合一烟雾传感器：温度告警
    OVD_HUMIDITY                = 36,       ///< 四合一烟雾传感器：湿度告警
    OVD_FAULT                   = 37,       ///< 四合一烟雾传感器：故障告警
    OVD_OFFLINE                 = 38,       ///< 四合一烟雾传感器：烟雾模块失联告警
    OVD_FIRE                    = 39,       ///< 四合一烟雾传感器：火焰识别告警
    OVD_HELMET                  = 40,       ///< 头盔检测，检测到未戴头盔
    OVD_VIDEO_CAPTURE           = 41,       ///< 视频抓拍
    OVD_FENCING                 = 42,       ///< 围栏检测
    OVD_PARCEL                  = 43,       ///< 包裹检测
    OVD_PET_DEPORTATION         = 44,       ///< 宠物驱逐
    OVD_SEDENTARY               = 45,       ///< 久坐提醒
    OVD_CELLPHONE               = 46,       ///< 玩手机识别
    OVD_HOMEWORK                = 47,       ///< 做作业识别
    OVD_WANDER                  = 48,       ///< 徘徊检测
    OVD_OUTANDIN                = 49,       ///< 出入通知
    OVD_SMOKE_FIRE              = 50,       ///< 烟火检测
    OVD_NONMOTORVEHICLE         = 51,       ///< 非机动车检测
    OVD_GESTURE                 = 52,       ///< 手势识别: 调用OVD_AlarmInfoStart时需带上扩展字段extension,json格式：{"tags": [{"tag": 枚举值, "rate":1}]}，枚举值参考ovd_gesture_obj_type，置信度rate固定为1
    OVD_FAMILY                  = 53,       ///< 家人检测
    OVD_GESTURE_OK              = 54,       ///< OK手势
    OVD_GESTURE_V               = 55,       ///< V手势
    OVD_WIFI_SENSING            = 56,       ///< WIFI运动感知事件
    OVD_INTRUSION               = 57,       ///< 入侵检测
    OVD_PET_EVICTION            = 58,       ///< 宠物驱逐
    OVD_INFANT_FENCE            = 59,       ///< 小孩围栏
    OVD_PET_OUT                 = 60,       ///< 宠物出栏
    OVD_PEOPLE_WANDER           = 61,       ///< 有人徘徊
    OVD_FALL                    = 62,       ///< 跌倒检测
    OVD_3D_SENSOR_FALL          = 63,       ///< 3D光传感器:疑似跌倒
    OVD_SLEEP_DETECTION         = 64,       ///< 入睡告警,单次告警15s -sleep_monitor
    OVD_FACE_COVER              = 65,       ///< 遮脸告警,单次告警15s -sleep_monitor
    OVD_WAKEUP_DERECTION        = 66,       ///< 觉醒告警,单次告警15s -sleep_monitor

    /*AI白盒疑似告警类型定义 (67-86)*/
    OVD_AI_GENERATE_SUSPECTED(67)
    /* AI告警保留字段(87~110)*/
    OVD_AI_GENERATE_RESERVED(87)
    OVD_SLEEP_MOVE_DETECTION   = 120,      ///<睡眠状态中四肢活动告警

    OVD_WC_TRAPPED              = 121,        ///< 3D光传感器:卫生间滞留
    OVD_EXPRESSION_RECOGNITION_DETECTION = 122, ///< 表情识别
    OVD_OTHER                   = 255,
}OVDAlarmType;

/**
 * @brief 告警描述
 *
 */
typedef struct
{
    OVD_int32    channel;           ///< 通道号
    OVD_int64    startTimeStamp;    ///< 报警开始时间戳 (单位为秒)
    OVDAlarmType AlarmType;         ///< 报警类型
    OVD_char*    desc;              ///< 告警描述
    OVDImageInfo ImageInfo;         ///< 背景图信息
    OVD_char*    extension;         ///< gps位置信息,json格式：{“gps”:{“longitude”: 经度坐标信息 “latitude”:纬度坐标信息}}
}OVDUpLoadAlarmInfo;

/**
 * @brief 疑似告警信息
 *
 */
typedef struct {
    OVD_char            label[MAX_LEN_32];      ///< 疑似告警具体标签，如”babyCry”
    OVD_float           score;                  ///< 对应标签的分值，如 0.7，若无法得到该值，填 1
} OVD_AI_Lable;

typedef struct {
    OVD_char            model[MAX_LEN_64];      ///< 调用的算法模型，命名方式: “厂商-模型”，如华为婴儿看护:”hw-babyCare”，杭研智能识别:”hy-smartCheck”
    OVD_char            *params;                ///< json格式字符串，包含模型调用相关参数，通用字段，根据 model 字段进行相应解析
} OVD_AI_Lable_Verify;

typedef struct {
    OVD_char            type[MAX_LEN_8];        ///< 图片类别；”main”：主图，要过算法的原图，”cover”:封面图，”small”：小图
    OVD_char            format[MAX_LEN_8];      ///< 图片格式，如"jpg", "png"
    OVD_int32           height;                 ///< 图片高度
    OVD_int32           width;                  ///< 图片宽度
    OVD_int32           size;                   ///< 图片大小，单位为字节
    OVD_int64           timestamp;              ///< 图片产生时间戳，单位秒
    OVD_char            *data;                  ///< 图片数据
} OVD_AI_Image_Info;

typedef struct {
    OVD_int32           channel;                  ///< 通道号
    OVDAlarmType        alarm_type;               ///< 对应的疑似报警类型
    OVD_char            ai_ability [MAX_LEN_32];  ///< 设备具备的AI能力名称

    OVD_int32           ai_label_count;           ///< 端侧识别到的ai标签 个数
    OVD_AI_Lable        *ai_labels;               ///< 端侧识别到的ai标签 数组

    OVD_AI_Lable_Verify ai_label_verify;          ///< 期望进行标签核验而调用的算法模型，若为null，则表示不需要核验

    OVD_int32           ai_image_count;           ///< AI识别到的图像信息 个数
    OVD_AI_Image_Info   *ai_image_infos;          ///< AI识别到的图像信息 数组
}OVD_AIDIY_Info;

typedef struct {
    OVD_int32           min_report_interval;       ///< 最小上报间隔，单位为秒
}OVD_AIDIY_Config;


/**
 * @brief 机器人告警类型
 *
 */
typedef enum
{
    ROBOT_ALARM_EXAMPLE = 0,
    ROBOT_ALARM_MAX,
}RobotAlarmType;

/**
 * @brief 机器人告警描述
 *
 */
typedef struct
{
    OVD_int64           timeStamp;          ///< 告警时间戳 (单位为秒)
    RobotAlarmType      AlarmType;          ///< 报警类型
    OVD_char*           desc;               ///< 告警描述
}OVDRobotAlarmInfo;

/**
 * @brief 低功耗蓝牙信息
 *
 */
typedef struct
{
    OVD_uchar device_type_id[16];               ///< 联楹平台上分配的产品ID
    OVD_uchar device_ble_mac[16];               ///< 设备低功耗蓝牙MAC地址，eg：0FAB4C2ADB10
}ovd_ble_info_t;

/**
 * @brief 低功耗蓝牙配网Server参数
 *
 */
typedef struct
{
    OVD_uchar local_name[16];                   ///< 蓝牙本地名称
    OVD_uchar adv_manufacture_data[16];         ///< 蓝牙广播厂商数据
    OVD_int32 adv_manufacture_data_len;         ///< 蓝牙广播厂商数据长度
    OVD_ushort service_uuid;                    ///< 服务UUID
    OVD_ushort characteristic_uuid_down_write;  ///< 写特征UUID
    OVD_ushort characteristic_uuid_up_notify;   ///< 通知特征UUID
    OVD_ushort characteristic_uuid_up_indicate; ///< 指示特征UUID
    OVD_ushort duration;                        ///< 广播时间，单位秒，0表示默认设置
    OVD_ushort timeout;                         ///< 超时时间，单位秒，0表示默认设置
} ovd_ble_gatt_t;

/**
 * @brief 蓝牙配网解析结果
 *
 */
typedef enum {
    OVD_BLE_INVALID_FRAME = -2,     ///< 无效数据帧
    OVD_BLE_PARSE_UNKNOW = -1,      ///< 未知错误
    OVD_BLE_PARSE_SUCCESS = 0,      ///< 解析成功
    OVD_BLE_UNCOMPLETE = 1         ///< 正确解析，但未完成，需要传入下一帧继续解析
} ovd_ble_parse_e;

/**
 * @brief 蓝牙配网数据类型
 *
 */
typedef enum {
    OVD_BLE_CHRS_WRITE = 0,     ///< 低功耗蓝牙写
    OVD_BLE_CHRS_NOTIFY = 1,    ///< 低功耗蓝牙通知
    OVD_BLE_CHRS_INDICATE = 2   ///< 低功耗蓝牙指示
} ovd_ble_chrs_e;

/**
 * @brief WIFI信息描述
 *
 */
typedef enum
{
    OVD_WIFI = 0,           ///< IWiFi
    OVD_SSID_WIFI = 1,      ///< ssid和WIFI密码
    OVD_PHONE = 2,          ///< bindid
    OVD_STRING = 3,
    OVD_XX_SSID_WIFI = 7,   ///< 数据读取完整(包含ssid，密码及phoneid/bindid)
    OVD_XX_LESS_DATA = 9    ///< 厂商继续输入新的图片或者字符串
} OVDWifiInfoType;

/**
 * @brief WIFI ssid 描述
 *
 */
typedef struct
{
    OVD_char ssid[33]; ///<wifi ssid
    OVD_int32 ssidLen; ///<ssid长度
    OVD_char pwd[80]; ///<wifi密码
    OVD_int32 pwdLen; ///<密码长度
}OVDSSIDWiFiInfo;

/**
 * @brief WIFI ssid 扩展描述(例如二维码解析结果)
 *
 */
typedef struct
{
    OVD_char ssid[WIFI_SSID_LEN];   ///< wifi的ssid
    OVD_int32 ssidLen; ///< wifi ssid长度
    OVD_char pwd[WIFI_PWD_LEN];     ///< wifi密码
    OVD_int32 pwdLen;///<密码长度
    OVD_char phone[16+1];///<app用户绑定信息
    OVD_int32 phoneLen;///<绑定信息长度
    OVD_char extension[256];        ///< 扩展信息字段
    OVD_int32 extensionLen;///<扩展字段长度
}OVDXXSSIDWiFiInfo;

/**
 * @brief 休眠(视频通道关闭推流)原因枚举
 *
 */
typedef enum
{
    OVD_HIBERNATE_OVC_NOTIFY = 0,   ///< OVC信令交互，需要保持一段时间不断电
    OVD_HIBERNATE_P2P_NAT = 1,      ///< P2P开始NAT穿透打洞
    OVD_HIBERNATE_P2P_OPEN = 2,     ///< 正在P2P通话
    OVD_HIBERNATE_OTHER = 3
}OVDHibernateReason;

/**
 * @brief p2p 卡录像结构体
 *
 */
typedef struct
{

    OVD_uint64 StartStamp;       ///< 录像开始时间（单位为秒）
    OVD_uint64 EndStamp;         ///< 录像接收时间（单位为秒）

}OVD_DMERecordFileInfo;

/**
 * @brief 卡录像区间信息
 *
 */
typedef struct
{
    OVD_int32               Count;            ///< 卡录像片段个数
    OVD_DMERecordFileInfo sectioninfo[64];    ///< 卡录像一个连续区间的信息
}OVD_DMERecordFileListPerPage;

/**
 * @brief ovd 状态
 *
 */
enum ovd_state
{
    OVD_ONLINE = 1, ///< 设备在线
    OVD_UPDATING=2,  ///< 设备升级
    OVD_ABNORMAL=3, ///< 异常
    OVD_LOWPOWER_OFFLINESLEEP=4  ///< 不支持远程唤醒的低功耗设备，在进入休眠时需要上报该状态，app借此展示为休眠状态而非离线状态。
};

/**
 * @brief 供电类型枚举
 *
 */
enum ovd_power_mode
{
    OVD_POE_POWER = 1,      ///< POE供电
    OVD_5V_POWER  = 2,      ///< 5V供电
    OVD_BATTERY_POWER = 3,  ///< 电池供电
};


/**
 * @brief channels 状态
 *
 */
enum ovd_channel_state
{
    OVD_CHANNEL_OFFLINE=0,  ///< 离线
    OVD_CHANNEL_ONLINE=1,   ///< 在线
    OVD_CHANNEL_ABNORMAL=3, ///< 异常
    OVD_CHANNEL_DISABLE=4,  ///< 不使能（休眠）
    OVD_CHANNEL_UNUSE=5,    ///< 通道未启用
};

/**
 * @brief 设备通道简单信息，在回调中传递给SDK
 *
 */
typedef struct
{
    OVD_int32 channelnum;           ///< 通道编号，从0开始
    enum ovd_channel_state state;   ///< 通道状态
    OVD_char name[MAX_LEN_256];     ///< 通道字符串名称
} Simplechannelinfo;

/**
 * @brief 设备简单信息，在回调中传递给SDK
 *
 */
typedef struct
{
    enum ovd_state state;                               ///< 1：设备在线  2：设备升级  3：异常
    Simplechannelinfo channelarray[4];                  ///< 默认4个channel及以下的设备，使用channelarray
    Simplechannelinfo *ext_channelarray;                ///< 超过4个channel以上的设备，则使用ext_channelarray，由厂商自行分配内存
    OVD_int32 channelcount;                             ///< 设备总共的视频通道数，例如双通道摄像机的通道个数就是2
} SimpleOVDinfo;


/**
 * @brief 数据帧信息
 *
 */
typedef struct
{
    OVD_bool iskey_frame;  ///< 是否是I帧(必填)
    OVD_uchar rev[3];      ///< 预留1
    OVD_ushort width;      ///<宽
    OVD_ushort height;     ///<高
    OVD_ushort framerate;  ///< 帧率(必填)
    OVD_ushort gop;        ///< (必填)
    OVD_uchar rev2[20];    ///< 预留2
}OVD_VideoFrameInfo;

/**
 * @brief 音频帧信息定义
 *
 */
typedef struct
{
    OVD_uint32 samplesRate;    ///< 采样率(必填)
    OVD_ushort channelNumber;  ///< 声道数
    OVD_ushort bitsPerSample;  ///< 位深
    OVD_uint32 samplePerFrame; ///< 每帧采样点数
    OVD_uchar rev[20];         ///< 预留
}OVD_AudioFrameInfo;

/**
 * @brief 音视频帧信息定义
 *
 */
typedef struct
{
    OVDContentType frame_type;          ///< 帧类型，音频or 视频
    OVDAVCodec codec_type;              ///< 编码类型
    OVD_uint64 timestamp;               ///< 绝对时间戳，单位为毫秒
    OVD_uint64 relative_timestamp;                     ///< 相对时间戳，单位为毫秒，用以转换为真实的pts>
    OVD_VideoFrameInfo video_info;      ///< 视频帧
    OVD_AudioFrameInfo audio_info;      ///< 音频帧
    OVD_uint32 framebuf_len;            ///< 帧长度
    OVD_char *frame_buf;                ///< 帧内容
    OVD_uchar rev[8];                   ///< 预留
}OVD_FrameInfo;

/**
 * @brief 存储设备状态
 *
 */
typedef enum OvdLocalStorageStatus{
    OVD_LOCAL_STORAGE_STATUS_READY,                     /**< 设备外部存储(如TFCard)插入，且状态正常 */
    OVD_LOCAL_STORAGE_STATUS_NOT_READY,                 /**< 设备无外部存储   */
    OVD_LOCAL_STORAGE_STATUS_ABNORMAL,                  /**< 设备外部存储异常 */
    OVD_LOCAL_STORAGE_FILE_SYSTEM_NOT_SUPPORT,          /**< 不支持的文件系统 */
} ovd_local_storage_status_e;

/**
 * @brief 存储设备类型
 *
 */
typedef enum OvdLocalStorageType{
    OVD_LOCAL_STORAGE_TYPE_TFCARD,  ///< TF卡
} ovd_local_storage_type_e;


/**
 * @brief 存储设备状态和路径信息
 *
 */
typedef struct {
    enum OvdLocalStorageType   storage_type;                    /**< 存储类型 */
    enum OvdLocalStorageStatus storage_status;                  /**< 存储设备状态 */
    OVD_char storage_path[OVD_MAX_LOCAL_STORAGE_PATH_LEN];      /**< 存储设备路径，storage_type=OVD_LOCAL_STORAGE_STORAGE_STATUS_READY 时有效 */
} OvdLocalStorageInfo, ovd_local_storage_info_t;



/**
 * @brief 设置云存套餐信息
 *
 */
typedef enum
{
    /// 无云存套餐
    OVD_CMD_SET_CLOUD_NONE,
    /// 云存全天套餐
    OVD_CMD_SET_CLOUD_ALL_DAY,
    /// AIGC事件智能套餐
    OVD_CMD_SET_CLOUD_AIGC_EVENT,
    /// 云存事件套餐
    OVD_CMD_SET_CLOUD_EVENT,
} OVD_SetCloudInfo_e;


/**
 * @brief 设置直播状态信息
 *
 */
typedef enum
{
    /// 无任何信息
    OVD_CMD_SET_LIVE_NONE = 0,
    /// 直播开启
    OVD_CMD_SET_LIVE_START,
    /// 直播关闭
    OVD_CMD_SET_LIVE_STOP,
    /// 最大状态信息
    OVD_CMD_SET_LIVE_ALL,
} OVD_SetLiveInfo_e;


/**
 * @brief 设置aov功能状态开关
 *
 */
typedef enum OvdAovStatus
{
    /// 无效状态
    OVD_CMD_AOV_STATUS_NONE = 0,
    /// AOV功能开启
    OVD_CMD_AOV_STATUS_ON = 1,
    /// AOV功能关闭
    OVD_CMD_AOV_STATUS_OFF = 2,
    /// 最大状态信息
    OVD_CMD_AOV_STATUS_ALL,
} OVD_AOV_STATUS_e;


/*******************  软探针相关  **********************/
/**
 * @brief ping和traceroute结构体
*/
typedef struct
{
    OVD_char   url[256];  ///<下发ping和traceroute地址
    OVD_int32     delay_time; ///< ping和traceroute延时
    OVD_int32     result_data; ///< ping成功率/traceroute跳数
} OvdProbeData, ovd_probe_data_t;

/**
 * @brief 获取设备运行状态信息回调
 *
 */
typedef enum OvdProbeDevRunningInfo
{
    /// 获取设备丢包率，单位%。int型。设备端使用ifconfig指令来获取网卡的数据传输情况。packetLossRate=(drop_rx+drop_tx)/(rx+tx)
    OVD_CMD_GET_PACKET_LOSSRATE,
    /// 获取设备WIFI强度，单位dB(负值)。int型。
    OVD_CMD_GET_WIFI_QUALITY,
    /// 获取设备CPU占用率，单位%。int型。
    OVD_CMD_GET_CPU_LOAD,
    /// 获取设备内存占用率，单位%。int型。
    OVD_CMD_GET_MEMORY_LOAD,
    /// 获取设备剩余内存，单位KB。int型。
    OVD_CMD_GET_MEMORY_AVAILABLE,
    /// 获取设备运行时长，单位s。int型。
    OVD_CMD_GET_RUNTIME,
    /// 获取设备运行总内存，单位KB。int型。
    OVD_CMD_GET_MEMORY_TOTAL,
    /// 获取设备运行总闪存，单位KB。int型。
    OVD_CMD_GET_FLASH_TOTAL,
    /// 获取芯片厂家。结构体ovd_common_info_t。
    OVD_CMD_GET_CHIP_SUPPLIER,
    /// 获取芯片型号。结构体ovd_common_info_t。
    OVD_CMD_GET_CHIP_MODEL,
    /// 获取和家智话SDK版本号。结构体ovd_common_info_t。
    OVD_CMD_GET_HJZH_SDKVER,
    /// 获取Andlink SDK版本号。结构体ovd_common_info_t。
    OVD_CMD_GET_ANDLINK_SDKVER,
    /// 下发ping地址，返回ping 5次的平均延时ms和成功率（丢包率）（要求3s内返回回调结果）。结构体ovd_probe_data_t。
    OVD_CMD_GET_PING,
    /// 下发Traceroute地址，返回traceroute跳到目的地跳了多少次，跳到目的地用了多少时间（ms）（要求10min内返回回调结果）。结构体ovd_probe_data_t
    OVD_CMD_GET_TRACEROUTE,
    /// 获取设备4G信号强度
    OVD_CMD_GET_RSSI_4G,
    /// 获取设备供电方式。int型，具体类型参考枚举ovd_power_mode
    OVD_CMD_GET_POWER_MODE
} ovd_probe_devrunning_info_e;

/**
 * @brief 异常场景
 *
 */
typedef enum OvdProbeExceptionType
{
    /// 服务器连接状态
    /// 设备和服务器建立TCP连接出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_TCP_ERROR,
    /// 设备分配服务器出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_ASSIGN_ERROR,
    /// 设备分配的服务器为空  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_ASSIGN_NULL,
    /// 设备连接服务器时域名解析失败  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_HOST_RESOLVE_ERROR,
    /// 设备连接服务器时证书校验失败  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_CA_ERROR,
    /// 设备和服务器建立加密连接出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_SSL_ERROR,
    /// 设备和服务器的心跳出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_HEARTBEAT_ERROR,
    /// 设备从服务器接收数据超时  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_READ_TIMEOUT,
    /// 设备从服务器接收数据出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_READ_ERROR,
    /// 设备向服务器发送数据超时  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_WRITE_TIMEOUT,
    /// 设备向服务器发送数据出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_WRITE_ERROR,
    /// 设备系统时间出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_SYS_TIME_ERROR,
    /// 设备向服务器校验失败  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_AUTH_ERROR,
    /// 码流状态
    /// 检测到没有视频流  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_NO_VIDEO,
    /// 检测到没有音频流  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_NO_AUDIO,
    /// 检测到帧大小超过限制  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_LARGE_FRAME,
    /// 检测到音视频存在不同步  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_VIDEO_AUDIO_UNSYNC,
    /// 流数据缓存目录路径IO出错  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_STREAM_DATA_PATH_ERROR,
    /// 视频模糊  \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_VIDEO_BLUR,
    /// SD卡异常，停止录制  \n  回调指针类型： NULL
    OVD_RUNNING_STATUS_SDCARD_ABNORMAL,
    /// 镜头遮挡  \n  回调指针类型： NULL
    OVD_RUNNING_STATUS_CAMERA_BLOCK,
    /// senser异常  \n  回调指针类型： NULL
    OVD_RUNNING_STATUS_SENSER_ABNORMAL,
    //// 第一个I帧时间 \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_IFRAME_TIME,
    /// 供电模式转变 \n 回调指针类型：NULL
    OVD_RUNNING_STATUS_PWOERMODE_CHANGE,

    /// 直播时延汇报
    OVD_LIVEPUBLISH_REPORT,
    /// 直播拉流失败汇报
    OVD_LIVEPUBLISH_FAIL_REPORT,
    /// 配网成功相关指标汇报
    OVD_BIND_REPORT,
    OVD_ALARM_CSEG_REPORT, //告警云存上传时延汇报
    /// 配网失败
    OVD_BIND_FAIL_REPORT,
    /// 告警失败
    OVD_ALARM_FAIL_REPORT,
    /// 告警指标
    OVD_ALARM_REPORT,
    /// 重启信息上报
    OVD_REBOOT_REPORT,
    /// 信令事件上报
    OVD_DME_EVENT_REPORT,
    /// 信令请求上报
    OVD_DME_REQUEST_REPORT,
    /// 直播丢包信息上报
    OVD_LIVE_DROP_PACKET_REPORT,
    /// 云存异常
    OVD_CSEG_FAIL_REPORT,
    /// 云存状态上报
    OVD_CSEG_STAT_REPORT,
    /// AI进程异常
    OVD_AI_PROCESS_EXCEPTION,

    //其他信息上报
    /// 声音来源上报
    /// 注: 该事件会对ovd_probe_except_info_t中content字段的字符串格式进行校验,json协议内容如下：
    /// 校验规则如下：
    /// 1、content字符串需要为json类型
    /// 2、建议json字符串进行压缩
    /// 3、json格式要求：
    /// {
    ///   "sound_type": "string",  // 业务类型
    ///   "description": {
    ///       "action": "start" | "end",  // 声音播放的开始或结束
    ///       "sound_source": "string",   // 声音来源（例如：本地文件（如路径+名称）、网络流等）
    ///       "speaker": "open" | "close",  // 扬声器的开/关状态
    ///       "volume": 0-100,  // 音量，范围通常是 0-100，表示音量级别
    ///       "timestamp":"string", //声音播放的开始或结束时间点（e.g.,"2025-03-07T12:34:56"）
    ///   }
    /// }
    /// 4、sound_type字段可选项
    ///   重启提示音: "sound_reboot"     升级提示音: "sound_upgrade"
    ///   配网提示音: "sound_net"        异常音频来源: "sound_other"
    ///   对讲语音: "sound_talk"         告警语音: "sound_alarm"
    ///   云广播语音: "sound_broadcast"  媒体播放（提示音、音乐、视频等）:"sound_media"
    ///
    OVD_SOUND_SOURCE_REPORT,
    /// AOV切换慢状态上报
    OVD_AOV_CHANGE_TOO_SLOW_REPORT,
    /// 直播卡顿率信息上报
    OVD_LIVE_DELAY_REPORT,
    /// 看门狗检测上报
    OVD_MONITOR_REPORT,
    /// 看门头重启上报
    OVD_MONITOR_REBOOT_REPORT,
    OVD_TRACEROUTE_REPORT,

    OVD_EXCEPTION_MAX,
} ovd_probe_exception_type_e;

/**
 * @brief 软探针接口工作模式
 *
 */
typedef enum OvdProbeSyncType
{
    OVD_PROBE_SYNC_MFR      =   1,      ///< 同步接口，回调给设备
    OVD_PROBE_ASYNC_MFR     =   2,      ///< 异步接口，回调给设备
    OVD_PROBE_SYNC_NOMFR    =   3,      ///< 同步接口，不回调给设备
    OVD_PROBE_ASYNC_NOMFR   =   4,      ///< 异步接口，不回调给设备
} ovd_probe_sync_type_e;

/**
 * @brief 软探针异常信息描述
 *
 */
typedef struct {
    enum OvdProbeExceptionType  exception_type; ///< 异常事件详细类型
    OVD_int64 createTime;                       ///< 产生异常的时间,单位s，有需要就传值，不需要传0，软探针内部获取时间
    OVD_char content[4096];                     ///< 详细信息,比如关键打印
    OVD_char module[64];                        ///< 调用模块,比如关键打印
    enum OvdProbeSyncType synctype;             ///< 接口同异步选择
} OvdProbeExceptInfo, ovd_probe_except_info_t;

/**
 * @brief 休眠服务器信息结构体
 */
typedef struct{
    OVD_char hdomain[256];  ///< 休眠服务器域名
    OVD_int32 hport;        ///< 休眠服务器端口
    OVD_int32 hb_interval;  ///< 休眠心跳间隔
    OVD_char  hb_token[17]; ///< 休眠心跳口令
}ovd_hserverinfo_t;

#define OVD_CLOUD_LOG_CAPTURE_SIZE  2048  ///< 上传到云平台的实时日志的采集buffer长度

#define LOG_GET_FILENAME() \
    (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__) ///< 获取日志文件名称

OVD_void ovd_printf(OVDLogLevel level,OVD_char *tag, OVD_char *content, ...);///< SDK内部接口

/**
 * @brief 本接口用于将厂商系统日志、固话日志、andlink等日志信息采集至SDK日志文件，由SDK统一管理并定时上传至云平台。
 *        每条日志自动附加文件名、行号及函数名，便于问题追踪，且单条日志长度需限制在1024字节以内。
 * @param level  日志级别（见 OVDLogLevel）
 * @param tag    日志标签，用于分类过滤,格式"LOG_DOMAIN/MY_TAG",LOG_DOMAIN标识业务领域，MY_TAG标识模块日志tag,
 *               如："OVDSDK/CSEG",标识安防SDK中的云存业务。
 * @param fmt    格式化字符串（需符合 printf 语法）
 * @param ...    可变参数（与 fmt 中的占位符匹配）
 * @note  单条日志长度不得超过 1024 字节，超长部分将被截断。
 */
#define OVD_LOG(level, tag, fmt, ...) \
    ovd_printf( \
        (level), \
        (tag), \
        "[%s:%d][%s] " fmt, \
        LOG_GET_FILENAME(), __LINE__, __func__, \
        ##__VA_ARGS__ \
    )

/**
 * @brief 告警回调状态类型
 *
 */
typedef enum
{
    ALARMNOTIFYTYPE_NULL                = 0,
    ALARMNOTIFYTYPE_SETMINDURATION      = 1,        ///< 设置状态告警最短时长，缺省为10秒，厂商触发的告警时长应不低于该最短时长
    ALARMNOTIFTYPE_SETMAXDURATION       = 2,        ///< 设置状态告警最长时长，缺省为3600秒，厂商触发的告警时长应不高于该最长时长
    ALARMNOTIFYTYPE_ALARMEXTEND         = 3,        ///< 通知厂商，有状态告警结束过短，sdk将自动延长本次告警
    ALARMNOTIFYTYPE_ALARMTRUNCATE,                  ///< 通知厂商，有状态告警高于最长时长，已被截断

    ALARMNOTIFYTYPE_MAX,

} OVD_AlarmNotifyType_e;


/**
 * @brief 推流回调状态类型
 *
 */
typedef enum
{
    STREAMNOTIFYTYPE_NULL               = 0,
    STREAMNOTIFYTYPE_LIVEPULISH         = 1,        ///< 直播开启
    STREAMNOTIFYTYPE_LIVESTOP           = 2,        ///< 直播关闭
    STREAMNOTIFYTYPE_EVENTCSEGSTART,                ///< 事件直存套餐开启
    STREAMNOTIFYTYPE_FULLCSEGSTART,                 ///< 全天直存套餐开启
    STREAMNOTIFYTYPE_CSEGSTOP,                      ///< 直存关闭

    STREAMNOTIFYTYPE_MAX,

} OVD_StreamNotifyType_e;

/**
 * @brief 告警信息
 *
 */
typedef struct
{
    int channel;  ///<通道号
    OVDAlarmType type; ///<告警类型
}OVD_AlarmNotifyMsg_t;

/**
 * @brief AOV数据信息
 *
 */
typedef struct
{
    OVD_int32 size;         ///< 未被读取的AOV数据大小
    OVD_int64 start_ts;     ///< 录像开始时间(UTC毫秒时间)
    OVD_int32 frame_num;    ///< 未被读取的AOV帧数量（I帧+P帧）
    OVD_int64 durataion;    ///< 未被读取的AOV时长（毫秒）
}OVD_AOV_Info;

/**
 * @brief AOV数据读取状态
 *
 */
typedef enum
{
    OVD_aov_read_finish = 1,         ///< AOV数据读完
    OVD_aov_read_frame_error = 2,    ///< 帧异常
}OVD_aov_read_status_e;

/**
 * @brief 设备绑定状态
 *
 */
typedef enum
{
    /// 设备解绑
    OVD_unbinding_status = 0,
    /// 设备绑定
    OVD_binding_status = 1,
} OVD_binding_status_e;

/**
 * @brief 智能语音交互状态
 *
 */
typedef enum
{
    SPEECHSTATUSNOTIFYTYPE_NULL                 = 0,
    SPEECHSTATUSNOTIFYTYPE_MEDIACONNECTSUCCESS,         ///< 媒体通道建立成功 回调数据指针类型:NULL
    SPEECHSTATUSNOTIFYTYPE_MEDIACONNECTFAIL,            ///< 媒体通道连接失败 回调数据指针类型:NULL
    SPEECHSTATUSNOTIFYTYPE_MEDIADISCONNECT,             ///< 媒体通道连接断开 回调数据指针类型:NULL
    SPEECHSTATUSNOTIFYTYPE_MEDIAASRTEXT,                ///< 媒体语音识别结果转文本输出回调 回调数据指针类型: OVDSpeechBackMediaText_t
    SPEECHSTATUSNOTIFYTYPE_MEDIAAUDIOUNIDENTIFIED,      ///< 媒体语音识别未识别到关键词 回调数据指针类型:NULL
    SPEECHSTATUSNOTIFYTYPE_BACKMEDIA_TEXT,              ///< 云端流式下发合成的文本数据 回调数据指针类型：OVDSpeechBackMediaText_t
    SPEECHSTATUSNOTIFYTYPE_VADEND,                      ///< VAD识别结束通知
    SPEECHSTATUSNOTIFYTYPE_MEDIAEND,                    ///< 云端单次下行音频结束通知
    //SPEECHSTATUSNOTIFYTYPE_NEWSESSION,                ///< 被动录音/多轮会话开启
    SPEECHSTATUSNOTIFYTYPE_MAX,

}OVDSpeechStatusNotifyType_e;

/**
 * @brief 智能语音交互云台控制
 *
 */
typedef enum
{
    SPEECHPTZCONTROLTYPE_NULL                   = -1,
    SPEECHPTZCONTROLTYPE_NOD                    = 0,    ///< 点头
    SPEECHPTZCONTROLTYPE_SHAKE,                         ///< 摇头
    SPEECHPTZCONTROLTYPE_STEPUP,                        ///< 向上步进
    SPEECHPTZCONTROLTYPE_STEPDOWN,                      ///< 向下步进
    SPEECHPTZCONTROLTYPE_STEPLEFT,                      ///< 向左步进
    SPEECHPTZCONTROLTYPE_STEPRIGHT,                     ///< 向右步进
    SPEECHPTZCONTROLTYPE_MAX,

}OVDSpeechPTZControlType_e;

/**
 * @brief 智能语音交互设备控制
 *
 */
typedef enum
{
    SPEECHDEVCONTROLTYPE_NULL                       = -1,
    SPEECHDEVCONTROLTYPE_TRACE                      = 0,    ///< 移动追踪
    SPEECHDEVCONTROLTYPE_LED,                               ///< 指示灯
    SPEECHDEVCONTROLTYPE_NIGHTVISION,                       ///< 夜视
    SPEECHDEVCONTROLTYPE_SLEEP,                             ///< 休眠
    SPEECHPDEVCONTROLTYPE_ALERTAREA,                        ///< 警戒区域
    SPEECHPDEVCONTROLTYPE_MAX,

}OVDSpeechDEVControlType_e;

/**
 * @brief 智能语音交互文本信息
 *
 */
typedef struct
{
    OVD_char text[4096]; ///<文本内容
    int last_flag;      ///< 是否是最后一条
}OVDSpeechBackMediaText_t;

/**
 * @brief 智能语音交互控制固话呼叫类型
 *
 */
typedef enum
{
    SPEECHCALLTYPE_NULL = 0,
    SPEECHCALLTYPE_VOICECALL,           ///< 音频呼叫
    SPEECHCALLTYPE_VIDEOCALL,           ///< 视频呼叫
    SPEECHCALLTYPE_MAX,
}OVDSpeechCallType_e;

/**
 *
 * @brief 云端AI控制指令
 *
 *
 */

enum OVD_CLOUD_AI_CtrlCmd
{
    OVD_CLOUD_AI_GET_PACK_INSTALL_STATUS        = 1,      ///< 获取算法包的下载和安装进度 data 数据结构参考 \ref OVD_InstallStatus
    OVD_CLOUD_AI_GET_ALL_ALG_ID                 = 2,      ///< 获取所有已安装的算法包的ID data 数据结构参考 \ref OVD_Alg_PackListInfo
    OVD_CLOUD_AI_GET_ALG_RUNNING_INFO           = 3,      ///< 获取算法应用运行信息 data 数据结构参考 \ref OVD_Alg_RunningInfo
    OVD_CLOUD_AI_GET_SOFTPROBE_INFO             = 4,      ///< 获取软探针上报信息 data 数据结构参考 \ref OVD_ALG_SoftprobeInfo
    OVD_CLOUD_AI_GET_DEVICE_RESOURCE            = 5,      ///< 获取设备资源，内存，flash等
    OVD_CLOUD_AI_GET_ALL_MODEL_PACKAGE            = 6,      ///< 获取设备安装的所有算法模型包

    OVD_CLOUD_AI_INSTALL_ALG_PACKAGE            = 101,    ///< 安装算法包 data 数据结构参考 \ref OVD_ALG_PackInfo
    OVD_CLOUD_AI_REMOVE_ALG_PACKAGE             = 102,    ///< 删除指定的算法包 数据类型 char *
    OVD_CLOUD_AI_RESET_ALG_CONFIG               = 103,    ///< 重置算法应用的配置 数据类型 char *
    OVD_CLOUD_AI_CLEAN_DIR                      = 104,    ///< 删除ai算法包安装目录里的全部内容 数据类型 NULL
    OVD_CLOUD_AI_FORMAT_AI_PARTITION            = 105,    ///< 格式化算法包存储分区 数据类型 NULL
    OVD_CLOUD_AI_INSTALL_UUID                   = 106,    ///< 批量安装算法包时的uuid(废弃)
    OVD_CLOUD_AI_INSTALL_ALL_PACKAGE            = 107,    ///< 批量安装算法包 data 数据结构参考 \ref OVD_ALL_ALG_PackInfo
};

/**
 * @brief 算法包信息
 *
 */
typedef struct OVD_ALG_PackInfo
{
    char *algId;     ///< 算法包标识
    char *version;  ///< 算法包版本
    char *url;      ///< 算法包下载地址
    char *md5;      ///< 算法包MD5值
    int size;       ///< 算法包大小，单位字节
    char *success_tone;  ///<成功提示音
    char *failure_tone;  ///<失败提示音
    int pkg_type; ///<包类型，1是AI进程包，2是算法模型包
}OVD_ALG_PackInfo;

/**
 * @brief 全量算法包信息
 *
 */
typedef struct OVD_ALL_ALG_PackInfo
{
    char *uuid;     //任务id
    int count;      //算法包个数
    OVD_ALG_PackInfo *info;     //算法包信息
}OVD_ALL_ALG_PackInfo;

/**
 * @brief 算法包下载安装状态
 *
 */
enum OVD_AI_SubscribeStatus
{
    SUBSCRIBE_STATUS_INIT = 0,
    SUBSCRIBE_STATUS_DOWNLOADING = 1, ///<下载中
    SUBSCRIBE_STATUS_INSTALLING = 2, ///<安装中
    SUBSCRIBE_STATUS_DONE = 3, ///<完成安装
    SUBSCRIBE_STATUS_FAIL = 4, ///<失败
    SUBSCRIBE_STATUS_DOWNLOADFAIL = 5,///<下载失败
    SUBSCRIBE_STATUS_SIGNFAIL = 6///<鉴权失败
};

#define OVD_ALG_ID_LENGTH   64      ///< 算法ID最大长度

/**
 * @brief 算法包下载安装状态
 *
 */
typedef struct OVD_InstallStatus
{
    enum OVD_AI_SubscribeStatus status; ///<状态
    OVD_uint32 progress;///<进度
    char algId[OVD_ALG_ID_LENGTH];///<算法id
}OVD_InstallStatus;

#define OVD_ALG_VERSION_INFO    32     ///<模型包版本信息最大长度
#define OVD_ALG_MD5_LENGTH      40    ///<算法包MD5长度
#define OVD_ALG_RUNNING_STRING_LEN 32 ///<算法包基础数据类型
#define OVD_ALG_PROCESS_MAX    8     ///< 最大支持的进程个数，修改改值时，不要超过32
#define OVD_ALG_PACKAGE_MAX    16    ///< 最多支持的模型包个数
#define OVD_ALG_ID_LENGTH       64   ///< 算法包ID长度

/**
 * @brief 算法包运行状态
 *
 */
enum OVD_AI_RunningStatus
{
    OVD_AI_PROCESS_CLOSED = 0,      // 关闭
    OVD_AI_PROCESS_RUNNING = 1,     // 运行
    OVD_AI_PROCESS_DOWNLOADING = 2, // 下载中
    OVD_AI_PROCESS_OPENING = 3,     // 开启中
    OVD_AI_PROCESS_CLOSEING = 4,    // 关闭中
    OVD_AI_PROCESS_NOTFOUND = 5,    // 未安装
    OVD_AI_PROCESS_DOWNLOADFAIL = 6,// 下载失败
    OVD_AI_PROCESS_INSTALLING = 7,  // 安装中
    OVD_AI_PROCESS_INSATLLFAIL = 8, // 安装失败
    OVD_AI_PROCESS_SIGNFAIL = 9,     // 签名验证失败
    OVD_AI_PROCESS_INSTALLDONE = 10  // 安装成功
};

/**
 * @brief 算法删除信息
 *
 */
typedef struct OVD_Alg_DeleteInfo
{
    char algId[OVD_ALG_ID_LENGTH]; ///<算法包标识
    int pkg_type; ///<包类型，1是AI进程包，2是算法模型包
}OVD_Alg_DeleteInfo;

/**
 * @brief 算法运行状描述
 *
 */
typedef struct OVD_Alg_RunningInfo
{
    char algId[OVD_ALG_ID_LENGTH];///<算法包标识
    char version[OVD_ALG_VERSION_INFO];///<算法包版本
    char md5[OVD_ALG_MD5_LENGTH];///<算法包MD5值
    enum OVD_AI_RunningStatus status;///<运行状态
    char statusDesc[OVD_ALG_RUNNING_STRING_LEN];///<运行状态描述
    int pkg_type; ///<包类型，1是AI进程包，2是算法模型包
}OVD_Alg_RunningInfo;

/**
 * @brief 获取算法包列表信息
 *
 */
typedef struct OVD_Alg_PackListInfo
{
    int algCount;       ///< 数组中ID个数
    char algIds[OVD_ALG_PROCESS_MAX][OVD_ALG_ID_LENGTH];///<算法包标识
}OVD_Alg_PackListInfo;

/**
 * @brief 算法包软探针上报信息
 *
 */
typedef struct OVD_ALG_SoftprobeInfo
{
    char *buffer;   ///<软探针数据
    int size;///<数据长度
}OVD_ALG_SoftprobeInfo;

/**
 * @brief 算法包业务设备信息
 *
 */
typedef struct OVD_ALG_DeviceResource
{
    int free_cma_memory;   ///<剩余内存大小 单位：字节
    int free_flash;        ///<剩余flash大小 单位：字节
} OVD_ALG_DeviceResource;

/**
 * @brief 算法模型包信息
 *
 */
typedef struct OVD_ALG_ModelPackageInfo
{
    char algId[OVD_ALG_ID_LENGTH]; ///<算法包标识
    char version[OVD_ALG_VERSION_INFO];///<算法包版本
    char md5[OVD_ALG_MD5_LENGTH];///<算法包MD5值
    char install_status[OVD_ALG_RUNNING_STRING_LEN];///<安装状态
} OVD_ALG_ModelPackageInfo;

/**
 * @brief 所有算法模型包信息
 *
 */
typedef struct OVD_ALG_AllModelPackageInfo
{
    int package_num; ///<模型包数量
    OVD_ALG_ModelPackageInfo model_pkg_info[OVD_ALG_PACKAGE_MAX];///<模型包信息
} OVD_ALG_AllModelPackageInfo;






#ifdef __cplusplus
}
#endif

#endif /* __OVD_DEFINE_H__ */
