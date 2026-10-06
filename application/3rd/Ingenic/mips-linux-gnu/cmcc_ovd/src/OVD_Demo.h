#ifndef __DEMO_DEMO_H_INCLUDED
#define __DEMO_DEMO_H_INCLUDED

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <malloc.h>
#include "cJSON.h"

#include "OVD_OpenAPI.h"
#include "OVD_define.h"
#include "OVD_openapi_lock.h"

#ifdef CONFIG_VOIP_SWITCH
#include "cmcc_ovd_rtc_demo.h"
#endif


#define GETVERSION "getversion"
#define QUIT     "quit"
#define BLE_NET  "ble_net"
#define WAVE_NET "wave_net"
#define AP_NET "ap_net"
#define PIC_NET  "pic_net"
#define PIC_STR_NET  "pic_str_net"
#define SERVICE_OK "service_ok"
#define SERVICE_STOP "service_stop"
#define DME_STOP "dme_stop"
#define DME_START "dme_start"
#define PROBE "probe"
#define WIFI_DOWN "wifi_down"
#define TRIGGER_ALARM "trigger_alarm"
#define ALARM_END "alarm_end"
#define AIDIY_REPORT "AIDIY_report"
#define AV_READY "av_ready"
#define AV_MOD "av_mod"
#define PUSH_AV "push_av"
#define PUSH_VIDEO "push_video"
#define PUSH_CHANNEL "push_channel"
#define PUSH_AUDIO "push_audio"
#define PUSH_END "push_end"
#define BATTERY "battery"
#define PTZ_REPORT "ptz_report"
#define EXCEPTION "exception"
#define SLEEP "sleep"
#define SLEEP1 "sleep1"
#define SLEEP2 "sleep2"
#define WAKEUP "wakeup"
#define CHANGECODEC "change_codec"
#define CHANGECODEC2 "change_codec2"
#define CHANGEAOV "change_aov"
#define STARTLPAOV "start_lp_aov"
#define STOPLPAOV "stop_lp_aov"
#define LIVECAPTURESTART "liveCapture_start"
#define LIVECAPTURESTOP  "liveCapture_stop"
#define RECORDCAPTURESTART "recordCapture_start"
#define RECORDCAPTURESTOP "recordCapture_stop"
#define USER   "user"
#define ABSNORMAL_EVENT "abevent"
#define NETWORKCONNECTION "networkconnection"
#define PC_READY "pc_ready"     // 点云数据准备好
#define START_PUSH_PC "start_push_pc"       // 开始推送点云数据
#define STOP_PUSH_PC "stop_push_pc"       // 停止推送点云数据


//卡录像相关指令
#define PLUGINCARD "plugincard"
#define PLUGOUTCARD "plugoutcard"
#define STARTCARDENCRYPT "startcardencrypt"  //启动卡加密
#define STOPCARDENCRYPT "stopcardencrypt"   //关闭卡加密
#define GETCARDENCRYPTSTATUS "getcardencryptstatus" //获取卡加密状态
#define CHANGECARDREADSPEED "changecardspeed" //调整卡读倍率
#define CHANGEFREEDISK  "changefreedisk"    //调整剩余卡容量

//AI智能语音交互
#define AISPEECHQUERYAUTH "aispeechqueryauth"
#define AISPEECHOPENSESSION "aispeechopensession"
#define AISPEECHCLOSESESSION "aispeechclosesession"
#define AISPEECHINTERRUPTSESSION "aispeechinterruptsession"

//门锁相关测试指令
#define UNLOCKWAYNOTIFY             "unlockwaynotify"
#define UNLOCKEXCEPTIONNOTIFY       "unlockexceptionnotify"
#define LOCKEVENTNOTIFY             "lockeventnotify"
#define LOCKBATTERYREPORT           "lockbatteryreport"
#define LOCKVOLTAGEREPORT           "lockvoltagereport"

#define GETHSERVERINFO                  "gethserverinfo"


extern char FILEPATH[256];
extern char CFG_FILEPATH[256];
extern char alarmFILEPATH[256];
extern char snapshotFILEPATH[256];
extern char videoFILEPATH[256];
extern char h264filepath[256];
extern char h265filepath[256];
extern char p2ptfh264filepath[256];
extern char p2ptfh265filepath[256];
extern char audioFILEPATH[256];
extern char qrFILEPATH[256];
extern char qrSTRING[256];
extern char waveFILEPATH[256];
extern char audioplayFILEPATH[256];
extern char uploadlogFILEPATH[256];
extern char g_sdcardpath[256];
extern int videoFPS;
extern int VideoCap;
extern int AudioCap;
extern int Device_IMAGE_BUFF_SIZE; //截图图片初始大小
extern int Device_AV_BUFF_DURATION;		//设备上电后，音视频内容最长缓存时间
extern int Device_DEBUG_TS;                //是否将每一帧的音视频时间戳输出
extern int Device_CSEG_DURATION;           //单个云存切片时长
extern int Device_CSEG_MAXCOUNT;           //云存模块最多切片
extern int Device_SNDDROPDELAY;            //底层流媒体srt连接缓存最大时延
extern char Device_Live_Filepath[128];     //直播dump文件的文件路径
extern char Device_Live_Fileprefix[32];    //直播dump文件的文件前缀
extern char Device_Record_Filepath[128];   //卡录像dump文件的文件路径
extern char Device_Record_Fileprefix[32];  //卡录像dump文件的文件前缀

