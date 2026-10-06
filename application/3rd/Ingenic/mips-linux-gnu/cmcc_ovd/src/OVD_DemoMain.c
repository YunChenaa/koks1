#include "OVD_Demo.h"
#include "OVD_DemoCmd.h"
#include "OVD_DemoTime.h"
#include "OVD_DemoHibernation.h"
#include <stdlib.h>
#include "demoAVPushing.h"
#include "demoAVPushing_util.h"
#include <pthread.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>
#include <fcntl.h>
#include <math.h>
#include "OVD_define.h"
#include <string.h>

char FILEPATH[256];
char CFG_FILEPATH[256];
char alarmFILEPATH[256];
char snapshotFILEPATH[256];
char videoFILEPATH[256];
char h264filepath[256];
char h265filepath[256];
char AOVfilepath[256];
char PointCloudfilepath[256];
char p2ptfh264filepath[256];
char p2ptfh265filepath[256];

char audioFILEPATH[256];
char qrFILEPATH[256];
char qrSTRING[256];
char waveFILEPATH[256];
char audioplayFILEPATH[256];
char uploadlogFILEPATH[256];
char g_sdcardpath[256];

int videoFPS;

int VideoCap;
int AudioCap;


int Device_IMAGE_BUFF_SIZE; //截图图片初始大小
int Device_AV_BUFF_DURATION;		//设备上电后，音视频内容最长缓存时间
int Device_DEBUG_TS;                //是否将每一帧的音视频时间戳输出
int Device_CSEG_DURATION;           //单个云存切片时长
int Device_CSEG_MAXCOUNT;           //云存模块最多切片
int Device_SNDDROPDELAY;            //底层流媒体srt连接缓存最大时延

char Device_Live_Filepath[128];     //直播dump文件的文件路径
char Device_Live_Fileprefix[32];    //直播dump文件的文件前缀
char Device_Record_Filepath[128];   //卡录像dump文件的文件路径
char Device_Record_Fileprefix[32];  //卡录像dump文件的文件前缀

int OVD_DEMO_status; //模拟设备状态1：在线，2：升级中，3：异常
OVDConfigrationInfo OVD_DEMO_ConfigureInfo;   //设备配置
OVDDeviceInfo       OVD_DEMO_DeviceInfo;
OVDLogParam         OVD_DEMO_LogParam;
OVDClientParam      OVD_DEMO_Client;
OVDCapInfo          OVD_DEMO_CapInfo;
OVD_DEMO_Capinfo_t  OVD_DEMO_CapInfoV2;
OVDSDInfo           OVD_DEMO_SdInfo;
OVD_char           *OVD_DEMO_Interfaces[4]; //网络接口名称列表
OVD_CallBackFunList callBackFunList = {0};
char jsonParam[200];

int SDKServiceOK;
int SDKAVContentOK;

int SDKDemoStarted;

extern int recgnizrsucess;
extern int keepawake_time;
int netWorkConfigVoiceResult;
//int netWorkConfigPicResult;
int g_sdcardstate =1; //模拟TF卡状态(0代表为插卡，1代表未插卡)

extern long long global_timeStamp;
extern void setCallBackFunList(OVD_CallBackFunList *pcallBackFunList);
extern void OVD_DEMOE_InitChannelCap(OVD_ChannelCap_t *cap);

pthread_t OVDDEMO_VIDEO_push_pid;
pthread_cond_t OVDDEMO_VIDEO_push_cond;
pthread_mutex_t OVDDEMO_VIDEO_push_mutex;

pthread_t OVDDEMO_AUDIO_push_pid;
pthread_cond_t OVDDEMO_AUDIO_push_cond;
pthread_mutex_t OVDDEMO_AUDIO_push_mutex;


int parralelPushNum;
void* PushAVDataHandler[128][MAX_STREAM_COUNT];
static const char TAG[] = "ovd";

extern char bindId[];

//是否后台执行,默认否
int run_daemon=0;
int TF_H264_H265_CTRL = 1;
int g_freedisk=500;//设置默认剩余卡容量为500MB

#ifdef __LITEOS__
#define INT		"int"
#endif

void OVD_DEMO_PushAVCallback(AVPushContext *cpush, unsigned char* contentData,int dataLen, long long timestamp);

static int exit_flag = 0;
static char avfileName[][128] = {"./resources/video.h264", "./resources/audio.aac"};
static int ParallelAlarmNum[128] = {0};
static OVDUpLoadAlarmInfo *ParallelAlarmInfo[128][10];

unsigned char do_cmd_default(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "命令 %s 未实现\n", argv[0]);
    return CMD_NO_ERR;
}


// 获取版本号
unsigned char do_cmd_getversion(int argc, char *argv[])
{
    char version[64]={0};
    OVD_GetSDKVersion(version,64);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","====SDK 版本号为 %s \n",version);
    return CMD_NO_ERR;
}


unsigned char do_cmd_quit(int argc, char *argv[])
{
#ifdef CONFIG_VOIP_SWITCH
    //固话资源析构
    cmcc_rtc_demo_quit();
#endif
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->设备服务退出 \n");
    OVD_ServiceStop();
    cmd_exec(PUSH_END);
    OVD_deint();
    SDKDemoStarted = 0;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->设备退出<-----\n");
    exit_flag = 1;
    return CMD_NO_ERR;
}

int demo_ap_conf()
{
	OVDXXSSIDWiFiInfo *info;
	info = malloc(sizeof(OVDXXSSIDWiFiInfo));
	int ret=-1, qr_len=0;
	while (ret < 0 ) {
	    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->请连接ap热点\n");
		ret = OVD_ApConf_start(info);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_ap conf ret: %d\n",ret);

		if (ret == -1) {
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ap配网失败");
			//return -1;//输入数据错误
		}
	}
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ssid: %s ssid_len:%d pwd:%s pwd_len:%d phone:%s, phone_len:%d extension:%s, extension_len:%d\n", info->ssid, info->ssidLen,info->pwd,info->pwdLen,info->phone,info->phoneLen,info->extension,info->extensionLen);
	if(!((strlen(bindId) == 1) && (bindId[0] == '0')))
	{
		if(strlen(bindId) < 17){
			memcpy(info->phone, bindId, 17);
		}
	}
    OVD_DeviceBindInfo(info->phone);  //上报bindid
    memcpy(bindId,info->phone,strlen(info->phone));
	free(info);
	//free(p);
	return ret;
}

unsigned char do_cmd_ap_net(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->若设备已经联网，则不需要进行配置网络<------------\n\n");
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->开始ap配网\n");
	if(!((strlen(bindId) == 1) && (bindId[0] == '0'))){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","bindid:%s \n",bindId);
        OVD_DeviceBindInfo(bindId);
        return CMD_NO_ERR;
    }
	int result = demo_ap_conf();
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","二维码字符串解析结果2 %d\n",result);
    if(-1 == result){
        return CMD_NO_ERR;
    }
    return CMD_NO_ERR;
}

static int demo_qr_string(char *qr_str)
{
    OVD_QRString_init();
	OVDXXSSIDWiFiInfo *info;
	info = malloc(sizeof(OVDXXSSIDWiFiInfo));
	int ret=0, qr_len=0;

    qr_len = strlen(qr_str);
    ret = OVD_QRString(info, qr_str, qr_len);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_QRString ret: %d\n",ret);
    if (ret != OVD_XX_SSID_WIFI)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "qr string is wrong. [%s]\n", qr_str);
        return -1;
    }

	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ssid: %s ssid_len:%d pwd:%s pwd_len:%d phone:%s, phone_len:%d extension:%s, extension_len:%d\n", info->ssid, info->ssidLen,info->pwd,info->pwdLen,info->phone,info->phoneLen,info->extension,info->extensionLen);
	if(!((strlen(bindId) == 1) && (bindId[0] == '0')))
	{
		if(strlen(bindId) < 17){
			memcpy(info->phone, bindId, 17);
		}
	}
    OVD_DeviceBindInfo(info->phone);  //上报bindid
    memcpy(bindId,info->phone,strlen(info->phone));
	free(info);
	return ret;
}

unsigned char do_cmd_scan_code(int argc, char *argv[])
{
    FILE *fp;
    char result[1035];
    char cmd[1024];

    sprintf(cmd, "python scan.py %s", argv[1]);
    printf("cmd %s\n", cmd);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "Failed to run command\n");
        return CMD_EXEC_ERR;
    }

    while (fgets(result, sizeof(result)-1, fp) != NULL) {
        printf("scan result: %s", result);
    }

    pclose(fp);

    if (strlen(result) > 0)
    {
        int ret = demo_qr_string(result);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","二维码字符串解析结果2 %d\n", ret);
        if(-1 == ret){
            return CMD_EXEC_ERR;
        }
        return CMD_NO_ERR;
    }

    return CMD_EXEC_ERR;
}
unsigned char do_cmd_str_net(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->若设备已经联网，则不需要进行配置网络<------------\n\n");
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->开始解析二维码字符串2\n");
	if(!((strlen(bindId) == 1) && (bindId[0] == '0'))){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","bindid:[%s]\n",bindId);
        OVD_DeviceBindInfo(bindId);
        return CMD_NO_ERR;
    }
	int result = demo_qr_string(argv[1]);
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","二维码字符串解析结果2 %d\n",result);
    if(-1 == result){
        return CMD_EXEC_ERR;
    }
    return CMD_NO_ERR;
}

OVD_int32 OVD_DEMO_BLE_send_data_cb(IN const ovd_ble_chrs_e chrs, IN const OVD_uchar *data, IN const OVD_int32 data_len)
{
    int i;
    if(chrs == OVD_BLE_CHRS_INDICATE)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "BLE send data cb, indicate data:");
        for (i = 0; i < data_len; i++)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "%02x ", data[i]);
        }
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "\n");
    }
    else if(chrs == OVD_BLE_CHRS_NOTIFY)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "BLE send data cb, notify data:");
        for (i = 0; i < data_len; i++)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "%02x ", data[i]);
        }
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "\n");
    }
    return 0;
}

