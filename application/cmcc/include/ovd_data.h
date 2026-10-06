#pragma onnce

#include "OVD_define.h"

typedef struct {
    bool on;               // 使能开关
    int sensitivity;       // 灵敏度 0~100
    int staymode_time;     // 逗留模式时长（秒），-1 表示字段不存在/不支持
} OVD_pir_config_t;

typedef struct
{
    char deviceid[MAX_LEN_64+1];  //设备ID，必填项
    char cmei[MAX_LEN_64+1];      //设备cmei，必填项
    char password[MAX_LEN_128+1];  //设备密码，必填项
    char mac[MAX_LEN_32+1];
}OVDLoginInfo;

typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    int sensitivity;               //探测灵敏度，范围为0-100，在isEffect为true时有效
    int mode;
}OVDAlarmInfo;

typedef struct{
    OVD_pir_config_t   pirAlarm;         //PIR侦测，必填
    OVDAlarmInfo   lossLockAlarm;        //撬锁侦测，必填
}OVDAlarmsSet;

typedef struct{
    int channel;                  //通道号， 必填
    int on;                       //<必填，可读可写,布尔型：通道使能开关，通道关闭后，应停止该通道的流媒体采集、告警、云台操作等相关功能>
    int smart_mode_switch_2;      //<1.56版本新增，可选，当低功耗协议版本为2时，替代之前的smart_mode_switch, 整型，0代表节电模式，1智能模式（缺省），2常电模式，其他备用

    int bitrate_mode;       //整数：码率模式设置,1.44
    int bitrate_average;    //整数：平均码率设置
    int bitrate_peak;       //整数：峰值码率设置
    int bitrate_statically; //整数：静态码率设置
    int framerate;          //整数：帧率设置

    OVDVideoDataFormat videoinfo; 	//视频信息,必填,详细可见结构体OVDVideoDataFormat
    OVDAudioDataFormat audioinfo; 	//视频信息,必填,详细可见结构体OVDAudioDataFormat
    OVDAlarmsSet          alarms;      //各种告警设置信息，必填,详见结构体OVDAlarmsSet

    int     audioOutValume;  //扬声器输出音量，可选，0-100为正常值。小于0为不支持
}OVDChannelsInfo;

typedef struct
{
    int on;         //<必填，可读可写,布尔型：使能开关>
    long long cycle;     //<必填，可读可写,整型：自动维护（重启）的最短周期，单位秒，例如7天自动重启，可以设置为604800>
    char start[64];     //<必填，可读可写,整型：自动维护（重启）开始时间，格式“HH:MM:SS”>设备可在start到end时间内随机选择一个时间重启>
    char end[64];   //<必填，可读可写,整型：自动维护（重启）结束时间，格式“HH:MM:SS”>
    char last_reboot[64]; //<必填，只读,字符串型：上次重启时间，格式”YY-MM-DDTHH:MM:SS”>
}OVDAutorebootSet;

typedef struct
{
    int               channelsInfoCount;   //指示有多少个有效的channelsInfo
    int               tz;                  //时区信息，必填，例如东八区为8
    int               normal_nightvision_mode; // <可选，可读可写，整形，0：自动，1：开启，2：关闭，若OVD不具备普通摄像头夜视模式配置功能，则该字段不存在>
    bool               awake_on; //休眠开关
    OVDAutorebootSet  autorebootinfo; //重启相关配置
    OVDChannelsInfo   *channelsInfo;       //channel信息列表
}OVDConfigrationInfo;