#define DEMO_CONFIG_ENABLE_VIDEO_SUB    0

extern int OVD_DEMOQR(const char *fileName);
extern int OVD_DEMOVoice(const char *fileName);

extern int OVD_DEMOE_DeviceInit(char *path);
extern int OVD_DEMOE_InitCapbility(OVDCapInfoV2_t *demo_cap);
//extern int OVD_DEMOE_InitDeviceInfo();
//extern int OVD_DEMOE_InitDeviceConfInfo();
//extern int OVD_DEMOE_InitClientInfo();
//extern void OVD_DEMOE_InitLogConfInfo();
extern void OVD_DEMOE_PrintDeviceInfo();
extern int OVD_DEMOE_InitSetableKeys(const char *path);
extern int OVD_DEMO_SetConfig();
extern void *get_data(FILE * in,int32_t *pwidth,int32_t *pheight,uint32_t *pbitCount,uint32_t *size);
extern void OVD_DEMOE_RecognizerStart(void);
extern void OVD_DEMOE_RecognizerFinish(int type, void *info, int infoLen);
extern int OVD_Demo_init_channel(int channel);
extern int OVD_Demo_deinit_channel(int channel);
extern int OVD_Demo_start_push(int channel);
extern int OVD_Demo_stop_push(int channel);
extern unsigned char do_cmd_ChangeCodec(int argc, char *argv[]);
extern unsigned char do_cmd_ChangeCodec2(int argc, char *argv[]);
extern unsigned char do_cmd_ChangeAOV(int argc, char *argv[]);
extern unsigned char do_cmd_start_lowpower_aov(int argc, char *argv[]);
extern unsigned char do_cmd_stop_lowpower_aov(int argc, char *argv[]);




typedef struct{
    char on[64];// <1.41.0新增，可读可写,字符串,是否开启对应检测区域，默认值为空字符串>
    int left_up_x;
    int left_up_y;
    int left_bottom_x;
    int left_bottom_y;
    int right_up_x;
    int right_up_y;
    int right_bottom_x;
    int right_bottom_y;
}OVDZoneInfo;


typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    int sensitivity;               //探测灵敏度，范围为0-100，在isEffect为true时有效
    int mode;
    OVDZoneInfo zoneInfo;
}OVDAlarmInfo;

typedef struct
{
    int on;
    char starttime[64]; //<必填，可读可写,字符串型：通道使能时间，格式“HH:MM:SS”>
    char endtime[64]; //<必填，可读可写,字符串型：通道关闭时间，格式“HH:MM:SS”>
    int repeat;//<必填，位运算替代，字符串数组: 计划重复的星期号，数组内的的字符串枚举值可选为Mon(第0位)/Tue(第1位)/Wed(第2位)/Thu(第3位)/Fri(第4位)/Sat(第5位)/Sun(第六位)> 用位运算（0~127）
}OVDAreaScheduleInfo;

//可选，通道定时使能计划，若设备不支持，该字段不存在
typedef struct
{
    int on;  //<必填，可读可写,布尔型：定时使能计划开关>
    char start_time[64]; //<必填，可读可写,字符串型：通道使能时间，格式“HH:MM:SS”>
    char shutdown_time[64]; //<必填，可读可写,字符串型：通道关闭时间，格式“HH:MM:SS”>
    int repeat;//<必填，位运算替代，字符串数组: 计划重复的星期号，数组内的的字符串枚举值可选为Mon(第0位)/Tue(第1位)/Wed(第2位)/Thu(第3位)/Fri(第4位)/Sat(第5位)/Sun(第六位)> 用位运算（0~127）
}OVDSwitchScheduleInfo;

typedef struct
{
    int speech_on;
    int speech_vol;
    int speech_repeat;
    OVD_bool speech_repeat_constant;
    int speech_interval;
    char speech_url[1024];
    int light_on;
    int light_mode;
    int light_dur;
    OVD_bool light_dur_constant;
    int buz_on;
    int buz_dur;
}OVDAreaLinkage;


typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    int expel;                     //智能驱离开关
    int sensitivity;               //探测灵敏度，范围为0-100，在isEffect为true时有效
    OVDAreaScheduleInfo areaschedule;
    OVDAreaLinkage arealinkage;
    OVDZoneInfo zoneInfo[10];
    int targetType;
    int staymode_time;
}OVDAlarmInfo_area;

typedef struct{ //1.41.0新增,支持多个越界线
    OVD_bool on;
    int statistics_line_A_x;
    int statistics_line_A_y;
    int statistics_line_B_x;
    int statistics_line_B_y;
    int statistics_mode_style;
    int statistics_mode_direction;
}OVDLineInfo;

typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    OVDAreaScheduleInfo areaschedule;
    OVDAreaLinkage arealinkage;
    int targetType;
    OVDLineInfo LineInfo;   // 第一条规则线
    OVDLineInfo LineInfo_another[10];   //额外规则线
}OVDAlarmInfo_transgression;

typedef struct{
    char name[1024];
    char option60[1024];
    char ip[256];
    char mac[256];
    int on;                      //在线状态
}OVDDevManagementInfo;

typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    int linkage_on;                //摄像头联动开关
    int sensing_mode;              //感知模式
    OVDAreaScheduleInfo areaschedule;
    int entire_dev_num;
    OVDDevManagementInfo entire_devmanagement[10];      //全量设备列表
    int added_dev_num;
    OVDDevManagementInfo added_devmanagement[10];       //新增设备列表

}OVDAlarmInfo_wifisensing;

typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    int sensitivity;               //探测灵敏度，范围为0-100，在isEffect为true时有效
    int crying_pacify_on;                        //啼哭安抚使能开关
    int crying_pacify_audio_playing_count;       //啼哭安抚音频播放次数
    int crying_pacify_audio_playing_volumn;      //啼哭安抚音频播放音量值
    char crying_pacify_audio_playing_url[1024]; //啼哭安抚音频内容的获取url，默认为空字符串
    OVDZoneInfo zoneInfo;
}OVDAlarmInfo_cry;

typedef struct{
    int on;                         //跌倒检测配置启动开关
    int time;                       //跌倒检测告警时延范围，（单位：秒），默认值为60秒
}falldown_detect;

typedef struct{
    int on;                         //卫生间滞留启动开关
    int time;                       //卫生间滞留时长，（单位：分钟），默认值为20分钟
}wc_trapped;

typedef struct{
    int isEffect;                  //是否具备此能力，若设备无此能力，置为0；具备此能力，置为1
    int on;                        //是否打开此功能，在isEffect为1时有效，0时无效
    falldown_detect detect;        //跌倒检测
    wc_trapped trapped;            //卫生间滞留
}OVDAlarmInfo_3d_sensor;

//sleep_monitor
typedef struct{
    int on;
    OVDAlarmInfo sleep_detection;
    OVDAlarmInfo face_cover;
    OVDAlarmInfo wakeup_detection;
}OVDDemoAISleepMonitor;

//表情识别
typedef struct{
    int on;
}OVDDemoAIExpressionRecognition;

typedef struct{
    OVDAlarmInfo   ioAlarm;              //外部报警配置，必填
    OVDAlarmInfo   faceAlarm;            //人脸识别配置，必填
    OVDAlarmInfo_cry   cryAlarm;             //哭声侦测配置，必填
    OVDAlarmInfo   voiceAlarm;           //声音侦测配置，必填
    OVDAlarmInfo   motionAlarm;          //移动侦测配置，必填
    OVDAlarmInfo_area   alertareaAlarm;          //警戒区域告警配置，必填
    OVDAlarmInfo_transgression   transgressionAlarm;          //警戒区域告警配置，必填
    OVDAlarmInfo_wifisensing   wifisensingAlarm;          //wifi感知配置，必填
    OVDAlarmInfo   crossAlarm;           //拌网配置，必填
    OVDAlarmInfo   bodyAlarm;            //人形侦测，必填
    OVDAlarmInfo   pirAlarm;             //PIR侦测，必填
    OVDAlarmInfo   lossLockAlarm;        //撬锁侦测，必填
    OVDAlarmInfo_3d_sensor falldown_3dAlarm;   //3d传感器跌倒检测
    OVDAlarmInfo   petAlarm;            //宠物侦测
}OVDAlarmsSet;

typedef struct
{
    int on;                   //是否开启图像翻转，0否     1是
    int horflip;              //水平翻转，1表示翻转，0表示正常,必填,
    int verflip;              //垂直翻转，1表示翻转，0表示正常,必填,
}OVDMirrorFlip;

typedef struct
{
    int on;//<必填，是否上报AI平台>
    OVDAreaScheduleInfo maskSchedule;
    OVDAreaLinkage masklinkage;

}OVDMaskDeteSet; //口罩检测


typedef struct{
    int point_a_x;
    int point_a_y;
    int point_b_x;
    int point_b_y;
}OVDStatistcsLineInfo;

typedef struct
{
    int speech_on;
    int speech_vol;
    int speech_repeat;
    char speech_url_enter[1024];
    char speech_url_leave[1024];
}OVDDemoAIPassengerSpeechSet;

typedef struct
{
    int on;//<必填，是否上报AI平台>
    int exposure_on;//<必填，可读可写,布尔型，人脸曝光度调节功能开关
    int osd_status;//<可读写，布尔型，能力集支持客流统计support_osd_status时才会展示，开关开启时，osd时间戳下面展示客流统计相关信息，该字段出厂默认值为true>
    int exposure_adjust;//<必填，可读可写，整形，人脸曝光度调节，0~100>
    char mode[64];//<可选，可读可写，字符串，人脸抓拍模式，“quality”:代表质量抓拍，“quick”:代表快速抓拍
    int quality_value;//<可选，可读写，整形，选择质量抓拍时，人脸质量抓拍的阈值设置，0~100>
    int statistics_mode;//<可读写，整型，统计方式,0代表上出下进，1代表上进下出,  A朝向B方向左侧为上，A朝向B方向右侧为下>
    int sensitivity; // <必填，可读可写，整型，人脸探测灵敏度，0~100>
    OVDStatistcsLineInfo lineInfo;//<客流规则线,将图形10000等分，A的x小于等于B的x，当A的x等于B的x时，A的y小于B的y>
    OVDZoneInfo zoneInfo;
    OVDSwitchScheduleInfo switchschedule; //检测时间段
    OVDDemoAIPassengerSpeechSet speech; //语音播放
}OVDDemoAIPassengerSet; //人脸AI上报