static int demo_ble_net() {
    // 模拟蓝牙配网待解析数据
    OVD_uchar data[][21] = {
    {0x00, 0x00, 0x10, 0x00, 0x0F, 0x01, 0x06, 0xA2, 0x04, 0x4B, 0x50, 0x00},
    {0x00, 0x00, 0x7B, 0x22, 0x6F, 0x70, 0x22, 0x3A, 0x22, 0x73, 0x65, 0x6E, 0x64, 0x57, 0x69, 0x66, 0x69, 0x43, 0x6F, 0x6E, 0x00},
    {0x00, 0x01, 0x66, 0x69, 0x67, 0x22, 0x2C, 0x22, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x22, 0x3A, 0x22, 0x31, 0x2E, 0x00},
    {0x00, 0x02, 0x30, 0x22, 0x2C, 0x22, 0x64, 0x61, 0x74, 0x61, 0x22, 0x3A, 0x7B, 0x22, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F, 0x00},
    {0x00, 0x03, 0x72, 0x64, 0x22, 0x3A, 0x22, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x22, 0x2C, 0x22, 0x65, 0x6E, 0x00},
    {0x00, 0x04, 0x63, 0x72, 0x79, 0x70, 0x74, 0x22, 0x3A, 0x22, 0x22, 0x2C, 0x22, 0x43, 0x47, 0x57, 0x22, 0x3A, 0x7B, 0x22, 0x00},
    {0x00, 0x05, 0x67, 0x77, 0x41, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x22, 0x3A, 0x22, 0x63, 0x67, 0x77, 0x2E, 0x6B, 0x6F, 0x00},
    {0x00, 0x06, 0x6D, 0x65, 0x63, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x3A, 0x35, 0x36, 0x38, 0x33, 0x22, 0x2C, 0x22, 0x75, 0x73, 0x00},
    {0x00, 0x07, 0x65, 0x72, 0x5F, 0x6B, 0x65, 0x79, 0x22, 0x3A, 0x22, 0x33, 0x35, 0x32, 0x34, 0x62, 0x38, 0x37, 0x66, 0x33, 0x00},
    {0x00, 0x08, 0x39, 0x39, 0x61, 0x34, 0x62, 0x66, 0x30, 0x38, 0x32, 0x31, 0x62, 0x37, 0x34, 0x39, 0x30, 0x36, 0x38, 0x30, 0x00},
    {0x00, 0x09, 0x63, 0x63, 0x35, 0x30, 0x37, 0x22, 0x2C, 0x22, 0x67, 0x77, 0x41, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x32, 0x00},
    {0x00, 0x0A, 0x22, 0x3A, 0x22, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x63, 0x67, 0x77, 0x2E, 0x6B, 0x6F, 0x6D, 0x00},
    {0x00, 0x0B, 0x65, 0x63, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x3A, 0x34, 0x34, 0x33, 0x22, 0x7D, 0x2C, 0x22, 0x63, 0x68, 0x61, 0x00},
    {0x00, 0x0C, 0x6E, 0x6E, 0x65, 0x6C, 0x22, 0x3A, 0x22, 0x30, 0x22, 0x2C, 0x22, 0x65, 0x78, 0x74, 0x72, 0x61, 0x22, 0x3A, 0x00},
    {0x00, 0x0D, 0x22, 0x22, 0x2C, 0x22, 0x53, 0x53, 0x49, 0x44, 0x22, 0x3A, 0x22, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49, 0x2D, 0x00},
    {0x00, 0x0E, 0x49, 0x50, 0x43, 0x5F, 0x53, 0x44, 0x4B, 0x22, 0x7D, 0x7D, 0x00}};

    ovd_ble_info_t ble_info;
    ovd_ble_gatt_t ble_gatt;
    OVDXXSSIDWiFiInfo wifi_info;
    ovd_ble_parse_e parse_result;
    int i, data_len;

    // 初始化BLE配网
    strncpy(ble_info.device_type_id, "591128", sizeof(ble_info.device_type_id));
    strncpy(ble_info.device_ble_mac, "00A61811ABFA", sizeof(ble_info.device_ble_mac));
    OVD_BLE_init(ble_info, &ble_gatt, OVD_DEMO_BLE_send_data_cb);

    // 根据ble_gatt开启蓝牙广播以及service、characteristic
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble localname: %s\n", ble_gatt.local_name);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble service uuid: %x\n", ble_gatt.service_uuid);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble write uuid: %d\n", ble_gatt.characteristic_uuid_down_write);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble notify uuid: %d\n", ble_gatt.characteristic_uuid_up_notify);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble indicate uuid: %d\n", ble_gatt.characteristic_uuid_up_indicate);

    // 模拟解析Wi-Fi数据和bindid的过程
    i = 0;
    while (OVD_TRUE) {
        for(data_len = 20; data[i][data_len] == 0x00 && data_len >= 0; --data_len);
        data_len += 1;
        parse_result = OVD_BLE_recv_parser(data[i], data_len, &wifi_info);
        i++;

        // 解析成功
        if (parse_result == OVD_BLE_PARSE_SUCCESS){
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble parse success");
            break;
        }

        // 和家亲已经完成蓝牙帧的发送，但设备未能正确解析数据
        if (i >= sizeof(data)/sizeof(data[0])) {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "ble parse fail");
            break;
        }
    }

    // 关闭蓝牙进入后续流程，或重新进行蓝牙配网
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "ble parse complete, close ble");
    return 0;
}

unsigned char do_cmd_ble_net(int argc, char *argv[])
{
    int result;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->开始蓝牙配网\n");
    result = demo_ble_net();
    if(0 != result){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "ble net error: [%d]\n", result);
        return CMD_EXEC_ERR;
    }
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","蓝牙配网执行结果: [%d]\n",result);

    if((strlen(bindId) == 1) && (bindId[0] == '0')){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "Invalid bindid\n");
        return CMD_EXEC_ERR;
    }
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","bindid:[%s] （为方便调试，demo从配置文件中读取bindid）\n",bindId);

    result = OVD_DeviceBindInfo(bindId);
    if(0 != result){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "OVD_DeviceBindInfo error:[%d]\n", result);
        return CMD_EXEC_ERR;
    }

    return CMD_NO_ERR;
}

unsigned char do_cmd_service_ok(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->系统网络及音视频服务已经准备好，启动服务<------------\n\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","网络连接好，服务正式启动，通知SDK去连接开放平台服务器\n");
    OVD_ServiceStart();
    SDKServiceOK = 1;  //SDK准备好
    return CMD_NO_ERR;
}

unsigned char do_cmd_service_stop(int argc, char *argv[])
{
    OVD_int32  ret;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->系统网络及音视频服务已经准备好，启动服务<------------\n\n");
    ret = OVD_ServiceStop();
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }
    return CMD_NO_ERR;
}



void get_gps_json_param(char *jsonParam, double longtiude, double latitude){
    char *data=NULL;
    cJSON *root= cJSON_CreateObject();
    cJSON *gps_info_entry = NULL;

    cJSON_AddItemToObject(root,"gps",gps_info_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(gps_info_entry,"longitude",longtiude);
    cJSON_AddNumberToObject(gps_info_entry,"latitude",latitude);

    data=(char *)cJSON_PrintUnformatted(root);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","data len..%d\n", strlen(data));
    strncpy(jsonParam, data, strlen(data));
    cJSON_free(data);
    cJSON_Delete(root);
}

unsigned char do_cmd_trig_alarm(int argc, char *argv[])
{
    int channel;
    int alarm_type;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->设备触发告警<------------\n\n");
    channel = atoi(argv[1]);
    alarm_type = atoi(argv[2]);
    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }
    OVDUpLoadAlarmInfo *uploadAlarmInfo = (OVDUpLoadAlarmInfo *)malloc(sizeof(OVDUpLoadAlarmInfo));
    if(NULL == uploadAlarmInfo){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","malloc uploadAlarmInfo fail\n");
        return CMD_EXEC_ERR;
    }
    memset(uploadAlarmInfo, 0, sizeof(OVDUpLoadAlarmInfo));
    uploadAlarmInfo->channel = channel;

    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }

    time_t localTimeStamp;
    localTimeStamp = time(NULL);
	global_timeStamp = localTimeStamp;

	//clock_t localTimeStamp = clock();
    //global_timeStamp = (long long)localTimeStamp;
    uploadAlarmInfo->startTimeStamp = global_timeStamp;
    uploadAlarmInfo->AlarmType = alarm_type;

    uploadAlarmInfo->desc = (char*)malloc(100);
    if(NULL != uploadAlarmInfo->desc){
        sprintf(uploadAlarmInfo->desc, "触发了告警，类型为 %d", (int)alarm_type);
    }

    //从文件读取告警图片数据
    FILE *in = NULL;
#if 0
    in = fopen(alarmFILEPATH,"r");
    if(NULL==in){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","open alarmPic.bmp fail\n");
        return;
    }


    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin alarm pic read\n");
    unsigned int width;
    unsigned int height;
    unsigned int bitCount;
    unsigned char buf[128];
    unsigned int  size;
    unsigned char* praw=get_data(in,&width,&height,&bitCount,&size);

    fclose(in);

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","read alarm pic end\n");
    uploadAlarmInfo->ImageInfo.buf = praw;
    uploadAlarmInfo->ImageInfo.size = size;
#endif
    int ret = 0;
    int size = 0;
    int buf_size =0;
    char *buf = NULL;
    struct stat stat_buf;

    ret = stat(alarmFILEPATH, &stat_buf);
    if(ret){
        free(uploadAlarmInfo->desc);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo] stat pic filepath %s fail\n",__func__,__LINE__,alarmFILEPATH);
        return CMD_EXEC_ERR;

    }
    size =(int)stat_buf.st_size;

    buf = (char *)malloc(size);

    FILE *f = fopen(alarmFILEPATH, "rb");
    if(!f){
        free(buf);
        free(uploadAlarmInfo->desc);
        ret = -1;
        return CMD_EXEC_ERR;
    }

   buf_size = fread(buf, 1, size, f);
   OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo]=== buf_size:%d ===\n",__func__,__LINE__,buf_size);
   uploadAlarmInfo->ImageInfo.buf = buf;
   uploadAlarmInfo->ImageInfo.size=size;
    //从文件读取告警图片结束


    if(uploadAlarmInfo->AlarmType == OVD_PARKING_NOTICE ||
        uploadAlarmInfo->AlarmType == OVD_DRIVING_COLLISION ||
        uploadAlarmInfo->AlarmType == OVD_PARKING_COLLISION ||
        uploadAlarmInfo->AlarmType == OVD_VOICE_CAPTURE){
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "uploadAlarmInfo->AlarmType...%d\n", uploadAlarmInfo->AlarmType);
            uploadAlarmInfo->extension = malloc(64);
            if(NULL == uploadAlarmInfo->extension){
                OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "uploadAlarmInfo->AlarmType...%d\n", uploadAlarmInfo->AlarmType);
                return -1;
            }

            get_gps_json_param(uploadAlarmInfo->extension, 1111.1111, 2222.2222);

        }

    if (uploadAlarmInfo->AlarmType == OVD_GESTURE)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "uploadAlarmInfo->AlarmType...%d\n", uploadAlarmInfo->AlarmType);
        uploadAlarmInfo->extension = malloc(128);
        if(NULL == uploadAlarmInfo->extension)
        {
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "uploadAlarmInfo->AlarmType...%d\n", uploadAlarmInfo->AlarmType);
            return -1;
        }
        sprintf(uploadAlarmInfo->extension, "{\"tags\":[{\"tag\":44, \"rate\":1}]}");   //手势呼叫
    }

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","send to api\n");
    int result = OVD_AlarmInfoStart(uploadAlarmInfo);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","触发告警的结果 %d\n",result);
    if (result == 0)
    {
        ParallelAlarmInfo[channel][ParallelAlarmNum[channel]] = uploadAlarmInfo;
        ParallelAlarmNum[channel]++;
    }