typedef struct
{
    int on;//<必填，是否上报AI平台>
    int exposure_on;//<必填，可读可写,布尔型，人脸曝光度调节功能开关
    int exposure_adjust;//<必填，可读可写，整形，人脸曝光度调节，0~100>
    char mode[64];//<可选，可读可写，字符串，人脸抓拍模式，“quality”:代表质量抓拍，“quick”:代表快速抓拍
    int quality_value;//<可选，可读写，整形，选择质量抓拍时，人脸质量抓拍的阈值设置，0~100>
    int sensitivity; // <必填，可读可写，整型，人脸探测灵敏度，0~100>
    OVDZoneInfo zoneInfo;
    OVDMaskDeteSet maskDeteInfo;
}OVDDemoAIFaceSet; //人脸AI上报

typedef struct
{
    int on;//布尔型，使能开关
    int mask_on;//布尔型，是否开启检测口罩
    int cap_on;//布尔型，是否检测帽子
    int clothes_on;//布尔型，是否检测工服
    char clothes_color[64];////字符串，默认白色，目前端侧可选配置为：white,black,red
    int sensitivity; // <必填，可读可写,整型：探测灵敏度， 0 - 100, 目前PC侧设置1~10；可按比例下发>
    OVDZoneInfo zoneInfo;
}OVDDemoAIKitchenSet; //明厨亮灶检测


typedef struct
{
    int on;//布尔型，使能开关
    int sensitivity; // <必填，可读可写,整型：探测灵敏度， 0 - 100, 目前PC侧设置1~10；可按比例下发>
    int quality;//<可读写，整型，质量评分（0~100）>
    int capture_mode;//<可读写，枚举整型，0：最优推图，1最快推图，2定时推图>
    int scheduled_capture_time;//<可读写,整型，定时推图间隔时间>
    int detect_site;//<可读可写，整型，检测地点，1：道路，2：岗亭（默认）>
    int exit_mode;//<可读可写，整型，出口标识，检测地点只为岗亭时有效，1：出口，车头离开，车尾进入2：入口，车头进入，车尾离开>
    OVDAreaLinkage vehlinkage;
    OVDZoneInfo zoneInfo;
}OVDDemoAIVehicleSet; //车型检测

typedef struct
{
    int on;//布尔型，使能开关
    char detect_mode_list[64];//<必填，字符串，"electromobile"、"bike"，默认电瓶车>
    int sensitivity; //<可读可写,整型：探测灵敏度， 0 - 100, 目前PC侧设置0~100>，准确度算法内部处理了
    int quality;//<可读写，整型，质量评分（0~100）>
    int capture_mode;//<可读写，整型，值对应枚举，0:最优推图(默认)，1最快推图，2定时推图>
    int scheduled_capture_time;//<可读写,整型，定时推图间隔时间>
    OVDAreaLinkage nonvehlinkage;
    OVDZoneInfo zoneInfo;
}OVDDemoAINonmotorvehicleSet; //非机动车检测（梯控）


typedef struct{
    int  floor_id;                    //<可读写，整型，floor_id 从0 开始>
    char floor_detail[64];            //<可读写，字符串，楼层信息描述最大支持8个汉字，16个字符>
    OVDStatistcsLineInfo floor_line;  //<楼层标记线,将图形10000等分，A点坐标在B点坐标左侧>
}OVDSingleFloorInfo;

typedef struct{
    int  floorNum;                    //<可读写，整型，floor_id 从0 开始>
    OVDSingleFloorInfo *floorInfos;  //<楼层标记线,将图形10000等分，A点坐标在B点坐标左侧>
}OVDAIParabolicAerialFloorConf;


//point
typedef struct{
    int pointId;
    int point_x;
    int point_y;
}OVDSinglePointInfo;

typedef struct  {
    int pointsNum;                /**< 侦测区域点个数 */
    OVDSinglePointInfo *points;         /**< 侦测区域点坐标信息列表，包含每个检测区域的坐标 */
} OVDAreaPointsInfo;

typedef struct{
    int areaId;
    OVDAreaPointsInfo *areaPoints;
}OVDSingleAreaInfo;

typedef struct  {
    int areasNum;                /**< 侦测区域个数 */
    OVDSingleAreaInfo *areas;         /**< 侦测区域信息列表，包含每个检测区域的信息 */
} OVDAreasInfo;


typedef struct
{
    int on;//<必填，是否上报AI平台>
    int sensitivity; // <必填，可读可写，整型，人脸探测灵敏度，0~100>
    OVDAIParabolicAerialFloorConf *pfloorInfo;//楼层设置, 用于标记楼层
    OVDAreasInfo *pDetectZoneInfo;            //检测区域
    OVDAreasInfo *pShieldZoneInfo;            //屏蔽区域
}OVDDemoAIParabolicAerialSet;//高空抛物

typedef struct
{
    int on;//布尔型，使能开关
    int regional_id;//<可读写，整型，区域id号，从0开始>
    char regional_name[64];//<可读写，字符串，区域信息描述，最大支持8个汉字，16个字符>
    int regional_people_count;// <可读写，整型，默认30,范围1-100；一旦超过预警人数触发告警，告警上报走SDK>
    int alarm_report_duration;//<可读可写，整型，告警上报间隔时间：默认5分钟，1-30分钟>
    int detect_result_report_duration;//<可读可写，整型，区域人数定时上传：默认5分钟（取5分钟内平均值），范围1-30分钟。区域人数通过1400协议上传 >
    int osd_status;//<可读可写，布尔型, OSD叠加人数：默认为支持>
    OVDAreaScheduleInfo schedule; //<检测时间段，可读可写>
    OVDAreaLinkage linkage;       //告警联动策略，若不支持则无此选项
    OVDZoneInfo zoneInfo;         //若支持区域配置，则有改配置，将图形10000等分
}OVDDemoAIRegionalPeopleStatPlan;//区域人数统计


typedef struct
{
    int on;//布尔型，使能开关
    OVDDemoAIRegionalPeopleStatPlan detect_plan;
}OVDDemoAIRegionalPeopleStatSet;//离岗检测


typedef struct
{
    int on;//布尔型，使能开关
    int lane_line_num;//<可读写，整型：1：单个车道线， 2：双车道线， 目前只支持配置1和2>
    int direction[2]; //可读写，车道线防线方向，枚举整型，0：向上，1向下
    OVDZoneInfo zoneInfo[2];         //若支持区域配置，则有改配置，将图形10000等分
}OVDDemoAILaneLine;//车道线

typedef struct
{
    int on;//布尔型，使能开关
    int regional_id;//<可读写，整型，区域id号，从0开始>
    char regional_name[128];//<可读写，字符串，区域信息描述，最大支持127个字符，不支持特殊字符>
    int on_duty_people_count;// <可读写，整型，默认1,范围1-能力集支持的最大值>
    int off_duty_duration;//<可读可写，整型，离岗时长：默认10分钟，1-能力集支持的最大值>
    OVDAreaScheduleInfo schedule; //<检测时间段，可读可写>
    OVDZoneInfo zoneInfo;         //若支持区域配置，则有改配置，将图形10000等分
}OVDDemoAIOffDutyPlan;//离岗检测计划配置


typedef struct
{
    int on;//布尔型，使能开关
    OVDDemoAIOffDutyPlan detect_plan;
}OVDDemoAIOffDuty;//离岗检测


typedef struct
{
    int on;//布尔型，使能开关
    OVDAreaScheduleInfo schedule; //<检测时间段，可读可写>
    OVDZoneInfo zoneInfo;
    OVDAreaLinkage linkage;       //告警联动策略，若不支持则无此选项
}OVDDemoAIHelmet;//头盔检测

typedef struct
{
    int on;//布尔型，使能开关
    int call_on;//布尔型，手势呼叫功能开关
    int call_duration;//
    OVDAreaScheduleInfo call_schedule; //<检测时间段，可读可写>
    int help_on;//布尔型，手势求助功能开关

}OVDDemoAIGesture;//头盔检测

typedef struct
{
    int on;//布尔型，使能开关
    OVDAreaScheduleInfo schedule;   // 检测时间段，可读可写
    OVDAreasInfo *areas;            // 侦测区域信息列表，包含每个检测区域的信息
    OVDAreaLinkage linkage;         // 告警联动策略，若不支持则无此选项
}OVDDemoAISmokefire;//烟火检测

typedef struct
{
    char name[128];                /**< 人脸名字 */
    char url[128];                /**< 人脸图片地址 */
}OVDSingleFaceInfo;//人脸信息

typedef struct
{
    int facesNum;                /**< 人脸个数 */
    OVDSingleFaceInfo *face_info;       /**< 人脸信息 */
}OVDDemoAIFaceManagement;//人脸管理

typedef struct
{
    int on;//布尔型，使能开关
    OVDDemoAIFaceManagement face_management; //人脸管理
    OVDAreaScheduleInfo schedule;   // 检测时间段，可读可写
    OVDAreaLinkage linkage;         // 告警联动策略，若不支持则无此选项
}OVDDemoAIFamilyRecognition;//家人识别

typedef struct
{
    int on;//布尔型，使能开关
    OVDZoneInfo zoneInfo;
}OVDDemoAIPrivacyMasking;//隐私遮蔽

typedef struct
{
    int on;//布尔型，使能开关
    int sensitivity; //<可读可写,整型：探测灵敏度， 0 - 100, 目前PC侧设置0~100>，准确度算法内部处理了
    OVDZoneInfo zoneInfo;
    OVDAreaScheduleInfo schedule;   // 检测时间段，可读可写
    OVDAreaLinkage linkage;         // 告警联动策略，若不支持则无此选项
}OVDDemoAIFallDetection;//跌倒检测