#if 0 
    // 更改为在告警结束后释放告警信息
    free(buf);
    free(uploadAlarmInfo->desc);
    if(NULL != uploadAlarmInfo->extension){
        free(uploadAlarmInfo->extension);
        uploadAlarmInfo->extension = NULL;
    }
#endif 
    fclose(f);
    return CMD_NO_ERR;
}

unsigned char do_cmd_alarm_end(int argc, char *argv[])
{
    int channel;
    OVDUpLoadAlarmInfo *uploadAlarmInfo = NULL;
    channel = atoi(argv[1]);
    if(ParallelAlarmNum[channel] <= 0){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","所有的告警都已结束\n");
        return CMD_NO_ERR;
    }
    ParallelAlarmNum[channel]--;
    uploadAlarmInfo = ParallelAlarmInfo[channel][ParallelAlarmNum[channel]];
    if(NULL == uploadAlarmInfo){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","告警信息不存在\n");
        return CMD_NO_ERR;
    }
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","结束告警的channel %d, type %d\n", channel, uploadAlarmInfo->AlarmType);
	time_t localTimeStamp;
    localTimeStamp = time(NULL);
	global_timeStamp = localTimeStamp;

	//clock_t localTimeStamp = clock();
    //global_timeStamp = (long long)localTimeStamp;
    int result = OVD_AlarmInfoEnd(channel, uploadAlarmInfo->AlarmType, global_timeStamp);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","结束告警的结果 %d\n",result);

    // 释放告警信息
    if(NULL != uploadAlarmInfo->ImageInfo.buf) {
        free(uploadAlarmInfo->ImageInfo.buf);
    }
    if(NULL != uploadAlarmInfo->desc) {
        free(uploadAlarmInfo->desc);
    }
    if(NULL != uploadAlarmInfo->extension){
        free(uploadAlarmInfo->extension);
    }
    free(uploadAlarmInfo);
    ParallelAlarmInfo[channel][ParallelAlarmNum[channel]] = NULL;
    return CMD_NO_ERR;
}

unsigned char do_cmd_aidiy_report(int argc, char *argv[]) {
    OVD_int32 result = -1;
    int channel;
    int alarm_type;
    time_t timestamp;
    OVD_AIDIY_Info info;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->AI DIY 上报<------------\n\n");
    channel = atoi(argv[1]);
    alarm_type = atoi(argv[2]);
    OVDUpLoadAlarmInfo *uploadAlarmInfo = ParallelAlarmInfo[channel][ParallelAlarmNum[channel] - 1];

    info.channel = channel;
    info.alarm_type = uploadAlarmInfo->AlarmType;
    strncpy(info.ai_ability, "babyCryDetector", MAX_LEN_32);

    // 设置标签
    info.ai_label_count = 1; // 假设只有一个标签
    info.ai_labels = (OVD_AI_Lable *)malloc(sizeof(OVD_AI_Lable) * info.ai_label_count);
    strncpy(info.ai_labels[0].label, "babyCry", MAX_LEN_32);
    info.ai_labels[0].score = 0.7; 

    // 设置模型验证
    strncpy(info.ai_label_verify.model, "hw-babyCare", MAX_LEN_64);
    info.ai_label_verify.params = strdup("{\"threshold\":0.6,\"sensitivity\":\"high\"}");

    // 设置图像信息
    info.ai_image_count = 1;
    info.ai_image_infos = (OVD_AI_Image_Info *)malloc(sizeof(OVD_AI_Image_Info) * info.ai_image_count);
    strncpy(info.ai_image_infos[0].type, "main", MAX_LEN_8);
    strncpy(info.ai_image_infos[0].format, "jpg", MAX_LEN_8);
    info.ai_image_infos[0].height = 1080;
    info.ai_image_infos[0].width = 1920;
    info.ai_image_infos[0].size = 102400; // 100KB 示例
    info.ai_image_infos[0].data = (char *)malloc(info.ai_image_infos[0].size);
    timestamp = time(NULL);
    info.ai_image_infos[0].timestamp = timestamp;

    // 调用接口
    #ifdef CONFIG_MODULE_AIDIY
    result = OVD_AIDIY_Report(&info);
    #endif
    if (result == 0) {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","上报成功！\n");
    } else {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","上报失败，错误码: %d\n", result);
    }

    // 清理动态分配的内存
    free(info.ai_labels);
    free(info.ai_image_infos[0].data);
    free(info.ai_image_infos);
    free(info.ai_label_verify.params);

    return 0;
}

unsigned char do_cmd_networkconnection(int argc, char *argv[])
{
    int channel;
    int networkType;
    char SSID[1024];
    char IpAddr[1024];

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->网络连接类型上报<------------\n\n");

    channel = atoi(argv[1]);
    networkType = atoi(argv[2]);
    sprintf(SSID, argv[3]);
    sprintf(IpAddr, argv[4]);

    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }

    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }
    int tmpret=OVD_NetworkConnection(channel, networkType, SSID, IpAddr, QR_CONFIG);
    if(tmpret !=0 )
    {
    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->networkconnection failed\n");
    }

    return CMD_NO_ERR;

}

unsigned char do_cmd_unlockwaynotify(int argc, char *argv[])
{
    int channel;
    int type;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->设备开锁记录上报<------------\n\n");
    channel = atoi(argv[1]);
    type = atoi(argv[2]);
    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }


    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }

    time_t localTimeStamp;
    localTimeStamp = time(NULL);


    char desc[64]={0};
    strncpy(desc,"unlockwaynotify test",sizeof(desc));


    //从文件读取告警图片数据
    FILE *in = NULL;

    int ret = 0;
    int size = 0;
    int buf_size =0;
    char *buf = NULL;
    struct stat stat_buf;

    ret = stat(alarmFILEPATH, &stat_buf);
    if(ret){

        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo] stat pic filepath %s fail\n",__func__,__LINE__,alarmFILEPATH);
        return CMD_EXEC_ERR;

    }
    size =(int)stat_buf.st_size;

    buf = (char *)malloc(size);

    FILE *f = fopen(alarmFILEPATH, "rb");
    if(!f){

        ret = -1;
        return CMD_EXEC_ERR;
    }

   buf_size = fread(buf, 1, size, f);
   OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo]=== buf_size:%d ===\n",__func__,__LINE__,buf_size);

   OvdLockImageInfo imageinfo={0};
   imageinfo.buf = buf;
   imageinfo.size=size;

   int tmpret=ovd_unlock_way_report(channel,type,localTimeStamp*1000,&imageinfo,desc);
   if(tmpret !=0 )
   {
      OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->unlockway failed\n");
   }
   free(buf);
   fclose(f);
   return CMD_NO_ERR;
}


unsigned char do_cmd_unlockexceptionnotify(int argc, char *argv[])
{
    int channel;
    int type;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->门锁异常事件上报<------------\n\n");
    channel = atoi(argv[1]);
    type = atoi(argv[2]);
    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }


    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }

    time_t localTimeStamp;
    localTimeStamp = time(NULL);


    char desc[64]={0};
    strncpy(desc,"unlockexception notify test",sizeof(desc));


    //从文件读取告警图片数据
    FILE *in = NULL;

    int ret = 0;
    int size = 0;
    int buf_size =0;
    char *buf = NULL;
    struct stat stat_buf;

    ret = stat(alarmFILEPATH, &stat_buf);
    if(ret){

        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo] stat pic filepath %s fail\n",__func__,__LINE__,alarmFILEPATH);
        return CMD_EXEC_ERR;

    }
    size =(int)stat_buf.st_size;

    buf = (char *)malloc(size);

    FILE *f = fopen(alarmFILEPATH, "rb");
    if(!f){

        ret = -1;
        return CMD_EXEC_ERR;
    }

   buf_size = fread(buf, 1, size, f);
   OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo]=== buf_size:%d ===\n",__func__,__LINE__,buf_size);

   OvdLockImageInfo imageinfo={0};
   imageinfo.buf = buf;
   imageinfo.size=size;

   int tmpret=ovd_unlock_exception_report(channel,type,localTimeStamp*1000,&imageinfo,desc);
   if(tmpret !=0 )
   {
      OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->unlockexception notify failed\n");
   }

   free(buf);
   fclose(f);
   return CMD_NO_ERR;
}



unsigned char do_cmd_lockeventreport(int argc, char *argv[])
{
    int channel;
    int type;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->门锁业务上报<------------\n\n");
    channel = atoi(argv[1]);
    type = atoi(argv[2]);
    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }


    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }

    time_t localTimeStamp;
    localTimeStamp = time(NULL);


    char desc[64]={0};
    strncpy(desc,"lockevent notify test",sizeof(desc));


    //从文件读取告警图片数据
    FILE *in = NULL;

    int ret = 0;
    int size = 0;
    int buf_size =0;
    char *buf = NULL;
    struct stat stat_buf;

    ret = stat(alarmFILEPATH, &stat_buf);
    if(ret){

        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo] stat pic filepath %s fail\n",__func__,__LINE__,alarmFILEPATH);
        return CMD_EXEC_ERR;

    }
    size =(int)stat_buf.st_size;

    buf = (char *)malloc(size);

    FILE *f = fopen(alarmFILEPATH, "rb");
    if(!f){

        ret = -1;
        return CMD_EXEC_ERR;
    }

   buf_size = fread(buf, 1, size, f);
   OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","%s %d: [demo]=== buf_size:%d ===\n",__func__,__LINE__,buf_size);

   OvdLockImageInfo imageinfo={0};
   imageinfo.buf = buf;
   imageinfo.size=size;

   int tmpret=ovd_lock_event_report(channel,type,localTimeStamp*1000,&imageinfo,desc);
   if(tmpret !=0 )
   {
      OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->lockeventnotify failed\n");
   }

   free(buf);
   fclose(f);
   return CMD_NO_ERR;
}



unsigned char do_cmd_lockbatteryreport(int argc, char *argv[])
{
    int channel;
    int index;
    int battery;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->门锁电量上报<------------\n\n");
    channel = atoi(argv[1]);
    index = atoi(argv[2]);
    battery = atoi(argv[3]);

    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }


    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }



   int tmpret=ovd_lock_battery_report(channel,index,battery);
   if(tmpret !=0 )
   {
      OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->lockbatteryreport failed\n");
   }


   return CMD_NO_ERR;
}