typedef struct
{
    OVDDemoAIFaceSet AIface;
    OVDDemoAIKitchenSet AIkitchen;
    OVDDemoAIVehicleSet AIvehicle;
    OVDDemoAINonmotorvehicleSet AInonmotorvehicle;
    OVDDemoAIPassengerSet AIPassenger;
    OVDDemoAIParabolicAerialSet AIParabolicAerial;
    OVDDemoAIRegionalPeopleStatSet AIRegionalPeopleStat;
    OVDDemoAILaneLine AILaneLine;
    OVDDemoAIOffDuty AIOffDuty;
    OVDDemoAIHelmet AIHelmet;
    OVDDemoAIGesture AIGesture;
    OVDDemoAISmokefire AISmokefire;
    OVDDemoAIFamilyRecognition AIFamilyRecognition;
    OVDDemoAIPrivacyMasking AIPrivacyMasking;
    OVDDemoAIFallDetection AIFallDetection;
    OVDDemoAISleepMonitor   sleepAlarm;
    OVDDemoAIExpressionRecognition AIExpressionRecognition;

}OVDDemoAISet;

typedef struct
{
    int on;         //<必填，可读可写,布尔型：使能开关>
    long long cycle;     //<必填，可读可写,整型：自动维护（重启）的最短周期，单位秒，例如7天自动重启，可以设置为604800>
    char start[64];     //<必填，可读可写,整型：自动维护（重启）开始时间，格式“HH:MM:SS”>设备可在start到end时间内随机选择一个时间重启>
    char end[64];   //<必填，可读可写,整型：自动维护（重启）结束时间，格式“HH:MM:SS”>
    char last_reboot[64]; //<必填，只读,字符串型：上次重启时间，格式”YY-MM-DDTHH:MM:SS”>
}OVDAutorebootSet;

//可选 OSD字体大小设置
typedef enum {
    OVD_OSDFONT_VENDOR_DEF = 0,
    OVD_OSDFONT_SMALL = 1,
    OVD_OSDFONT_MEDIUM = 2,
    OVD_OSDFONT_BIG = 3,
} OVDOsdFontSize;

//可选 OSD字体颜色设置
typedef struct {
    int red;
    int green;
    int blue;
} OVDOsdColor;

//可选 OSD设置坐标
typedef struct{
    int x;
    int y;
} OVDOsdPos;

//可选， 通道视频的OSD配置，若设备不支持OSD，该字段不存在
typedef struct
{
    int on;
    int mode;//0-绝对模式，1-左上角，2-左下角，3-右下角，4-右上角，默认值为0
    int id;//正整数，mode非0时,1-5，文字位置从上到下，按照id从小到大排列，默认值为1
    char text[128]; //<必填，可读可写, 字符串类型：OSD文本内容，若为空串，则当前没有OSD显示>
    OVDOsdFontSize osdfont; //<必填，可读可写，整型：字号大小，1为小字号，2为大字号>
    OVDOsdPos osdpos;
    OVDOsdColor color;
}OVDOsdInfo;

typedef struct{
    int before; //整形，默认告警录制前时长
    int after; //整形，默认告警录制后时长
}recording_duration_t;

typedef struct{
    int parking_notice; //停车通知告警
    int parking_collision; //停车碰撞告警
    recording_duration_t driving_collision; //行车碰撞
    recording_duration_t voice_capture; //语音事件
}alarm_duration_t;

typedef struct{
    int collision_alarm_sensitivity; //碰撞告警灵敏度
    alarm_duration_t alarm_duration; //告警配置时长
}dashcam_info_t;

typedef struct{
    int tracetarget; //追踪目标
}OVDTraceSet;

typedef struct{
    int channel;                  //通道号， 必填
    int on;                       //<必填，可读可写,布尔型：通道使能开关，通道关闭后，应停止该通道的流媒体采集、告警、云台操作等相关功能>
    int smart_mode_switch_2;      //<1.56版本新增，可选，当低功耗协议版本为2时，替代之前的smart_mode_switch, 整型，0代表节电模式，1智能模式（缺省），2常电模式，其他备用
    int static_scence_ABR_on;     //<可读可写，bool型，支持静态场景码率压降功能时该字段存在，出厂默认值 为true，开启该功能时具体要求参考入库规范相关介绍>

    int bitrate_mode;       //整数：码率模式设置,1.44
    int bitrate_average;    //整数：平均码率设置
    int bitrate_peak;       //整数：峰值码率设置
    int bitrate_statically; //整数：静态码率设置
    int framerate;          //整数：帧率设置

    char *video_file;
    char *audio_file;
    char *pointcloud_file;
    OVDVideoDataFormat videoinfo; 	//视频信息,必填,详细可见结构体OVDVideoDataFormat
    OVDAudioDataFormat audioinfo; 	//视频信息,必填,详细可见结构体OVDAudioDataFormat
    OVDMirrorFlip         flipInfo;    //图像翻转信息，必填,详见结构体OVDMirrorFlip
    OVDDemoAISet        aiset;       //ai设置
    OVDAlarmsSet          alarms;      //各种告警设置信息，必填,详见结构体OVDAlarmsSet

    int                audioOutValume;  //扬声器输出音量，可选，0-100为正常值。小于0为不支持
    int                doorbell_volume;  //门铃输出音量，可选，0-100为正常值。小于0为不支持
    int                traceAbility;    //移动跟踪，可选，1支持跟踪，0不支持跟踪
    OVDTraceSet        traces;          //移动追踪相关配置，若设备不支持，该字段不存在
    int                has_speek;       // 是否外接了音柱设备，2 已连接， 1 未连接， -1 未知
    OVDSwitchScheduleInfo switchschedule; //可选，通道定时使能计划，若设备不支持，该字段不存在
    dashcam_info_t dashcam_info;//可选，行车记录仪配置参数
    int osd_logo;
    int osdcount; //在配置文件中设置osd组数
    OVDOsdInfo         osdinfo[10]; //可选， 通道视频的OSD配置，若设备不支持OSD，该字段不存在
}OVDChannelsInfo;