unsigned char do_cmd_lockvoltagereport(int argc, char *argv[])
{
    int channel;
    int index;
    int voltage;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->门锁电量上报<------------\n\n");
    channel = atoi(argv[1]);
    index = atoi(argv[2]);
    voltage = atoi(argv[3]);

    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请优先执行 %s 命令\n",SERVICE_OK);
        return CMD_EXEC_ERR;
    }


    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel %d disable\n",0);
        return CMD_NO_ERR;
    }



   int tmpret=ovd_lock_voltage_report(channel,index,(char)voltage);
   if(tmpret !=0 )
   {
      OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->lockvoltagereport failed\n");
   }


   return CMD_NO_ERR;
}

unsigned char do_cmd_gethserver(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","====获取低功耗休眠服务器信息===========\n\n");
    int ret =0;
    ovd_hserverinfo_t hserverinfo={0};
    ret= OVD_GetHServerInfo(&hserverinfo);
    if(ret !=0 )
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","===gethserverinfo failed\n");

    }
    else
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===hserverinfo:====\n");
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===domain:\t %s===\n",hserverinfo.hdomain);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===port:\t %d====\n",hserverinfo.hport);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===hb_interval:\t %d===\n", hserverinfo.hb_interval);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===hb_token:\t %s====\n", hserverinfo.hb_token);
    }

    return CMD_NO_ERR;
}

unsigned char do_cmd_speek(int argc, char *argv[])
{
    OVDChannelsInfo *chn_info = NULL;

    chn_info = &OVD_DEMO_ConfigureInfo.channelsInfo[0];
    if (!strcmp(argv[1], "on"))
    {
        chn_info->has_speek = 2;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","插入音柱\n");
    }
    else if (!strcmp(argv[1], "off"))
    {
        chn_info->has_speek = 1;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","拔下音柱\n");
    }

    OVD_DEMO_SetConfig();
}

unsigned char do_cmd_wave_net(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->若设备已经联网，则不需要进行配置网络<------------\n\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->开始声波识别\n");
    netWorkConfigVoiceResult = 0;
   if(!((strlen(bindId) == 1) && (bindId[0] == '0'))){
		OVD_DeviceBindInfo(bindId);
        return CMD_NO_ERR;
	}

    int result = OVD_DEMOVoice(waveFILEPATH);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","声波识别结果 %d\n",result);

    if(0 != result){
        return CMD_NO_ERR;
    }

    return 0;
}

unsigned char do_cmd_report_ptz(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->设备上报云台信息<------------\n\n");
    OVD_ptz_state_t ptz_state;
    ptz_state.channel = atoi(argv[1]);
    ptz_state.pos_info.limit = atoi(argv[2]);
    ptz_state.pos_info.pan = 10;
    ptz_state.pos_info.tilt = 10;
    ptz_state.pos_info.zoom = 2;
    ptz_state.pos_info.focus = 2;
    int result = OVD_ptz_state_report(&ptz_state);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","上报结果 %d\n",result);

    return CMD_NO_ERR;
}

int OVD_Demo_init_channel(int channel)
{
    int i = channel;
    unsigned int fps = videoFPS;
    OVD_int32 result;
    OVDVideoDataFormat *pVideoFmt = NULL;
    OVDAudioDataFormat *pAudioFmt = NULL;
    sourceVideoDataFormat videoParams;
    sourceAudioDataFormat audioParams;
    memset(&videoParams,0,sizeof(sourceVideoDataFormat));
    memset(&audioParams,0,sizeof(sourceAudioDataFormat));

    char *localVideoPath = NULL;
    char *localAudioPath = NULL;

    if (i>=128 || i >= OVD_DEMO_ConfigureInfo.channelsInfoCount)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "Channel %d is too large than %d\n", i, OVD_DEMO_ConfigureInfo.channelsInfoCount);
        return -1;
    }


    if (PushAVDataHandler[i][0] != NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "channel %d is pushing, not need to be start again.\n", i);
        return 0;
    }
    OVD_DEMO_ConfigureInfo.channelsInfo[i].on = 1;

    if(1 == VideoCap){
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        if(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec == source_STREAM_CODEC_H264)
        {
            memcpy(videoFILEPATH,h264filepath,strlen(h264filepath));
        }
        else if (OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec == source_STREAM_CODEC_H265)
        {
            memcpy(videoFILEPATH,h265filepath,strlen(h265filepath));
        }
        localVideoPath = videoFILEPATH;
    }
    if(1 == AudioCap){
        localAudioPath = audioFILEPATH;
    }

    OVD_DEMOAV_GetMediaFormat(localVideoPath, localAudioPath, PointCloudfilepath, &OVD_DEMO_ConfigureInfo.channelsInfo[i]);


    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音视频内容准备 channel %d \n", OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
    if (1 == VideoCap)
    {
        pVideoFmt = &(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo);
    }
    if (1 == AudioCap)
    {
        pAudioFmt = &(OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo);
    }
    //设置主码流参数
    result = OVD_AVPushStart(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel, OVD_HIGH_STREAM, pVideoFmt, pAudioFmt);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音频内容准备 报告结果 channel(%d), code_stream(%d), result(%d)\n", OVD_DEMO_ConfigureInfo.channelsInfo[i].channel, OVD_HIGH_STREAM, result);

    //若支持子码流，设置子码流参数
    result = OVD_AVPushStart(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel, OVD_LOW_STREAM, pVideoFmt, pAudioFmt);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音频内容准备 报告结果 channel(%d) code_stream(%d) result(%d)\n", OVD_DEMO_ConfigureInfo.channelsInfo[i].channel, OVD_LOW_STREAM, result);

    return 0;
}

int OVD_Demo_start_push(int channel)
{
    int sendtype;
    void *handler;
    int i = channel;

    if (!OVD_DEMO_ConfigureInfo.channelsInfo[i].on)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "av is not ready!\n");
        return -1;
    }
    if (PushAVDataHandler[i][0] != NULL)
    {
        return 0;
    }
    sendtype = 1;
    handler = OVD_DEMOAV_PushingStart(&OVD_DEMO_ConfigureInfo.channelsInfo[i], OVD_DEMO_PushAVCallback, &sendtype);
    if(NULL == handler){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","get av pushing handler fail\n");
        return -1;
    }
    PushAVDataHandler[i][0] = handler;
    parralelPushNum++;

    sendtype = 2;
    handler = OVD_DEMOAV_PushingStart(&OVD_DEMO_ConfigureInfo.channelsInfo[i], OVD_DEMO_PushAVCallback, &sendtype);
    if(NULL == handler){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","get av pushing handler fail\n");
        return -1;
    }
    PushAVDataHandler[i][1] = handler;
    parralelPushNum++;

    return 0;

}

int OVD_Demo_stop_push(int channel)
{
    if (PushAVDataHandler[channel][0] != NULL)
    {
        OVD_DEMOAVPushingStop(PushAVDataHandler[channel][0]);
        PushAVDataHandler[channel][0] = NULL;
    }
    if (PushAVDataHandler[channel][1] != NULL)
    {
        OVD_DEMOAVPushingStop(PushAVDataHandler[channel][1]);
        PushAVDataHandler[channel][1] = NULL;
    }

    return 0;
}
int OVD_Demo_deinit_channel(int channel)
{
    if (channel >= 128 || channel >= OVD_DEMO_ConfigureInfo.channelsInfoCount)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "Channel %d is too large than %d\n", channel, OVD_DEMO_ConfigureInfo.channelsInfoCount);
        return -1;
    }
    if (!OVD_DEMO_ConfigureInfo.channelsInfo[channel].on)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "Channel %d has been closed, nothing will be done.\n", channel);
        return -1;
    }

    OVD_AVPushEnd(channel, OVD_HIGH_STREAM);
    OVD_AVPushEnd(channel, OVD_LOW_STREAM);
    OVD_DEMO_ConfigureInfo.channelsInfo[channel].on = 0;

    return 0;
}


unsigned char  do_cmd_battery_change(int argc, char *argv[])
{
    int value;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->电量变化通知<------------\n\n");
    value = atoi(argv[1]);
    if(value < 50 ){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","保持唤醒时间段为：%d\n",keepawake_time);
        time_t i;
        i=time(NULL);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","当前时间为：%d\n",i);
        if(i < keepawake_time)
        {
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","It is still in the keepawake period\n");
        }
        else{
            cmd_exec(PUSH_END);

        }
    }

	OVD_DEMO_DeviceInfo.battery = value;

	OVD_BatteryChange(OVD_DEMO_DeviceInfo.battery);

    return CMD_NO_ERR;
}

void OVD_DEMOE_RecognizerStart(void)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","声波识别器开始识别声波\n");
}

void OVD_DEMOE_RecognizerFinish(int type, void *info, int infoLen)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","识别器识别声波结果， type = %d\n", type);
    if(info == NULL || infoLen <= 0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","recognize Error\n");
        return;
    }

    recgnizrsucess = 1;
    netWorkConfigVoiceResult = 1;  //标识有识别结果
    if(type == OVD_SSID_WIFI)
    {
        OVDSSIDWiFiInfo *wifiInfo = (OVDSSIDWiFiInfo *)info;
        memcpy(OVD_DEMO_DeviceInfo.wifi_ssid, wifiInfo->ssid, WIFI_SSID_LEN);

        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","wifi ssid %s, psd %s\n", wifiInfo->ssid, wifiInfo->pwd);
    }
    if(OVD_XX_SSID_WIFI == type){
        OVDXXSSIDWiFiInfo *wifiInfo = (OVDXXSSIDWiFiInfo *)info;
        memcpy(OVD_DEMO_DeviceInfo.wifi_ssid, wifiInfo->ssid, WIFI_SSID_LEN);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","wifi ssid %s, psd %s, phone %s\n", wifiInfo->ssid, wifiInfo->pwd, wifiInfo->phone);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->识别出结果，设备上报Bind ID<-----\n");
		//配合袁大联调，默认写为3
        if(!((strlen(bindId) == 1) && (bindId[0] == '0'))){
			if(strlen(bindId) < 17){
				memcpy(wifiInfo->phone, bindId, 17);
			}
		}
        OVD_DeviceBindInfo(wifiInfo->phone);  //上报bindid
        memcpy(bindId,wifiInfo->phone,strlen(wifiInfo->phone));
    }
	else{
		OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->识别失败<-----\n");
	}

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->识别结束<-----\n");
}

unsigned char do_cmd_exception(int argc, char *argv[])
{
    //测试事件上报
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->事件上报<------------\n\n");
    int cmd = 0;
    cmd = atoi(argv[1]);

    OvdProbeExceptInfo event_info={0};
    switch (cmd)
    {
        case OVD_RUNNING_STATUS_VIDEO_BLUR:
            event_info.exception_type = OVD_RUNNING_STATUS_VIDEO_BLUR;
            strncpy(event_info.content,"video blur",sizeof(event_info.content));
            break;
        case OVD_RUNNING_STATUS_SDCARD_ABNORMAL:
            event_info.exception_type = OVD_RUNNING_STATUS_SDCARD_ABNORMAL;
            strncpy(event_info.content,"sd card abnormal",sizeof(event_info.content));
            break;
        case OVD_RUNNING_STATUS_CAMERA_BLOCK:
            event_info.exception_type = OVD_RUNNING_STATUS_CAMERA_BLOCK;
            strncpy(event_info.content,"camera block",sizeof(event_info.content));
            break;
        case OVD_RUNNING_STATUS_SENSER_ABNORMAL:
            event_info.exception_type = OVD_RUNNING_STATUS_SENSER_ABNORMAL;
            strncpy(event_info.content,"senser abnormal",sizeof(event_info.content));
            break;
        case OVD_SOUND_SOURCE_REPORT:
            event_info.exception_type = OVD_SOUND_SOURCE_REPORT;
            strncpy(event_info.content, "{\"sound_type\":\"sound_upgrade\",\"description\":\"upgrade sound is play\"}", sizeof(event_info.content));
        default:
            break;
    }
    event_info.createTime=0;
    event_info.synctype = OVD_PROBE_SYNC_NOMFR;


    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_ProbeExceptionPost :%d,%d,%s\n",event_info.exception_type,event_info.synctype,event_info.content);
	OVD_ProbeExceptionPost(event_info);

    return CMD_NO_ERR;
}

unsigned char do_cmd_sleep_test1(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>设备进入休眠流程测试1<------------\n\n");
    pthread_t tid=0;
    pthread_create(&tid,NULL,pthread_OVD_DemoHibernation_test1,NULL);
    return CMD_NO_ERR;
}

unsigned char do_cmd_sleep_test2(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>设备进入休眠流程测试2<------------\n\n");
    pthread_t tid=0;
    pthread_create(&tid,NULL,pthread_OVD_DemoHibernation_test2,NULL);
    return CMD_NO_ERR;
}



unsigned char do_cmd_wakeup(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>设备退出休眠流程<------------\n\n");
    OVD_DEMO_Hibernation_end();
    cmd_exec(AV_READY);
    cmd_exec(PUSH_AV);
    return CMD_NO_ERR;
}

unsigned char do_cmd_plugin(int argc, char*argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>插入TF卡<------------\n\n");
    //模拟插卡
    OvdLocalStorageInfo info={0};
    info.storage_type=OVD_LOCAL_STORAGE_TYPE_TFCARD;
    strcpy(info.storage_path,g_sdcardpath);
    info.storage_status=OVD_LOCAL_STORAGE_STATUS_READY;
    OVD_SetStorageInfo(0, &info);
    g_sdcardstate=0;
    return CMD_NO_ERR;
}
unsigned char do_cmd_plugout(int argc, char*argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>拔出TF卡<------------\n\n");
    //模拟拔卡
    OvdLocalStorageInfo info={0};
    info.storage_status=OVD_LOCAL_STORAGE_STATUS_NOT_READY;
    OVD_SetStorageInfo(0, &info);
    g_sdcardstate=1;
    return CMD_NO_ERR;

}

unsigned char do_cmd_startcardencrypt(int argc, char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>开始卡加密<------------\n\n");
    OVD_SetStorageEncryptStatus(0,1);
    return CMD_NO_ERR;
}

unsigned char do_cmd_stopcardencrypt(int argc, char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>停止卡加密<------------\n\n");
    OVD_SetStorageEncryptStatus(0,0);
    return CMD_NO_ERR;
}

unsigned char do_cmd_getcardencrypt(int argc, char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------------>获取卡加密状态<------------\n\n");
    int encrypt_status=0;
    OVD_GetStorageEncryptStatus(0,&encrypt_status);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","card encrypt status:%d\n",encrypt_status);
    return CMD_NO_ERR;
}

unsigned char do_cmd_changecradreadspeed(int argc, char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------->设置卡读速率<-------\n\n");
    int channel=atoi(argv[1]);
    ovd_playback_speed_times_e speed=atoi(argv[2]);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","==channel:[%d], speed:[%d]==\n",channel,speed);
    OVD_SetStorageReadSpeed(channel,speed);
    return CMD_NO_ERR;
}

unsigned char do_cmd_changefreedisk(int argc, char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","------->设置剩余卡容量 %d MB<-------\n\n",atoi(argv[1]));
    int freedisk=atoi(argv[1]);
    g_freedisk=freedisk;

    return CMD_NO_ERR;
}
unsigned char do_cmd_changeloglevel(int argc, char* argv[])
{
    // 参数检查
    if (argc < 2) {
        OVD_LOG(OVD_LOGLEVEL_ERROR, "DEV/DEMO", "缺少日志等级参数\n");
        return CMD_EXEC_ERR;
    }

    // 转换并验证日志等级
    int log_level = atoi(argv[1]);
    if (log_level < OVD_LOGLEVEL_TRACE || log_level > OVD_LOGLEVEL_FATAL) {
        OVD_LOG(OVD_LOGLEVEL_ERROR, "DEV/DEMO", "无效的日志等级: %d (有效范围: %d-%d)\n", 
                log_level, OVD_LOGLEVEL_TRACE, OVD_LOGLEVEL_FATAL);
        return CMD_EXEC_ERR;
    }

    OVD_LOG(OVD_LOGLEVEL_DEBUG, "DEV/DEMO", "------->设置日志等级 %d <-------\n", log_level);

    // 设置日志等级并返回结果
    return (OVD_setloglevel(log_level) == 0) ? CMD_NO_ERR : CMD_EXEC_ERR;
}


// AI语音交互
OVD_bool ai_speech_auth_result = 0;
OVD_char ai_speech_session_id[256]={0};
unsigned char do_cmd_ai_speech_query_auth(int argc,char* argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk查询语音交互授权状态\n");

    int ret = OVD_QuerySpeechAuthResult("wjjtest123456",&ai_speech_auth_result);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk查询授权状态: %d 返回值:%d\n",(int)ai_speech_auth_result,ret);
    return CMD_NO_ERR;
}
unsigned char do_cmd_ai_speech_audio_session_open()
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk开始语音交互会话\n");
    memset(ai_speech_session_id,0,sizeof(ai_speech_session_id));
    int ret = OVD_SpeechAudioSessionOpen(ai_speech_session_id,sizeof(ai_speech_session_id));
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====返回的会话id: %s 返回值:%d\n",ai_speech_session_id,ret);
    return CMD_NO_ERR;
}
unsigned char do_cmd_ai_speech_audio_session_close()
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk结束语音交互会话\n");
    if(strlen(ai_speech_session_id)==0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====会话id为空,请先打开会话\n");
    }
    else
    {
        int ret = OVD_SpeechAudioSessionClose(ai_speech_session_id);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====返回的会话id: %s 返回值:%d\n",ai_speech_session_id,ret);
    }

    return CMD_NO_ERR;
}

unsigned char do_cmd_ai_speech_audio_session_interrupt()
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk打断语音交互会话\n");
    if(strlen(ai_speech_session_id)==0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====会话id为空,请先打开会话\n");
    }
    else
    {
        int ret = OVD_SpeechAudioSessionInterrupt(ai_speech_session_id);
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====返回的会话id: %s 返回值:%d\n",ai_speech_session_id,ret);
    }

    return CMD_NO_ERR;
}

#ifdef CONFIG_WHITE_BOX
// 测试函数
unsigned char do_cmd_testReportLicenseData() {
    OVD_License testLicense;

    // 初始化许可证状态
    testLicense.license_status.is_valid = OVD_TRUE;
    testLicense.license_status.is_expired = OVD_FALSE;
    testLicense.license_status.is_revoked = OVD_FALSE;

    // 设置授权文件类型
    testLicense.license_type = OVD_LICENSE_TYPE_FEATURE;

    // 设置功能列表
    testLicense.feature_list[0] = "live";
    testLicense.feature_list[1] = "cseg";
    testLicense.feature_count = 2;

    // 设置授权文件有效期
    strcpy(testLicense.validity.start_time, "1678095892");
    strcpy(testLicense.validity.end_time, "1743686111");

    // 设置元数据
    strcpy(testLicense.metadata.license_source, "hangyan");
    strcpy(testLicense.metadata.version, "v1.0.0");

    // 调用上报函数
    OVD_NotifyAuthorizedStatus(&testLicense);
}
#endif

unsigned char do_cmd_live_capture(int argc, char *argv[])
{
    int channel = atoi(argv[1]);

    OVD_StartLiveCaptureEx(channel, OVD_HIGH_STREAM, Device_Live_Filepath, Device_Live_Fileprefix);
    return CMD_NO_ERR;
}

unsigned char do_cmd_live_capture_stop(int argc, char *argv[])
{
    int channel = atoi(argv[1]);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","====设备sdk停止直播抓包\n");
    OVD_StopLiveCaptureEx(channel, OVD_HIGH_STREAM);
    return CMD_NO_ERR;
}
unsigned char do_cmd_record_capture(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===设备sdk 开始卡录像抓包， 输出到path:%s prefix:%s \n",Device_Record_Filepath,Device_Record_Fileprefix);
    OVD_StartRecordCapture(Device_Record_Filepath, Device_Record_Fileprefix);
    return CMD_NO_ERR;
}
unsigned char do_cmd_record_capture_stop(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====设备sdk停止卡录像抓包\n");
    OVD_StopRecordCapture();
    return CMD_NO_ERR;
}

unsigned char do_cmd_abevent(int argc, char *argv[])
{
    int ret = 0;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","媒体事件异常上报测试，请输入需要测试的事件\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","0:暂停推视频2s, 1:暂停推音频2s\n");
    int abnormal_event=0;
    ret = scanf("%d",&abnormal_event);
    if(ret !=0)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","scanf error\n");
    }
    switch(abnormal_event)
    {
        case 0:
            lcd_debug_stop_video();
            sleep(2);
            lcd_debug_start_video();
            break;
        case 1:
            lcd_debug_stop_audio();
            sleep(2);
            lcd_debug_start_audio();
            break;
        default:
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","not valid abnormal_event: %d\n",abnormal_event);
            ret=-1;
            break;
    }
    return CMD_NO_ERR;
}


unsigned char do_cmd_start_push_pointcloud(int argc, char *argv[])
{
    int ret = 0;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","开始推送点云码流\n");
    start_push_pointcloud();
    return CMD_NO_ERR;
}

unsigned char do_cmd_stop_push_pointcloud(int argc, char *argv[])
{
    int ret = 0;
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","停止推送点云码流\n");
    stop_push_pointcloud();
    return CMD_NO_ERR;
}



unsigned char do_cmd_user(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","======自定义测试用例=======\n\n");
    OVD_DEMO_USER(argv[1]);
    return CMD_NO_ERR;
}