typedef struct{
    char start_time[8];
    char end_time[8];
}timed_schedule_t;

typedef struct{
    int light_brightness;//庭院灯亮度控制
    int control_mode;//控制模式
    OVD_bool light_on;//庭院灯手动模式下灯控制
    timed_schedule_t timed_schedule[5];//休眠时间段控制
    int timed_schedule_size;//数组大小
}OVDYardLightConfig_t;

typedef struct{
    OVD_bool on; //使能开关
    int brightness; //<整形：亮度, 0 - 100，默认值为60
    OVDAreaScheduleInfo schedule;   // 检测时间段
}OVD_fill_light_Config_t;

typedef struct{
    OVD_bool EBO_nofall_on; //<可选，可读可写,布尔型：机器人防跌落是否打开。若OVD不具备机器人防跌落能力，该字段不存在>
    OVD_bool EBO_laser_on; //<可选，可读可写,布尔型：机器人激光是否打开。若OVD不具备机器人激光能力，该字段不存在>
    OVD_bool EBO_track_on;
    int EBO_track_type;
    OVD_bool EBO_collision_on;
    OVD_bool EBO_reminder_on;
    OVD_bool EBO_cruise_on;
    int EBO_speed_adjust; //可选,可读可写,整形:机器人速度调节,1-5，默认值为2, 1.40.0新增
    char start_time[8];
    int time_long;
    int repeat;
}OVD_EBO_Config_t;

typedef struct{
    OVD_bool OWL_food_on; //<可选，可读可写,布尔型：互动出粮是否打开。若OVD不具备OWL机器人能力，该字段不存在>
    OVD_bool OWL_track_mode; //<可选，可读可写,布尔型：碰撞互动是否打开。若OVD不具备OWL机器人能力，该字段不存在>
    OVD_bool OWL_touch_mode; //<可选，可读可写,布尔型：触摸互动是否打开。若OVD不具备OWL机器人能力，该字段不存在>
    OVD_bool OWL_sound_on; //<可选，可读可写,布尔型：互动音效是否打开。若OVD不具备OWL机器人能力，该字段不存在>
    OVD_bool OWL_speed_switch; //<可选，可读可写,int型：速度切换，0-低速，1-中速，2-高速。若OVD不具备OWL机器人能力，该字段不存在>
    char start_time[8];//<字符串,"hh:mm" 格式,开始巡航时间,默认当前时间>
    int  repeat;//<枚举字符串数组: 计划重复的星期号,数组内的的字符串枚举值可选为Mon/Tue/Wed/Thu/Fri/Sat/Sun,默认重复每天>
}OVD_OWL_Config_t;

typedef struct{
    OVD_bool dashcam_findcar_on; //<1.45.0-行车记录仪智能寻车功能开关，布尔型： 是否开启，默认值为false>
    OVD_bool dashcam_rescue_on;  //<1.45.0-行车记录仪智能救援功能开关，布尔型： 是否开启，默认值为false>
}
OVD_dashcam_Config_t;

typedef struct{
    char   start_time[8];    // <字符串，“hh:mm” 格式>
    char   end_time[8];    // <字符串，“hh:mm” 格式>
    int cruiseId;
    int time_long;
    int  repeat; //<必填，位运算替代，字符串数组: 计划重复的星期号，数组内的的字符串枚举值可选为Mon(第0位)/Tue(第1位)/Wed(第2位)/Thu(第3位)/Fri(第4位)/Sat(第5位)/Sun(第六位)> 用位运算（0~127）

}
OVD_mobocam_cruise_t;

typedef struct{
    int speedswith; 
    int mobocamcount; //下发自动巡检的数组数量
    OVD_mobocam_cruise_t cruise_config[3];
    int track_type;
    OVD_bool track_on;

}
OVD_mobocam_Config_t;

typedef struct{

    OVD_bool              on;  //<布尔型，是否开启此条计划记录，默认关>
    char              name[32]; //<字符串型，计划名称,UTF8>
    char              time[8];    // <字符串，“hh:mm” 格式>
    int               quantity;
    int               repeat; //<必填，位运算替代，字符串数组: 计划重复的星期号，数组内的的字符串枚举值可选为Mon(第0位)/Tue(第1位)/Wed(第2位)/Thu(第3位)/Fri(第4位)/Sat(第5位)/Sun(第六位)> 用位运算（0~127）
}OVD_feeder_config_t;