unsigned char do_cmd_channel(int argc, char *argv[])
{

    OVD_ChannelCap_t cap = {0};
    OVD_ChannelExtension_t ext = {0};
    OVD_int32 ret;
    int channel = atoi(argv[1]);
    if (channel >= 128)
    {
        return CMD_EXEC_ERR;
    }
    if (!strcmp(argv[2], "on") || !strcmp(argv[2], "ON"))
    {
        OVD_DEMOE_InitChannelCap(&cap);
        sprintf(ext.name, "live_cam_%d", channel);
        cap.extension = &ext;
        ret = OVD_InstallChannel(channel, &cap);
        if (ret == 0)
        {
            // OVD_ServiceStop();
            // OVD_ServiceStart();
            SDKServiceOK = 1;  //SDK准备好
            // 准备音视频
            ret = OVD_Demo_init_channel(channel);
            if (ret != 0)
            {
                return CMD_EXEC_ERR;
            }
            ret = OVD_Demo_start_push(channel);
            if (ret != 0)
            {
                return CMD_EXEC_ERR;
            }
        }

    }
    else if (!strcmp(argv[2], "off") || !strcmp(argv[2], "OFF"))
    {
        ret = OVD_Demo_stop_push(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
        ret = OVD_Demo_deinit_channel(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
        ret = OVD_UninstallChannel(channel);
        if (ret == 0)
        {
            // OVD_ServiceStop();
            // OVD_ServiceStart();
            SDKServiceOK = 1;  //SDK准备好
        }
        else
        {
            return CMD_EXEC_ERR;
        }
    }
    else
    {
        return CMD_EXEC_ERR;
    }
    return CMD_NO_ERR;
}

unsigned char do_cmd_factory(int argc, char *argv[])
{
    int ret = OVD_RestoreFactory();
    if (ret == 0)
    {
        return CMD_NO_ERR;
    }
    else
    {
        return CMD_EXEC_ERR;
    }
}

extern int OVD_DEMOE_GetRebootInfo(OvdRebootParam *rebootinfo);

unsigned char do_cmd_restart_sdk(int argc, char *argv[])
{
    OVDCapInfoV2_t demo_cap;

    OVD_ServiceStop();
    cmd_exec(PUSH_END);
    OVD_deint();
    OvdRebootParam rebootinfo={0};
    OVD_DEMOE_GetRebootInfo(&rebootinfo);

    OVD_DEMOE_InitCapbility(&demo_cap);
    OVD_InitV2(&demo_cap, &OVD_DEMO_Client, &OVD_DEMO_LogParam, &rebootinfo, &callBackFunList, jsonParam);
    return CMD_NO_ERR;
}




void OVD_DEMO_transVideoFormatFromDemoToOVD(sourceVideoDataFormat *videoParams, OVDVideoDataFormat *destVideo){
    destVideo->codec = videoParams->codec;
    destVideo->bitrate = videoParams->bitrate;
    destVideo->width = videoParams->width;
    destVideo->height = videoParams->height;
    destVideo->framerate = videoParams->framerate;
    destVideo->colorDepth = videoParams->colorDepth;
    destVideo->frameInterval = videoParams->frameInterval;
    destVideo->reserve = videoParams->reserve;
}

void OVD_DEMO_transAudioFormatFromDemoToOVD(sourceAudioDataFormat *audioParams, OVDAudioDataFormat *destAudio){
    destAudio->codec = audioParams->codec;
    destAudio->samplesRate = audioParams->samplesRate;
    destAudio->bitrate = audioParams->bitrate;
    destAudio->waveFormat = audioParams->waveFormat;
    destAudio->channelNumber = audioParams->channelNumber;
    destAudio->blockAlign = audioParams->blockAlign;
    destAudio->bitsPerSample = audioParams->bitsPerSample;
    destAudio->frameInterval = audioParams->frameInterval;
    destAudio->reserve = audioParams->reserve;
}
unsigned char do_cmd_av_ready(int argc, char *argv[])
{
    int ret;
    ret = OVD_Demo_init_channel(0);
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }
    SDKAVContentOK = 1;

    return CMD_NO_ERR;
}

unsigned char do_cmd_pc_ready(int argc, char *argv[])
{
     OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->点云码流准备好<------------\n\n");
    int i = 0;
    if ( i>=128 || i >= OVD_DEMO_ConfigureInfo.channelsInfoCount)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "Channel %d is too large than %d\n", i, OVD_DEMO_ConfigureInfo.channelsInfoCount);
        return -1;
    }
    OVDPointCloudDataFormat PointCloudFmt = {0};
    PointCloudFmt.codec = source_STREAM_CODEC_POINTCLOUD;
    PointCloudFmt.framerate = 10;
    //设置点云码流参数
    int ret = OVD_PointCloudPushStart(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel, OVD_HIGH_STREAM, &PointCloudFmt);
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }

    return CMD_NO_ERR;
}

unsigned char do_cmd_av_mode(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->音视频参数修改<------------\n\n");
    if(0 == SDKServiceOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","SDK服务还未准备好，请先执行 %s 命令\n",SERVICE_OK);
        return CMD_NO_ERR;
    }
    if(0 == SDKAVContentOK){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音视频内容还未准备好，请先执行 %s 命令\n",AV_READY);
        return CMD_NO_ERR;
    }

    if(OVD_DEMO_ConfigureInfo.channelsInfoCount <= 0){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","没有channel相关信息，设备并未准备好。请检查configureInfo.txt中文件是否正常，并重新启动demo\n");
        return CMD_NO_ERR;
    }

    OVDVideoDataFormat *pVideoinfo = NULL; 	//视频信息,必填,详细可见结构体OVDVideoDataFormat
    OVDAudioDataFormat *pAudioinfo = NULL; 	//视频信息,必填,详细可见结构体OVDAudioDataFormat
    if(1 == VideoCap){
        pVideoinfo = &(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo);
    }
    if(1 == AudioCap){
        pAudioinfo = &(OVD_DEMO_ConfigureInfo.channelsInfo[0].audioinfo);
    }


    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音视频格式修改准备 channel %d \n", OVD_DEMO_ConfigureInfo.channelsInfo[0].channel);
    int result = OVD_AVParamModify(OVD_DEMO_ConfigureInfo.channelsInfo[0].channel, 1, pVideoinfo, pAudioinfo);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","音频格式修改准备 报告结果 %d\n", result);

    return 0;
}
unsigned char do_cmd_av_push_start(int argc, char *argv[])
{
    int ret;
    ret = OVD_Demo_start_push(0);
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }
    return CMD_NO_ERR;
}