typedef struct{
    char              start_time[8];    // <字符串，“hh:mm” 格式>
    char              end_time[8];    // <字符串，“hh:mm” 格式>
    int               bizType;
}OVD_robot_home_t;
typedef struct
{
    int               channelsInfoCount;   //指示有多少个有效的channelsInfo
    OVDChannelsInfo   *channelsInfo;       //channel信息列表
    int               tz;                  //时区信息，必填，例如东八区为8
    OVDAutorebootSet      autorebootinfo; //重启相关配置
    OVDYardLightConfig_t    yardlightconfig;//庭院灯控制
    int               led;  //可选，可读可写,布尔型：Led灯是否打开。若OVD不具备LED灯控制能力，该字段不存在>
    int               light_supplement_lamp_on;//可选，庭院灯开关。
    int               screen; //可选，可读可写,布尔型：屏幕是否打开。若OVD不支持屏幕，该字段不存在>
    
    int               normal_nightvision_mode;// <可选，可读可写，整形，0：自动，1：开启，2：关闭，若OVD不具备普通摄像头夜视模式配置功能，则该字段不存在>
    int               color_nightvision_mode;// <可选，可读可写，整形，0：黑白夜视，1：全彩夜视，2：智能夜视，若OVD不具备彩色摄像头夜视模式配置功能，则该字段不存在>
    int               nightvision_detect_mode;// < 夜视侦测类型 0：人形侦测，1：移动侦测，默认值为0>
    OVD_fill_light_Config_t fill_light; //1.55-补光灯控制
    OVD_bool          AI_rules_mode_on;// <1.40.0新增,可读可写,布尔型:叠加智能规则信息显示开关,若OVD不具备叠加智能规则信息显示功能，则该字段不存在>
    OVD_EBO_Config_t  eboconfig;
    OVD_OWL_Config_t  robot_config;
    OVD_mobocam_Config_t  mobocam_config;
    OVD_dashcam_Config_t dashcam_config;
    int               feeder_immed_quantity; //<int型，立即喂食，喂食份数，默认2份>
    int feedercount; //在配置文件中设置计划喂食组数
    OVD_feeder_config_t feeder_plan_config[10];//计划喂食

    int robotcount; //下发机器人管家数组数量
    OVD_robot_home_t robot_home_config[20];//计划喂食

    /** 枪球联动配置项 */
    OVD_bool              linkage_tracking_smart_on;     // <布尔型，1.55.1新增，可读可写，智能枪球联动功能开关，默认关闭>
    OVD_bool              linkage_tracking_manual_on;    // <布尔型，1.55.1新增，可读可写，手动枪球联动功能开关，默认关闭>
}OVDConfigrationInfo;

typedef struct
{
    ovd_device_type_e device_type;

    OVD_CapInfoSdk sdk;
    OVD_CapInfoCamera camera;
    OVD_CapInfoRobot robot;
    OVD_CapInfoMobocam mobocam;
    OVD_CapInfoDashCam dashcam;
    OVD_CapInfoDoorLock doorlock;
    OVD_CapInfoDoorbell doorbell;
    OVD_CapInfoYardLight yardlight;
    OVD_CapInfoBase base;
    OVD_CapInfoPtz ptz;
    OVD_CapInfoLinkageTracking linkage_tracking;
    OVD_CapInfoSpecial special;
    OVD_CapInfoLowPower lowpower;
    OVD_CapInfoVideo video;
    OVD_CapInfoAudio audio;
    OVD_CapInfoAlarm alarm;
    OVD_CapInfoLinkage linkage;

    OVDAIFaceCapInfo ai_face;
    OVDAIKitchenCapInfo ai_kitchen;
    OVDAIVehicleCapInfo ai_vehicle;
    OVDAINonmotorvehicleCapInfo ai_nonvehicle;
    OVDAIPassengerCapInfo ai_passenger;
    OVDAIRegionalPeopleStatCapInfo ai_regional_people;
    OVDAIParabolicAerialCapInfo ai_parabolic;
    OVDAILaneLineCapInfo ai_laneline;
    OVDAIOffDutyCapInfo ai_offduty;
    OVDAIHelmetWearingCapInfo ai_helmetwearing;
    OVDAIGestureRecognitionCapInfo ai_gesturerecognition;
    OVDAISmokeFireCapInfo ai_smokefire;
    OVDAIFamilyRecognitionCapInfo ai_familyrecognition;
    OVDAIPrivacyMaskingCapInfo ai_privacymasking;
    OVDAIFallDetectionCapInfo ai_falldetection;
    OVDAISleepMonitorCapInfo ai_sleepmonitor;
    OVDAIExpressionRecognitionCapInfo ai_expressionrecognition;
    OVDCloudAICapInfo cloudai;
    OVDAISpeechCapInfo aispeech;
    OVDLullbyCapInfo lullby;
    OVDAIDIYCapInfo aidiy;
}OVD_DEMO_Capinfo_t;

extern OVDConfigrationInfo OVD_DEMO_ConfigureInfo;
extern OVDDeviceInfo       OVD_DEMO_DeviceInfo;
extern OVDLogParam         OVD_DEMO_LogParam;
extern OVDClientParam      OVD_DEMO_Client;
extern OVDCapInfo          OVD_DEMO_CapInfo;
extern OVD_DEMO_Capinfo_t  OVD_DEMO_CapInfoV2;
extern OVDSDInfo           OVD_DEMO_SdInfo;
extern OVD_char           *OVD_DEMO_Interfaces[4]; //网络接口名称列表

#endif // #ifndef __DEMO_DEMO_H_INCLUDED