unsigned char do_cmd_av_push_end(int argc, char *argv[])
{
    int ret;
    ret = OVD_Demo_stop_push(0);
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }
    ret = OVD_Demo_deinit_channel(0);
    if (ret != 0)
    {
        return CMD_EXEC_ERR;
    }
    return CMD_NO_ERR;
}
unsigned char do_cmd_av_push_channel(int argc, char *argv[])
{
    int ret;
    int channel = atoi(argv[1]);
    if (!strcmp(argv[2], "on"))
    {
        ret = OVD_Demo_init_channel(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
        ret = OVD_Demo_start_push(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
    }
    else if (!strcmp(argv[2], "off"))
    {
        ret = OVD_Demo_stop_push(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
        ret = OVD_Demo_deinit_channel(channel);
        if (ret != 0)
        {
            return CMD_EXEC_ERR;
        }
    }
    return CMD_NO_ERR;
}

void OVD_DEMO_PushAVCallback(AVPushContext *cpush, unsigned char* contentData,int dataLen, long long timestamp){
    //OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","视频线程推送内容 contentType %d\n",cpush->contentType);
    //读取视频文件，不断推送

    if((0 == VideoCap) && (OVD_Video == cpush->contentType)){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing video data, but device does not have video cap\n");
        return;
    }
    if((0 == AudioCap) && (OVD_Audio == cpush->contentType)){
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing audio data, but device does not have audio cap\n");
        return;
    }

    //int *pchannel = (int*)(userdata);

    if(OVD_DEMO_ConfigureInfo.channelsInfo[cpush->channel].on ==0)
    {
        // OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "channel %d unenabled,stop push av to this channel\n", cpush->channel);
        return ;
	}

    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","contentType:%d isIFrame:%d dataLen:%d timestamp:%lld (0x%x 0x%x 0x%x 0x%x 0x%x).\n",
    //    cpush->contentType, cpush->isKey, dataLen, timestamp, contentData[0], contentData[1], contentData[2], contentData[3], contentData[4]);

  //  OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing data %x,%x,%x,%x\n", contentData[0], contentData[1], contentData[2], contentData[3]);
#ifdef CONFIG_MODULE_HIoTRsk
  	if((OVD_DEMO_CapInfoV2.sdk.support_video_encrypt==1)&&(OVD_DEMO_ConfigureInfo.channelsInfo[cpush->channel].videoinfo.rsk_encrypt==1) )
  	{
		unsigned char *enc_buf = (unsigned char*)malloc(dataLen+128);
	    int enc_len = 0x00;
		int codec_enc=0;
		int enc_buf_len = dataLen+128;
		if(OVD_DEMO_ConfigureInfo.channelsInfo[cpush->channel].videoinfo.codec==4)
			codec_enc=2;
		else
			codec_enc=1;

	    int enc_frame_result=OVD_enc_frame(cpush->channel,cpush->isk,contentData, dataLen, enc_buf,enc_buf_len, &enc_len, codec_enc);
		if(0 != enc_frame_result)
		{
			OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","Video encryption failed %d\n",enc_frame_result);
			int result = OVD_AVPushData(cpush->channel, cpush->contentType, cpush->isKey, contentData, dataLen, timestamp);
        	if(0 != result)
            	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing data error1 \n");
		}
        else
        {

	        int result = OVD_AVPushData(cpush->channel , cpush->contentType, cpush->isKey, enc_buf, enc_len, timestamp);
	        if(0 != result)
	            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing data error2\n");
        }
        free(enc_buf);
        enc_buf = NULL;

	}
	else{
		int result = OVD_AVPushData(cpush->channel , cpush->contentType, cpush->isKey, contentData, dataLen, timestamp);
		if(0 != result)
        	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing data error3\n");

	}
#else
	int result = OVD_AVPushData(cpush->channel ,cpush->sendtype, cpush->contentType, cpush->isKey, contentData, dataLen, timestamp);
	if(0 != result){
			OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pushing data error4, channel %d, sendtype %d, contenttype %d\n", cpush->channel, cpush->sendtype, cpush->contentType);
	}


#endif

}


void OVD_DEMO_QuitSig(int sig){
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->设备服务退出 \n");
    OVD_ServiceStop();
    cmd_exec(PUSH_END);
    OVD_deint();
    SDKDemoStarted = 0;
   // OVD_DEMO_SetConfig();
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----->设备退出<-----\n");
    _exit(0);
}

void prepareJsonParam(char *jsonParam){
    char *data=NULL;
    cJSON *root= cJSON_CreateObject();
	cJSON_AddNumberToObject(root,"snapshotSize",Device_IMAGE_BUFF_SIZE);
	cJSON_AddNumberToObject(root,"buffDuration",Device_AV_BUFF_DURATION);
    if(Device_DEBUG_TS==1)
    {
        cJSON_AddBoolToObject(root,"debug_ts",1);
    }
    else
    {
        cJSON_AddBoolToObject(root,"debug_ts",0);
    }
    cJSON_AddNumberToObject(root,"cseg_duration",Device_CSEG_DURATION);
    cJSON_AddNumberToObject(root,"cseg_maxcount",Device_CSEG_MAXCOUNT);
    cJSON_AddNumberToObject(root,"snddropdelay",Device_SNDDROPDELAY);
	data=(char *)cJSON_PrintUnformatted(root);
	strncpy(jsonParam, data, strlen(data));
    cJSON_free(data);
    cJSON_Delete(root);
}

int init_daemon(void)
{
	int pid=0;
	int i=0;

	//忽略终端I/O信号，STOP信号
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);

	//建立一个新的进程组,在这个新的进程组中,子进程成为这个进程组的首进程,以使该进程脱离所有终端
	setsid();

	//再次新建一个子进程，退出父进程，保证该进程不是进程组长，同时让该进程无法再打开一个新的终端
	pid=fork();
	if( pid > 0) {
		exit(0);
	}
	else if( pid< 0) {
		return -1;
	}
#if !defined NOFILE && defined OPEN_MAX
#define NOFILE OPEN_MAX
#else
#define NOFILE 256
#endif
	//关闭所有从父进程继承的不再需要的文件描述符
	for(i=0;i< NOFILE;close(i++));

	//改变工作目录，使得进程不与任何文件系统联系
	//chdir("/");

	//将文件当时创建屏蔽字设置为0
	umask(0);

	//忽略SIGCHLD信号
	signal(SIGCHLD,SIG_IGN);

	return 0;
}

#ifdef __LITEOS__
unsigned char do_cmd_interrupt(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->中断<------------\n\n");

	pthread_mutex_lock(&g_mutex);
	while(0==g_count){
		pthread_cond_wait(&g_cond,&g_mutex);
	}
	g_count=0;
	pthread_mutex_unlock(&g_mutex);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","----------->中断恢复正常<------------\n\n");
    return CMD_NO_ERR;
}
#endif

#ifdef CONFIG_VOIP_SWITCH
unsigned char do_cmd_voip(int argc, char *argv[])
{
    cmcc_rtc_demo_process(inputCmd);
    return CMD_NO_ERR;
}
#endif


#define DETAIL_PARAM_ALARM "触发的告警类型\n"   \
                            "2-外部告警，3-移动，4-拌网，5-哭声，6-脸部, 7-声音，8-低电，9-撬锁，10-按铃, \n"   \
                            "11-人形， 12-PIR, 13-警戒功能, 14-(厨房)检测到口罩异常, 15-(厨房)检测到帽子异常, \n"   \
                            "16-(厨房)检测到工服异常, 17-(车型侦测)通用车辆侦测, 18-(车型侦测)检测到电瓶车,\n"  \
                            "19-(车型侦测)检测到自行车， 20-(口罩侦测)检测到未戴口罩）, 21-(区域人数统计)检测到人数超限,\n" \
                            "22-(高空抛物)检测到高空抛物）, 24-(离岗检测)检测到离岗,25-喂食检测,26-宠物出现,31-越界侦测\n"  \
                            "32-设备自检告警，33-设备防拆告警，34-烟雾告警，35-温度告警，36-湿度告警，37-传感器故障告警，38-烟雾模块失联告警，39-火焰告警\n" \
                            "64-入睡侦测，65-遮脸告警，66-觉醒侦测\n"  

#define DETAIL_PARAM_EXCEPTION "触发事件类型(18-视频模糊,19-SD卡异常,20-镜头遮挡,21-senser异常\n"


#define DETAIL_PARAM_UNLOCKWAYNOTIFY     "开锁记录类型\n"   \
                                         "0密码 1指纹 2射频卡 3App 4钥匙 5超级密码 6防尾随密码（劫持）开锁 \n"\
                                         "7防尾随指纹（劫持)开锁 8室内下拉把手打开锁  10临时密码开锁, 11超级SIM卡开锁 \n"\
                                         "12人脸识别开锁 13临时射频卡开锁 14管理指纹开锁 15管理射频卡开锁 16组合开锁 \n" \
                                         "17指纹+密码组合开锁 18指纹+门卡组合开锁 19密码+门卡组合开锁 20通话远程开锁\n"


#define DETAIL_PARAM_UNLOCKEXCEPTIONNOTIFY      "异常事件类型\n"\
                                                "0无异常 1撬门 2指纹输入异常 3密码输入异常 4虚掩 5挟持 6卡解锁异常\n"\
                                                "7恢复出厂设置报警8门栓阻塞报警9门锁冻结报警10临时密码开锁11非法用户（非法尝试开门）\n"\
                                                "12常开13门未锁好报警14室内开锁报警16取消常开17门口来人18门口人离开19门铃提醒20已反锁\n"\
                                                "21反锁解除22火灾提醒 23临时密码开锁异常 24门锁网络异常 25超级SIM卡解锁异常 26人脸识别解锁异常\n"\
                                                "27多方式解锁异常\n"

#define DETAIL_PARAM_LOCKEVENTNOTIFY            "门锁业务类型\n"\
                                                "0新增超级SIM卡 1 修改超级SIM卡 2删除超级SIM卡3-新增指纹\n"\
                                                "4-修改指纹5-删除指纹 6-新增卡片 7-修改卡片8-删除卡片9-新增密码\n"\
                                                "10-修改密码11-删除密码12-新增临时密码13-修改临时密码14-删除临时密码\n"\
                                                "15-新增人脸 16-删除人脸 17-已启用密码+指纹组合开锁模式 18-已禁用密码+指纹组合开锁模式\n"\
                                                "19-已启用密码+门卡组合开锁模式 20已禁用密码+门卡组合开锁模式 21已启用指纹+门卡组合开锁模式\n"\
                                                "22已禁用指纹+门卡组合开锁模式 23已启用组合开锁模式 24已禁用组合开锁模式\n"
static CmdObj cmd_array[] =
{
    {GETVERSION,    0, 0, NULL, do_cmd_getversion, "获取SDK版本号"},
    {QUIT     ,     0, 0, NULL, do_cmd_quit, "设备退出"},
    {BLE_NET ,      0, 0, NULL, do_cmd_ble_net, "蓝牙配网"},
    {WAVE_NET ,     0, 0, NULL, do_cmd_wave_net, "声波配网"},
    {AP_NET ,       0, 0, NULL, do_cmd_ap_net, "AP配网"},
    {PIC_NET,       0, 1, "<PIC_PATH>", do_cmd_scan_code, "二维码配网"},
    {PIC_STR_NET,   0, 1, "<QR_STR>", do_cmd_str_net, "二维码字符串解析", "举例：7s=hjkh27p=sssss8u=1457923e=1 s:wifi的ssid; p:wifi的password; u:userid信息 e:扩展信息"},
    {SERVICE_OK ,   0, 0, NULL, do_cmd_service_ok, "系统服务准备好"},
    {SERVICE_STOP,  0, 0, NULL, do_cmd_service_stop, "系统服务关闭"},
    {WIFI_DOWN ,    0, 0, NULL, do_cmd_default, "网络断开"},
    {TRIGGER_ALARM ,0, 2, "<CHANNEL> <ALARM>", do_cmd_trig_alarm, "触发告警", DETAIL_PARAM_ALARM},
    {ALARM_END ,    0, 1, "<CHANNEL>", do_cmd_alarm_end, "触发告警结束"},
    {AIDIY_REPORT,  0, 1, "<CHANNEL>", do_cmd_aidiy_report, "疑似告警后，上报疑似告警信息"},
    {AV_READY ,     0, 0, NULL, do_cmd_av_ready, "音视频准备好"},
    {AV_MOD ,       0, 0, NULL, do_cmd_av_mode, "音视频参数修改"},
    {PUSH_AV ,      0, 0, NULL, do_cmd_av_push_start, "开始推送音视频"},
    {PUSH_END ,     0, 0, NULL, do_cmd_av_push_end, "结束推送音视频"},
    {PUSH_CHANNEL,  0, 2, "<CHANNEL> <on/off>", do_cmd_av_push_channel, "按通道开启或关闭视频流"},
    {PUSH_VIDEO ,   0, 0, NULL, do_cmd_default, "推送视频"},
    {PUSH_AUDIO ,   0, 0, NULL, do_cmd_default, "推送音频"},
    {BATTERY ,      0, 1, "<VAR>", do_cmd_battery_change, "电量信息0~100"},
    {PTZ_REPORT,    0, 2, "<CHANNEL> <direction>", do_cmd_report_ptz, "type=0,限制位信息,direction=限制位方向"},
    {EXCEPTION ,    0, 1, "<EVENT>", do_cmd_exception, "事件上报", DETAIL_PARAM_EXCEPTION},
    //{SLEEP ,        0, 0, NULL, do_cmd_sleep, "设备进入休眠"},
    {SLEEP1 ,        0, 0, NULL, do_cmd_sleep_test1, "16位序列号设备进入休眠测试"},
    {SLEEP2 ,        0, 0, NULL, do_cmd_sleep_test2, "25位序列号设备进入休眠测试"},
    {WAKEUP ,       0, 0, NULL, do_cmd_wakeup, "设备从休眠状态唤醒"},
    {CHANGECODEC ,  0, 0, NULL, do_cmd_ChangeCodec, "设备更改视频格式"},
    {CHANGECODEC2 , 0, 0, NULL, do_cmd_ChangeCodec2, "设备更改视频格式2"},
    {CHANGEAOV , 0, 0, NULL, do_cmd_ChangeAOV, "设备切换AOV码流"},
    {STARTLPAOV , 0, 2, NULL, do_cmd_start_lowpower_aov, "设备开始低功耗AOV"},
    {STOPLPAOV , 0, 0, NULL, do_cmd_stop_lowpower_aov, "设备停止低功耗AOV"},
    {LIVECAPTURESTART ,  0, 1, "<CHANNEL>", do_cmd_live_capture, "媒体调试，直播抓包"},
    {LIVECAPTURESTOP  ,  0, 1, "<CHANNEL>", do_cmd_live_capture_stop, "媒体调试，停止直播抓包"},
    {RECORDCAPTURESTART, 0, 0, NULL, do_cmd_record_capture, "设备开始卡录像抓包"},
    {RECORDCAPTURESTOP , 0, 0, NULL, do_cmd_record_capture_stop, "设备停止卡录像抓包"},
    {ABSNORMAL_EVENT ,   0, 0, NULL, do_cmd_abevent, "异常事件上报测试"},
    {USER   ,       0, 1, "<PARAM>", do_cmd_user, "用户自定义，方便白盒测试"},
    {"channel" ,          0, 2, "<CHANNEL> <on/off>", do_cmd_channel, "视频通道启用或者关闭"},
    {"restart" ,          0, 0, NULL, do_cmd_restart_sdk, "重启SDK"},
    {"factory" ,   0, 0, NULL, do_cmd_factory, "恢复出厂设置"},
    //门锁相关
    {UNLOCKWAYNOTIFY ,0, 2, "<CHANNEL> <type>", do_cmd_unlockwaynotify, "开锁记录上报", DETAIL_PARAM_UNLOCKWAYNOTIFY},
    {UNLOCKEXCEPTIONNOTIFY ,0, 2, "<CHANNEL> <type>", do_cmd_unlockexceptionnotify, "门锁异常事件上报", DETAIL_PARAM_UNLOCKEXCEPTIONNOTIFY},
    {LOCKEVENTNOTIFY,0, 2, "<CHANNEL> <type>", do_cmd_lockeventreport, "开锁业务类型上报", DETAIL_PARAM_LOCKEVENTNOTIFY},
    {LOCKBATTERYREPORT,0,3,"<CHANNEL> <INDEX> <BATTERY_VAR>", do_cmd_lockbatteryreport,"门锁电量上报"},
    {LOCKVOLTAGEREPORT,0,3,"<CHANNEL> <INDEX> <VOLTAGE_VAR>", do_cmd_lockvoltagereport,"门锁电压上报"},
    {GETHSERVERINFO,0,0,NULL,do_cmd_gethserver,"获取休眠服务器信息"},
    {"speek", 0, 1, "<on/off>", do_cmd_speek,"插入或拔下音柱"},
    {NETWORKCONNECTION ,0,4,"<channel> <networkType> <SSID> <IpAddr>", do_cmd_networkconnection,"网络连接类型上报"},
    {PLUGINCARD ,       0, 0, NULL, do_cmd_plugin, "插入SD卡"},
    {PLUGOUTCARD ,       0, 0, NULL, do_cmd_plugout, "拔出SD卡"},
    {STARTCARDENCRYPT, 0, 0 , NULL, do_cmd_startcardencrypt, "开始加密"},
    {STOPCARDENCRYPT ,      0, 0 , NULL, do_cmd_stopcardencrypt, "停止加密"},
    {GETCARDENCRYPTSTATUS ,    0, 0 , NULL, do_cmd_getcardencrypt, "获取加密状态"},
    {CHANGECARDREADSPEED,0,2,NULL,do_cmd_changecradreadspeed,"设置卡读速率(1:0.25倍率 2:0.5倍率 3:0.75倍率 4:1倍率 5:2倍率 6:4倍率 7:8倍率 8:16倍率)"},
    {CHANGEFREEDISK,0,1,NULL,do_cmd_changefreedisk,"设置剩余卡容量(单位MB)"},
    {"changeloglevel",0,1,NULL,do_cmd_changeloglevel,"设置日志等级(0:TRACE,1:DEBUG,2:INFO,3:WARN,4:ERROR,5:FATAL)"},

    //AI语音交互
    {AISPEECHQUERYAUTH,0,0,NULL,do_cmd_ai_speech_query_auth,"查询设备是否支持AI语音交互"},
    {AISPEECHOPENSESSION,0,0,NULL,do_cmd_ai_speech_audio_session_open,"开启AI语音交互"},
    {AISPEECHCLOSESESSION,0,0,NULL,do_cmd_ai_speech_audio_session_close,"关闭AI语音交互"},
    {AISPEECHINTERRUPTSESSION,0,0,NULL,do_cmd_ai_speech_audio_session_interrupt,"中断AI语音本轮交互"},

    // 点云数据推送
    {PC_READY ,     0, 0, NULL, do_cmd_pc_ready, "点云码流准备好"},
    {START_PUSH_PC ,      0, 0, NULL, do_cmd_start_push_pointcloud, "开始推送点云码流"},
    {STOP_PUSH_PC ,      0, 0, NULL, do_cmd_stop_push_pointcloud, "停止推送点云码流"},
    

    // 白盒摄像头
#ifdef CONFIG_WHITE_BOX
    {"license",0,0,NULL,do_cmd_testReportLicenseData,"上报授权状态信息"},
#endif



#ifdef __LITEOS__
    {"interrupt" ,          0, 0, do_cmd_interrupt, "模拟liteos实时系统中断"},
#endif
#ifdef CONFIG_VOIP_SWITCH
    {"viop" ,          0, 0, do_cmd_voip, "固话命令处理"},
#endif
};

#ifdef __LITEOS__
static pthread_mutex_t g_mutex;
static pthread_cond_t g_cond;
static int g_count = 0;
void daemo_cond_init(){
	pthread_mutex_init(&g_mutex,NULL);
    pthread_cond_init(&g_cond,NULL);
}


void daemo_signal(){
	pthread_mutex_lock(&g_mutex);
	if(0==g_count){
		pthread_cond_signal(&g_cond);
	}
	g_count=1;
	pthread_mutex_unlock(&g_mutex);

}
int app_main(int argc, char* argv[]) {
    daemo_cond_init();
#elif defined(OVD_BUILDIN)
int ovd_main(ovd_get_fr_cb cb) {
    int argc = 2;
    char* argv[] = {"", "/config/DeviceConf.ini", "noinput"}; // TODO:
    get_fr_cb = cb;
#else
int main(int argc, char* argv[]) {
#endif

    int i;
    int ret=0;
    int result = 0;
    int init_result =-1;

    
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","====================power on====================\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=======%s=======\n", argv[1]);

    /*=======================启动参数检查=======================*/
    if(argc<2)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","请启动时输入configure路径\n");
        return -1;
    }

    memcpy(FILEPATH, argv[1], sizeof(FILEPATH));

    int run_demo_backend=0;
    if(argc>2 && strncmp(argv[2],"noinput",strlen("noinput")-1)==0)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","==============demo以后台方式启动==============\n");
        run_demo_backend=1;
    }
    /*=======================启动参数检查end=======================*/

    /*=======================demo启动=======================*/
    OVD_DEMO_status=1;
    SDKDemoStarted = 1;
    OVDCapInfoV2_t demo_cap;

    time_t localTimeStamp;
    time(&localTimeStamp);
    global_timeStamp = (long long)localTimeStamp;

    SDKServiceOK = 0;
    SDKAVContentOK = 0;

    //同时推送数据channel的数量
    parralelPushNum = 0;

    pthread_cond_init(&OVDDEMO_VIDEO_push_cond, NULL);
    pthread_mutex_init(&OVDDEMO_VIDEO_push_mutex, NULL);
    pthread_cond_init(&OVDDEMO_AUDIO_push_cond, NULL);
    pthread_mutex_init(&OVDDEMO_AUDIO_push_mutex, NULL);

    cmd_init(cmd_array, sizeof(cmd_array) / sizeof(CmdObj));
    char inputCmd[80]={0};
    char *cmd_param[10];
    char cmdChar=0;
    int nParam = 0;

    result = OVD_DEMOE_DeviceInit(FILEPATH);
    if(0 != result){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->初始化文件失败，请检查配置文件，重新启动<-----\n");
        return -1;
    }

    sprintf(CFG_FILEPATH, "%s/cfg_%s/device.cfg", OVD_DEMO_Client.ovd_data_path, OVD_DEMO_Client.OVDDeviceID);
    OVD_DEMOE_InitSetableKeys(FILEPATH);

    OvdRebootParam reboot_info={0};
    OVD_DEMOE_GetRebootInfo(&reboot_info);


    if(run_daemon==1)
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","自动执行 av_ready, service_ok, push_av\n");
        init_daemon();
       int fd1;
	    fd1=open("./file_stdout",O_CREAT | O_RDWR | O_APPEND ,0777);
        if(fd1<0)
        {
            perror("open log file failed\n");
            exit(fd1);
        }
	    dup2(fd1,1);
        dup2(fd1,2);
	    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","Successful file redirection\n");


       OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====================发起SDK注册=====================\n\n");
       setCallBackFunList(&callBackFunList);

	    //初始化的json参数配置
        memset(jsonParam,0,sizeof(jsonParam));
	    prepareJsonParam(jsonParam);

        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","======================SDK初始化======================\n\n");

        OVD_DEMOE_InitCapbility(&demo_cap);
        init_result = OVD_InitV2(&demo_cap, &OVD_DEMO_Client, &OVD_DEMO_LogParam, &reboot_info, &callBackFunList, jsonParam);

#ifdef thirdparty_gettimeofday
        OVD_register_thirdparty_gettimeofday(OVD_dut_getimeofday);
#endif

        cmd_exec(SERVICE_OK);
        sleep(1);

        cmd_exec(AV_READY);
        cmd_exec(PUSH_AV);


        do {
            sleep(1); //空转
        }while(1);
        return 0;

    }

    //交互式
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====================发起SDK注册=====================\n\n");
    setCallBackFunList(&callBackFunList);

	//初始化的json参数配置
	char jsonParam[200];
    memset(jsonParam,0,sizeof(jsonParam));
	prepareJsonParam(jsonParam);

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","======================SDK初始化======================\n\n");

    OVD_DEMOE_InitCapbility(&demo_cap);
    init_result = OVD_InitV2(&demo_cap, &OVD_DEMO_Client, &OVD_DEMO_LogParam, &reboot_info, &callBackFunList, jsonParam);
	if(init_result !=0)
	{
		OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","SDK初始化失败! \n");
		return -1;
	}

#ifdef thirdparty_gettimeofday
    OVD_register_thirdparty_gettimeofday(OVD_dut_getimeofday);
#endif

    char *new_line;
    //signal(SIGINT,OVD_DEMO_QuitSig);

    cmd_exec(SERVICE_OK);
    if (OVD_DEMO_CapInfoV2.device_type != DEVICE_TYPE_NVR)
    {
        for (i = 0; i<OVD_DEMO_ConfigureInfo.channelsInfoCount; i++)
        {
            OVD_Demo_init_channel(i);
            OVD_Demo_start_push(i);
            // cmd_exec(PC_READY);
            // cmd_exec(START_PUSH_PC);
        }
    }
    else    /* NVR 设备，默认启动所有通道推流 */
    {
        char cmd_buf[256];
        for (i = 0; i<OVD_DEMO_ConfigureInfo.channelsInfoCount; i++)
        {
            sprintf(cmd_buf, "channel %d on", i);
            cmd_exec(cmd_buf);
            usleep(100*1000);
        }
    }

    do{
        if(run_demo_backend)
        {
            sleep(2);
        }
        else
        {
            fgets(inputCmd, 79, stdin);

            if ((new_line = strchr(inputCmd, '\n')) != NULL)
            {
                *new_line = '\0';
            }
            else
            {
                continue;
            }
            cmd_exec(inputCmd);
        }


    }while(!exit_flag);

    return EXIT_SUCCESS;
}

