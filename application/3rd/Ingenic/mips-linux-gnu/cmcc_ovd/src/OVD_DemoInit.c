#include "OVD_Demo.h"
#include "inirw.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <menuconfig.h>


char deviceIDTmp[32];
char hardWareModelTmp[32];
char bindId[17];
extern int run_daemon;
extern int TF_H264_H265_CTRL ;
extern char AOVfilepath[256];
extern char PointCloudfilepath[256];



int substring(char* head, const char* tail, char* outStr)
{
    //定义字符指针 指向传递进来的ch地址
    char* pch=head;
    int index = 0;
    while(pch != tail){
        outStr[index] = *pch;
        pch++;
        index++;
    }
    outStr[index] = '\0';
    return index;
}

int OVD_DEMOE_GetRebootInfo(OvdRebootParam *rebootinfo)
{
    iniFileLoad(CFG_FILEPATH);
    rebootinfo->ts=iniGetInt("rebootinfo", "ts", 0);
    if(0==rebootinfo->ts)
    {

        struct timeval tv = {0};

        gettimeofday(&tv, NULL);
        rebootinfo->ts= tv.tv_sec;
    }
    rebootinfo->reason=iniGetInt("rebootinfo","reason",OVD_REBOOT_REASON_DUT_COMMON);
    iniGetString("rebootinfo", "message", rebootinfo->message, sizeof(rebootinfo->message), NULL);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","==wjj==reason:%d, ts:%lld, message:%s====\n",rebootinfo->reason,rebootinfo->ts,rebootinfo->message);
    iniFileFree();
    return 0;
}


int OVD_DEMOE_SetRebootInfo(OvdRebootParam *rebootinfo)
{
    iniFileLoad(CFG_FILEPATH);
    iniSetInt("rebootinfo","reason", rebootinfo->reason,10);
    iniSetInt("rebootinfo","ts",rebootinfo->ts,10);
    iniSetString("rebootinfo","message",rebootinfo->message);
    iniFileFree();
    return 0;
}

int OVD_DEMOE_InitDeviceInfo(){
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","读取deviceConfiguration\n");
    //FILE *fp = NULL;
    char buff[255]={0};
    int subStrLenth = 0;
    char keys[255]={0};

    /*

    typedef struct
    {
        char OVDDeviceID[MAX_LEN_32];                 //设备ID号，必填
        char OVDHardWareModel[MAX_LEN_32];         //设备型号，必填
        char OVDSystemVersion[MAX_LEN_32];        //设备固件版本号，必填
        int networkType;                          //设备当前网络连接类型， 1: 无线网络   2：有线网络   3：有线和无线都配置   4:4G供网   5:5G供网   ,必填
        char wifi_ssid[WIFI_SSID_LEN];             //设备当前连接的wifi的ssid, 该字段空串表示未连接wifi，必填
        int  wifi_signal;               //设备当前wifi的信号强度, 0-100, 当wifi_ssid不为空时有效，可选
        int  upBandwidth;               //设备探测到的上行最大带宽，单位bps，不存在则表示上行带宽未知，负值表示未知，可选
        int  downBandwidth;             //设备探测到的下行最大带宽，单位bps，不存在则表示下行带宽未知，负值表示未知，可选
        char ipAddr[MAX_LEN_128];               //IP地址    <局域网IP，支持IPV6>，空值表示未知，必填
        char macAddr[MAX_LEN_32];               //MAC地址，空值表示未知，可选
        int battery;
    }OVDDeviceInfo;
    */
    //先把值都清空
    memset(&OVD_DEMO_DeviceInfo, 0, sizeof(OVDDeviceInfo));

    char *sect="deviceConfiguration";

    run_daemon=iniGetInt(sect,"run_deamo" , 0);

    //设备id
    iniGetString(sect,"devId",OVD_DEMO_DeviceInfo.OVDDeviceID,sizeof(OVD_DEMO_DeviceInfo.OVDDeviceID),"");

  	if(strlen(OVD_DEMO_DeviceInfo.OVDDeviceID)!=16)
	{
		OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","error:OVDDeviceID输入不符合规范 \n");
		return -1;
	}

    //芯片型号
    iniGetString(sect,"modelId",OVD_DEMO_DeviceInfo.OVDChipModel,sizeof(OVD_DEMO_DeviceInfo.OVDChipModel),"chip");
    //硬件型号
    iniGetString(sect,"hardwareModel",OVD_DEMO_DeviceInfo.OVDHardWareModel,sizeof(OVD_DEMO_DeviceInfo.OVDHardWareModel),"hardware");
    //硬件版本号
    iniGetString(sect,"firmware_model",OVD_DEMO_DeviceInfo.OVDSystemVersion,sizeof(OVD_DEMO_DeviceInfo.OVDSystemVersion),"V1.0.0");

    //网络连接类型
    OVD_DEMO_DeviceInfo.networkType=iniGetInt(sect,"network_type",1);
    //配网类型，根据设备真实配网方式上报
    OVD_DEMO_DeviceInfo.DN_mode=iniGetInt(sect,"DN_mode",1);
    //wifissid
    iniGetString(sect,"wifi_ssid",OVD_DEMO_DeviceInfo.wifi_ssid,sizeof(OVD_DEMO_DeviceInfo.wifi_ssid),NULL);
    //wifi强度
    OVD_DEMO_DeviceInfo.wifi_signal=iniGetInt(sect,"wifi_signal",0);
    //上行带宽
    OVD_DEMO_DeviceInfo.upBandwidth=iniGetInt(sect,"upBandwidth",0);
    //下行带宽
    OVD_DEMO_DeviceInfo.downBandwidth=iniGetInt(sect,"downBandwith",0);
    //ipaddr
    iniGetString(sect,"ipAddr",OVD_DEMO_DeviceInfo.ipAddr,sizeof(OVD_DEMO_DeviceInfo.ipAddr),NULL);
    //macaddr
    iniGetString(sect,"macAddr",OVD_DEMO_DeviceInfo.macAddr,sizeof(OVD_DEMO_DeviceInfo.macAddr),NULL);
    //电量
    OVD_DEMO_DeviceInfo.battery=iniGetInt(sect, "battery", 0);
	//CPU占用率
    OVD_DEMO_DeviceInfo.cpuLoad=iniGetInt("deviceSoftProbe", "cpuLoad", 0);
	//全部内存 单位KB
	OVD_DEMO_DeviceInfo.memoryTotal=iniGetInt("deviceSoftProbe", "memoryTotal", 0);
	//剩余内存 单位KB
	OVD_DEMO_DeviceInfo.memoryAvailable=iniGetInt("deviceSoftProbe", "memoryAvailable", 0);

    return 0;
}

int OVD_DEMOE_showDeviceInfo()
{
    /*
    typedef struct
    {
        char OVDDeviceID[MAX_LEN_32];                 //设备ID号，必填
        char OVDHardWareModel[MAX_LEN_32];         //设备型号，必填
        char OVDSystemVersion[MAX_LEN_32];        //设备固件版本号，必填
        int32 networkType;                        //设备当前网络连接类型， 1: 无线网络   2：有线网络   3：有线和无线都配置   4:4G供网   5:5G供网   ,必填
        char wifi_ssid[WIFI_SSID_LEN];             //设备当前连接的wifi的ssid, 该字段空串表示未连接wifi，可选
        int  wifi_signal;               //设备当前wifi的信号强度, 0-100, 当wifi_ssid不为空时有效，可选
        int  upBandwidth;               //设备探测到的上行最大带宽，单位bps，不存在则表示上行带宽未知，负值表示未知，可选
        int  downBandwidth;             //设备探测到的下行最大带宽，单位bps，不存在则表示下行带宽未知，负值表示未知，可选
        char ipAddr[MAX_LEN_128];               //IP地址    <局域网IP，支持IPV6>，空值表示未知，可选
        char macAddr[MAX_LEN_32];               //MAC地址，空值表示未知，可选
        int battery;
    }OVDDeviceInfo;
    */
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===[OVD_DEMO_DeviceInfo]====\n");
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDDeviceID:%s \n",OVD_DEMO_DeviceInfo.OVDDeviceID);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDHardWareModel: %s\n", OVD_DEMO_DeviceInfo.OVDHardWareModel);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDSystemVersion: %s\n", OVD_DEMO_DeviceInfo.OVDSystemVersion);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t networkType: %d\n", OVD_DEMO_DeviceInfo.networkType);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t wifi_ssid: %s\n", OVD_DEMO_DeviceInfo.wifi_ssid);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t wifi_signal: %d\n",OVD_DEMO_DeviceInfo.wifi_signal);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t upBandwidth: %d\n", OVD_DEMO_DeviceInfo.upBandwidth);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t downBandwidth: %d\n", OVD_DEMO_DeviceInfo.downBandwidth);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t ipAddr: %s\n", OVD_DEMO_DeviceInfo.ipAddr);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t macAddr: %s\n", OVD_DEMO_DeviceInfo.macAddr);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t battery: %d \n", OVD_DEMO_DeviceInfo.battery);
    return 0;
}

int OVD_DEMO_InitInterface() {
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","读取网络接口名称\n");
    char buffer[64];
    char *sect="deviceConfiguration";

    iniGetString(sect,"network_interface",buffer,sizeof(buffer),"eth0");
    int i = 0;
    char *p = strtok(buffer, ",");
    while (p != NULL && i < 4) {
        OVD_DEMO_Interfaces[i] = malloc(strlen(p) + 1);
        if (OVD_DEMO_Interfaces[i] == NULL) {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","malloc failed for OVD_DEMO_Interfaces[%d]\n", i);
            return -1;
        }
        strncpy(OVD_DEMO_Interfaces[i], p, strlen(p) + 1);
        p = strtok(NULL, ",");
        i++;
    }
    return 0;
}

void OVD_DEMOE_InitChannelCap(OVD_ChannelCap_t *cap)
{
    char buffer[256];
    iniFileLoad(FILEPATH);
    cap->audiosamplerate=iniGetInt("devCap","audiosamplerate",0);
    cap->support_ptz=iniGetInt("devCap","have_ptz",1);
    cap->support_sd=iniGetInt("devCap","have_sd",1);
    cap->card_playback_speed = OVD_CARD_PLAYBACK_SPEED_MASK_QUARTER | OVD_CARD_PLAYBACK_SPEED_MASK_HALF | OVD_CARD_PLAYBACK_SPEED_MASK_1TIMES |  \
        OVD_CARD_PLAYBACK_SPEED_MASK_2TIMES | OVD_CARD_PLAYBACK_SPEED_MASK_4TIMES;
    cap->support_sdcapacity=iniGetInt("devCap","have_sdcapacity",1);
    cap->support_ptz_preset=iniGetInt("devCap","support_ptz_preset",1);
    cap->zoomcontrol=iniGetInt("devCap","zoomcontrol",1);
    cap->focuscontrol=iniGetInt("devCap","focuscontrol",1);
    cap->multi_stream=iniGetInt("devCap","multi_stream",0);
    cap->support_doorbell_preset=iniGetInt("devCap","support_doorbell_preset",1);
    iniGetString("devCap","video_quality", buffer,sizeof(buffer),"sd,hd");
    if (strstr(buffer, "ld"))
    {
        cap->video_quality |= OVD_VIDEO_QUALITY_LD;
    }
    if (strstr(buffer, "sd"))
    {
        cap->video_quality |= OVD_VIDEO_QUALITY_SD;
    }
    if (strstr(buffer, "hd"))
    {
        cap->video_quality |= OVD_VIDEO_QUALITY_HD;
    }
    if (strstr(buffer, "fhd"))
    {
        cap->video_quality |= OVD_VIDEO_QUALITY_FHD;
    }

    iniGetString("devCap","video_formats_supportlists", buffer,sizeof(buffer),"h265");
    if (strstr(buffer, "h264"))
    {
        cap->video_codec |= OVD_VIDEO_CODEC_H264;
    }
    if (strstr(buffer, "h265"))
    {
        cap->video_codec |= OVD_VIDEO_CODEC_H265;
    }
    cap->support_static_scence_ABR=iniGetInt("devCap","support_static_scence_ABR",0);
	//alarm
	cap->ovdCapInfo_alarms.have_alarms_io=iniGetInt("devCap","have_alarms_io",1);


    cap->ovdCapInfo_alarms.have_alarms_cry=iniGetInt("devCap","have_alarms_cry",1);
    cap->ovdCapInfo_alarms.support_crying_pacify=iniGetInt("devCap","support_crying_pacify",0);
    cap->ovdCapInfo_alarms.crying_pacify_audio_playing_count=iniGetInt("devCap","crying_pacify_audio_playing_count",0);
    cap->ovdCapInfo_alarms.support_crying_pacify_audio_playing_volumn=iniGetInt("devCap","support_crying_pacify_audio_playing_volumn",0);
    cap->ovdCapInfo_alarms.crying_pacify_audio_playing_type=iniGetInt("devCap","crying_pacify_audio_playing_type",0);

    cap->ovdCapInfo_alarms.support_crying_pacify_detect_on=iniGetInt("devCap","support_crying_pacify_detect_on",1);
    cap->ovdCapInfo_alarms.support_crying_pacify_sense=iniGetInt("devCap","support_crying_pacify_sense",1);
    cap->ovdCapInfo_alarms.support_cry_appease_music_selected=iniGetInt("devCap","support_crying_pacify_music_selected",1);
    cap->ovdCapInfo_alarms.support_cry_appease_music_schedule=iniGetInt("devCap","support_crying_pacify_music_schedule",1);

    cap->ovdCapInfo_alarms.have_alarms_voice=iniGetInt("devCap","have_alarms_voice",1);
    cap->ovdCapInfo_alarms.alarm_voice_detect_stand=iniGetInt("devCap","alarm_voice_detect_stand",2);
    cap->ovdCapInfo_alarms.have_alarms_motion=iniGetInt("devCap","have_alarms_motion",1);
    cap->ovdCapInfo_alarms.alarm_motion_detect_stand=iniGetInt("devCap","alarm_motion_detect_stand",2);
    cap->ovdCapInfo_alarms.support_alarms_motion_zone=iniGetInt("devCap","support_alarms_motion_zone",1);
    cap->ovdCapInfo_alarms.have_alarms_body=iniGetInt("devCap","have_alarms_body",1);
    cap->ovdCapInfo_alarms.support_alarms_body_zone=iniGetInt("devCap","support_alarms_body_zone",1);
    cap->ovdCapInfo_alarms.have_alarms_cross=iniGetInt("devCap","have_alarms_cross",1);
    cap->ovdCapInfo_alarms.have_alarms_pir=iniGetInt("devCap","have_alarms_pir",1);
    cap->ovdCapInfo_alarms.have_alarms_pir_staymode=iniGetInt("devCap","have_alarms_pir_staymode",1);
    cap->ovdCapInfo_alarms.have_alarms_lossLock=iniGetInt("devCap","have_alarms_lossLock",1);
    cap->ovdCapInfo_alarms.have_alarms_alertarea=iniGetInt("devCap","have_alarms_alertarea",1);
    cap->ovdCapInfo_alarms.have_alertarea_expel=iniGetInt("devCap","have_alertarea_expel",1);
    cap->ovdCapInfo_alarms.have_alarms_alertarea_zone=iniGetInt("devCap","have_alarms_alertarea_zone",1);
    cap->ovdCapInfo_alarms.alertarea_params.alertarea_num=iniGetInt("devCap","alertarea_num",1);
    cap->ovdCapInfo_alarms.have_alarms_pet=iniGetInt("devCap","have_alarms_pet",1);
    cap->ovdCapInfo_alarms.support_alarms_pet_zone=iniGetInt("devCap","support_alarms_pet_zone",1);

    cap->stream_encryption_mode=iniGetInt("devCap","stream_encryption_mode",0);
    cap->support_set_video_formats=iniGetInt("devCap","support_set_video_formats",1);
    cap->support_set_bitrate=iniGetInt("devCap","support_set_bitrate",0);
    cap->support_set_framerate=iniGetInt("devCap","support_set_framerate",0);
    cap->support_set_gop=iniGetInt("devCap","support_set_gop",0);

    int normal_nightvision = iniGetInt("devCap","support_set_normal_nightvision_mode",1);
    int color_nightvision = iniGetInt("devCap", "support_set_color_nightvision_mode", 1);
    if (color_nightvision)
    {
        cap->nightvision_mode = OVD_NIGHTVISION_MODE_COLOR;
    }
    else if (normal_nightvision)
    {
        cap->nightvision_mode = OVD_NIGHTVISION_MODE_NORMAL;
    }
    else
    {
        cap->nightvision_mode = OVD_NIGHTVISION_MODE_NONE;
    }

    cap->ai_nightvision_mask=iniGetInt("devCap", "support_detect_nightvision_mask", OVD_NIGHTVISION_DETECT_MOTION | OVD_NIGHTVISION_DETECT_HUMAN);
    cap->support_set_AI_rules_mode=iniGetInt("devCap","support_set_AI_rules_mode",0);;
    cap->osd_text=iniGetInt("devCap","osd_text",1);
    cap->osd_logo=iniGetInt("devCap","osd_logo",1);

    iniFileFree();
}

static int OVD_DEMO_CapsLinkageStr2mask(const char *str, OVD_dword *mask)
{
    if (str == NULL || mask == NULL)
    {
        return -1;
    }
    if (strstr(str, "speech"))
    {
        *mask |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(str, "light"))
    {
        *mask |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(str, "buz"))
    {
        *mask |= LINKAGE_MODE_BUZ;
    }

    return 0;
}
int OVD_DEMOE_InitCapbility(OVDCapInfoV2_t *demo_cap)
{
    int i;
    OVD_cap_params_t  cap_param = {0};
    char buffer[256];

    cap_param.max_channel = OVD_DEMO_ConfigureInfo.channelsInfoCount;
    cap_param.product_function = OVD_PRODUCT_FUNCTION_DASHCAM  | OVD_PRODUCT_FUNCTION_MOBOCAM| OVD_PRODUCT_FUNCTION_LOWPOWER;
    cap_param.ai_function = OVD_AI_FUNCTION_FACE      \
                | OVD_AI_FUNCTION_KITCHEN             \
                | OVD_AI_FUNCTION_VEHICLE             \
                | OVD_AI_FUNCTION_NON_MOTOR           \
                | OVD_AI_FUNCTION_PASSENGER           \
                | OVD_AI_FUNCTION_REGIONAL_PEOPLE     \
                | OVD_AI_FUNCTION_PARABOLIC_AERIAL    \
                | OVD_AI_FUNCTION_LANE_LINE           \
                | OVD_AI_FUNCTION_HELMET_WEARING      \
                | OVD_AI_FUNCTION_GESTURE_RECOGNITION \
                | OVD_AI_FUNCTION_OFF_DUTY            \
                | OVD_AI_FUNCTION_SMOKE_FIRE          \
                | OVD_AI_FUNCTION_FAMILY_RECOGNITION  \
                | OVD_AI_FUNCTION_PRIVACY_MASKING     \
                | OVD_AI_FUNCTION_FALL_DETECTION   \
                | OVD_AI_FUNCTION_SLEEP_MONITOR    \
                | OVD_AI_FUNCTION_EXPRESSION_RECOGNITION;   

    OVD_CapGetV2(OVD_DEMO_CapInfoV2.device_type, &cap_param, demo_cap);

    for (i = 0; i<demo_cap->count; i++)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "cap setting, channel %d, type %d, size %d\n", demo_cap->cap[i]->channel, demo_cap->cap[i]->cap_type, demo_cap->cap[i]->size);
        // demo 中，各个视频通道使用相同的能力集
        if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_BASE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.base, sizeof(OVD_CapInfoBase));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_LOW_POWER)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.lowpower, sizeof(OVD_CapInfoLowPower));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_CAMERA)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.camera, sizeof(OVD_CapInfoCamera));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_PTZ)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ptz, sizeof(OVD_CapInfoPtz));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_LINKAGE_TRACKING)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.linkage_tracking, sizeof(OVD_CapInfoLinkageTracking));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_SPECIAL)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.special, sizeof(OVD_CapInfoSpecial));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_ROBOT)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.robot, sizeof(OVD_CapInfoRobot));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_MOBOCAM)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.mobocam, sizeof(OVD_CapInfoMobocam));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_DASH_CAM)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.dashcam, sizeof(OVD_CapInfoDashCam));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_YARDLIGHT)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.yardlight, sizeof(OVD_CapInfoYardLight));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_VIDEO)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.video, sizeof(OVD_CapInfoVideo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AUDIO)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.audio, sizeof(OVD_CapInfoAudio));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_ALARM)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.alarm, sizeof(OVD_CapInfoAlarm));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_LINKAGE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.linkage, sizeof(OVD_CapInfoLinkage));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FACE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_face, sizeof(OVDAIFaceCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_KITCHEN)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_kitchen, sizeof(OVDAIKitchenCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_VEHICLE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_vehicle, sizeof(OVDAIVehicleCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_NON_MOTOR)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_nonvehicle, sizeof(OVDAINonmotorvehicleCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PASSENGER)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_passenger, sizeof(OVDAIPassengerCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_REGIONAL_PEOPLE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_regional_people, sizeof(OVDAIRegionalPeopleStatCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PARABOLIC_AERIAL)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_parabolic, sizeof(OVDAIParabolicAerialCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_LANE_LINE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_laneline, sizeof(OVDAILaneLineCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_OFF_DUTY)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_offduty, sizeof(OVDAIOffDutyCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_HELMET_WEARING)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_helmetwearing, sizeof(OVDAIHelmetWearingCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_GESTURE_RECOGNITION)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_gesturerecognition, sizeof(OVDAIGestureRecognitionCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SMOKE_FIRE)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_smokefire, sizeof(OVDAISmokeFireCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FAMILY_RECOGNITION)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_familyrecognition, sizeof(OVDAIFamilyRecognitionCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PRIVACY_MASKING)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_privacymasking, sizeof(OVDAIPrivacyMaskingCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FALL_DETECTION)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_falldetection, sizeof(OVDAIFallDetectionCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SLEEP_MONITOR) //sleep_monitor
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_sleepmonitor, sizeof(OVDAISleepMonitorCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_EXPRESSION_RECOGNITION)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.ai_expressionrecognition, sizeof(OVDAIExpressionRecognitionCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_CLOUDAI)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.cloudai, sizeof(OVDCloudAICapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SPEECH)
        {
            memcpy(demo_cap->cap[i]->cap_data,&(OVD_DEMO_CapInfoV2.aispeech),sizeof(OVDAISpeechCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_LULLBY)
        {
            memcpy(demo_cap->cap[i]->cap_data,&(OVD_DEMO_CapInfoV2.lullby),sizeof(OVDLullbyCapInfo));
        }
        else if (demo_cap->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_DIY)
        {
            memcpy(demo_cap->cap[i]->cap_data, &OVD_DEMO_CapInfoV2.aidiy, sizeof(OVDAIDIYCapInfo));
        }
        else
        {
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "ignore cap, channel %d, type %d\n", demo_cap->cap[i]->channel, demo_cap->cap[i]->cap_type);
            continue;
        }
    }

    return 0;
}
int OVD_DEMOE_InitDevCapV2(){

    char buffer[512];
	memset(&OVD_DEMO_CapInfoV2, 0, sizeof(OVD_DEMO_Capinfo_t));

    OVD_DEMO_CapInfoV2.device_type=iniGetInt("devCap", "device_type", 0);
    OVD_DEMO_CapInfoV2.base.stream_encryption_mode=iniGetInt("devCap","stream_encryption_mode",0);
    OVD_DEMO_CapInfoV2.base.support_auto_reboot = iniGetInt("devCap","have_auto_reboot",0);
    OVD_DEMO_CapInfoV2.base.support_ota_without_verify_header = iniGetInt("devCap","support_ota_without_verify_header", 0);

    // 低功耗
    OVD_DEMO_CapInfoV2.lowpower.support_lowpower_mode=iniGetInt("devCap","enable_low",1);
    OVD_DEMO_CapInfoV2.lowpower.support_awaked=iniGetInt("devCap","support_awaked",0);
    OVD_DEMO_CapInfoV2.lowpower.support_awaked_switch=iniGetInt("devCap","support_awaked_switch",0);
    OVD_DEMO_CapInfoV2.lowpower.support_smart_mode=iniGetInt("devCap","smart_mode",0);
    OVD_DEMO_CapInfoV2.lowpower.support_constant_power=iniGetInt("devCap","support_constant_power",0);

    // 特殊能力集
    OVD_DEMO_CapInfoV2.special.support_4G=iniGetInt("devCap","support_4G",0);
    OVD_DEMO_CapInfoV2.special.support_battery=iniGetInt("devCap","have_battery",1);
    OVD_DEMO_CapInfoV2.special.support_light_supplement_lamp=iniGetInt("devCap","light_supplement_lamp",1);
    OVD_DEMO_CapInfoV2.special.support_screen=iniGetInt("devCap", "have_screen", 0);
    OVD_DEMO_CapInfoV2.special.robot_house_keeper =iniGetInt("devCap", "robot_housekeeper", 0);
    OVD_DEMO_CapInfoV2.special.support_hjkh=iniGetInt("devCap","enable_hjkh",1);
    OVD_DEMO_CapInfoV2.special.hjkh_mode.support_preference_call=iniGetInt("devCap","preference_call",1);
    OVD_DEMO_CapInfoV2.special.hjkh_mode.support_set_calling_number=iniGetInt("devCap","set_calling_number",1);
    OVD_DEMO_CapInfoV2.special.hjkh_mode.support_not_disturb_mode=iniGetInt("devCap","not_disturb_mode",1);
    OVD_DEMO_CapInfoV2.special.hjkh_mode.support_not_disturb_device_mode=iniGetInt("devCap","not_disturb_device_mode",1);
    OVD_DEMO_CapInfoV2.special.hjkh_mode.support_auto_hangup=iniGetInt("devCap","auto_hangup",1);
	OVD_DEMO_CapInfoV2.special.hjkh_mode.support_voice_calling_number=iniGetInt("devCap","voice_calling_number",1);
    OVD_DEMO_CapInfoV2.special.camera_lens_type =iniGetInt("devCap", "camera_lens_type", 1);
    OVD_DEMO_CapInfoV2.special.support_smoke_sensor = iniGetInt("devCap","smoke_sensor",1);

    // 视频能力集
    VideoCap=iniGetInt("devCap","have_video_out",1);
    OVD_DEMO_CapInfoV2.video.support_multi_stream=iniGetInt("devCap","multi_stream",0);
    OVD_DEMO_CapInfoV2.video.support_static_scence_ABR=iniGetInt("devCap","support_static_scence_ABR",0);
    iniGetString("devCap","video_quality", buffer,sizeof(buffer),"sd,hd");
    if (strstr(buffer, "ld"))
    {
        OVD_DEMO_CapInfoV2.video.video_quality |= OVD_VIDEO_QUALITY_LD;
    }
    if (strstr(buffer, "sd"))
    {
        OVD_DEMO_CapInfoV2.video.video_quality |= OVD_VIDEO_QUALITY_SD;
    }
    if (strstr(buffer, "hd"))
    {
        OVD_DEMO_CapInfoV2.video.video_quality |= OVD_VIDEO_QUALITY_HD;
    }
    if (strstr(buffer, "fhd"))
    {
        OVD_DEMO_CapInfoV2.video.video_quality |= OVD_VIDEO_QUALITY_FHD;
    }

    iniGetString("devCap","video_formats_supportlists", buffer,sizeof(buffer),"h264");
    if (strstr(buffer, "h264"))
    {
        OVD_DEMO_CapInfoV2.video.video_codec |= OVD_VIDEO_CODEC_H264;
    }
    if (strstr(buffer, "h265"))
    {
        OVD_DEMO_CapInfoV2.video.video_codec |= OVD_VIDEO_CODEC_H265;
    }
    OVD_DEMO_CapInfoV2.video.support_set_video_formats=iniGetInt("devCap","support_set_video_formats",1);
    OVD_DEMO_CapInfoV2.video.support_set_bitrate=iniGetInt("devCap","support_set_bitrate",0);
    OVD_DEMO_CapInfoV2.video.support_set_framerate=iniGetInt("devCap","support_set_framerate",0);
    OVD_DEMO_CapInfoV2.video.support_set_gop=iniGetInt("devCap","support_set_gop",0);
    OVD_DEMO_CapInfoV2.video.support_set_AI_rules_mode=iniGetInt("devCap","support_set_AI_rules_mode",0);
    OVD_DEMO_CapInfoV2.video.support_osd_text=iniGetInt("devCap","osd_text",1);
    OVD_DEMO_CapInfoV2.video.support_osd_logo=iniGetInt("devCap","osd_logo",1);
    OVD_DEMO_CapInfoV2.video.support_aov=iniGetInt("devCap","support_aov",0);

    iniGetString("devCap","resolution_supportlists", buffer,sizeof(buffer),NULL);
    if (strstr(buffer, "640*360"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W640_H360;
    }
    if (strstr(buffer, "960*540"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W960_H540;
    }
    if (strstr(buffer, "1280*720"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W1280_H720;
    }
    if (strstr(buffer, "1920*1080"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W1920_H1080;
    }
    if (strstr(buffer, "2304*1296"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W2304_H1296;
    }
    if (strstr(buffer, "2560*1440"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W2560_H1440;
    }
    if (strstr(buffer, "2560*1920"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W2560_H1920;
    }
    if (strstr(buffer, "3840*2160"))
    {
        OVD_DEMO_CapInfoV2.video.resolution_supportlists |= OVD_VIDEO_RESOLUTION_W3840_H2160;
    }

    iniGetString("devCap","framerate_supportlists", buffer,sizeof(buffer),NULL);
    if (strstr(buffer, "15"))
    {
        OVD_DEMO_CapInfoV2.video.framerate_supportlists |= OVD_VIDEO_FRAMERATE_15;
    }
    if (strstr(buffer, "20"))
    {
        OVD_DEMO_CapInfoV2.video.framerate_supportlists |= OVD_VIDEO_FRAMERATE_20;
    }
    if (strstr(buffer, "25"))
    {
        OVD_DEMO_CapInfoV2.video.framerate_supportlists |= OVD_VIDEO_FRAMERATE_25;
    }
    if (strstr(buffer, "30"))
    {
        OVD_DEMO_CapInfoV2.video.framerate_supportlists |= OVD_VIDEO_FRAMERATE_30;
    }

    iniGetString("devCap","GOP_supportlists", buffer,sizeof(buffer),NULL);
    if (strstr(buffer, "2"))
    {
        OVD_DEMO_CapInfoV2.video.GOP_supportlists |= OVD_VIDEO_GOP_TIME_2S;
    }
    if (strstr(buffer, "3"))
    {
        OVD_DEMO_CapInfoV2.video.GOP_supportlists |= OVD_VIDEO_GOP_TIME_3S;
    }
    if (strstr(buffer, "4"))
    {
        OVD_DEMO_CapInfoV2.video.GOP_supportlists |= OVD_VIDEO_GOP_TIME_4S;
    }
    if (strstr(buffer, "5"))
    {
        OVD_DEMO_CapInfoV2.video.GOP_supportlists |= OVD_VIDEO_GOP_TIME_5S;
    }
    if (strstr(buffer, "6"))
    {
        OVD_DEMO_CapInfoV2.video.GOP_supportlists |= OVD_VIDEO_GOP_TIME_6S;
    }

    // 音频能力集
    OVD_DEMO_CapInfoV2.audio.support_audio_out=iniGetInt("devCap","have_audio_out",1);
    if(1==OVD_DEMO_CapInfoV2.audio.support_audio_out)
    {
        AudioCap=1;
    }
    OVD_DEMO_CapInfoV2.audio.support_voice_out=iniGetInt("devCap","have_voice_out",1);
    OVD_DEMO_CapInfoV2.audio.audiosamplerate=iniGetInt("devCap","audiosamplerate",0);
    OVD_DEMO_CapInfoV2.audio.support_voiceout_volume=iniGetInt("devCap", "have_voiceout_volume", 1);

    // 摄像机能力
    OVD_DEMO_CapInfoV2.camera.card_playback_speed=iniGetInt("devCap","card_playback_speed", OVD_CARD_PLAYBACK_SPEED_MASK_1TIMES);
    OVD_DEMO_CapInfoV2.camera.AIGC_cloud_record = iniGetInt("devCap", "AIGC_cloud_record", 0);
    OVD_DEMO_CapInfoV2.camera.support_DN_mode = iniGetInt("devCap", "support_DN_mode", 0);
    OVD_DEMO_CapInfoV2.camera.support_led=iniGetInt("devCap", "have_led", 1);
    OVD_DEMO_CapInfoV2.camera.support_sd = iniGetInt("devCap", "have_sd", 1);
    OVD_DEMO_CapInfoV2.camera.support_switch = iniGetInt("devCap", "have_switch", 1);
    OVD_DEMO_CapInfoV2.camera.support_sdcapacity=iniGetInt("devCap","have_sdcapacity",0);

    int normal_nightvision = iniGetInt("devCap","support_set_normal_nightvision_mode",1);
    int color_nightvision = iniGetInt("devCap", "support_set_color_nightvision_mode", 1);
    int ultraviolet_nightvision = iniGetInt("devCap", "support_ultraviolet_nightvision_mode", 1);
    if (color_nightvision)
    {
        OVD_DEMO_CapInfoV2.camera.nightvision_mode = OVD_NIGHTVISION_MODE_COLOR;
    }
    else if (normal_nightvision)
    {
        OVD_DEMO_CapInfoV2.camera.nightvision_mode = OVD_NIGHTVISION_MODE_NORMAL;
    }
    else if (ultraviolet_nightvision)
    {
        OVD_DEMO_CapInfoV2.camera.nightvision_mode = OVD_NIGHTVISION_MODE_ULTRAVIOLET;
    }
    else
    {
        OVD_DEMO_CapInfoV2.camera.nightvision_mode = OVD_NIGHTVISION_MODE_NONE;
    }
    OVD_DEMO_CapInfoV2.camera.ai_nightvision_mask=iniGetInt("devCap", "support_detect_nightvision_mask", OVD_NIGHTVISION_DETECT_MOTION | OVD_NIGHTVISION_DETECT_HUMAN);
    OVD_DEMO_CapInfoV2.camera.support_set_fill_light=iniGetInt("devCap","support_set_fill_light", 0);
    OVD_DEMO_CapInfoV2.camera.support_set_fill_light_brightness=iniGetInt("devCap","support_set_fill_light_brightness", 0);
    OVD_DEMO_CapInfoV2.camera.support_set_fill_light_schedule_time=iniGetInt("devCap","support_set_fill_light_schedule_time", 0);
    OVD_DEMO_CapInfoV2.camera.support_set_fill_light_auto=iniGetInt("devCap","support_set_fill_light_auto", 0);

    // PTZ能力集
    OVD_DEMO_CapInfoV2.ptz.support_ptz=iniGetInt("devCap","have_ptz",1);
    OVD_DEMO_CapInfoV2.ptz.support_ptz_preset=iniGetInt("devCap","support_ptz_preset",1);
    OVD_DEMO_CapInfoV2.ptz.support_zoom_control=iniGetInt("devCap","zoomcontrol",1);
    OVD_DEMO_CapInfoV2.ptz.support_focus_control=iniGetInt("devCap","focuscontrol",1);
    OVD_DEMO_CapInfoV2.ptz.support_trace=iniGetInt("devCap","have_trace",1);
    OVD_DEMO_CapInfoV2.ptz.support_set_target=iniGetInt("devCap","have_trace_set_target",0);
    OVD_DEMO_CapInfoV2.ptz.support_doorbell_preset=iniGetInt("devCap","have_doorbell_preset",0);

    // 枪球联动能力集
    OVD_DEMO_CapInfoV2.linkage_tracking.support_smart=iniGetInt("devCap","support_linkage_tracking_smart",0);
    OVD_DEMO_CapInfoV2.linkage_tracking.support_manual=iniGetInt("devCap","support_linkage_tracking_manual",0);
    OVD_DEMO_CapInfoV2.linkage_tracking.support_fix=iniGetInt("devCap","support_linkage_tracking_fix",0);

    //庭院灯配置
    OVD_DEMO_CapInfoV2.yardlight.support_yard_light_brightness=iniGetInt("devCap", "yard_light_brightness", 0);
    OVD_DEMO_CapInfoV2.yardlight.support_yard_light_manual_control=iniGetInt("devCap", "yard_light_manual_control", 0);
    OVD_DEMO_CapInfoV2.yardlight.yard_light_timed_schedule=iniGetInt("devCap", "yard_light_timed_schedule", 2);

    // 是否支持行车记录仪功能
    OVD_DEMO_CapInfoV2.dashcam.support_dashcam=iniGetInt("devCap", "dashcam", 0);
    OVD_DEMO_CapInfoV2.dashcam.support_dashcam_findcar=iniGetInt("devCap", "dashcam_findcar", 0);
    OVD_DEMO_CapInfoV2.dashcam.support_dashcam_rescue=iniGetInt("devCap", "dashcam_findcar", 0);
    //AI相关
    //(1)人脸
    OVD_DEMO_CapInfoV2.ai_face.support_face=iniGetInt("devCap","AI_supportface",0);
    OVD_DEMO_CapInfoV2.ai_face.support_faceExposureAdjust=iniGetInt("devCap","AI_face_exposureadjust",60);
    iniGetString("devCap", "AI_face_capturemodelists", buffer, sizeof(buffer), "quality");
    if (strstr(buffer, "quality"))
    {
        OVD_DEMO_CapInfoV2.ai_face.capture_mode_mask |= OVD_CAPTURE_MODE_QUALITY;
    }
    if (strstr(buffer, "quick"))
    {
        OVD_DEMO_CapInfoV2.ai_face.capture_mode_mask |= OVD_CAPTURE_MODE_QUICK;
    }

	OVD_DEMO_CapInfoV2.ai_face.alarm_zone=iniGetInt("devCap","AI_face_zone",1);
	OVD_DEMO_CapInfoV2.ai_face.support_mask_detection=iniGetInt("devCap","AI_face_maskDetection",0);
    OVD_DEMO_CapInfoV2.ai_face.support_speech_keep_alarm=iniGetInt("devCap","AI_face_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_face.support_speech_set_interval=iniGetInt("devCap","AI_face_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_face.support_light_keep_alarm=iniGetInt("devCap","AI_face_support_light_keep_alarm",0);
	//(2)厨房
	OVD_DEMO_CapInfoV2.ai_kitchen.support_kitchen_detection=iniGetInt("devCap","AI_supportkitchen",0);
	iniGetString("devCap", "AI_kitchen_capturemodelists", buffer, sizeof(buffer), "mask,cap,clothes");
    if (strstr(buffer, "mask"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_MASK;
    }
    if (strstr(buffer, "cap"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_CAP;
    }
    if (strstr(buffer, "clothes"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_CLOTHES;
    }
	iniGetString("devCap", "AI_kitchen_clothesColorList", buffer, sizeof(buffer), "");
    if (strstr(buffer, "white"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_WHITE;
    }
    if (strstr(buffer, "black"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_BLACK;
    }
    if (strstr(buffer, "red"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_RED;
    }
    if (strstr(buffer, "blue"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_BLUE;
    }
    if (strstr(buffer, "green"))
    {
        OVD_DEMO_CapInfoV2.ai_kitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_GREEN;
    }
	OVD_DEMO_CapInfoV2.ai_kitchen.alarm_zone=iniGetInt("devCap","AI_kitchen_zone",1);
    //(3)车辆检测
	OVD_DEMO_CapInfoV2.ai_vehicle.support_vehicle_detection=iniGetInt("devCap","AI_supportvehicle",1);
	OVD_DEMO_CapInfoV2.ai_vehicle.alarm_zone=iniGetInt("devCap","AI_vehicle_zone",1);
	OVD_DEMO_CapInfoV2.ai_vehicle.support_vehicle_detect_site=iniGetInt("devCap","AI_vehicle_detect_site",1);
    OVD_DEMO_CapInfoV2.ai_vehicle.support_speech_keep_alarm=iniGetInt("devCap","AI_vehicle_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_vehicle.support_speech_set_interval=iniGetInt("devCap","AI_vehicle_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_vehicle.support_light_keep_alarm=iniGetInt("devCap","AI_vehicle_support_light_keep_alarm",0);
    //(4)非机动车检测（梯控）
	OVD_DEMO_CapInfoV2.ai_nonvehicle.support_nonvehicle_detection=iniGetInt("devCap","AI_supportnonmotorvehicle",1);
	iniGetString("devCap", "AI_nonmotorvehicle_capturemodelists", buffer, sizeof(buffer), "electromobile,bike");
    if (strstr(buffer, "electromobile"))
    {
        OVD_DEMO_CapInfoV2.ai_nonvehicle.capture_mode_mask |= OVD_VEHICLE_CAPTURE_MODE_ELECTROMOBILE;
    }
    if (strstr(buffer, "bike"))
    {
        OVD_DEMO_CapInfoV2.ai_nonvehicle.capture_mode_mask |= OVD_VEHICLE_CAPTURE_MODE_BIKE;
    }
	OVD_DEMO_CapInfoV2.ai_nonvehicle.alarm_zone=iniGetInt("devCap","AI_nonmotorvehicle_zone",1);
    OVD_DEMO_CapInfoV2.ai_nonvehicle.support_speech_keep_alarm=iniGetInt("devCap","AI_nonmotorvehicle_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_nonvehicle.support_speech_set_interval=iniGetInt("devCap","AI_nonmotorvehicle_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_nonvehicle.support_light_keep_alarm=iniGetInt("devCap","AI_nonmotorvehicle_support_light_keep_alarm",0);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AI_nonmotorvehicle_capturemodelists:%d",OVD_DEMO_CapInfoV2.ai_nonvehicle.capture_mode_mask);
	//(5)客流统计
	OVD_DEMO_CapInfoV2.ai_passenger.support_passenger=iniGetInt("devCap","AI_supportpassenger",1);
	OVD_DEMO_CapInfoV2.ai_passenger.support_passenger_exposure_adjust=iniGetInt("devCap","AI_passenger_exposureadjust",1);
	iniGetString("devCap", "AI_passenger_capturemodelists", buffer, sizeof(buffer), "quality,quick");
    if (strstr(buffer, "quality"))
    {
        OVD_DEMO_CapInfoV2.ai_passenger.capture_mode_mask |= OVD_CAPTURE_MODE_QUALITY;
    }
    if (strstr(buffer, "quick"))
    {
        OVD_DEMO_CapInfoV2.ai_passenger.capture_mode_mask |= OVD_CAPTURE_MODE_QUICK;
    }
	OVD_DEMO_CapInfoV2.ai_passenger.alarm_zone=iniGetInt("devCap","AI_passenger_zone",1);
    OVD_DEMO_CapInfoV2.ai_passenger.support_osd_status=iniGetInt("devCap","AI_passenger_osd",1);
    OVD_DEMO_CapInfoV2.ai_passenger.support_welcome_promotion=iniGetInt("devCap","AI_passenger_supportwelcome",0);
    OVD_DEMO_CapInfoV2.ai_passenger.support_alert_time=iniGetInt("devCap","AI_passenger_supportalerttime",0);

    //高空抛物
    OVD_DEMO_CapInfoV2.ai_parabolic.support_parabolicaerial = iniGetInt("devCap","AI_support_parabolic_aerial",1);
    OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone_num   = iniGetInt("devCap","AI_parabolic_aerial_detect_zone_num",1);
    OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone      = iniGetInt("devCap","AI_parabolic_aerial_detect_zone",1);
    OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone_num   = iniGetInt("devCap","AI_parabolic_aerial_shield_zone_num",1);
    OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone      = iniGetInt("devCap","AI_parabolic_aerial_shield_zone",1);
    OVD_DEMO_CapInfoV2.ai_parabolic.floor_conf_num    = iniGetInt("devCap","AI_parabolic_aerial_floor_configuration_num",1);

    //区域人数统计
    OVD_DEMO_CapInfoV2.ai_regional_people.support_regional_people_stat                     = iniGetInt("devCap","AI_support_regional_people_statistics",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.support_alert_time                     = iniGetInt("devCap","AI_support_regional_people_alert_time",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.support_osd_status                     = iniGetInt("devCap","AI_support_regional_people_osd_status",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.regional_people_count          = iniGetInt("devCap","AI_support_regional_people_count",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.alarm_report_duration          = iniGetInt("devCap","AI_support_regional_people_alarm_report_duration",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.detect_result_report_duration  = iniGetInt("devCap","AI_support_regional_people_detect_result_report_duration",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.detect_plans_num                       = iniGetInt("devCap","AI_support_regional_people_detect_plans_num",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.alarm_area_num                         = iniGetInt("devCap","AI_support_regional_people_alarm_area_num",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.alarm_zone                             = iniGetInt("devCap","AI_support_regional_people_alarm_zone",1);
    OVD_DEMO_CapInfoV2.ai_regional_people.support_speech_keep_alarm=iniGetInt("devCap","AI_regional_people_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_regional_people.support_speech_set_interval=iniGetInt("devCap","AI_regional_people_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_regional_people.support_light_keep_alarm=iniGetInt("devCap","AI_regional_people_support_light_keep_alarm",0);

    //车道线
    OVD_DEMO_CapInfoV2.ai_laneline.support_lane_line          = iniGetInt("devCap","AI_support_lane_line",1);
    OVD_DEMO_CapInfoV2.ai_laneline.lane_line_num        = iniGetInt("devCap","AI_lane_line_num",1);
    OVD_DEMO_CapInfoV2.ai_laneline.lane_line_alarm_zone = iniGetInt("devCap","AI_lane_line_zone",1);

    //off duty
    OVD_DEMO_CapInfoV2.ai_offduty.off_duty_durtion          = iniGetInt("devCap","AI_support_off_duty",1);    //此处是否应为“OVD_DEMO_CapInfoV2.ai_offduty.AI_support_off_duty”
    OVD_DEMO_CapInfoV2.ai_offduty.support_off_duty_detection          = iniGetInt("devCap","AI_support_off_duty",1);
    OVD_DEMO_CapInfoV2.ai_offduty.support_alert_time         = iniGetInt("devCap","AI_support_off_duty_alert_time",1);
    OVD_DEMO_CapInfoV2.ai_offduty.on_duty_count      = iniGetInt("devCap","AI_support_on_duty_count",1);
    OVD_DEMO_CapInfoV2.ai_offduty.off_duty_durtion   = iniGetInt("devCap","AI_support_off_duty_durtion",1);
    OVD_DEMO_CapInfoV2.ai_offduty.detect_plan_num             = iniGetInt("devCap","AI_support_off_duty_detect_plans_num",1);
    OVD_DEMO_CapInfoV2.ai_offduty.alarm_area_num             = iniGetInt("devCap","AI_support_off_duty_alarm_area_num",1);
    OVD_DEMO_CapInfoV2.ai_offduty.alarm_zone                 = iniGetInt("devCap","AI_off_duty_zone",1);

    //头盔检测
    OVD_DEMO_CapInfoV2.ai_helmetwearing.support_helmet_wearing_detection = iniGetInt("devCap","AI_support_helmet_wearing_detection",1);
    OVD_DEMO_CapInfoV2.ai_helmetwearing.detect_zone = iniGetInt("devCap","AI_helmet_wearing_zone",1);
    OVD_DEMO_CapInfoV2.ai_helmetwearing.support_detect_time = iniGetInt("devCap","AI_support_detect_time",1);
    OVD_DEMO_CapInfoV2.ai_helmetwearing.support_speech_keep_alarm=iniGetInt("devCap","AI_helmet_wearing_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_helmetwearing.support_speech_set_interval=iniGetInt("devCap","AI_helmet_wearing_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_helmetwearing.support_light_keep_alarm=iniGetInt("devCap","AI_helmet_wearing_support_light_keep_alarm",0);

    //手势识别
    OVD_DEMO_CapInfoV2.ai_gesturerecognition.support_gesture_recognition = iniGetInt("devCap","AI_support_gesture_recognition",0);
    OVD_DEMO_CapInfoV2.ai_gesturerecognition.support_gesture_call = iniGetInt("devCap","AI_support_gesture_call",0);
    OVD_DEMO_CapInfoV2.ai_gesturerecognition.support_set_duration = iniGetInt("devCap","AI_support_set_duration",0);
    OVD_DEMO_CapInfoV2.ai_gesturerecognition.support_set_calltime = iniGetInt("devCap","AI_support_set_calltime",0);
    OVD_DEMO_CapInfoV2.ai_gesturerecognition.support_gesture_help = iniGetInt("devCap","AI_support_gesture_help",0);

    // 烟雾火焰检测
    OVD_DEMO_CapInfoV2.ai_smokefire.support_smoke_fire_detection = iniGetInt("devCap","AI_support_smoke_fire_detection",0);
    OVD_DEMO_CapInfoV2.ai_smokefire.detect_zone = iniGetInt("devCap","AI_smokefire_detect_zone",1);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_detect_time = iniGetInt("devCap","AI_smokefire_support_detect_time",0);
    OVD_DEMO_CapInfoV2.ai_smokefire.alertarea_num = iniGetInt("devCap", "AI_smokefire_alertarea_num", 1);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_linkage_speech = iniGetInt("devCap", "AI_smokefire_support_linkage_speech", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_speech_keep_alarm = iniGetInt("devCap", "AI_smokefire_support_speech_keep_alarm", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_speech_set_interval = iniGetInt("devCap", "AI_smokefire_support_speech_set_interval", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_linkage_light = iniGetInt("devCap", "AI_smokefire_support_linkage_light", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_light_keep_alarm = iniGetInt("devCap", "AI_smokefire_support_light_keep_alarm", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_linkage_buz = iniGetInt("devCap", "AI_smokefire_support_linkage_buz", 0);
    OVD_DEMO_CapInfoV2.ai_smokefire.support_buz_keep_alarm = iniGetInt("devCap", "AI_smokefire_support_buz_keep_alarm", 0);

	//家人识别
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_family_recognition = iniGetInt("devCap","AI_support_family_recognition",0);
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_face_management = iniGetInt("devCap","AI_support_face_management",0);
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_set_schedule_time = iniGetInt("devCap","AI_support_family_set_schedule_time",0);
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_speech_keep_alarm = iniGetInt("devCap","AI_support_family_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_speech_set_interval = iniGetInt("devCap","AI_support_family_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_familyrecognition.support_light_keep_alarm = iniGetInt("devCap","AI_support_family_light_keep_alarm",0);

    //隐私遮蔽
    OVD_DEMO_CapInfoV2.ai_privacymasking.support_privacy_masking = iniGetInt("devCap","AI_support_privacy_masking",1);
    OVD_DEMO_CapInfoV2.ai_privacymasking.zone = iniGetInt("devCap","AI_support_privacy_masking_zone",0);
    OVD_DEMO_CapInfoV2.ai_privacymasking.zone_mode=iniGetInt("devCap","AI_support_privacy_masking_zone_mode",1);

    //跌倒检测
    OVD_DEMO_CapInfoV2.ai_falldetection.support_fall_detection = iniGetInt("devCap","AI_support_fall_detection",0);
    OVD_DEMO_CapInfoV2.ai_falldetection.zone = iniGetInt("devCap","AI_support_fall_detection_zone",0);
    OVD_DEMO_CapInfoV2.ai_falldetection.support_set_schedule_time = iniGetInt("devCap","AI_support_fall_detection_set_schedule_time",0);
    OVD_DEMO_CapInfoV2.ai_falldetection.support_speech_keep_alarm = iniGetInt("devCap","AI_support_fall_detection_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.ai_falldetection.support_speech_set_interval = iniGetInt("devCap","AI_support_fall_detection_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.ai_falldetection.support_light_keep_alarm = iniGetInt("devCap","AI_support_fall_detection_light_keep_alarm",0);

    //联动方式
	iniGetString("devCap", "linkagemode_alertarea", buffer, sizeof(buffer), "");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.alertarea);
	iniGetString("devCap", "linkagemode_vehicledetection", buffer, sizeof(buffer), "");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.vehicle_detection);
    iniGetString("devCap", "linkagemode_nonmotorvehicledetection", buffer, sizeof(buffer), "");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.nonmotorvehicle_detection);
	iniGetString("devCap", "linkagemode_maskdetection", buffer, sizeof(buffer), "");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.mask_detection);
    iniGetString("devCap", "linkagemode_regionalpeopledetection", buffer, sizeof(buffer), "");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.regionalPeopleStat_detection);
    iniGetString("devCap","linkagemode_transgression", buffer,sizeof(buffer),"");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.transgression);
    iniGetString("devCap","linkagemode_helmet_wearing_detection", buffer,sizeof(buffer),"");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.helmet_wearing_detection);
    iniGetString("devCap","linkagemode_family_recognition", buffer,sizeof(buffer),"");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.family_recognition);
    iniGetString("devCap","linkagemode_fall_detection", buffer,sizeof(buffer),"");
    OVD_DEMO_CapsLinkageStr2mask(buffer, &OVD_DEMO_CapInfoV2.linkage.fall_detection);
	//alarm
    OVD_DEMO_CapInfoV2.alarm.support_stop_alarm=iniGetInt("devCap","stopalarm",1);
	OVD_DEMO_CapInfoV2.alarm.support_alarms_io=iniGetInt("devCap","have_alarms_io",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_cry=iniGetInt("devCap","have_alarms_cry",1);
    OVD_DEMO_CapInfoV2.alarm.support_crying_pacify=iniGetInt("devCap","support_crying_pacify",1);
    OVD_DEMO_CapInfoV2.alarm.crying_pacify_audio_playing_count=iniGetInt("devCap","crying_pacify_audio_playing_count",0);
    OVD_DEMO_CapInfoV2.alarm.support_crying_pacify_audio_playing_volumn=iniGetInt("devCap","support_crying_pacify_audio_playing_volumn",0);
    OVD_DEMO_CapInfoV2.alarm.crying_pacify_audio_playing_type=iniGetInt("devCap","crying_pacify_audio_playing_type",0);
    OVD_DEMO_CapInfoV2.alarm.support_crying_pacify_detect_on=iniGetInt("devCap","crying_pacify_detect_on",1);
    OVD_DEMO_CapInfoV2.alarm.support_crying_pacify_sense=iniGetInt("devCap","crying_pacify_sense",1);
    OVD_DEMO_CapInfoV2.alarm.support_cry_appease_music_selected=iniGetInt("devCap","cry_appease_music_selected",1);
    OVD_DEMO_CapInfoV2.alarm.support_cry_appease_music_schedule=iniGetInt("devCap","crying_pacify_sense_schedule",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_voice=iniGetInt("devCap","have_alarms_voice",1);
    OVD_DEMO_CapInfoV2.alarm.alarm_voice_detect_stand=iniGetInt("devCap","alarm_voice_detect_stand",2);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_motion=iniGetInt("devCap","have_alarms_motion",1);
    OVD_DEMO_CapInfoV2.alarm.alarms_motion_zone=iniGetInt("devCap","support_alarms_motion_zone",1);
    OVD_DEMO_CapInfoV2.alarm.alarm_motion_detect_stand=iniGetInt("devCap","alarm_motion_detect_stand",2);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_body=iniGetInt("devCap","have_alarms_body",1);
    OVD_DEMO_CapInfoV2.alarm.alarms_body_zone=iniGetInt("devCap","support_alarms_body_zone",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_cross=iniGetInt("devCap","have_alarms_cross",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_pir=iniGetInt("devCap","have_alarms_pir",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_pir_staymode=iniGetInt("devCap","have_alarms_pir_staymode",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_lossLock=iniGetInt("devCap","have_alarms_lossLock",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_alertarea=iniGetInt("devCap","have_alarms_alertarea",1);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_alertarea_zone=iniGetInt("devCap","have_alarms_alertarea_zone",1);
    OVD_DEMO_CapInfoV2.alarm.support_speech_keep_alarm=iniGetInt("devCap","have_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.alarm.support_speech_set_interval=iniGetInt("devCap","have_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.alarm.support_light_keep_alarm=iniGetInt("devCap","have_light_keep_alarm",0);
    OVD_DEMO_CapInfoV2.alarm.support_alertarea_expel=iniGetInt("devCap","have_alertarea_expel",1);

    OVD_DEMO_CapInfoV2.alarm.alertarea_params.alarms_zone=iniGetInt("devCap","have_alarms_alertarea_zone",1);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_set_target =iniGetInt("devCap", "alertarea_support_set_target", 0);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_staymode_time =iniGetInt("devCap", "alertarea_support_staymode_time", 0);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_set_alerttime =iniGetInt("devCap", "alertarea_support_set_alerttime", 0);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.alarms_zone=iniGetInt("devCap","support_alarms_alertarea_zone",1);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.alertarea_num=iniGetInt("devCap","alertarea_num",1);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_speech_keep_alarm=iniGetInt("devCap","alertarea_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_speech_set_interval=iniGetInt("devCap","alertarea_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.alarm.alertarea_params.support_light_keep_alarm=iniGetInt("devCap","alertarea_support_light_keep_alarm",0);

    OVD_DEMO_CapInfoV2.alarm.support_alarms_transgression =iniGetInt("devCap", "enable_transgression", 0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_set_target =iniGetInt("devCap", "transgression_support_set_target", 0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_set_alerttime =iniGetInt("devCap", "transgression_support_set_alerttime", 0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_set_statistics_line =iniGetInt("devCap", "transgression_upport_set_statistics_line", 0);// 此处应为"transgression_upport_set_statistics_line"
    OVD_DEMO_CapInfoV2.alarm.transgression_params.statistics_line_another_num =iniGetInt("devCap", "statistics_line_another_num", 0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_speech_keep_alarm=iniGetInt("devCap","transgression_support_speech_keep_alarm",0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_speech_set_interval=iniGetInt("devCap","transgression_support_speech_set_interval",0);
    OVD_DEMO_CapInfoV2.alarm.transgression_params.support_light_keep_alarm=iniGetInt("devCap","transgression_support_light_keep_alarm",0);

    OVD_DEMO_CapInfoV2.alarm.support_alarms_wifi_sensing = iniGetInt("devCap","support_wifi_sensing",0);
    OVD_DEMO_CapInfoV2.alarm.wifi_sensing_params.support_set_schedule_time= iniGetInt("devCap","wifi_sensing_support_set_schedule_time",0);
    OVD_DEMO_CapInfoV2.alarm.wifi_sensing_params.support_set_sensing_mode= iniGetInt("devCap","wifi_sensing_support_set_sensing_mode",0);

    OVD_DEMO_CapInfoV2.alarm.support_3d_light_sensor = iniGetInt("devCap","support_3d_light_sensor",0);
    OVD_DEMO_CapInfoV2.alarm.support_alarms_pet=iniGetInt("devCap","have_alarms_pet",1);
    OVD_DEMO_CapInfoV2.alarm.alarms_pet_zone=iniGetInt("devCap","support_alarms_pet_zone",1);
    
    // 门锁
    OVD_DEMO_CapInfoV2.doorlock.battery=iniGetInt("devCap","lock_battery",1);
    OVD_DEMO_CapInfoV2.doorlock.tmppwd=iniGetInt("devCap","lock_tmppwd",0);
    OVD_DEMO_CapInfoV2.doorlock.unlockway=iniGetInt("devCap","lock_unlockway",15);
    OVD_DEMO_CapInfoV2.doorlock.usermanage=iniGetInt("devCap","lock_usermanage",0);

    // 门铃
    OVD_DEMO_CapInfoV2.doorbell.have_doorbell_volume=iniGetInt("devCap", "have_doorbell_volume", 1);

    // 机器人
    OVD_DEMO_CapInfoV2.robot.support_EBO=iniGetInt("devCap","enable_EBO",1);
    OVD_DEMO_CapInfoV2.robot.support_move=iniGetInt("devCap","EBOmove",1);
    OVD_DEMO_CapInfoV2.robot.support_nofall=iniGetInt("devCap","EBOnofall",1);
    OVD_DEMO_CapInfoV2.robot.support_laser=iniGetInt("devCap","EBOlaser",1);
    OVD_DEMO_CapInfoV2.robot.support_onrush=iniGetInt("devCap","EBOonrush",1);
    OVD_DEMO_CapInfoV2.robot.support_recharge=iniGetInt("devCap","EBOrecharge",1);
    OVD_DEMO_CapInfoV2.robot.support_shake=iniGetInt("devCap","EBOshake",1);
    OVD_DEMO_CapInfoV2.robot.support_spin=iniGetInt("devCap","EBOspin",1);
    OVD_DEMO_CapInfoV2.robot.support_track=iniGetInt("devCap","EBOtrack",1);
    OVD_DEMO_CapInfoV2.robot.support_body_track=iniGetInt("devCap","EBObody_track",1);
    OVD_DEMO_CapInfoV2.robot.support_pets_track=iniGetInt("devCap","EBOpets_track",1);
    OVD_DEMO_CapInfoV2.robot.support_cruise=iniGetInt("devCap","EBOcruise",1);
    OVD_DEMO_CapInfoV2.robot.support_collision=iniGetInt("devCap","EBOcollision",1);
    OVD_DEMO_CapInfoV2.robot.support_reminder=iniGetInt("devCap","EBOreminder",1);
    OVD_DEMO_CapInfoV2.robot.support_speed=iniGetInt("devCap","EBOspeed",1);
    iniGetString("devCap","robot_ctrl",OVD_DEMO_CapInfoV2.robot.robot_ctrl,sizeof(OVD_DEMO_CapInfoV2.robot.robot_ctrl),"");

    //喂食器
    OVD_DEMO_CapInfoV2.robot.support_petfeeder=iniGetInt("devCap", "enable_petfeeder", 0);
    OVD_DEMO_CapInfoV2.robot.petfeeder_plan_num=iniGetInt("devCap", "petfeeder_plan_num", 3);
    OVD_DEMO_CapInfoV2.robot.petfeeder_feed_num=iniGetInt("devCap", "petfeeder_feed_num", 2);

    //睡眠检测-sleep_monitor
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_sleep_monitor = iniGetInt("devCap","support_sleep_monitor",1);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_sleep_detection = iniGetInt("devCap","support_sleep_detection",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_sleep_detection_sens = iniGetInt("devCap","support_sleep_detection_sens",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_face_cover = iniGetInt("devCap","support_face_cover",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_face_cover_sens = iniGetInt("devCap","support_face_cover_sens",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_face_cover_mode = iniGetInt("devCap","support_face_cover_mode",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_wakeup_detection = iniGetInt("devCap","support_wakeup_detection",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_wakeup_detection_sens = iniGetInt("devCap","support_wakeup_detection_sens",0);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_sleep_move_detection = iniGetInt("devCap","support_sleep_move_detection",1);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_sleep_move_detection_sens = iniGetInt("devCap","support_sleep_move_detection_sens",1);
    OVD_DEMO_CapInfoV2.ai_sleepmonitor.support_face_cover_area_mode = iniGetInt("devCap","support_face_cover_area_mode",1);

    //表情识别
    OVD_DEMO_CapInfoV2.ai_expressionrecognition.support_expression_recognition = iniGetInt("devCap","support_expression_recognition",1);
    OVD_DEMO_CapInfoV2.ai_expressionrecognition.support_expression_recognition_onoff=iniGetInt("devCap","support_expression_recognition_onoff",1);

     // 可移动摄像头
     OVD_DEMO_CapInfoV2.mobocam.support_interaction=iniGetInt("mobocamCap","interaction",1);
     OVD_DEMO_CapInfoV2.mobocam.support_cruise=iniGetInt("mobocamCap","cruise",1);
     OVD_DEMO_CapInfoV2.mobocam.support_onrush=iniGetInt("mobocamCap","onrush",1);
     OVD_DEMO_CapInfoV2.mobocam.support_shake=iniGetInt("mobocamCap","shake",1);
     OVD_DEMO_CapInfoV2.mobocam.support_nod=iniGetInt("mobocamCap","nod",1);
     OVD_DEMO_CapInfoV2.mobocam.support_turnACW=iniGetInt("mobocamCap","turnACW",1);
     OVD_DEMO_CapInfoV2.mobocam.support_turnCW=iniGetInt("mobocamCap","turnCW",1);
     OVD_DEMO_CapInfoV2.mobocam.support_figure_8=iniGetInt("mobocamCap","figure_8",1);
     OVD_DEMO_CapInfoV2.mobocam.support_figure_Z=iniGetInt("mobocamCap","figure_Z",1);
     OVD_DEMO_CapInfoV2.mobocam.support_figure_S=iniGetInt("mobocamCap","figure_S",1);
     OVD_DEMO_CapInfoV2.mobocam.support_spin=iniGetInt("mobocamCap","spin",1);
     OVD_DEMO_CapInfoV2.mobocam.support_abrupt=iniGetInt("mobocamCap","abrupt",1);
     OVD_DEMO_CapInfoV2.mobocam.support_waddle=iniGetInt("mobocamCap","waddle",1);
     OVD_DEMO_CapInfoV2.mobocam.support_backpedal=iniGetInt("mobocamCap","backpedal",1);
     OVD_DEMO_CapInfoV2.mobocam.support_move=iniGetInt("mobocamCap","move",1);
     OVD_DEMO_CapInfoV2.mobocam.support_speedswitch=iniGetInt("mobocamCap","speedswitch",5);
     OVD_DEMO_CapInfoV2.mobocam.support_motioncal=iniGetInt("mobocamCap","motioncal",1);
     OVD_DEMO_CapInfoV2.mobocam.support_cruiseroute=iniGetInt("mobocamCap","cruise_route",1);
     OVD_DEMO_CapInfoV2.mobocam.support_cruisemanual=iniGetInt("mobocamCap","cruise_manual",1);
     OVD_DEMO_CapInfoV2.mobocam.support_cruisetime=iniGetInt("mobocamCap","cruise_time",1);
     OVD_DEMO_CapInfoV2.mobocam.support_cruiserepeat=iniGetInt("mobocamCap","cruise_repeat",1);
     OVD_DEMO_CapInfoV2.mobocam.support_petTrack=iniGetInt("mobocamCap","petTrack",1);
     OVD_DEMO_CapInfoV2.mobocam.support_track=iniGetInt("mobocamCap","track",1);
     OVD_DEMO_CapInfoV2.mobocam.support_track_on=iniGetInt("mobocamCap","track_on",1);
     OVD_DEMO_CapInfoV2.mobocam.support_track_type=iniGetInt("mobocamCap","track_type",1);
     OVD_DEMO_CapInfoV2.mobocam.support_recharge=iniGetInt("mobocamCap","recharge",1);
     OVD_DEMO_CapInfoV2.mobocam.support_escape=iniGetInt("mobocamCap","escape",1);
     OVD_DEMO_CapInfoV2.mobocam.support_laser_on=iniGetInt("mobocamCap","laser_on",1);
     OVD_DEMO_CapInfoV2.mobocam.support_nofall_on=iniGetInt("mobocamCap","nofall_on",1);
     OVD_DEMO_CapInfoV2.mobocam.support_collision_on=iniGetInt("mobocamCap","collision_on",1);
     OVD_DEMO_CapInfoV2.mobocam.support_reminder_on=iniGetInt("mobocamCap","reminder_on",1);
    // CloudAI
    OVD_DEMO_CapInfoV2.cloudai.support_CloudAI=iniGetInt("devCap","CloudAI",0);

    //AIspeech
    OVD_DEMO_CapInfoV2.aispeech.support_AISpeech=iniGetInt("devCap","AISpeech",1);

    //童谣
    OVD_DEMO_CapInfoV2.lullby.support_lullby=iniGetInt("devCap","lullby",1);

    OVD_DEMO_CapInfoV2.aidiy.support_AIDIY=iniGetInt("devCap","support_AIDIY",1);
    if(iniGetInt("devCap","support_AIDIY_BABY_CARE",1)){
        OVD_DEMO_CapInfoV2.aidiy.support_AIDIY_mask |= OVD_AIDIY_MASK_BABY_MASK;
    }

    return 0;
}

int OVD_DEMOE_InitDevCap(){

    char buffer[512];
	OVD_CapInit(&OVD_DEMO_CapInfo);

    OVD_DEMO_CapInfo.device_type=iniGetInt("devCap", "device_type", 0);
    OVD_DEMO_CapInfo.camera_channel_max=iniGetInt("devCap", "max_channel", 1);
    OVD_DEMO_CapInfo.audiosamplerate=iniGetInt("devCap","audiosamplerate",0);
    OVD_DEMO_CapInfo.enable_low=iniGetInt("devCap","enable_low",0);
    OVD_DEMO_CapInfo.support_awaked=iniGetInt("devCap","support_awaked",0);
    OVD_DEMO_CapInfo.support_awaked_switch=iniGetInt("devCap","support_awaked_switch",0);
    OVD_DEMO_CapInfo.support_constant_power=iniGetInt("devCap","support_constant_power",0);
    OVD_DEMO_CapInfo.smart_mode=iniGetInt("devCap","smart_mode",0);
    OVD_DEMO_CapInfo.support_4G=iniGetInt("devCap","support_4G",0);
    OVD_DEMO_CapInfo.multi_stream=iniGetInt("devCap","multi_stream",0);
    OVD_DEMO_CapInfo.card_playback_speed=iniGetInt("devCap","card_playback_speed", OVD_CARD_PLAYBACK_SPEED_MASK_1TIMES);

    OVD_DEMO_CapInfo.have_ptz=iniGetInt("devCap","have_ptz",1);
    OVD_DEMO_CapInfo.support_ptz_preset=iniGetInt("devCap","support_ptz_preset",1);
    OVD_DEMO_CapInfo.stopalarm=iniGetInt("devCap","stopalarm",1);
    OVD_DEMO_CapInfo.zoomcontrol=iniGetInt("devCap","zoomcontrol",1);
    OVD_DEMO_CapInfo.focuscontrol=iniGetInt("devCap","focuscontrol",1);

    OVD_DEMO_CapInfo.have_battery=iniGetInt("devCap","have_battery",1);
    VideoCap=iniGetInt("devCap","have_video_out",1);
    OVD_DEMO_CapInfo.have_audio_out=iniGetInt("devCap","have_audio_out",1);
    if(1==OVD_DEMO_CapInfo.have_audio_out)
    {
        AudioCap=1;
    }
    OVD_DEMO_CapInfo.have_voice_out=iniGetInt("devCap","have_voice_out",1);
    OVD_DEMO_CapInfo.have_trace=iniGetInt("devCap","have_trace",1);

    //庭院灯配置
    OVD_DEMO_CapInfo.yard_light_brightness=iniGetInt("devCap", "yard_light_brightness", 0);
    OVD_DEMO_CapInfo.yard_light_manual_control=iniGetInt("devCap", "yard_light_manual_control", 0);
    OVD_DEMO_CapInfo.yard_light_timed_schedule=iniGetInt("devCap", "yard_light_timed_schedule", 2);

    // 是否支持行车记录仪功能
    OVD_DEMO_CapInfo.dashcam=iniGetInt("devCap", "dashcam", 0);
    OVD_DEMO_CapInfo.dashcam_findcar=iniGetInt("devCap", "dashcam_findcar", 0);
    OVD_DEMO_CapInfo.dashcam_rescue=iniGetInt("devCap", "dashcam_rescue", 0);
    OVD_DEMO_CapInfo.AIGC_cloud_record = iniGetInt("devCap", "AIGC_cloud_record", 0);
    //AI相关
    //(1)人脸
    OVD_DEMO_CapInfo.ovdAICapInfo.AIface.support_face=iniGetInt("devCap","AI_supportface",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIface.support_faceExposureAdjust=iniGetInt("devCap","AI_face_exposureadjust",1);
    iniGetString("devCap", "AI_face_capturemodelists", buffer, sizeof(buffer), "quality,quick");
    if (strstr(buffer, "quality"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIface.capture_mode_mask |= OVD_CAPTURE_MODE_QUALITY;
    }
    if (strstr(buffer, "quick"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIface.capture_mode_mask |= OVD_CAPTURE_MODE_QUICK;
    }

	OVD_DEMO_CapInfo.ovdAICapInfo.AIface.alarm_zone=iniGetInt("devCap","AI_face_zone",1);
	OVD_DEMO_CapInfo.ovdAICapInfo.AIface.support_mask_detection=iniGetInt("devCap","AI_face_maskDetection",1);
	//(2)厨房
	OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.support_kitchen_detection=iniGetInt("devCap","AI_supportkitchen",1);
	iniGetString("devCap", "AI_kitchen_capturemodelists", buffer, sizeof(buffer), "mask,cap,clothes");
    if (strstr(buffer, "mask"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_MASK;
    }
    if (strstr(buffer, "cap"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_CAP;
    }
    if (strstr(buffer, "clothes"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.capture_mode_mask |= OVD_KITCHEN_CAPTURE_CLOTHES;
    }
	iniGetString("devCap", "AI_kitchen_clothesColorList", buffer, sizeof(buffer), "");
    if (strstr(buffer, "white"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_WHITE;
    }
    if (strstr(buffer, "black"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_BLACK;
    }
    if (strstr(buffer, "red"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_RED;
    }
    if (strstr(buffer, "blue"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_BLUE;
    }
    if (strstr(buffer, "green"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.clothes_color_mask |= OVD_CLOTHES_COLOR_GREEN;
    }
	OVD_DEMO_CapInfo.ovdAICapInfo.AIkitchen.alarm_zone=iniGetInt("devCap","AI_kitchen_zone",1);
    //(3)车辆检测
	OVD_DEMO_CapInfo.ovdAICapInfo.AIvehicle.support_vehicle_detection=iniGetInt("devCap","AI_supportvehicle",1);
	OVD_DEMO_CapInfo.ovdAICapInfo.AIvehicle.alarm_zone=iniGetInt("devCap","AI_vehicle_zone",1);
	OVD_DEMO_CapInfo.ovdAICapInfo.AIvehicle.support_vehicle_detect_site=iniGetInt("devCap","AI_vehicle_detect_site",1);
    //(4)非机动车检测（梯控）
	OVD_DEMO_CapInfo.ovdAICapInfo.AInonmotorvehicle.support_nonvehicle_detection=iniGetInt("devCap","AI_supportnonmotorvehicle",1);
	iniGetString("devCap", "AI_nonmotorvehicle_capturemodelists", buffer, sizeof(buffer), "electromobile,bike");
    if (strstr(buffer, "electromobile"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AInonmotorvehicle.capture_mode_mask |= OVD_VEHICLE_CAPTURE_MODE_ELECTROMOBILE;
    }
    if (strstr(buffer, "bike"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AInonmotorvehicle.capture_mode_mask |= OVD_VEHICLE_CAPTURE_MODE_BIKE;
    }
	OVD_DEMO_CapInfo.ovdAICapInfo.AInonmotorvehicle.alarm_zone=iniGetInt("devCap","AI_nonmotorvehicle_zone",1);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AI_nonmotorvehicle_capturemodelists:%d",OVD_DEMO_CapInfo.ovdAICapInfo.AInonmotorvehicle.capture_mode_mask);
	//(5)客流统计
	OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.support_passenger=iniGetInt("devCap","AI_supportpassenger",1);
	OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.support_passenger_exposure_adjust=iniGetInt("devCap","AI_passenger_exposureadjust",1);
	iniGetString("devCap", "AI_passenger_capturemodelists", buffer, sizeof(buffer), "quality,quick");
    if (strstr(buffer, "quality"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.capture_mode_mask |= OVD_CAPTURE_MODE_QUALITY;
    }
    if (strstr(buffer, "quick"))
    {
        OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.capture_mode_mask |= OVD_CAPTURE_MODE_QUICK;
    }
	OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.alarm_zone=iniGetInt("devCap","AI_passenger_zone",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.support_osd_status=iniGetInt("devCap","AI_passenger_osd",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.support_welcome_promotion=iniGetInt("devCap","AI_passenger_supportwelcome",0);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIpassenger.support_alert_time=iniGetInt("devCap","AI_passenger_supportalerttime",0);


    //高空抛物
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.support_parabolicaerial = iniGetInt("devCap","AI_support_parabolic_aerial",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.detect_zone_num   = iniGetInt("devCap","AI_parabolic_aerial_detect_zone_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.detect_zone      = iniGetInt("devCap","AI_parabolic_aerial_detect_zone",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.shield_zone_num   = iniGetInt("devCap","AI_parabolic_aerial_shield_zone_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.shield_zone      = iniGetInt("devCap","AI_parabolic_aerial_shield_zone",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIparabolicAerial.floor_conf_num    = iniGetInt("devCap","AI_parabolic_aerial_floor_configuration_num",1);

    //区域人数统计
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.support_regional_people_stat                     = iniGetInt("devCap","AI_support_regional_people_statistics",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.support_alert_time                     = iniGetInt("devCap","AI_support_regional_people_alert_time",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.support_osd_status                     = iniGetInt("devCap","AI_support_regional_people_osd_status",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.regional_people_count          = iniGetInt("devCap","AI_support_regional_people_count",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.alarm_report_duration          = iniGetInt("devCap","AI_support_regional_people_alarm_report_duration",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.detect_result_report_duration  = iniGetInt("devCap","AI_support_regional_people_detect_result_report_duration",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.detect_plans_num                       = iniGetInt("devCap","AI_support_regional_people_detect_plans_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.alarm_area_num                         = iniGetInt("devCap","AI_support_regional_people_alarm_area_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIregionalPeopleStat.alarm_zone                             = iniGetInt("devCap","AI_support_regional_people_alarm_zone",1);

    //车道线
    OVD_DEMO_CapInfo.ovdAICapInfo.AILaneLine.support_lane_line          = iniGetInt("devCap","AI_support_lane_line",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AILaneLine.lane_line_num        = iniGetInt("devCap","AI_lane_line_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AILaneLine.lane_line_alarm_zone = iniGetInt("devCap","AI_lane_line_zone",1);

    //off duty
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.off_duty_durtion          = iniGetInt("devCap","AI_support_off_duty",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.support_alert_time         = iniGetInt("devCap","AI_support_off_duty_alert_time",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.on_duty_count      = iniGetInt("devCap","AI_support_on_duty_count",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.off_duty_durtion   = iniGetInt("devCap","AI_support_off_duty_durtion",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.detect_plan_num             = iniGetInt("devCap","AI_support_off_duty_detect_plans_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.alarm_area_num             = iniGetInt("devCap","AI_support_off_duty_alarm_area_num",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIOffDuty.alarm_zone                 = iniGetInt("devCap","AI_off_duty_zone",1);

    //头盔检测
    OVD_DEMO_CapInfo.ovdAICapInfo.AIHelmetWearing.support_helmet_wearing_detection = iniGetInt("devCap","AI_support_helmet_wearing_detection",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIHelmetWearing.detect_zone = iniGetInt("devCap","AI_helmet_wearing_zone",1);
    OVD_DEMO_CapInfo.ovdAICapInfo.AIHelmetWearing.support_detect_time = iniGetInt("devCap","AI_support_detect_time",1);

    //手势识别
    OVD_DEMO_CapInfo.ovdAICapInfo.AIGestureRecognition.support_gesture_recognition = iniGetInt("devCap","AI_support_gesture_recognition",0);

	//联动方式
	iniGetString("devCap", "linkagemode_alertarea", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.alertarea |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.alertarea |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.alertarea |= LINKAGE_MODE_BUZ;
    }
	iniGetString("devCap", "linkagemode_vehicledetection", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.vehicle_detection |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.vehicle_detection |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.vehicle_detection |= LINKAGE_MODE_BUZ;
    }
    iniGetString("devCap", "linkagemode_nonmotorvehicledetection", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.nonmotorvehicle_detection |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.nonmotorvehicle_detection |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.nonmotorvehicle_detection |= LINKAGE_MODE_BUZ;
    }
	iniGetString("devCap", "linkagemode_maskdetection", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.mask_detection |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.mask_detection |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.mask_detection |= LINKAGE_MODE_BUZ;
    }
    iniGetString("devCap", "linkagemode_regionalpeopledetection", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.regionalPeopleStat_detection |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.regionalPeopleStat_detection |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.regionalPeopleStat_detection |= LINKAGE_MODE_BUZ;
    }
    iniGetString("devCap","linkagemode_transgression", buffer,sizeof(buffer),"");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.transgression |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.transgression |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.transgression |= LINKAGE_MODE_BUZ;
    }
    iniGetString("devCap", "linkagemode_helmet_wearing_detection", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.helmet_wearing_detection |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.helmet_wearing_detection |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.helmet_wearing_detection |= LINKAGE_MODE_BUZ;
    }
    iniGetString("devCap", "linkagemode_family_recognition", buffer, sizeof(buffer), "");
    if (strstr(buffer, "speech"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.family_recognition |= LINKAGE_MODE_SPEECH;
    }
    if (strstr(buffer, "light"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.family_recognition |= LINKAGE_MODE_LIGHT;
    }
    if (strstr(buffer, "buz"))
    {
        OVD_DEMO_CapInfo.ovdlinkage_mode.family_recognition |= LINKAGE_MODE_BUZ;
    }
	//alarm
	OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_io=iniGetInt("devCap","have_alarms_io",1);


    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_cry=iniGetInt("devCap","have_alarms_cry",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_crying_pacify=iniGetInt("devCap","support_crying_pacify",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.crying_pacify_audio_playing_count=iniGetInt("devCap","crying_pacify_audio_playing_count",0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_crying_pacify_audio_playing_volumn=iniGetInt("devCap","support_crying_pacify_audio_playing_volumn",0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.crying_pacify_audio_playing_type=iniGetInt("devCap","crying_pacify_audio_playing_type",0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_crying_pacify_detect_on=iniGetInt("devCap","support_crying_pacify_detect_on",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_crying_pacify_sense=iniGetInt("devCap","support_crying_pacify_sense",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_cry_appease_music_selected=iniGetInt("devCap","support_cry_appease_music_selected",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_cry_appease_music_schedule=iniGetInt("devCap","support_crying_pacify_music_schedule",1);

    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_voice=iniGetInt("devCap","have_alarms_voice",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alarm_voice_detect_stand=iniGetInt("devCap","alarm_voice_detect_stand",2);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_motion=iniGetInt("devCap","have_alarms_motion",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alarm_motion_detect_stand=iniGetInt("devCap","alarm_motion_detect_stand",2);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_alarms_motion_zone=iniGetInt("devCap","support_alarms_motion_zone",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_body=iniGetInt("devCap","have_alarms_body",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_alarms_body_zone=iniGetInt("devCap","support_alarms_body_zone",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_cross=iniGetInt("devCap","have_alarms_cross",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_pir=iniGetInt("devCap","have_alarms_pir",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_pir_staymode=iniGetInt("devCap","have_alarms_pir_staymode",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_lossLock=iniGetInt("devCap","have_alarms_lossLock",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_alertarea=iniGetInt("devCap","have_alarms_alertarea",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alertarea_expel=iniGetInt("devCap","have_alertarea_expel",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_alertarea_zone=iniGetInt("devCap","have_alarms_alertarea_zone",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.alertarea_num=iniGetInt("devCap","alertarea_num",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_pet=iniGetInt("devCap","have_alarms_pet",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.support_alarms_pet_zone=iniGetInt("devCap","support_alarms_pet_zone",1);

    OVD_DEMO_CapInfo.have_switch=iniGetInt("devCap","have_switch",1);
    OVD_DEMO_CapInfo.have_led=iniGetInt("devCap", "have_led", 1);
    OVD_DEMO_CapInfo.light_supplement_lamp=iniGetInt("devCap","light_supplement_lamp",1);
    OVD_DEMO_CapInfo.have_screen=iniGetInt("devCap", "have_screen", 0);
	OVD_DEMO_CapInfo.have_support_softprobe=iniGetInt("devCap", "have_support_softprobe", 1);
    OVD_DEMO_CapInfo.have_sd = iniGetInt("devCap", "have_sd", 1);
    //OVD_DEMO_CapInfo.have_auto_reboot=iniGetInt("devCap", "have_auto_reboot", 1);

    OVD_DEMO_CapInfo.support_static_scence_ABR=iniGetInt("devCap","support_static_scence_ABR",0);
    OVD_DEMO_CapInfo.lock_params.battery=iniGetInt("devCap","lock_battery",1);
    OVD_DEMO_CapInfo.lock_params.tmppwd=iniGetInt("devCap","lock_tmppwd",0);
    OVD_DEMO_CapInfo.lock_params.unlockway=iniGetInt("devCap","lock_unlockway",15);
    OVD_DEMO_CapInfo.lock_params.usermanage=iniGetInt("devCap","lock_usermanage",0);

    OVD_DEMO_CapInfo.stream_encryption_mode=iniGetInt("devCap","stream_encryption_mode",0);

    iniGetString("devCap","video_quality",OVD_DEMO_CapInfo.video_quality,sizeof(OVD_DEMO_CapInfo.video_quality),"sd,hd");
    iniGetString("devCap","video_formats_supportlists",OVD_DEMO_CapInfo.video_formats_supportlists,sizeof(OVD_DEMO_CapInfo.video_formats_supportlists),"h265");

    OVD_DEMO_CapInfo.support_set_video_formats=iniGetInt("devCap","support_set_video_formats",1);
    OVD_DEMO_CapInfo.support_set_bitrate=iniGetInt("devCap","support_set_bitrate",0);
    OVD_DEMO_CapInfo.support_set_framerate=iniGetInt("devCap","support_set_framerate",0);
    OVD_DEMO_CapInfo.support_set_gop=iniGetInt("devCap","support_set_gop",0);
    OVD_DEMO_CapInfo.have_voiceout_volume=iniGetInt("devCap", "have_voiceout_volume", 60);
    OVD_DEMO_CapInfo.have_doorbell_volume=iniGetInt("devCap", "have_doorbell_volume", 60);
    OVD_DEMO_CapInfo.support_set_normal_nightvision_mode=iniGetInt("devCap","support_set_normal_nightvision_mode",1);
    OVD_DEMO_CapInfo.support_set_color_nightvision_mode=iniGetInt("devCap", "support_set_color_nightvision_mode", 1);
    OVD_DEMO_CapInfo.support_detect_nightvision_mask=iniGetInt("devCap", "support_detect_nightvision_mask", OVD_NIGHTVISION_DETECT_MOTION | OVD_NIGHTVISION_DETECT_HUMAN);
    OVD_DEMO_CapInfo.support_set_AI_rules_mode=iniGetInt("devCap","support_set_AI_rules_mode",0);
    OVD_DEMO_CapInfo.osd_text=iniGetInt("devCap","osd_text",1);
    OVD_DEMO_CapInfo.osd_logo=iniGetInt("devCap","osd_logo",1);
    OVD_DEMO_CapInfo.enable_hjkh=iniGetInt("devCap","enable_hjkh",1);
    OVD_DEMO_CapInfo.hjkh_mode.support_preference_call=iniGetInt("devCap","preference_call",1);
    OVD_DEMO_CapInfo.hjkh_mode.support_set_calling_number=iniGetInt("devCap","set_calling_number",1);
    OVD_DEMO_CapInfo.hjkh_mode.support_not_disturb_mode=iniGetInt("devCap","not_disturb_mode",1);
    OVD_DEMO_CapInfo.hjkh_mode.support_not_disturb_device_mode=iniGetInt("devCap","not_disturb_device_mode",1);

    OVD_DEMO_CapInfo.hjkh_mode.support_auto_hangup=iniGetInt("devCap","auto_hangup",1);
	OVD_DEMO_CapInfo.hjkh_mode.support_voice_calling_number=iniGetInt("devCap","voice_calling_number",1);

    OVD_DEMO_CapInfo.enable_EBO=iniGetInt("devCap","enable_EBO",1);
    OVD_DEMO_CapInfo.EBO_mode.support_move=iniGetInt("devCap","EBOmove",1);
    OVD_DEMO_CapInfo.EBO_mode.support_nofall=iniGetInt("devCap","EBOnofall",1);
    OVD_DEMO_CapInfo.EBO_mode.support_laser=iniGetInt("devCap","EBOlaser",1);
    OVD_DEMO_CapInfo.EBO_mode.support_onrush=iniGetInt("devCap","EBOonrush",1);
    OVD_DEMO_CapInfo.EBO_mode.support_recharge=iniGetInt("devCap","EBOrecharge",1);
    OVD_DEMO_CapInfo.EBO_mode.support_shake=iniGetInt("devCap","EBOshake",1);
    OVD_DEMO_CapInfo.EBO_mode.support_spin=iniGetInt("devCap","EBOspin",1);
    OVD_DEMO_CapInfo.EBO_mode.support_track=iniGetInt("devCap","EBOtrack",1);
    OVD_DEMO_CapInfo.EBO_mode.support_body_track=iniGetInt("devCap","EBObody_track",1);
    OVD_DEMO_CapInfo.EBO_mode.support_pets_track=iniGetInt("devCap","EBOpets_track",1);
    OVD_DEMO_CapInfo.EBO_mode.support_cruise=iniGetInt("devCap","EBOcruise",1);
    OVD_DEMO_CapInfo.EBO_mode.support_collision=iniGetInt("devCap","EBOcollision",1);
    OVD_DEMO_CapInfo.EBO_mode.support_reminder=iniGetInt("devCap","EBOreminder",1);
    OVD_DEMO_CapInfo.EBO_mode.support_speed=iniGetInt("devCap","EBOspeed",0);
    //机器人类型
    iniGetString("devCap","robot_ctrl",OVD_DEMO_CapInfo.robot_ctrl,sizeof(OVD_DEMO_CapInfo.robot_ctrl),"");

    //喂食器
    OVD_DEMO_CapInfo.enable_petfeeder=iniGetInt("devCap", "enable_petfeeder", 0);
    OVD_DEMO_CapInfo.petfeeder_plan_num=iniGetInt("devCap", "petfeeder_plan_num", 3);
    OVD_DEMO_CapInfo.petfeeder_feed_num=iniGetInt("devCap", "petfeeder_feed_num", 2);

    OVD_DEMO_CapInfo.enable_EBO=iniGetInt("devCap","enable_EBO",1);
	iniGetString("devCap","have_support_extension",OVD_DEMO_CapInfo.have_support_extension,sizeof(OVD_DEMO_CapInfo.have_support_extension),"");

    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.support_set_target =iniGetInt("devCap", "alertarea_support_set_target", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.support_staymode_time =iniGetInt("devCap", "alertarea_support_staymode_time", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.support_set_alerttime =iniGetInt("devCap", "alertarea_support_set_alerttime", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.alarms_zone=iniGetInt("devCap","support_alarms_alertarea_zone",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.alertarea_params.alertarea_num=iniGetInt("devCap","alertarea_num",1);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.have_alarms_transgression =iniGetInt("devCap", "enable_transgression", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.transgression_params.support_set_target =iniGetInt("devCap", "transgression_support_set_target", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.transgression_params.support_set_alerttime =iniGetInt("devCap", "transgression_support_set_alerttime", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.transgression_params.support_set_statistics_line =iniGetInt("devCap", "transgression_upport_set_statistics_line", 0);
    OVD_DEMO_CapInfo.ovdCapInfo_alarms.transgression_params.statistics_line_another_num =iniGetInt("devCap", "statistics_line_another_num", 0);
    OVD_DEMO_CapInfo.robot_housekeeper =iniGetInt("devCap", "robot_housekeeper", 0);
    OVD_DEMO_CapInfo.camera_lens_type =iniGetInt("devCap", "camera_lens_type", 1);

    OVD_DEMO_CapInfo.smoke_sensor = iniGetInt("devCap","smoke_sensor",1);

    // CloudAI
    OVD_DEMO_CapInfo.ovdCloudAICapInfo.support_CloudAI=iniGetInt("devCap","CloudAI",0);

    //AISpeech
    OVD_DEMO_CapInfo.ovdAISpeechCapinfo.support_AISpeech=iniGetInt("devCap","AISpeech",1);

    //童谣
    OVD_DEMO_CapInfo.ovdLullbyCapInfo.support_lullby=iniGetInt("devCap","lullby",1);

    return 0;
}

int OVD_DEMOE_InitDeviceConfInfo(){
    OVDChannelsInfo *chn;
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","读取deviceConfigInfo\n");
    int i=0;
    //char values[255];

    //先把值都清空
    memset(&OVD_DEMO_ConfigureInfo, 0, sizeof(OVDConfigrationInfo));

    OVD_DEMO_ConfigureInfo.channelsInfoCount=iniGetInt("deviceConfigInfo", "channelsInfoCount", 1);
    OVD_DEMO_ConfigureInfo.tz=iniGetInt("deviceConfigInfo","tz",8);
    //OVD_DEMO_ConfigureInfo.led=iniGetInt("deviceConfigInfo","led",1);


    //OVD_DEMO_ConfigureInfo.autorebootinfo.on=iniGetInt("deviceConfigInfo","autoboot_on",1);
    //OVD_DEMO_ConfigureInfo.autorebootinfo.cycle=iniGetDouble("deviceConfigInfo", "cycle", 0);
    //iniGetString("deviceConfigInfo","autoreboot_start",OVD_DEMO_ConfigureInfo.autorebootinfo.start,sizeof(OVD_DEMO_ConfigureInfo.autorebootinfo.start),NULL);
    //iniGetString("deviceConfigInfo","autoreboot_end",OVD_DEMO_ConfigureInfo.autorebootinfo.end,sizeof(OVD_DEMO_ConfigureInfo.autorebootinfo.end),NULL);
    iniGetString("deviceConfigInfo","autoreboot_last_reboot",OVD_DEMO_ConfigureInfo.autorebootinfo.last_reboot,sizeof(OVD_DEMO_ConfigureInfo.autorebootinfo.last_reboot),NULL);

    OVD_DEMO_ConfigureInfo.channelsInfo = (OVDChannelsInfo *)malloc(OVD_DEMO_ConfigureInfo.channelsInfoCount * sizeof(OVDChannelsInfo));
    for(i=0;i<OVD_DEMO_ConfigureInfo.channelsInfoCount;i++)
    {
        char sect[256];
        memset(sect,0,256);
        snprintf(sect,256,"channel%d",i);
        chn = &OVD_DEMO_ConfigureInfo.channelsInfo[i];
        chn->channel=i;
        if (i < OVD_DEMO_ConfigureInfo.channelsInfoCount)
        {
            //chn->on=iniGetInt(sect,"channel_on",1);
            //chn->flipInfo.horflip=iniGetInt(sect,"horflip",0);
            //chn->flipInfo.verflip=iniGetInt(sect,"verflip",0);

            chn->videoinfo.codec=iniGetInt(sect,"video_codec",2);
            //chn->videoinfo.quality=iniGetInt(sect,"video_quality",2);
            chn->videoinfo.bitrate=iniGetInt(sect,"video_bitrate",0);
            chn->videoinfo.width=(short)iniGetInt(sect,"video_width",0);
            chn->videoinfo.height=(short)iniGetInt(sect,"video_height",0);
            chn->videoinfo.framerate=iniGetInt(sect,"video_framerate",0);
            chn->videoinfo.framerate_aov=1;
            chn->videoinfo.colorDepth=iniGetInt(sect,"video_colorDepth",0);
            chn->videoinfo.frameInterval=iniGetInt(sect,"video_frameInterval",0);

            chn->audioinfo.codec=iniGetInt(sect,"audio_codec",3);
            chn->audioinfo.samplesRate=iniGetInt(sect,"audio_samplesRate",16000);
            chn->audioinfo.bitrate=iniGetInt(sect,"audio_bitrate",0);
            chn->audioinfo.waveFormat=iniGetInt(sect,"audio_waveFormat",0);
            chn->audioinfo.channelNumber=iniGetInt(sect,"audio_channelNumber",1);
            chn->audioinfo.blockAlign=iniGetInt(sect,"audio_blockAlign",0);
            chn->audioinfo.bitsPerSample=iniGetInt(sect,"audio_bitsPerSample",0);
            chn->audioinfo.sampleperframe=iniGetInt(sect,"audio_samplePerframe",0);
            chn->audioinfo.frameInterval=iniGetInt(sect,"audio_frameInterval",0);
        }

    }

    return 0;
}


int OVD_DEMOE_InitClientInfo(){
    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","读取deviceClientInfo\n");


    //先把值都清空
    memset(&OVD_DEMO_Client, 0, sizeof(OVDClientParam));

    iniGetString("deviceConfiguration", "devId", OVD_DEMO_Client.OVDDeviceID, sizeof(OVD_DEMO_Client.OVDDeviceID), NULL);
    iniGetString("deviceConfiguration", "devCmei", OVD_DEMO_Client.OVDDeviceCMEI, sizeof(OVD_DEMO_Client.OVDDeviceCMEI), NULL);
    iniGetString("deviceClientInfo","OVDLoginPassword",OVD_DEMO_Client.OVDLoginPassword,sizeof(OVD_DEMO_Client.OVDLoginPassword), NULL);
	iniGetString("deviceClientInfo","OVDMediaEncPassword",OVD_DEMO_Client.OVDMediaEncPassword,sizeof(OVD_DEMO_Client.OVDMediaEncPassword), NULL);
	if(strlen(OVD_DEMO_Client.OVDLoginPassword) < 6  )
	{
		OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","error:OVDLoginPassword输入不符合规范 \n");
		return -1;
	}
    iniGetString("deviceConfiguration","hardwareModel",OVD_DEMO_Client.OVDHardWareModel,sizeof(OVD_DEMO_Client.OVDHardWareModel),"hardware");
    //软件版本号
    iniGetString("deviceConfiguration","firmware_model",OVD_DEMO_Client.OVDSystemVersion,sizeof(OVD_DEMO_Client.OVDSystemVersion),"V1.0.0");
    iniGetString("deviceConfiguration","modelId",OVD_DEMO_Client.OVDModelId,sizeof(OVD_DEMO_Client.OVDModelId),"");
    iniGetString("deviceConfiguration","macAddr",OVD_DEMO_Client.OVDmacaddress,sizeof(OVD_DEMO_Client.OVDmacaddress),"");
    iniGetString("deviceConfiguration","chipId",OVD_DEMO_Client.chip_id,sizeof(OVD_DEMO_Client.chip_id),"");


    memset(bindId,0,sizeof(bindId));
    iniGetString("deviceClientInfo","bindId",bindId,sizeof(bindId),NULL);



    iniGetString("deviceClientInfo","servicescheduleurl",OVD_DEMO_Client.servicescheduleurl,sizeof(OVD_DEMO_Client.servicescheduleurl),NULL);




    //iniGetString("sourceFile","dnsfile",OVD_DEMO_Client.https_dns_path,sizeof(OVD_DEMO_Client.https_dns_path),NULL);
    iniGetString("deviceConfigInfo","ovd_log_path",OVD_DEMO_Client.ovd_log_path,sizeof(OVD_DEMO_Client.ovd_log_path),"/tmp/");
    iniGetString("deviceConfigInfo","storage_path",OVD_DEMO_Client.local_storage_path,sizeof(OVD_DEMO_Client.local_storage_path), NULL);
    iniGetString("deviceConfigInfo","ovd_data_path", OVD_DEMO_Client.ovd_data_path,sizeof(OVD_DEMO_Client.ovd_data_path), "./");

    return 0;
}

int OVD_DEMOE_showClientInfo()
{

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===[OVD_DEMO_Client]====\n");
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDDeviceID:%s\n",OVD_DEMO_Client.OVDDeviceID);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDLoginPassword:%s\n",OVD_DEMO_Client.OVDLoginPassword);
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDMediaEncPassword:%s\n",OVD_DEMO_Client.OVDMediaEncPassword);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDHardWareModel:%s\n", OVD_DEMO_Client.OVDHardWareModel);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDSystemVersion:%s\n", OVD_DEMO_Client.OVDSystemVersion);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\t OVDModelId:%s\n", OVD_DEMO_Client.OVDModelId);

    return 0;
}




extern int SDKDemoStarted;
int LOGThreadStarted;

pthread_t OVD_DEMO_LOG_OUTPUT_pid;
//pthread_cond_t OVD_DEMO_LOG_OUTPUT_cond;
pthread_mutex_t OVD_DEMO_LOG_OUTPUT_mutex;
sem_t full;
sem_t empty;

char ovdLogOutputBuff[2048];

void *pthread_OVDDemoLogOutput(void *args){
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","pthread_OVDDemoLogOutput started\n");
    prctl(PR_SET_NAME,"demo_logout");
    FILE *fp = NULL;
    fp = fopen(uploadlogFILEPATH, "w");
    if(NULL == fp){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","打开失败OVDSdkDemo.log\n");
        return NULL;
    }

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pthread_OVDDemoLogOutput file open succ, SDKDemoStarted %d\n",SDKDemoStarted);

    while(1 == SDKDemoStarted){
        LOGThreadStarted = 1;
        sem_wait(&full);
        pthread_mutex_lock(&OVD_DEMO_LOG_OUTPUT_mutex);
        //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","get buff: %s\n", ovdLogOutputBuff);
        fprintf(fp, "OVD SDK: %s", ovdLogOutputBuff);
        pthread_mutex_unlock(&OVD_DEMO_LOG_OUTPUT_mutex);
        //sem_post(&empty);
    }

    fclose(fp);
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","pthread_OVDDemoLogOutput quit\n");
    return NULL;
}

void OVD_DEMO_LogOutCallBack(const char* buff){
    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","receive log %s\n",buff);
    //sem_wait(&empty);
    if(0 == LOGThreadStarted){
        return;
    }
    pthread_mutex_lock(&OVD_DEMO_LOG_OUTPUT_mutex);
    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","write to buff\n");
    snprintf(ovdLogOutputBuff, 2048, "%s", buff);
    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","write content %s\n", ovdLogOutputBuff);
    pthread_mutex_unlock(&OVD_DEMO_LOG_OUTPUT_mutex);

    sem_post(&full);
}

static const char *level_str[] = {
    [0] = "Trace",
    [1] = "Debug",
    [2] = "Info",
    [3] = "Warn",
    [4] = "Error",
    [5] = "Fatal",
    [6] = "Unknown"
};

#ifdef CONFIG_LOG_SUPPORT_LOGCALLBACK_V2
void OVD_DEMO_LogOutCallBackV2(OVDLogLevel level, const char *tag, const char *buff)
{
    if (level < 6)
    {
        printf("[%s] [%s] %s", level_str[level], tag, buff);
    }

}
#endif

int OVD_DEMOE_InitLogConfInfo(){
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","读取logConfiguration\n");
    int subStrLenth = 0;

    LOGThreadStarted = 0;


    //先把值都清空
    memset(&OVD_DEMO_LogParam, 0, sizeof(OVDLogParam));
    /*
    typedef struct
    {
        OVDLogLevel logLevel;           //日志输出级别，详细见枚举值LogLevel，可选
        OVDLogSTD   logSTD;             //日志输出位置，可选，详细见枚举值LogSTD，可选
        void (*pOVDLogOutCallBack)(const char* buff);  //设备提供的日志输出回调，SDK的输出日志可以保存到device的存储文件中，可选，若未空为不支持
    }OVDLogParam;
    */

    OVD_DEMO_LogParam.logSTD=iniGetInt("logConfiguration","logSTD",1);
    
    OVD_DEMO_LogParam.max_size=iniGetInt("logConfiguration","max_size",1);

    char value[2];
    memset(value,0,2);
    iniGetString("logCOnfiguration","callback",value,sizeof(value),NULL);
    sem_init(&full, 0, 0);
    if(0 == strcmp(value, "y"))
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","创建线程 设置回调\n");
        pthread_mutex_init(&OVD_DEMO_LOG_OUTPUT_mutex, NULL);
#ifdef __ARCH_RTOS__

        pthread_attr_t attr;
		pthread_attr_init(&attr);
	    pthread_attr_setstacksize(&attr, 128*1024);
		pthread_create(&OVD_DEMO_LOG_OUTPUT_pid, &attr, pthread_OVDDemoLogOutput, NULL);
#else
		pthread_create(&OVD_DEMO_LOG_OUTPUT_pid, NULL, pthread_OVDDemoLogOutput, NULL);
#endif
        OVD_DEMO_LogParam.pOVDLogOutCallBack = OVD_DEMO_LogOutCallBack;
#ifdef CONFIG_LOG_SUPPORT_LOGCALLBACK_V2
        OVD_DEMO_LogParam.pOVDLogOutCallBackV2 = OVD_DEMO_LogOutCallBackV2;
#endif
     }
    


    return 0;
}

int OVD_DEMOE_InitSourceFile(){
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","读取sourcefileConfiguration\n");


	Device_IMAGE_BUFF_SIZE = 512*1024;
	Device_AV_BUFF_DURATION = 10;


    iniGetString("sourceFile", "alarm", alarmFILEPATH, sizeof(alarmFILEPATH),NULL);
    iniGetString("sourceFile", "snapshot", snapshotFILEPATH,sizeof(snapshotFILEPATH),NULL);
    iniGetString("sourceFile", "video", videoFILEPATH,sizeof(videoFILEPATH),NULL);
    iniGetString("sourceFile","h264file",h264filepath,sizeof(h264filepath),"./configure/resources/video.h264");
    iniGetString("sourceFile","h265file",h265filepath,sizeof(h265filepath),"./configure/resources/video.h265");
    iniGetString("sourceFile","AOVfile",AOVfilepath,sizeof(AOVfilepath),"./configure/resources/aov.h265");
    iniGetString("sourceFile","pointcloud_file",PointCloudfilepath,sizeof(PointCloudfilepath),NULL);
    TF_H264_H265_CTRL=iniGetInt("sourceFile","p2ptfflag",1);
    iniGetString("sourceFile","p2ptfh264file",p2ptfh264filepath,sizeof(p2ptfh264filepath),"./configure/resources/video.h264");
    iniGetString("sourceFile","p2ptfh265file",p2ptfh265filepath,sizeof(p2ptfh265filepath),"./configure/resources/video.h265");
    videoFPS=iniGetInt("sourceFile","fps",0);
    iniGetString("sourceFile","audio",audioFILEPATH,sizeof(audioFILEPATH),NULL);
    iniGetString("sourceFile","qr",qrFILEPATH,sizeof(qrFILEPATH),NULL);
    iniGetString("sourceFile","wave",waveFILEPATH,sizeof(waveFILEPATH),NULL);
	iniGetString("sourceFile","audioplay",audioplayFILEPATH,sizeof(audioplayFILEPATH),NULL);
    iniGetString("sourceFile","sdcardpath",g_sdcardpath,sizeof(g_sdcardpath),"./sdcard/");
    Device_IMAGE_BUFF_SIZE=iniGetInt("sourceFile","imageSize",0);
    Device_AV_BUFF_DURATION=iniGetInt("sourceFile","avDuration",30);
    Device_DEBUG_TS=iniGetInt("debug","debug_ts",0);
    Device_CSEG_DURATION=iniGetInt("debug","cseg_duration",0);
    Device_CSEG_MAXCOUNT=iniGetInt("debug","cseg_maxcount",0);
    Device_SNDDROPDELAY=iniGetInt("debug","snddropdelay",0);

    iniGetString("debug","live_filepath",Device_Live_Filepath,sizeof(Device_Live_Filepath),"./");
    iniGetString("debug","live_filepre",Device_Live_Fileprefix,sizeof(Device_Live_Fileprefix),"");
    iniGetString("debug","record_filepath",Device_Record_Filepath,sizeof(Device_Record_Filepath),"./");
    iniGetString("debug","record_filepre",Device_Record_Fileprefix,sizeof(Device_Record_Fileprefix),"");
	iniGetString("sourceFile","logfile",uploadlogFILEPATH,sizeof(uploadlogFILEPATH),NULL);
    if (uploadlogFILEPATH[0] == 0)
    {
        sprintf(uploadlogFILEPATH, "/tmp/OVDSdkDemo-%d.log", getpid());
    }

	return 0;
}


int OVD_DEMO_InitOSD(OVDChannelsInfo *chn)
{
	int osd_num=0, len=16;
	char tmp_on[len];
	char tmp_font[len];
	char tmp_pos_x[len];
	char tmp_pos_y[len];
	char tmp_red[len];
	char tmp_green[len];
	char tmp_blue[len];
	char tmp_text[len];

    chn->osdcount=iniGetInt("deviceOSD","OSD_count",10);
    chn->osd_logo=iniGetInt("deviceOSD","OSD_logo_on",0);
	for (osd_num=0; osd_num<chn->osdcount; osd_num++) {
		sprintf(tmp_on, "%s_%d", "OSD_on", osd_num);
		chn->osdinfo[osd_num].on=iniGetInt("deviceOSD",tmp_on,1);
		sprintf(tmp_text, "%s_%d", "OSD_text", osd_num);
		iniGetString("deviceOSD",tmp_text,chn->osdinfo[osd_num].text,sizeof(chn->osdinfo[osd_num].text),NULL);
		sprintf(tmp_pos_x, "%s_%d", "OSD_pos_x", osd_num);
		chn->osdinfo[osd_num].osdpos.x=iniGetInt("deviceOSD",tmp_pos_x,10);
		sprintf(tmp_pos_y, "%s_%d", "OSD_pos_y", osd_num);
		chn->osdinfo[osd_num].osdpos.y=iniGetInt("deviceOSD",tmp_pos_y,10);
		sprintf(tmp_red, "%s_%d", "red", osd_num);
		chn->osdinfo[osd_num].color.red=iniGetInt("deviceOSD",tmp_red,10);
		sprintf(tmp_green, "%s_%d", "green", osd_num);
		chn->osdinfo[osd_num].color.green=iniGetInt("deviceOSD",tmp_green,10);
		sprintf(tmp_blue, "%s_%d", "blue", osd_num);
		chn->osdinfo[osd_num].color.blue=iniGetInt("deviceOSD",tmp_blue,10);
		sprintf(tmp_font, "%s_%d", "OSD_font", osd_num);
		chn->osdinfo[osd_num].osdfont=iniGetInt("deviceOSD",tmp_font,10);
	}

	return 0;
}
int OVD_DEMO_Init_Feeder()
{
    int feeder_num=0, len=50;
    OVD_DEMO_ConfigureInfo.feeder_immed_quantity=iniGetInt("DeviceFeeder","feeder_immed_quantity",2);
    OVD_DEMO_ConfigureInfo.feedercount=iniGetInt("DeviceFeeder","feeder_count",3);
    char tmp_on[len];
    char tmp_name[len];
    char tmp_time[len];
    char tmp_quantity[len];
    char tmp_repeat[len];
    for (feeder_num=0; feeder_num<(OVD_DEMO_ConfigureInfo.feedercount); feeder_num++) {
        sprintf(tmp_on, "%s_%d", "feeder_plan_on", feeder_num);
        OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].on=iniGetInt("DeviceFeeder",tmp_on,2);
        sprintf(tmp_name, "%s_%d", "feeder_plan_name", feeder_num);
        iniGetString("DeviceFeeder",tmp_name,OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name,sizeof(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name),NULL);
        sprintf(tmp_time, "%s_%d", "feeder_plan_time", feeder_num);
        iniGetString("DeviceFeeder",tmp_time,OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time,sizeof(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time),NULL);
        sprintf(tmp_quantity, "%s_%d", "feeder_plan_quantity", feeder_num);
        OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].quantity=iniGetInt("DeviceFeeder",tmp_quantity,2);
        sprintf(tmp_repeat, "%s_%d", "feeder_plan_repeat", feeder_num);
        OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].repeat=iniGetInt("DeviceFeeder",tmp_repeat,2);
    }
    return 0;
}

int OVD_DEMO_Init_robot()
{
    int robot_num=0, len=50;
    OVD_DEMO_ConfigureInfo.robotcount=iniGetInt("DeviceRobot","robot_count",10);
    //int sss = iniGetInt("DeviceRobot","robot_count",10);
    char tmp_on[len];
    char tmp_name[len];
    char tmp_time[len];
    for (robot_num=0; robot_num<(OVD_DEMO_ConfigureInfo.robotcount); robot_num++) {
        sprintf(tmp_on, "%s_%d", "bizType", robot_num);
        OVD_DEMO_ConfigureInfo.robot_home_config[robot_num].bizType=iniGetInt("DeviceRobot",tmp_on,33);
        sprintf(tmp_name, "%s_%d", "startTime", robot_num);
        iniGetString("DeviceRobot",tmp_name,OVD_DEMO_ConfigureInfo.robot_home_config[robot_num].start_time,sizeof(OVD_DEMO_ConfigureInfo.robot_home_config[robot_num].start_time),NULL);
        sprintf(tmp_time, "%s_%d", "endTime", robot_num);
        iniGetString("DeviceRobot",tmp_time,OVD_DEMO_ConfigureInfo.robot_home_config[robot_num].end_time,sizeof(OVD_DEMO_ConfigureInfo.robot_home_config[robot_num].end_time),NULL);
    }
    return 0;
}

int OVD_DEMO_Init_mobocam()
{
    int robot_num=0, len=50;
    OVD_DEMO_ConfigureInfo.mobocam_config.speedswith=iniGetInt("mobocamDevice","speedSwitch",13);
    OVD_DEMO_ConfigureInfo.mobocam_config.track_on=iniGetInt("mobocamDevice","track_on",1);
    OVD_DEMO_ConfigureInfo.mobocam_config.track_type=iniGetInt("mobocamDevice","track_type",2);
    OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount=iniGetInt("mobocamDevice","mobocamcount",10);
    char tmp_on[len];
    char tmp_name[len];
    char tmp_time[len];
    char tmp_repeat[len];
    for (robot_num=0; robot_num<(OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount); robot_num++) {
        sprintf(tmp_on, "%s_%d", "cruiseId", robot_num);
        OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].cruiseId=iniGetInt("mobocamDevice",tmp_on,0);
        sprintf(tmp_name, "%s_%d", "start_time", robot_num);
        iniGetString("mobocamDevice",tmp_name,OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].start_time,sizeof(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].start_time),NULL);
        sprintf(tmp_time, "%s_%d", "end_time", robot_num);
        iniGetString("mobocamDevice",tmp_time,OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].end_time,sizeof(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].end_time),NULL);
        sprintf(tmp_on, "%s_%d", "time_long", robot_num);
        OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].time_long=iniGetInt("mobocamDevice",tmp_on,0);
        sprintf(tmp_repeat, "%s_%d", "repeat", robot_num);
        OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].repeat=iniGetInt("mobocamDevice",tmp_repeat,0);
    }
    return 0;
}

static int OVD_DEMO_free_areas_info(OVDAreasInfo *pDetectAreasdata)
{
    int i = 0;
	int j = 0;
    if (NULL == pDetectAreasdata)
	{
	    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO",  "freeDetectAreasInfoData input point is NULL!\n");
	    return -1;
	}

	int areasNum = pDetectAreasdata->areasNum;
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","freeDetectAreasInfoData areasNum[%d]\n", areasNum);


    OVDSingleAreaInfo *pArea = pDetectAreasdata->areas;
	if (NULL == pArea)
	{
        free(pDetectAreasdata);
		return -1;
	}

	for (i=0; i<areasNum; i++)
	{
	     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","freeDetectAreasInfoData areaId[%d]\n", pArea[i].areaId);
         OVDAreaPointsInfo *pAreaPoints = pArea[i].areaPoints;

		 int pointsNum = pAreaPoints->pointsNum;

		 OVDSinglePointInfo *point = pAreaPoints->points;
		 if (NULL == point)
	     {
             break;
	     }

	     for (j=0; j<pointsNum; j++)
	     {
             OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","freeDetectAreasInfoData pointId[%d] point_x[%d] point_y[%d]\n", point[j].pointId, point[j].point_x, point[j].point_y);
		 }

         free(point);

		 free(pAreaPoints);
	}

	free(pArea);
	free(pDetectAreasdata);

    return 0;
}

static OVDAreasInfo* OVD_DEMO_get_areas_info(int areaNum, int pointNum)
{

    int i = 0;
    int j = 0;

    OVDAreasInfo *pDetectAreasdata=malloc(sizeof(OVDAreasInfo));
    if (NULL == pDetectAreasdata)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo malloc OVDDetectAreasInfo failed!\n");
        return NULL;

    }
    memset(pDetectAreasdata, 0, sizeof(OVDAreasInfo));

    pDetectAreasdata->areasNum = areaNum;
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo: detect areas num[%d]!\n", pDetectAreasdata->areasNum);


    OVDSingleAreaInfo *pArea = malloc(areaNum*sizeof(OVDSingleAreaInfo));
    if (NULL == pArea)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo malloc OVDAreaInfo failed!\n");
        return NULL;

    }
    memset(pArea, 0, areaNum*sizeof(OVDSingleAreaInfo));

    pDetectAreasdata->areas = pArea;
    for(i = 0; i < areaNum; i++)
    {
        OVDAreaPointsInfo *pAreaPointsData = malloc(sizeof(OVDAreaPointsInfo));
        if (NULL == pAreaPointsData)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo malloc OVDAreaPointsInfo failed!\n");
            return NULL;

        }
        memset(pAreaPointsData, 0, sizeof(OVDAreaPointsInfo));

        pArea[i].areaPoints = pAreaPointsData;

        pAreaPointsData->pointsNum = pointNum;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo: detect areaId[%d], points num[%d]!\n", pArea[i].areaId, pAreaPointsData->pointsNum);

        OVDSinglePointInfo * point = malloc(pointNum * sizeof(OVDSinglePointInfo));
        if (NULL == point)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo malloc OVDPointInfo failed!\n");
            return NULL;

        }
        memset(point, 0, pointNum * sizeof(OVDSinglePointInfo));

        pAreaPointsData->points = point;

        /*
        for (j = 0; j < pointNum; j++)
        {

            point[j].pointId = pointId->valueint;
            point[j].point_x = point_x->valueint;
            point[j].point_y = point_y->valueint;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","getDetectAreasInfo: detect pointId[%d] point_x[%d] point_y[%d]!\n", point[j].pointId, point[j].point_x, point[j].point_y);
        }*/
    }

    return pDetectAreasdata;
}

int OVD_DEMO_InitParabolicAerial(OVDAIParabolicAerialCapInfo *cap, OVDDemoAIParabolicAerialSet *pAIParabolicAerialSet)
{
    int i = 0;
    int j = 0;

    char tmp_floor_id[200];
    char tmp_floor_detail[200];
    char tmp_floor_line_A_x[200];
    char tmp_floor_line_A_y[200];
    char tmp_floor_line_B_x[200];
    char tmp_floor_line_B_y[200];

    char tmp_detect_zone_areaId[200];
    char tmp_detect_zone_areaId_pointId[200];
    char tmp_detect_zone_areaId_point_x[200];
    char tmp_detect_zone_areaId_point_y[200];

    char tmp_shield_zone_areaId[200];
    char tmp_shield_zone_areaId_pointId[200];
    char tmp_shield_zone_areaId_point_x[200];
    char tmp_shield_zone_areaId_point_y[200];

    if (cap == NULL || pAIParabolicAerialSet == NULL)
    {
        return -1;
    }
    OVDAreasInfo * pDetectAreasInfo = NULL;
    OVDAreasInfo * pShieldAreasInfo = NULL;
    OVDAIParabolicAerialFloorConf* pFloorConfInfo = NULL;
    OVDSingleAreaInfo *pSingleAreaInfo = NULL;
    OVDAreaPointsInfo *pAreaPointsInfo = NULL;


    int floorInfoNums = cap->floor_conf_num;
    int detectZoneNum = cap->detect_zone_num;
    int shieldZoneNum = cap->shield_zone_num;
    int detectZone = cap->detect_zone;
    int shieldZone = cap->shield_zone;


    memset(tmp_floor_id, 0, sizeof(tmp_floor_id));
    memset(tmp_floor_detail, 0, sizeof(tmp_floor_detail));
    memset(tmp_floor_line_A_x, 0, sizeof(tmp_floor_line_A_x));
    memset(tmp_floor_line_A_y, 0, sizeof(tmp_floor_line_A_y));
    memset(tmp_floor_line_B_x, 0, sizeof(tmp_floor_line_B_x));
    memset(tmp_floor_line_B_y, 0, sizeof(tmp_floor_line_B_y));

    memset(tmp_detect_zone_areaId, 0, sizeof(tmp_detect_zone_areaId));
    memset(tmp_detect_zone_areaId_pointId, 0, sizeof(tmp_detect_zone_areaId_pointId));
    memset(tmp_detect_zone_areaId_point_x, 0, sizeof(tmp_detect_zone_areaId_point_x));
    memset(tmp_detect_zone_areaId_point_y, 0, sizeof(tmp_detect_zone_areaId_point_y));

    memset(tmp_shield_zone_areaId, 0, sizeof(tmp_shield_zone_areaId));
    memset(tmp_shield_zone_areaId_pointId, 0, sizeof(tmp_shield_zone_areaId_pointId));
    memset(tmp_shield_zone_areaId_point_x, 0, sizeof(tmp_shield_zone_areaId_point_x));
    memset(tmp_shield_zone_areaId_point_y, 0, sizeof(tmp_shield_zone_areaId_point_y));


    //申请空间
    if ((detectZoneNum > 0)&&(detectZone > 0))
    {
        pDetectAreasInfo = OVD_DEMO_get_areas_info(detectZoneNum, 200);
        if (NULL == pDetectAreasInfo)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pDetectAreasInfo mallco failed!\n");
            return -1;
        }
        pAIParabolicAerialSet->pDetectZoneInfo = pDetectAreasInfo;
    }

    if ((shieldZoneNum > 0)&&(shieldZone > 0))
    {
        pShieldAreasInfo = OVD_DEMO_get_areas_info(shieldZoneNum, 200);
        if (NULL == pShieldAreasInfo)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pShieldAreasInfo mallco failed!\n");

            //释放pDetectAreasInfo
            OVD_DEMO_free_areas_info(pDetectAreasInfo);
            return -1;
        }
        pAIParabolicAerialSet->pShieldZoneInfo = pShieldAreasInfo;
    }

    if (floorInfoNums > 0)
    {
        pFloorConfInfo = malloc(sizeof(OVDAIParabolicAerialFloorConf));
        if (NULL == pFloorConfInfo)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","malloc pFloorInfo failed!\n");
            return -1;
        }
        memset(pFloorConfInfo, 0, sizeof(OVDAIParabolicAerialFloorConf));

        pFloorConfInfo->floorNum = floorInfoNums;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","floorInfoNums num[%d]!\n", floorInfoNums);


        OVDSingleFloorInfo *pFloorInfos = malloc(floorInfoNums*sizeof(OVDSingleFloorInfo));
        if (NULL == pFloorInfos)
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","malloc pFloorInfo failed!\n");

            //释放pDetectAreasInfo, pShieldAreasInfo
            OVD_DEMO_free_areas_info(pDetectAreasInfo);
            OVD_DEMO_free_areas_info(pShieldAreasInfo);
            return -1;

        }
        memset(pFloorInfos, 0, floorInfoNums*sizeof(OVDSingleAreaInfo));

        pFloorConfInfo->floorInfos = pFloorInfos;

        pAIParabolicAerialSet->pfloorInfo = pFloorConfInfo;
    }

    pAIParabolicAerialSet->on = iniGetInt("deviceAI","AI_parabolic_aerial_on",0);
    pAIParabolicAerialSet->sensitivity = iniGetInt("deviceAI","AI_parabolic_aerial_sensitivity",60);

    if (NULL != pFloorConfInfo)
    {
        pFloorConfInfo->floorNum = floorInfoNums;

        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","floorNum[%d]\n", pFloorConfInfo->floorNum);
        for (i=0; i<floorInfoNums; i++)
        {
            sprintf(tmp_floor_id, "%s_%d", "AI_parabolic_aerial_floor_id", i);
            pFloorConfInfo->floorInfos[i].floor_id =iniGetInt("deviceAI",tmp_floor_id,1);
            sprintf(tmp_floor_detail, "%s_%d", "AI_parabolic_aerial_floor_detail", i);
            iniGetString("deviceAI",tmp_floor_detail,pFloorConfInfo->floorInfos[i].floor_detail,sizeof(pFloorConfInfo->floorInfos[i].floor_detail),NULL);
            sprintf(tmp_floor_line_A_x, "%s_%d", "AI_parabolic_aerial_floor_line_A_x", i);
            pFloorConfInfo->floorInfos[i].floor_line.point_a_x=iniGetInt("deviceAI",tmp_floor_line_A_x,10);
            sprintf(tmp_floor_line_A_y, "%s_%d", "AI_parabolic_aerial_floor_line_A_y", i);
            pFloorConfInfo->floorInfos[i].floor_line.point_a_y=iniGetInt("deviceAI",tmp_floor_line_A_y,10);
            sprintf(tmp_floor_line_B_x, "%s_%d", "AI_parabolic_aerial_floor_line_B_x", i);
            pFloorConfInfo->floorInfos[i].floor_line.point_b_x=iniGetInt("deviceAI",tmp_floor_line_B_x,10);
            sprintf(tmp_floor_line_B_y, "%s_%d", "AI_parabolic_aerial_floor_line_B_y", i);
            pFloorConfInfo->floorInfos[i].floor_line.point_b_y=iniGetInt("deviceAI",tmp_floor_line_B_y,10);

            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","id[%d], floorId[%d]\n", i, pFloorConfInfo->floorInfos[i].floor_id);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","floor Detail[%s]\n", pFloorConfInfo->floorInfos[i].floor_detail);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","point_a_x[%d]\n", pFloorConfInfo->floorInfos[i].floor_line.point_a_x);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","point_a_y[%d]\n", pFloorConfInfo->floorInfos[i].floor_line.point_a_y);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","point_b_x[%d]\n", pFloorConfInfo->floorInfos[i].floor_line.point_b_x);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","point_b_y[%d]\n", pFloorConfInfo->floorInfos[i].floor_line.point_b_y);



        }
    }


    if (NULL != pShieldAreasInfo)
    {
        pShieldAreasInfo->areasNum = shieldZoneNum;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pShieldAreasInfo->areasNum[%d] pointsNum[%d]\n", pShieldAreasInfo->areasNum, shieldZone);
        for (i=0; i<shieldZoneNum; i++)
        {
            //area
            pSingleAreaInfo = pShieldAreasInfo->areas;

            sprintf(tmp_shield_zone_areaId, "%s_%d", "AI_parabolic_aerial_shield_zone_areaId", i);
            pSingleAreaInfo[i].areaId = iniGetInt("deviceAI",tmp_shield_zone_areaId,1);

            //point
            pAreaPointsInfo = pSingleAreaInfo[i].areaPoints;

            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","areaId[%d]\n", pSingleAreaInfo[i].areaId);

            pAreaPointsInfo->pointsNum=shieldZone;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pointsNum[%d]\n", pAreaPointsInfo->pointsNum);

            for (j=0; j<shieldZone; j++)
            {

                sprintf(tmp_shield_zone_areaId_pointId, "%s_%d_%s_%d", "AI_parabolic_aerial_shield_zone_areaId", i, "pointId",j);
                pAreaPointsInfo->points[j].pointId = iniGetInt("deviceAI",tmp_shield_zone_areaId_pointId,0);

                sprintf(tmp_shield_zone_areaId_point_x, "%s_%d_%s_%d", "AI_parabolic_aerial_shield_zone_areaId", i, "point_x",j);
                pAreaPointsInfo->points[j].point_x = iniGetInt("deviceAI",tmp_shield_zone_areaId_point_x,0);

                sprintf(tmp_shield_zone_areaId_point_y, "%s_%d_%s_%d", "AI_parabolic_aerial_shield_zone_areaId", i, "point_y",j);
                pAreaPointsInfo->points[j].point_y = iniGetInt("deviceAI",tmp_shield_zone_areaId_point_y,0);

                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pointId[%d] point_x[%d], point_y[%d]\n", pAreaPointsInfo->points[j].pointId, pAreaPointsInfo->points[j].point_x, pAreaPointsInfo->points[j].point_y);
            }

        }
    }

    if (NULL != pDetectAreasInfo)
    {
        pDetectAreasInfo->areasNum = detectZoneNum;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pDetectAreasInfo->areasNum[%d] pointsNum[%d]\n", pDetectAreasInfo->areasNum, detectZone);

        for (i=0; i<detectZoneNum; i++)
        {
            pSingleAreaInfo = pDetectAreasInfo->areas;

            sprintf(tmp_detect_zone_areaId, "%s_%d", "AI_parabolic_aerial_detect_zone_areaId", i);
            pSingleAreaInfo[i].areaId = iniGetInt("deviceAI",tmp_detect_zone_areaId,1);

            pAreaPointsInfo = pSingleAreaInfo[i].areaPoints;

            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","areaId[%d]\n", pSingleAreaInfo[i].areaId);

            pAreaPointsInfo->pointsNum=detectZone;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pointsNum[%d]\n", pAreaPointsInfo->pointsNum);

            for (j=0; j<detectZone; j++)
            {

                sprintf(tmp_detect_zone_areaId_pointId, "%s_%d_%s_%d", "AI_parabolic_aerial_detect_zone_areaId", i, "pointId",j);
                pAreaPointsInfo->points[j].pointId = iniGetInt("deviceAI",tmp_detect_zone_areaId_pointId,0);

                sprintf(tmp_detect_zone_areaId_point_x, "%s_%d_%s_%d", "AI_parabolic_aerial_detect_zone_areaId", i, "point_x",j);
                pAreaPointsInfo->points[j].point_x = iniGetInt("deviceAI",tmp_detect_zone_areaId_point_x,0);

                sprintf(tmp_detect_zone_areaId_point_y, "%s_%d_%s_%d", "AI_parabolic_aerial_detect_zone_areaId", i, "point_y",j);
                pAreaPointsInfo->points[j].point_y = iniGetInt("deviceAI",tmp_detect_zone_areaId_point_y,0);

                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pointId[%d] point_x[%d], point_y[%d]\n", pAreaPointsInfo->points[j].pointId, pAreaPointsInfo->points[j].point_x, pAreaPointsInfo->points[j].point_y);
            }
        }
    }

    return 0;
}

int OVD_DEMO_InitRegionalPeopleStat(OVDDemoAIRegionalPeopleStatSet *regionPeopleConfig)
{
    int i = 0;
    if (regionPeopleConfig == NULL)
    {
        return -1;
    }

    regionPeopleConfig->on = iniGetInt("deviceAI","regional_people_stat_detection_on",0);
    regionPeopleConfig->detect_plan.on = iniGetInt("deviceAI","regional_people_stat_plan0_on",0);
    regionPeopleConfig->detect_plan.regional_id = iniGetInt("deviceAI","regional_people_stat_plan0_id",0);
    iniGetString("deviceAI", "regional_people_stat_plan0_name", regionPeopleConfig->detect_plan.regional_name, sizeof(regionPeopleConfig->detect_plan.regional_name), NULL);
    regionPeopleConfig->detect_plan.regional_people_count =iniGetInt("deviceAI","regional_people_stat_plan0_count",30);
    regionPeopleConfig->detect_plan.alarm_report_duration =iniGetInt("deviceAI","regional_people_stat_plan0_alarm_report_duration",15);
    regionPeopleConfig->detect_plan.detect_result_report_duration   =iniGetInt("deviceAI","regional_people_stat_plan0_detect_result_report_duration",15);
    regionPeopleConfig->detect_plan.osd_status     =iniGetInt("deviceAI","regional_people_stat_plan0_osd_status",1);

    regionPeopleConfig->detect_plan.schedule.on = iniGetInt("deviceAI","regional_people_stat_plan0_alerttime_on",0);
    iniGetString("deviceAI","regional_people_stat_plan0_alerttime_start",regionPeopleConfig->detect_plan.schedule.starttime,sizeof(regionPeopleConfig->detect_plan.schedule.starttime),"08:00:00");
    iniGetString("deviceAI","regional_people_stat_plan0_alerttime_end",regionPeopleConfig->detect_plan.schedule.endtime,sizeof(regionPeopleConfig->detect_plan.schedule.endtime),"20:00:00");
    regionPeopleConfig->detect_plan.schedule.repeat   = iniGetInt("deviceAI","regional_people_stat_plan0_alerttime_repeat",127);

    regionPeopleConfig->detect_plan.linkage.speech_on = iniGetInt("deviceAI","regional_people_stat_plan0_speech_on",0);
    regionPeopleConfig->detect_plan.linkage.speech_vol = iniGetInt("deviceAI","regional_people_stat_plan0_speech_vol",60);
    regionPeopleConfig->detect_plan.linkage.speech_repeat = iniGetInt("deviceAI","regional_people_stat_plan0_speech_repeat",3);
    regionPeopleConfig->detect_plan.linkage.speech_repeat_constant=iniGetInt("deviceAI","regional_people_stat_plan0_speech_repeat_constant",0);
    regionPeopleConfig->detect_plan.linkage.speech_interval=iniGetInt("deviceAI","regional_people_stat_plan0_speech_interval",3);
    regionPeopleConfig->detect_plan.linkage.light_dur_constant=iniGetInt("deviceAI","regional_people_stat_plan0_light_dur_constant",0);
    regionPeopleConfig->detect_plan.linkage.light_on = iniGetInt("deviceAI","regional_people_stat_plan0_light_on",0);
    regionPeopleConfig->detect_plan.linkage.light_mode = iniGetInt("deviceAI","regional_people_stat_plan0_light_mode",1);
    regionPeopleConfig->detect_plan.linkage.light_dur = iniGetInt("deviceAI","regional_people_stat_plan0_light_dur",5);
    regionPeopleConfig->detect_plan.linkage.buz_on = iniGetInt("deviceAI","regional_people_stat_plan0_buz_on",0);
    regionPeopleConfig->detect_plan.linkage.buz_dur = iniGetInt("deviceAI","regional_people_stat_plan0_buz_dur",5);

    regionPeopleConfig->detect_plan.zoneInfo.left_up_x=iniGetInt("deviceAI","regional_people_stat_plan0_left_up_x",0);
    regionPeopleConfig->detect_plan.zoneInfo.left_up_y=iniGetInt("deviceAI","regional_people_stat_plan0_left_up_y",10000);
    regionPeopleConfig->detect_plan.zoneInfo.right_bottom_x=iniGetInt("deviceAI","regional_people_stat_plan0_right_bottom_x",10000);
    regionPeopleConfig->detect_plan.zoneInfo.right_bottom_y=iniGetInt("deviceAI","regional_people_stat_plan0_right_bottom_y",0);
    regionPeopleConfig->detect_plan.zoneInfo.left_bottom_x=iniGetInt("deviceAI","regional_people_stat_plan0_left_bottom_x",0);
    regionPeopleConfig->detect_plan.zoneInfo.left_bottom_y=iniGetInt("deviceAI","regional_people_stat_plan0_left_bottom_y",0);
    regionPeopleConfig->detect_plan.zoneInfo.right_up_x=iniGetInt("deviceAI","regional_people_stat_plan0_right_up_x",10000);
    regionPeopleConfig->detect_plan.zoneInfo.right_up_y=iniGetInt("deviceAI","regional_people_stat_plan0_right_up_y",10000);


    return 0;
}

int OVD_DEMO_InitLaneLine(OVDDemoAILaneLine *lanLineConfig)
{
    int i = 0;
    if (lanLineConfig == NULL)
    {
        return -1;
    }

    lanLineConfig->on = iniGetInt("deviceAI","lane_line_detection_on",0);
    lanLineConfig->lane_line_num = iniGetInt("deviceAI","lane_line_num",1);

    lanLineConfig->direction[0] =iniGetInt("deviceAI","lane_line_direction_0",0);

    lanLineConfig->zoneInfo[0].left_up_x=iniGetInt("deviceAI","lane_line_zone_0_left_up_x",0);
    lanLineConfig->zoneInfo[0].left_up_y=iniGetInt("deviceAI","lane_line_zone_0_left_up_y",10000);
    lanLineConfig->zoneInfo[0].left_bottom_x=iniGetInt("deviceAI","lane_line_zone_0_left_bottom_x",0);
    lanLineConfig->zoneInfo[0].left_bottom_y=iniGetInt("deviceAI","lane_line_zone_0_left_bottom_y",0);
    lanLineConfig->zoneInfo[0].right_bottom_x=iniGetInt("deviceAI","lane_line_zone_0_right_bottom_x",10000);
    lanLineConfig->zoneInfo[0].right_bottom_y=iniGetInt("deviceAI","lane_line_zone_0_right_bottom_y",0);
    lanLineConfig->zoneInfo[0].right_up_x=iniGetInt("deviceAI","lane_line_zone_0_right_up_x",10000);
    lanLineConfig->zoneInfo[0].right_up_y=iniGetInt("deviceAI","lane_line_zone_0_right_up_y",10000);

    lanLineConfig->direction[1] =iniGetInt("deviceAI","lane_line_direction_1",0);

    lanLineConfig->zoneInfo[1].left_up_x=iniGetInt("deviceAI","lane_line_zone_1_left_up_x",0);
    lanLineConfig->zoneInfo[1].left_up_y=iniGetInt("deviceAI","lane_line_zone_1_left_up_y",10000);
    lanLineConfig->zoneInfo[1].left_bottom_x=iniGetInt("deviceAI","lane_line_zone_1_left_bottom_x",0);
    lanLineConfig->zoneInfo[1].left_bottom_y=iniGetInt("deviceAI","lane_line_zone_1_left_bottom_y",0);
    lanLineConfig->zoneInfo[1].right_bottom_x=iniGetInt("deviceAI","lane_line_zone_1_right_bottom_x",10000);
    lanLineConfig->zoneInfo[1].right_bottom_y=iniGetInt("deviceAI","lane_line_zone_1_right_bottom_y",0);
    lanLineConfig->zoneInfo[1].right_up_x=iniGetInt("deviceAI","lane_line_zone_1_right_up_x",10000);
    lanLineConfig->zoneInfo[1].right_up_y=iniGetInt("deviceAI","lane_line_zone_1_right_up_y",10000);



    return 0;
}

int OVD_DEMO_InitOffDuty(OVDDemoAIOffDuty *offDutyConfig)
{
    int i = 0;
    if (offDutyConfig == NULL)
    {
        return -1;
    }

    offDutyConfig->on = iniGetInt("deviceAI","off_duty_detection_on",0);
    offDutyConfig->detect_plan.on = iniGetInt("deviceAI","off_duty_plan0_on",0);
    offDutyConfig->detect_plan.regional_id = iniGetInt("deviceAI","off_duty_plan0_id",0);
    iniGetString("deviceAI", "off_duty_plan0_name", offDutyConfig->detect_plan.regional_name, sizeof(offDutyConfig->detect_plan.regional_name), NULL);
    offDutyConfig->detect_plan.on_duty_people_count = iniGetInt("deviceAI","off_duty_plan0_on_duty_count",1);
    offDutyConfig->detect_plan.off_duty_duration = iniGetInt("deviceAI","off_duty_plan0_off_duty_duration",10);

    offDutyConfig->detect_plan.schedule.on = iniGetInt("deviceAI","off_duty_plan0_alerttime_on",1);
    iniGetString("deviceAI","off_duty_plan0_alerttime_start",offDutyConfig->detect_plan.schedule.starttime,sizeof(offDutyConfig->detect_plan.schedule.starttime),"00:00:00");
    iniGetString("deviceAI","off_duty_plan0_alerttime_end",offDutyConfig->detect_plan.schedule.endtime,sizeof(offDutyConfig->detect_plan.schedule.endtime),"23:59:59");
    offDutyConfig->detect_plan.schedule.repeat   = iniGetInt("deviceAI","off_duty_plan0_alerttime_repeat",127);


    offDutyConfig->detect_plan.zoneInfo.left_up_x=iniGetInt("deviceAI","off_duty_plan0_left_up_x",0);
    offDutyConfig->detect_plan.zoneInfo.left_up_y=iniGetInt("deviceAI","off_duty_plan0_left_up_y",10000);
    offDutyConfig->detect_plan.zoneInfo.right_bottom_x=iniGetInt("deviceAI","off_duty_plan0_right_bottom_x",10000);
    offDutyConfig->detect_plan.zoneInfo.right_bottom_y=iniGetInt("deviceAI","off_duty_plan0_right_bottom_y",0);
    offDutyConfig->detect_plan.zoneInfo.left_bottom_x=iniGetInt("deviceAI","off_duty_plan0_left_bottom_x",0);
    offDutyConfig->detect_plan.zoneInfo.left_bottom_y=iniGetInt("deviceAI","off_duty_plan0_left_bottom_y",0);
    offDutyConfig->detect_plan.zoneInfo.right_up_x=iniGetInt("deviceAI","off_duty_plan0_right_up_x",10000);
    offDutyConfig->detect_plan.zoneInfo.right_up_y=iniGetInt("deviceAI","off_duty_plan0_right_up_y",10000);


    return 0;
}

// 烟火检测
int OVD_DEMO_InitSmokefire(OVDAISmokeFireCapInfo *cap, OVDDemoAISmokefire *smokefireConfig){
    int i = 0;
    int j = 0;
    char tmp_detect_zone_areaId[200] = {0};
    char tmp_detect_zone_areaId_pointId[200] = {0};
    char tmp_detect_zone_areaId_point_x[200] = {0};
    char tmp_detect_zone_areaId_point_y[200] = {0};

    if(cap == NULL || smokefireConfig == NULL){
        return -1;
    }
    OVDAreasInfo * pDetectAreasInfo = NULL;
    OVDSingleAreaInfo *pSingleAreaInfo = NULL;
    OVDAreaPointsInfo *pAreaPointsInfo = NULL;

    int detectZone = cap->detect_zone;
    int detectZoneNum = cap->alertarea_num;

    smokefireConfig->on = iniGetInt("deviceAI","smokefire_detection_on",0);

    smokefireConfig->schedule.on = iniGetInt("deviceAI","smokefire_detecttime_on",1);
    iniGetString("deviceAI","smokefire_detecttime_start",smokefireConfig->schedule.starttime,sizeof(smokefireConfig->schedule.starttime),"00:00:00");
    iniGetString("deviceAI","smokefire_detecttime_end",smokefireConfig->schedule.endtime,sizeof(smokefireConfig->schedule.endtime),"23:59:59");
    smokefireConfig->schedule.repeat = iniGetInt("deviceAI","smokefire_detecttime_repeat",127);

    smokefireConfig->linkage.speech_on = iniGetInt("deviceAI","smokefire_speech_on",0);
    smokefireConfig->linkage.speech_vol = iniGetInt("deviceAI","smokefire_speech_vol",60);
    smokefireConfig->linkage.speech_repeat = iniGetInt("deviceAI","smokefire_speech_repeat",3);
    smokefireConfig->linkage.light_on = iniGetInt("deviceAI","smokefire_light_on",0);
    smokefireConfig->linkage.light_mode = iniGetInt("deviceAI","smokefire_light_mode",1);
    smokefireConfig->linkage.light_dur = iniGetInt("deviceAI","smokefire_light_dur",5);
    smokefireConfig->linkage.buz_on = iniGetInt("deviceAI","smokefire_buz_on",0);
    smokefireConfig->linkage.buz_dur = iniGetInt("deviceAI","smokefire_buz_dur",5);

    //申请空间
    if ((detectZoneNum > 0)&&(detectZone > 0))
    {
        pDetectAreasInfo = OVD_DEMO_get_areas_info(detectZoneNum, 200);
        if (NULL == pDetectAreasInfo) {
            return -1;
        }
        smokefireConfig->areas = pDetectAreasInfo;
    }

    if (NULL != pDetectAreasInfo)
    {
        pDetectAreasInfo->areasNum = detectZoneNum;
        for (i=0; i<detectZoneNum; i++)
        {
            pSingleAreaInfo = pDetectAreasInfo->areas;

            sprintf(tmp_detect_zone_areaId, "%s_%d", "AI_smokefire_detect_zone_areaId", i);
            pSingleAreaInfo[i].areaId = iniGetInt("deviceAI",tmp_detect_zone_areaId,1);
            pAreaPointsInfo = pSingleAreaInfo[i].areaPoints;
            pAreaPointsInfo->pointsNum=detectZone;

            for (j=0; j<detectZone; j++)
            {
                sprintf(tmp_detect_zone_areaId_pointId, "%s_%d_%s_%d", "AI_smokefire_detect_zone_areaId", i, "pointId",j);
                pAreaPointsInfo->points[j].pointId = iniGetInt("deviceAI",tmp_detect_zone_areaId_pointId,0);

                sprintf(tmp_detect_zone_areaId_point_x, "%s_%d_%s_%d", "AI_smokefire_detect_zone_areaId", i, "point_x",j);
                pAreaPointsInfo->points[j].point_x = iniGetInt("deviceAI",tmp_detect_zone_areaId_point_x,0);

                sprintf(tmp_detect_zone_areaId_point_y, "%s_%d_%s_%d", "AI_smokefire_detect_zone_areaId", i, "point_y",j);
                pAreaPointsInfo->points[j].point_y = iniGetInt("deviceAI",tmp_detect_zone_areaId_point_y,0);

                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pointId[%d] point_x[%d], point_y[%d]\n", pAreaPointsInfo->points[j].pointId, pAreaPointsInfo->points[j].point_x, pAreaPointsInfo->points[j].point_y);
            }
        }
    }
    smokefireConfig->areas = pDetectAreasInfo;
    return 0;
}

//头盔检测
int OVD_DEMO_InitHelmet(OVDDemoAIHelmet *helmetConfig)
{
    int i = 0;
    if (helmetConfig == NULL)
    {
        return -1;
    }

    helmetConfig->on = iniGetInt("deviceAI","helmet_detection_on",0);

    helmetConfig->schedule.on = iniGetInt("deviceAI","helmet_detecttime_on",1);
    iniGetString("deviceAI","helmet_detecttime_start",helmetConfig->schedule.starttime,sizeof(helmetConfig->schedule.starttime),"00:00:00");
    iniGetString("deviceAI","helmet_detecttime_end",helmetConfig->schedule.endtime,sizeof(helmetConfig->schedule.endtime),"23:59:59");
    helmetConfig->schedule.repeat = iniGetInt("deviceAI","helmet_detecttime_repeat",127);

    helmetConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","helmet_left_up_x",0);
    helmetConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","helmet_left_up_y",10000);
    helmetConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","helmet_right_bottom_x",10000);
    helmetConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","helmet_right_bottom_y",0);
    helmetConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","helmet_left_bottom_x",0);
    helmetConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","helmet_left_bottom_y",0);
    helmetConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","helmet_right_up_x",10000);
    helmetConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","helmet_right_up_y",10000);

    helmetConfig->linkage.speech_on = iniGetInt("deviceAI","helmet_speech_on",0);
    helmetConfig->linkage.speech_vol = iniGetInt("deviceAI","helmet_speech_vol",60);
    helmetConfig->linkage.speech_repeat = iniGetInt("deviceAI","helmet_speech_repeat",3);
    helmetConfig->linkage.speech_repeat_constant=iniGetInt("deviceAI","helmet_speech_repeat_constant",0);
    helmetConfig->linkage.speech_interval=iniGetInt("deviceAI","helmet_speech_interval",3);
    helmetConfig->linkage.light_dur_constant=iniGetInt("deviceAI","helmet_light_dur_constant",0);
    helmetConfig->linkage.light_on = iniGetInt("deviceAI","helmet_light_on",0);
    helmetConfig->linkage.light_mode = iniGetInt("deviceAI","helmet_light_mode",1);
    helmetConfig->linkage.light_dur = iniGetInt("deviceAI","helmet_light_dur",5);
    helmetConfig->linkage.buz_on = iniGetInt("deviceAI","helmet_buz_on",0);
    helmetConfig->linkage.buz_dur = iniGetInt("deviceAI","helmet_buz_dur",5);

}

int OVD_DEMO_InitGesture(OVDDemoAIGesture *gestureConfig)
{
    int i = 0;
    if (gestureConfig == NULL)
    {
        return -1;
    }

    gestureConfig->on = iniGetInt("deviceAI","gesture_recognition_on",1);
    gestureConfig->call_on = iniGetInt("deviceAI","gesture_call_on",1);
    gestureConfig->call_duration = iniGetInt("deviceAI","gesture_call_duration",1);
    gestureConfig->call_schedule.on = iniGetInt("deviceAI","gesture_call_schedule_on",1);
    iniGetString("deviceAI","gesture_call_schedule_start",gestureConfig->call_schedule.starttime,sizeof(gestureConfig->call_schedule.starttime),"00:00:00");
    iniGetString("deviceAI","gesture_call_schedule_end",gestureConfig->call_schedule.endtime,sizeof(gestureConfig->call_schedule.endtime),"23:59:59");
    gestureConfig->call_schedule.repeat = iniGetInt("deviceAI","gesture_call_schedule_repeat",127);
    gestureConfig->help_on = iniGetInt("deviceAI","gesture_help_on",1);

}

//家人识别
int OVD_DEMO_InitFamily(OVDDemoAIFamilyRecognition *familyConfig)
{
    int i = 0;
    if (familyConfig == NULL)
    {
        return -1;
    }

    familyConfig->on = iniGetInt("deviceAI","family_recognition_on",0);

    familyConfig->face_management.facesNum = iniGetInt("deviceAI","family_face_num",2);
    familyConfig->face_management.face_info = (OVDSingleFaceInfo *)malloc(familyConfig->face_management.facesNum * sizeof(OVDSingleFaceInfo));
    if (familyConfig->face_management.face_info == NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Memory allocation failed\n");
        return -1;
    }
    iniGetString("deviceAI","family_face_name_0",familyConfig->face_management.face_info[0].name,sizeof(familyConfig->face_management.face_info[0].name),NULL);
    iniGetString("deviceAI","family_face_url_0",familyConfig->face_management.face_info[0].url,sizeof(familyConfig->face_management.face_info[0].url),NULL);
    iniGetString("deviceAI","family_face_name_1",familyConfig->face_management.face_info[1].name,sizeof(familyConfig->face_management.face_info[1].name),NULL);
    iniGetString("deviceAI","family_face_url_1",familyConfig->face_management.face_info[1].url,sizeof(familyConfig->face_management.face_info[1].url),NULL);

    familyConfig->schedule.on = iniGetInt("deviceAI","family_detecttime_on",1);
    iniGetString("deviceAI","family_detecttime_start",familyConfig->schedule.starttime,sizeof(familyConfig->schedule.starttime),"00:00:00");
    iniGetString("deviceAI","family_detecttime_end",familyConfig->schedule.endtime,sizeof(familyConfig->schedule.endtime),"23:59:59");
    familyConfig->schedule.repeat = iniGetInt("deviceAI","family_detecttime_repeat",127);

    familyConfig->linkage.speech_on = iniGetInt("deviceAI","family_speech_on",0);
    familyConfig->linkage.speech_vol = iniGetInt("deviceAI","family_speech_vol",60);
    familyConfig->linkage.speech_repeat = iniGetInt("deviceAI","family_speech_repeat",3);
    familyConfig->linkage.speech_repeat_constant=iniGetInt("deviceAI","family_speech_repeat_constant",0);
    familyConfig->linkage.speech_interval=iniGetInt("deviceAI","family_speech_interval",3);
    familyConfig->linkage.light_dur_constant=iniGetInt("deviceAI","family_light_dur_constant",0);
    familyConfig->linkage.light_on = iniGetInt("deviceAI","family_light_on",0);
    familyConfig->linkage.light_mode = iniGetInt("deviceAI","family_light_mode",1);
    familyConfig->linkage.light_dur = iniGetInt("deviceAI","family_light_dur",5);
    familyConfig->linkage.buz_on = iniGetInt("deviceAI","family_buz_on",0);
    familyConfig->linkage.buz_dur = iniGetInt("deviceAI","family_buz_dur",5);
}

// 隐私遮蔽
int OVD_DEMO_InitPrivacyMasking(OVDDemoAIPrivacyMasking *privacymaskingConfig)
{
    int i = 0;
    if (privacymaskingConfig == NULL)
    {
        return -1;
    }
    privacymaskingConfig->on = iniGetInt("deviceAI","privacy_masking_on",0);

    privacymaskingConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","privacy_masking_left_up_x",0);
    privacymaskingConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","privacy_masking_left_up_y",10000);
    privacymaskingConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","privacy_masking_right_bottom_x",10000);
    privacymaskingConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","privacy_masking_right_bottom_y",0);
    privacymaskingConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","privacy_masking_left_bottom_x",0);
    privacymaskingConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","privacy_masking_left_bottom_y",0);
    privacymaskingConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","privacy_masking_right_up_x",10000);
    privacymaskingConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","privacy_masking_right_up_y",10000);
}

int OVD_DEMO_InitFallDetection(OVDDemoAIFallDetection * falldetectionConfig)
{
    if (falldetectionConfig == NULL)
    {
        return -1;
    }
    falldetectionConfig->on = iniGetInt("deviceAI","fall_detection_on",0);
    falldetectionConfig->sensitivity=iniGetInt("deviceAI","fall_detection_sensitivity",0);
    falldetectionConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","fall_detection_left_up_x",0);
    falldetectionConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","fall_detection_left_up_y",10000);
    falldetectionConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","fall_detection_right_bottom_x",10000);
    falldetectionConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","fall_detection_right_bottom_y",0);
    falldetectionConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","fall_detection_left_bottom_x",0);
    falldetectionConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","fall_detection_left_bottom_y",0);
    falldetectionConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","fall_detection_right_up_x",10000);
    falldetectionConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","fall_detection_right_up_y",10000);
    falldetectionConfig->schedule.on = iniGetInt("deviceAI","fall_detectiontime_on",1);
    iniGetString("deviceAI","fall_detectiontime_start",falldetectionConfig->schedule.starttime,sizeof(falldetectionConfig->schedule.starttime),"00:00:00");
    iniGetString("deviceAI","fall_detectiontime_end",falldetectionConfig->schedule.endtime,sizeof(falldetectionConfig->schedule.endtime),"23:59:59");
    falldetectionConfig->schedule.repeat = iniGetInt("deviceAI","fall_detectiontime_repeat",127);
    falldetectionConfig->linkage.speech_on=iniGetInt("deviceAI","fall_detection_speech_on",0);
    falldetectionConfig->linkage.speech_vol=iniGetInt("deviceAI","fall_detection_speech_vol",60);
    falldetectionConfig->linkage.speech_repeat=iniGetInt("deviceAI","fall_detection_speech_repeat",3);
    falldetectionConfig->linkage.speech_repeat_constant=iniGetInt("deviceAI","fall_detection_speech_repeat_constant",0);
    falldetectionConfig->linkage.speech_interval=iniGetInt("deviceAI","fall_detection_speech_interval",3);
    falldetectionConfig->linkage.light_dur_constant=iniGetInt("deviceAI","fall_detection_light_dur_constant",0);
    falldetectionConfig->linkage.light_on=iniGetInt("deviceAI","fall_detection_light_on",0);
    falldetectionConfig->linkage.light_mode=iniGetInt("deviceAI","fall_detection_light_mode",1);
    falldetectionConfig->linkage.light_dur=iniGetInt("deviceAI","fall_detection_light_dur",5);
    falldetectionConfig->linkage.buz_on=iniGetInt("deviceAI","fall_detection_buz_on",0);
    falldetectionConfig->linkage.buz_dur=iniGetInt("deviceAI","fall_detection_buz_dur",5);
}

static void OVD_DEMO_InitSleepMnitor(OVDDemoAISleepMonitor * sleepmonitorConfig)
{
    if (sleepmonitorConfig == NULL)
    {
        return ;
    }
    sleepmonitorConfig->on= iniGetInt("deviceAI","sleep_monitor_on",0);
    sleepmonitorConfig->sleep_detection.on = iniGetInt("deviceAI","sleep_detection_on",0);
    sleepmonitorConfig->sleep_detection.sensitivity=iniGetInt("deviceAI","sleep_detection_sensitivity",2);
    sleepmonitorConfig->face_cover.on = iniGetInt("deviceAI","face_cover_on",0);
    sleepmonitorConfig->face_cover.sensitivity=iniGetInt("deviceAI","face_cover_sensitivity",2);
    sleepmonitorConfig->face_cover.mode=iniGetInt("deviceAI","face_cover_mode",0);
    sleepmonitorConfig->wakeup_detection.on = iniGetInt("deviceAI","wakeup_detection_on",0);
    sleepmonitorConfig->wakeup_detection.sensitivity=iniGetInt("deviceAI","wakeup_detection_sensitivity",0);
}    

static void OVD_DEMO_InitExpressionRecognition(OVDDemoAIExpressionRecognition* expressionrecognitionConfig)
{
    if(expressionrecognitionConfig == NULL)
    {
        return;
    }
    expressionrecognitionConfig->on = iniGetInt("deviceAI","expression_recognition_on",0);
}
static void OVD_DEMO_Init_Alarm_Config(ovd_alertarea_params_t *cap_alertarea, ovd_transgression_params_t *cap_transgression, OVDAlarmsSet *alarmConfig)
{
    int i = 0;
    int zone_size = cap_alertarea->alertarea_num;
    int line_another_size = cap_transgression->statistics_line_another_num;

    char tmp_alertareaAlarm_zone_on[200];
    char tmp_alertareaAlarm_left_up_x[200];
    char tmp_alertareaAlarm_left_up_y[200];
    char tmp_alertareaAlarm_right_bottom_x[200];
    char tmp_alertareaAlarm_right_bottom_y[200];
    char tmp_alertareaAlarm_left_bottom_x[200];
    char tmp_alertareaAlarm_left_bottom_y[200];
    char tmp_alertareaAlarm_right_up_x[200];
    char tmp_alertareaAlarm_right_up_y[200];

    char tmp_statistics_line_on[200];
    char tmp_statistics_line_A_x[200];
    char tmp_statistics_line_A_y[200];
    char tmp_statistics_line_B_x[200];
    char tmp_statistics_line_B_y[200];
    char tmp_transgression_statistics_mode_style[200];
    char tmp_transgression_statistics_mode_direction[200];

    memset(tmp_alertareaAlarm_zone_on, 0, sizeof(tmp_alertareaAlarm_zone_on));
    memset(tmp_alertareaAlarm_left_up_x, 0, sizeof(tmp_alertareaAlarm_left_up_x));
    memset(tmp_alertareaAlarm_left_up_y, 0, sizeof(tmp_alertareaAlarm_left_up_y));
    memset(tmp_alertareaAlarm_right_bottom_x, 0, sizeof(tmp_alertareaAlarm_right_bottom_x));
    memset(tmp_alertareaAlarm_right_bottom_y, 0, sizeof(tmp_alertareaAlarm_right_bottom_y));
    memset(tmp_alertareaAlarm_left_bottom_x, 0, sizeof(tmp_alertareaAlarm_left_bottom_x));
    memset(tmp_alertareaAlarm_left_bottom_y, 0, sizeof(tmp_alertareaAlarm_left_bottom_y));
    memset(tmp_alertareaAlarm_right_up_x, 0, sizeof(tmp_alertareaAlarm_right_up_x));
    memset(tmp_alertareaAlarm_right_up_y, 0, sizeof(tmp_alertareaAlarm_right_up_y));

    memset(tmp_statistics_line_on, 0, sizeof(tmp_statistics_line_on));
    memset(tmp_statistics_line_A_x, 0, sizeof(tmp_statistics_line_A_x));
    memset(tmp_statistics_line_A_y, 0, sizeof(tmp_statistics_line_A_y));
    memset(tmp_statistics_line_B_x, 0, sizeof(tmp_statistics_line_B_x));
    memset(tmp_statistics_line_B_y, 0, sizeof(tmp_statistics_line_B_y));
    memset(tmp_transgression_statistics_mode_style, 0, sizeof(tmp_transgression_statistics_mode_style));
    memset(tmp_transgression_statistics_mode_direction, 0, sizeof(tmp_transgression_statistics_mode_direction));

    char *alarm_sect = "deviceAlarms";
    alarmConfig->ioAlarm.isEffect=iniGetInt(alarm_sect,"ioAlarm_isEffect",1);
    alarmConfig->ioAlarm.on=iniGetInt(alarm_sect,"ioAlarm_on",0);
    alarmConfig->ioAlarm.sensitivity=iniGetInt(alarm_sect,"ioAlarm_sensitivity",60);

    alarmConfig->voiceAlarm.isEffect=iniGetInt(alarm_sect,"voiceAlarm_isEffect",1);
    alarmConfig->voiceAlarm.on=iniGetInt(alarm_sect,"voiceAlarm_on",1);
    alarmConfig->voiceAlarm.sensitivity=iniGetInt(alarm_sect,"voiceAlarm_sensitivity",90);

    alarmConfig->pirAlarm.isEffect=iniGetInt(alarm_sect,"pirAlarm_isEffect",1);
    alarmConfig->pirAlarm.on=iniGetInt(alarm_sect,"pirAlarm_on",0);
    alarmConfig->pirAlarm.sensitivity=iniGetInt(alarm_sect,"pirAlarm_sensitivity",60);

    alarmConfig->lossLockAlarm.isEffect=iniGetInt(alarm_sect,"lossLockAlarm_isEffect",1);
    alarmConfig->lossLockAlarm.on=iniGetInt(alarm_sect,"lossLockAlarm_on",0);

    alarmConfig->crossAlarm.isEffect=iniGetInt(alarm_sect,"crossAlarm_isEffect",1);
    alarmConfig->crossAlarm.on=iniGetInt(alarm_sect,"crossAlarm_on",0);
    alarmConfig->crossAlarm.sensitivity=iniGetInt(alarm_sect,"crossAlarm_sensitivity",60);

    alarmConfig->cryAlarm.isEffect=iniGetInt(alarm_sect,"cryAlarm_isEffect",1);
    alarmConfig->cryAlarm.on=iniGetInt(alarm_sect,"cryAlarm_on",0);
    alarmConfig->cryAlarm.sensitivity=iniGetInt(alarm_sect,"cryAlarm_sensitivity",60);
    alarmConfig->cryAlarm.crying_pacify_on=iniGetInt(alarm_sect,"crying_pacify_on",0);
    alarmConfig->cryAlarm.crying_pacify_audio_playing_count=iniGetInt(alarm_sect,"crying_pacify_audio_playing_count",5);
    alarmConfig->cryAlarm.crying_pacify_audio_playing_volumn=iniGetInt(alarm_sect,"crying_pacify_audio_playing_volumn",1);
    iniGetString(alarm_sect,"crying_pacify_audio_playing_url",alarmConfig->cryAlarm.crying_pacify_audio_playing_url,sizeof(alarmConfig->cryAlarm.crying_pacify_audio_playing_url),NULL);

    alarmConfig->bodyAlarm.isEffect=iniGetInt(alarm_sect,"bodyAlarm_isEffect",1);
    alarmConfig->bodyAlarm.on=iniGetInt(alarm_sect,"bodyAlarm_on",1);
    alarmConfig->bodyAlarm.sensitivity=iniGetInt(alarm_sect,"bodyAlarm_sensitivity",60);
    alarmConfig->bodyAlarm.zoneInfo.left_up_x=iniGetInt(alarm_sect,"bodyAlarm_left_up_x",0);
    alarmConfig->bodyAlarm.zoneInfo.left_up_y=iniGetInt(alarm_sect,"bodyAlarm_left_up_y",10000);
    alarmConfig->bodyAlarm.zoneInfo.left_bottom_x=iniGetInt(alarm_sect,"bodyAlarm_left_bottom_x",0);
    alarmConfig->bodyAlarm.zoneInfo.left_bottom_y=iniGetInt(alarm_sect,"bodyAlarm_left_bottom_y",0);
    alarmConfig->bodyAlarm.zoneInfo.right_up_x=iniGetInt(alarm_sect,"bodyAlarm_right_up_x",10000);
    alarmConfig->bodyAlarm.zoneInfo.right_up_y=iniGetInt(alarm_sect,"bodyAlarm_right_up_y",10000);
    alarmConfig->bodyAlarm.zoneInfo.right_bottom_x=iniGetInt(alarm_sect,"bodyAlarm_right_bottom_x",10000);
    alarmConfig->bodyAlarm.zoneInfo.right_bottom_y=iniGetInt(alarm_sect,"bodyAlarm_right_bottom_y",0);

    alarmConfig->faceAlarm.isEffect=iniGetInt(alarm_sect,"faceAlarm_isEffect",1);
    alarmConfig->faceAlarm.on=iniGetInt(alarm_sect,"faceAlarm_on",1);
    alarmConfig->faceAlarm.sensitivity=iniGetInt(alarm_sect,"faceAlarm_sensitivity",60);

    alarmConfig->motionAlarm.isEffect=iniGetInt(alarm_sect,"motionAlarm_isEffect",1);
    alarmConfig->motionAlarm.on=iniGetInt(alarm_sect,"motionAlarm_on",1);
    alarmConfig->motionAlarm.sensitivity=iniGetInt(alarm_sect,"motionAlarm_sensitivity",60);

    alarmConfig->petAlarm.isEffect=iniGetInt(alarm_sect,"petAlarm_isEffect",1);
    alarmConfig->petAlarm.on=iniGetInt(alarm_sect,"petAlarm_on",1);
    alarmConfig->petAlarm.sensitivity=iniGetInt(alarm_sect,"petAlarm_sensitivity",60);
    alarmConfig->petAlarm.zoneInfo.left_up_x=iniGetInt(alarm_sect,"petAlarm_left_up_x",0);
    alarmConfig->petAlarm.zoneInfo.left_up_y=iniGetInt(alarm_sect,"petAlarm_left_up_y",10000);
    alarmConfig->petAlarm.zoneInfo.left_bottom_x=iniGetInt(alarm_sect,"petAlarm_left_bottom_x",0);
    alarmConfig->petAlarm.zoneInfo.left_bottom_y=iniGetInt(alarm_sect,"petAlarm_left_bottom_y",0);
    alarmConfig->petAlarm.zoneInfo.right_up_x=iniGetInt(alarm_sect,"petAlarm_right_up_x",10000);
    alarmConfig->petAlarm.zoneInfo.right_up_y=iniGetInt(alarm_sect,"petAlarm_right_up_y",10000);
    alarmConfig->petAlarm.zoneInfo.right_bottom_x=iniGetInt(alarm_sect,"petAlarm_right_bottom_x",10000);
    alarmConfig->petAlarm.zoneInfo.right_bottom_y=iniGetInt(alarm_sect,"petAlarm_right_bottom_y",0);

    alarmConfig->alertareaAlarm.isEffect=iniGetInt(alarm_sect,"alertareaAlarm_isEffect",1);
    alarmConfig->alertareaAlarm.on=iniGetInt(alarm_sect,"alertareaAlarm_on",0);
    alarmConfig->alertareaAlarm.expel=iniGetInt(alarm_sect,"alertareaAlarm_expel",0);
    alarmConfig->alertareaAlarm.sensitivity=iniGetInt(alarm_sect,"alertareaAlarm_sensitivity",60);
    for(i=0; i<zone_size; i++)
    {
        sprintf(tmp_alertareaAlarm_zone_on, "%s_%d", "alertareaAlarm_zone_on", i);
        iniGetString(alarm_sect,tmp_alertareaAlarm_zone_on,alarmConfig->alertareaAlarm.zoneInfo[i].on,sizeof(alarmConfig->alertareaAlarm.zoneInfo[i].on),NULL);  // 1.41
        sprintf(tmp_alertareaAlarm_left_up_x, "%s_%d", "alertareaAlarm_left_up_x", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].left_up_x=iniGetInt(alarm_sect,tmp_alertareaAlarm_left_up_x,0);
        sprintf(tmp_alertareaAlarm_left_up_y, "%s_%d", "alertareaAlarm_left_up_y", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].left_up_y=iniGetInt(alarm_sect,tmp_alertareaAlarm_left_up_y,10000);
        sprintf(tmp_alertareaAlarm_right_bottom_x, "%s_%d", "alertareaAlarm_right_bottom_x", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].right_bottom_x=iniGetInt(alarm_sect,tmp_alertareaAlarm_right_bottom_x,10000);
        sprintf(tmp_alertareaAlarm_right_bottom_y, "%s_%d", "alertareaAlarm_right_bottom_y", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].right_bottom_y=iniGetInt(alarm_sect,tmp_alertareaAlarm_right_bottom_y,0);
        sprintf(tmp_alertareaAlarm_left_bottom_x, "%s_%d", "alertareaAlarm_left_bottom_x", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].left_bottom_x=iniGetInt(alarm_sect,tmp_alertareaAlarm_left_bottom_x,0);
        sprintf(tmp_alertareaAlarm_left_bottom_y, "%s_%d", "alertareaAlarm_left_bottom_y", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].left_bottom_y=iniGetInt(alarm_sect,tmp_alertareaAlarm_left_bottom_y,0);
        sprintf(tmp_alertareaAlarm_right_up_x, "%s_%d", "alertareaAlarm_right_up_x", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].right_up_x=iniGetInt(alarm_sect,tmp_alertareaAlarm_right_up_x,10000);
        sprintf(tmp_alertareaAlarm_right_up_y, "%s_%d", "alertareaAlarm_right_up_y", i);
        alarmConfig->alertareaAlarm.zoneInfo[i].right_up_y=iniGetInt(alarm_sect,tmp_alertareaAlarm_right_up_y,10000);
    }

    alarmConfig->alertareaAlarm.areaschedule.on=iniGetInt(alarm_sect,"alerttime_on",1);
    iniGetString(alarm_sect,"alerttime_start",alarmConfig->alertareaAlarm.areaschedule.starttime,sizeof(alarmConfig->alertareaAlarm.areaschedule.starttime),"00:00:00");
    iniGetString(alarm_sect,"alerttime_end",alarmConfig->alertareaAlarm.areaschedule.endtime,sizeof(alarmConfig->alertareaAlarm.areaschedule.endtime),"23:59:59");
    alarmConfig->alertareaAlarm.areaschedule.repeat=iniGetInt(alarm_sect,"alerttime_repeat",127);
    alarmConfig->alertareaAlarm.arealinkage.speech_on=iniGetInt(alarm_sect,"alertspeech_on",0);
    alarmConfig->alertareaAlarm.arealinkage.speech_vol=iniGetInt(alarm_sect,"alertspeech_vol",60);
    alarmConfig->alertareaAlarm.arealinkage.speech_repeat=iniGetInt(alarm_sect,"alertspeech_repeat",3);
    alarmConfig->alertareaAlarm.arealinkage.speech_repeat_constant=iniGetInt(alarm_sect,"alertspeech_repeat_constant",0);
    alarmConfig->alertareaAlarm.arealinkage.speech_interval=iniGetInt(alarm_sect,"alertspeech_interval",3);
    alarmConfig->alertareaAlarm.arealinkage.light_dur_constant=iniGetInt(alarm_sect,"alertlight_dur_constant",0);
    alarmConfig->alertareaAlarm.arealinkage.light_on=iniGetInt(alarm_sect,"alertlight_on",0);
    alarmConfig->alertareaAlarm.arealinkage.light_mode=iniGetInt(alarm_sect,"alertlight_mode",1);
    alarmConfig->alertareaAlarm.arealinkage.light_dur=iniGetInt(alarm_sect,"alertlight_dur",5);
    alarmConfig->alertareaAlarm.arealinkage.buz_on=iniGetInt(alarm_sect,"alertbuz_on",0);
    alarmConfig->alertareaAlarm.arealinkage.buz_dur=iniGetInt(alarm_sect,"alertbuz_dur",5);
    alarmConfig->alertareaAlarm.targetType=iniGetInt(alarm_sect,"alert_targetType",0);
    alarmConfig->alertareaAlarm.staymode_time=iniGetInt(alarm_sect,"alert_staymode_time",0);
    //越界侦测
    alarmConfig->transgressionAlarm.isEffect=iniGetInt(alarm_sect,"transgression_isEffect",1);
    alarmConfig->transgressionAlarm.on=iniGetInt(alarm_sect,"transgression_on",0);
    alarmConfig->transgressionAlarm.targetType=iniGetInt(alarm_sect,"transgression_targetType",0);
    alarmConfig->transgressionAlarm.areaschedule.on=iniGetInt(alarm_sect,"transgressiontime_on",1);
    iniGetString(alarm_sect,"transgressiontime_start",alarmConfig->transgressionAlarm.areaschedule.starttime,sizeof(alarmConfig->transgressionAlarm.areaschedule.starttime),"00:00:00");
    iniGetString(alarm_sect,"transgressiontime_end",alarmConfig->transgressionAlarm.areaschedule.endtime,sizeof(alarmConfig->transgressionAlarm.areaschedule.endtime),"23:59:59");
    alarmConfig->transgressionAlarm.areaschedule.repeat=iniGetInt(alarm_sect,"transgressiontime_repeat",127);
    // 第一条规则线
    alarmConfig->transgressionAlarm.LineInfo.on=iniGetInt(alarm_sect,"statistics_line_on",1);
    alarmConfig->transgressionAlarm.LineInfo.statistics_line_A_x=iniGetInt(alarm_sect,"statistics_line_A_x",1);
    alarmConfig->transgressionAlarm.LineInfo.statistics_line_A_y=iniGetInt(alarm_sect,"statistics_line_A_y",1);
    alarmConfig->transgressionAlarm.LineInfo.statistics_line_B_x=iniGetInt(alarm_sect,"statistics_line_B_x",100);
    alarmConfig->transgressionAlarm.LineInfo.statistics_line_B_y=iniGetInt(alarm_sect,"statistics_line_B_y",100);
    alarmConfig->transgressionAlarm.LineInfo.statistics_mode_style=iniGetInt(alarm_sect,"transgression_statistics_mode_style",1);
    alarmConfig->transgressionAlarm.LineInfo.statistics_mode_direction=iniGetInt(alarm_sect,"transgression_statistics_mode_direction",1);
    //额外的规则线
    for(i=0; i<line_another_size; i++)
    {
        sprintf(tmp_statistics_line_on, "%s_%d", "statistics_line_another_on", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].on=iniGetInt(alarm_sect,tmp_statistics_line_on,1);
        sprintf(tmp_statistics_line_A_x, "%s_%d", "statistics_line_another_A_x", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_line_A_x=iniGetInt(alarm_sect,tmp_statistics_line_A_x,1);
        sprintf(tmp_statistics_line_A_y, "%s_%d", "statistics_line_another_A_y", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_line_A_y=iniGetInt(alarm_sect,tmp_statistics_line_A_y,1);
        sprintf(tmp_statistics_line_B_x, "%s_%d", "statistics_line_another_B_x", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_line_B_x=iniGetInt(alarm_sect,tmp_statistics_line_B_x,100);
        sprintf(tmp_statistics_line_B_y, "%s_%d", "statistics_line_another_B_y", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_line_B_y=iniGetInt(alarm_sect,tmp_statistics_line_B_y,100);
        sprintf(tmp_transgression_statistics_mode_style, "%s_%d", "transgression_statistics_another_mode_style", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_mode_style=iniGetInt(alarm_sect,tmp_transgression_statistics_mode_style,1);
        sprintf(tmp_transgression_statistics_mode_direction, "%s_%d", "transgression_statistics_another_mode_direction", i);
        alarmConfig->transgressionAlarm.LineInfo_another[i].statistics_mode_direction=iniGetInt(alarm_sect,tmp_transgression_statistics_mode_direction,1);
    }
    alarmConfig->transgressionAlarm.arealinkage.speech_on=iniGetInt(alarm_sect,"transgression_speech_on",0);
    alarmConfig->transgressionAlarm.arealinkage.speech_vol=iniGetInt(alarm_sect,"transgression_speech_vol",60);
    alarmConfig->transgressionAlarm.arealinkage.speech_repeat=iniGetInt(alarm_sect,"transgression_speech_repeat",3);
    alarmConfig->transgressionAlarm.arealinkage.speech_repeat_constant=iniGetInt(alarm_sect,"transgression_speech_repeat_constant",0);
    alarmConfig->transgressionAlarm.arealinkage.speech_interval=iniGetInt(alarm_sect,"transgression_speech_interval",3);
    alarmConfig->transgressionAlarm.arealinkage.light_dur_constant=iniGetInt(alarm_sect,"transgression_light_dur_constant",0);
    alarmConfig->transgressionAlarm.arealinkage.light_on=iniGetInt(alarm_sect,"transgression_light_on",0);
    alarmConfig->transgressionAlarm.arealinkage.light_mode=iniGetInt(alarm_sect,"transgression_light_mode",1);
    alarmConfig->transgressionAlarm.arealinkage.light_dur=iniGetInt(alarm_sect,"transgression_light_dur",5);
    alarmConfig->transgressionAlarm.arealinkage.buz_on=iniGetInt(alarm_sect,"transgression_buz_on",0);
    alarmConfig->transgressionAlarm.arealinkage.buz_dur=iniGetInt(alarm_sect,"transgression_buz_dur",5);

    //WIFI感知联动
    alarmConfig->wifisensingAlarm.isEffect=iniGetInt(alarm_sect,"wifisensing_isEffect",1);
    alarmConfig->wifisensingAlarm.on=iniGetInt(alarm_sect,"wifisensing_on",0);
    alarmConfig->wifisensingAlarm.linkage_on=iniGetInt(alarm_sect,"wifisensing_linkage_on",1);
    alarmConfig->wifisensingAlarm.sensing_mode=iniGetInt(alarm_sect,"wifisensing_sensing_mode",0);
    alarmConfig->wifisensingAlarm.areaschedule.on=iniGetInt(alarm_sect,"wifisensingtime_on",1);
    iniGetString(alarm_sect,"wifisensingtime_start",alarmConfig->wifisensingAlarm.areaschedule.starttime,sizeof(alarmConfig->wifisensingAlarm.areaschedule.starttime),"00:00:00");
    iniGetString(alarm_sect,"wifisensingtime_end",alarmConfig->wifisensingAlarm.areaschedule.endtime,sizeof(alarmConfig->wifisensingAlarm.areaschedule.endtime),"23:59:59");
    alarmConfig->wifisensingAlarm.areaschedule.repeat=iniGetInt(alarm_sect,"wifisensingtime_repeat",127);
    alarmConfig->wifisensingAlarm.entire_dev_num=iniGetInt(alarm_sect,"wifisensing_entire_dev_num",0);
    char entire_dev_name[64];
    char entire_dev_on[64];
    for(i=0; i<alarmConfig->wifisensingAlarm.entire_dev_num; i++)
    {
        sprintf(entire_dev_name,"wifisensing_entire_dev_name_%d",i);
        iniGetString(alarm_sect,entire_dev_name,alarmConfig->wifisensingAlarm.entire_devmanagement[i].name,sizeof(alarmConfig->wifisensingAlarm.entire_devmanagement[i].name),"");
        sprintf(entire_dev_name,"wifisensing_entire_dev_option60_%d",i);
        iniGetString(alarm_sect,entire_dev_name,alarmConfig->wifisensingAlarm.entire_devmanagement[i].option60,sizeof(alarmConfig->wifisensingAlarm.entire_devmanagement[i].option60),"");
        sprintf(entire_dev_name,"wifisensing_entire_dev_ip_%d",i);
        iniGetString(alarm_sect,entire_dev_name,alarmConfig->wifisensingAlarm.entire_devmanagement[i].ip,sizeof(alarmConfig->wifisensingAlarm.entire_devmanagement[i].ip),"");
        sprintf(entire_dev_name,"wifisensing_entire_dev_mac_%d",i);
        iniGetString(alarm_sect,entire_dev_name,alarmConfig->wifisensingAlarm.entire_devmanagement[i].mac,sizeof(alarmConfig->wifisensingAlarm.entire_devmanagement[i].mac),"");
        sprintf(entire_dev_on,"wifisensing_entire_dev_on_%d",i);
        alarmConfig->wifisensingAlarm.entire_devmanagement[i].on=iniGetInt(alarm_sect, entire_dev_on, 0);
    }
    alarmConfig->wifisensingAlarm.added_dev_num=iniGetInt(alarm_sect,"wifisensing_added_dev_num",0);
    char added_dev_name[64];
    for(i=0; i<alarmConfig->wifisensingAlarm.added_dev_num; i++)
    {
        sprintf(added_dev_name,"wifisensing_added_dev_name_%d",i);
        iniGetString(alarm_sect,added_dev_name,alarmConfig->wifisensingAlarm.added_devmanagement[i].name,sizeof(alarmConfig->wifisensingAlarm.added_devmanagement[i].name),"");
        sprintf(added_dev_name,"wifisensing_added_dev_option60_%d",i);
        iniGetString(alarm_sect,added_dev_name,alarmConfig->wifisensingAlarm.added_devmanagement[i].option60,sizeof(alarmConfig->wifisensingAlarm.added_devmanagement[i].option60),"");
        sprintf(added_dev_name,"wifisensing_added_dev_ip_%d",i);
        iniGetString(alarm_sect,added_dev_name,alarmConfig->wifisensingAlarm.added_devmanagement[i].ip,sizeof(alarmConfig->wifisensingAlarm.added_devmanagement[i].ip),"");
        sprintf(added_dev_name,"wifisensing_added_dev_mac_%d",i);
        iniGetString(alarm_sect,added_dev_name,alarmConfig->wifisensingAlarm.added_devmanagement[i].mac,sizeof(alarmConfig->wifisensingAlarm.added_devmanagement[i].mac),"");
    }
}
static void OVD_DEMO_Init_AI_Passenger_Config(OVDDemoAIPassengerSet *passengerConfig)
{
    if (passengerConfig == NULL)
    {
        return;
    }
    passengerConfig->on = iniGetInt("deviceAI","AI_passenger_on",0);
    passengerConfig->exposure_on = iniGetInt("deviceAI","AI_passenger_exposure_on",1);
    passengerConfig->osd_status=iniGetInt("deviceAI","AI_passenger_osd_status",1);

    passengerConfig->exposure_adjust = iniGetInt("deviceAI","AI_passenger_exposure_adjust",60);
    passengerConfig->quality_value = iniGetInt("deviceAI","AI_passenger_quality_value",60);
    iniGetString("deviceAI", "AI_passenger_mode", passengerConfig->mode, sizeof(passengerConfig->mode), "quality");
    passengerConfig->sensitivity=iniGetInt("deviceAI","AI_passenger_alarm_sensitivity",60);
    passengerConfig->statistics_mode=iniGetInt("deviceAI","passenger_staticstic_mode",0);
    passengerConfig->lineInfo.point_a_x=iniGetInt("deviceAI","passenger_staticstic_line_a_x",2500);
    passengerConfig->lineInfo.point_a_y=iniGetInt("deviceAI","passenger_staticstic_line_a_y",5000);
    passengerConfig->lineInfo.point_b_x=iniGetInt("deviceAI","passenger_staticstic_line_b_x",7500);
    passengerConfig->lineInfo.point_b_y=iniGetInt("deviceAI","passenger_staticstic_line_b_y",5000);
    passengerConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","passenger_Alarm_left_up_x",0);
    passengerConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","passenger_Alarm_left_up_y",10000);
    passengerConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","passenger_Alarm_left_bottom_x",0);
    passengerConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","passenger_Alarm_left_bottom_y",0);
    passengerConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","passenger_larm_right_bottom_x",10000);
    passengerConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","passenger_Alarm_right_bottom_y",0);
    passengerConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","passenger_larm_right_up_x",10000);
    passengerConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","passenger_Alarm_right_up_y",10000);
    passengerConfig->speech.speech_on = iniGetInt("deviceAI","passenger_welcome_speech_on",0);
    passengerConfig->speech.speech_vol = iniGetInt("deviceAI","passenger_welcome_speech_vol",60);
    passengerConfig->speech.speech_repeat = iniGetInt("deviceAI","passenger_welcome_speech_repeat",3);
    iniGetString("deviceAI", "passenger_welcome_speech_url_enter", passengerConfig->speech.speech_url_enter, sizeof(passengerConfig->speech.speech_url_enter), NULL);
    iniGetString("deviceAI", "passenger_welcome_speech_url_leave", passengerConfig->speech.speech_url_leave, sizeof(passengerConfig->speech.speech_url_leave), NULL);
    iniGetString("deviceAI", "passenger_welcome_alerttime_end", passengerConfig->switchschedule.start_time, sizeof(passengerConfig->switchschedule.start_time), "08:00:00");
    iniGetString("deviceAI", "passenger_welcome_alerttime_start", passengerConfig->switchschedule.shutdown_time, sizeof(passengerConfig->switchschedule.shutdown_time), "20:00:00");
    passengerConfig->switchschedule.repeat = iniGetInt("deviceAI","passenger_welcome_alerttime_repeat",127);
}
static void OVD_DEMO_Init_AI_Nonvehicle_Config(OVDDemoAINonmotorvehicleSet *nonvehicleConfig)
{
    if (nonvehicleConfig == NULL)
    {
        return;
    }
    nonvehicleConfig->on = iniGetInt("deviceAI","nonmotorvehicle_detection_on",0);
    iniGetString("deviceAI", "nonmotorvehicle_detect_mode_list", nonvehicleConfig->detect_mode_list, sizeof(nonvehicleConfig->detect_mode_list), "electromobile");
    nonvehicleConfig->sensitivity=iniGetInt("deviceAI","nonmotorvehicle_alarm_sensitivity",60);
    nonvehicleConfig->quality=iniGetInt("deviceAI","nonmotorvehicle_quality",60);
    nonvehicleConfig->capture_mode=iniGetInt("deviceAI","nonmotorvehicle_capture_mode",0);
    nonvehicleConfig->scheduled_capture_time=iniGetInt("deviceAI","nonmotorvehicle_scheduled_capture_time",10);
    nonvehicleConfig->nonvehlinkage.speech_on=iniGetInt("deviceAI","veh_speech_on",0);
    nonvehicleConfig->nonvehlinkage.speech_vol=iniGetInt("deviceAI","veh_speech_vol",60);
    nonvehicleConfig->nonvehlinkage.speech_repeat=iniGetInt("deviceAI","nonveh_speech_repeat",3);
    nonvehicleConfig->nonvehlinkage.speech_repeat_constant=iniGetInt("deviceAI","nonveh_speech_repeat_constant",0);
    nonvehicleConfig->nonvehlinkage.speech_interval=iniGetInt("deviceAI","nonveh_speech_interval",3);
    nonvehicleConfig->nonvehlinkage.light_dur_constant=iniGetInt("deviceAI","nonveh_light_dur_constant",0);
    nonvehicleConfig->nonvehlinkage.light_on=iniGetInt("deviceAI","nonveh_light_on",0);
    nonvehicleConfig->nonvehlinkage.light_mode=iniGetInt("deviceAI","nonveh_light_mode",1);
    nonvehicleConfig->nonvehlinkage.light_dur=iniGetInt("deviceAI","nonveh_light_dur",5);
    nonvehicleConfig->nonvehlinkage.buz_on=iniGetInt("deviceAI","nonveh_buz_on",0);
    nonvehicleConfig->nonvehlinkage.buz_dur=iniGetInt("deviceAI","nonveh_buz_dur",5);
    nonvehicleConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","nonmotorvehicleAlarm_left_up_x",0);
    nonvehicleConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","nonmotorvehicleAlarm_left_up_y",10000);
    nonvehicleConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","nonmotorvehicleAlarm_left_bottom_x",0);
    nonvehicleConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","nonmotorvehicleAlarm_left_bottom_y",0);
    nonvehicleConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","nonmotorvehicleAlarm_right_bottom_x",10000);
    nonvehicleConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","nonmotorvehicleAlarm_right_bottom_y",0);
    nonvehicleConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","nonmotorvehicleAlarm_right_up_x",10000);
    nonvehicleConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","nonmotorvehicleAlarm_right_up_y",10000);

}
static void OVD_DEMO_Init_AI_Vehicle_Config(OVDDemoAIVehicleSet *vehicleConfig)
{
    if (vehicleConfig == NULL)
    {
        return;
    }
    vehicleConfig->on = iniGetInt("deviceAI","vehicle_detection_on",0);
    vehicleConfig->sensitivity=iniGetInt("deviceAI","vehicle_alarm_sensitivity",60);
    vehicleConfig->quality = iniGetInt("deviceAI","vehicle_quality",60);
    vehicleConfig->capture_mode = iniGetInt("deviceAI","vehicle_capture_mode",0);
    vehicleConfig->scheduled_capture_time=iniGetInt("deviceAI","vehicle_scheduled_capture_time",10);
    vehicleConfig->vehlinkage.speech_on=iniGetInt("deviceAI","veh_speech_on",0);
    vehicleConfig->vehlinkage.speech_vol=iniGetInt("deviceAI","veh_speech_vol",60);
    vehicleConfig->vehlinkage.speech_repeat=iniGetInt("deviceAI","veh_speech_repeat",3);
    vehicleConfig->vehlinkage.speech_repeat_constant=iniGetInt("deviceAI","veh_speech_repeat_constant",0);
    vehicleConfig->vehlinkage.speech_interval=iniGetInt("deviceAI","veh_speech_interval",3);
    vehicleConfig->vehlinkage.light_dur_constant=iniGetInt("deviceAI","veh_light_dur_constant",0);
    vehicleConfig->vehlinkage.light_on=iniGetInt("deviceAI","veh_light_on",0);
    vehicleConfig->vehlinkage.light_mode=iniGetInt("deviceAI","veh_light_mode",1);
    vehicleConfig->vehlinkage.light_dur=iniGetInt("deviceAI","veh_light_dur",5);
    vehicleConfig->vehlinkage.buz_on=iniGetInt("deviceAI","veh_buz_on",0);
    vehicleConfig->vehlinkage.buz_dur=iniGetInt("deviceAI","veh_buz_dur",5);
    vehicleConfig->detect_site=iniGetInt("deviceAI","veh_detect_site",2);
    vehicleConfig->exit_mode=iniGetInt("deviceAI","veh_exit_mode",1);
    vehicleConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","vehicleAlarm_left_up_x",0);
    vehicleConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","vehicleAlarm_left_up_y",10000);
    vehicleConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","vehicleAlarm_left_bottom_x",0);
    vehicleConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","vehicleAlarm_left_bottom_y",0);
    vehicleConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","vehicleAlarm_right_bottom_x",10000);
    vehicleConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","vehicleAlarm_right_bottom_y",0);
    vehicleConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","vehicleAlarm_right_up_x",10000);
    vehicleConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","vehicleAlarm_right_up_y",10000);

}
static void OVD_DEMO_Init_AI_Kitchen_Config(OVDDemoAIKitchenSet *kitchenConfig)
{
    if (kitchenConfig == NULL)
    {
        return;
    }
    kitchenConfig->on = iniGetInt("deviceAI","kitchen_detection_on",0);
    kitchenConfig->mask_on = iniGetInt("deviceAI","kitchen_mask_on",0);
    kitchenConfig->cap_on = iniGetInt("deviceAI","kitchen_cap_on",0);
    kitchenConfig->clothes_on = iniGetInt("deviceAI","kitchen_clothes_on",0);
    iniGetString("deviceAI", "kitchen_clothes_color", kitchenConfig->clothes_color, sizeof(kitchenConfig->clothes_color), "white");
    kitchenConfig->sensitivity=iniGetInt("deviceAI","kitchen_alarm_sensitivity",60);
    kitchenConfig->zoneInfo.left_up_x=iniGetInt("deviceAI","kitchenAlarm_left_up_x",0);
    kitchenConfig->zoneInfo.left_up_y=iniGetInt("deviceAI","kitchenAlarm_left_up_y",10000);
    kitchenConfig->zoneInfo.right_bottom_x=iniGetInt("deviceAI","kitchenAlarm_right_bottom_x",10000);
    kitchenConfig->zoneInfo.right_bottom_y=iniGetInt("deviceAI","kitchenAlarm_right_bottom_y",0);
    kitchenConfig->zoneInfo.left_bottom_x=iniGetInt("deviceAI","kitchenAlarm_left_bottom_x",0);
    kitchenConfig->zoneInfo.left_bottom_y=iniGetInt("deviceAI","kitchenAlarm_left_bottom_y",0);
    kitchenConfig->zoneInfo.right_up_x=iniGetInt("deviceAI","kitchenAlarm_right_up_x",10000);
    kitchenConfig->zoneInfo.right_up_y=iniGetInt("deviceAI","kitchenAlarm_right_up_y",10000);

}
static void OVD_DEMO_Init_AI_Face_Config(OVDDemoAIFaceSet *faceConfig)
{
    char *sect="deviceAI";
    if (faceConfig == NULL)
    {
        return;
    }

    faceConfig->on = iniGetInt(sect,"AI_on",0);
    faceConfig->exposure_on = iniGetInt(sect,"AI_exposure_on",0);
    faceConfig->exposure_adjust = iniGetInt(sect,"AI_exposure_adjust",60);
    faceConfig->quality_value = iniGetInt(sect,"AI_quality_value",60);
    iniGetString(sect, "AI_mode", faceConfig->mode, sizeof(faceConfig->mode), "quality");
    faceConfig->sensitivity=iniGetInt(sect,"AI_alarm_sensitivity",60);
    faceConfig->zoneInfo.left_up_x=iniGetInt(sect,"faceAlarm_left_up_x",0);
    faceConfig->zoneInfo.left_up_y=iniGetInt(sect,"faceAlarm_left_up_y",10000);
    faceConfig->zoneInfo.left_bottom_x=iniGetInt(sect,"faceAlarm_left_bottom_x",0);
    faceConfig->zoneInfo.left_bottom_y=iniGetInt(sect,"faceAlarm_left_bottom_y",0);
    faceConfig->zoneInfo.right_bottom_x=iniGetInt(sect,"faceAlarm_right_bottom_x",10000);
    faceConfig->zoneInfo.right_bottom_y=iniGetInt(sect,"faceAlarm_right_bottom_y",0);
    faceConfig->zoneInfo.right_up_x=iniGetInt(sect,"faceAlarm_right_up_x",10000);
    faceConfig->zoneInfo.right_up_y=iniGetInt(sect,"faceAlarm_right_up_y",10000);

    //口罩检测
    faceConfig->maskDeteInfo.maskSchedule.on = iniGetInt(sect,"mask_detection_on",1);
    iniGetString(sect,"maskDete_alerttime_start",faceConfig->maskDeteInfo.maskSchedule.starttime,sizeof(faceConfig->maskDeteInfo.maskSchedule.starttime),"00:00:00");
    iniGetString(sect,"maskDete_alerttime_end",faceConfig->maskDeteInfo.maskSchedule.endtime,sizeof(faceConfig->maskDeteInfo.maskSchedule.endtime),"23:59:59");
    faceConfig->maskDeteInfo.maskSchedule.repeat   = iniGetInt(sect,"maskDete_alerttime_repeat",127);
    faceConfig->maskDeteInfo.masklinkage.speech_on = iniGetInt(sect,"maskDete_speech_on",0);
    faceConfig->maskDeteInfo.masklinkage.speech_vol = iniGetInt(sect,"maskDete_speech_vol",60);
    faceConfig->maskDeteInfo.masklinkage.speech_repeat = iniGetInt(sect,"maskDete_speech_repeat",3);
    faceConfig->maskDeteInfo.masklinkage.speech_repeat_constant=iniGetInt(sect,"maskDete_speech_repeat_constant",0);
    faceConfig->maskDeteInfo.masklinkage.speech_interval=iniGetInt(sect,"maskDete_speech_interval",3);
    faceConfig->maskDeteInfo.masklinkage.light_dur_constant=iniGetInt(sect,"maskDete_light_dur_constant",0);
    faceConfig->maskDeteInfo.masklinkage.light_on = iniGetInt(sect,"maskDete_light_on",0);
    faceConfig->maskDeteInfo.masklinkage.light_mode = iniGetInt(sect,"maskDete_light_mode",1);
    faceConfig->maskDeteInfo.masklinkage.light_dur = iniGetInt(sect,"maskDete_light_dur",5);
    faceConfig->maskDeteInfo.masklinkage.buz_on = iniGetInt(sect,"maskDete_buz_on",0);
    faceConfig->maskDeteInfo.masklinkage.buz_dur = iniGetInt(sect,"maskDete_buz_dur",5);

}

int OVD_DEMOE_InitSetableKeys(const char *path)
{
    OVDChannelsInfo *chn_info = NULL;
    int i=0;
    char *sect="setableKeys";

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","whole file name %s\n", path);
    iniFileLoad(path);
    //EBO
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on=iniGetInt(sect,"EBO_nofall_on",1);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_laser_on=iniGetInt(sect,"EBO_laser_on",0);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_on=iniGetInt(sect,"EBO_track_on",0);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_type=iniGetInt(sect,"EBO_track_type",1);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_collision_on=iniGetInt(sect,"EBO_collision_on",0);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_reminder_on=iniGetInt(sect,"EBO_reminder_on",0);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_speed_adjust=iniGetInt(sect,"EBO_speed_adjust",2);
    OVD_DEMO_ConfigureInfo.eboconfig.EBO_cruise_on=iniGetInt(sect,"EBO_cruise_on",0);
    iniGetString(sect,"EBO_cruise_start_time",OVD_DEMO_ConfigureInfo.eboconfig.start_time,sizeof(OVD_DEMO_ConfigureInfo.eboconfig.start_time),NULL);
    OVD_DEMO_ConfigureInfo.eboconfig.time_long=iniGetInt(sect,"EBO_cruise_time_long",3);
    OVD_DEMO_ConfigureInfo.eboconfig.repeat=iniGetInt(sect,"EBO_cruise_repeat",127);
    //OWL
    OVD_DEMO_ConfigureInfo.robot_config.OWL_food_on=iniGetInt(sect,"OWL_food_on",0);
    OVD_DEMO_ConfigureInfo.robot_config.OWL_track_mode=iniGetInt(sect,"OWL_track_mode",0);
    OVD_DEMO_ConfigureInfo.robot_config.OWL_touch_mode=iniGetInt(sect,"OWL_touch_mode",0);
    OVD_DEMO_ConfigureInfo.robot_config.OWL_sound_on=iniGetInt(sect,"OWL_sound_on",0);
    OVD_DEMO_ConfigureInfo.robot_config.OWL_speed_switch=iniGetInt(sect,"OWL_speed_switch",0);
    iniGetString(sect,"OWL_cruise_start_time",OVD_DEMO_ConfigureInfo.robot_config.start_time,sizeof(OVD_DEMO_ConfigureInfo.robot_config.start_time),NULL);
    OVD_DEMO_ConfigureInfo.robot_config.repeat=iniGetInt(sect,"OWL_cruise_repeat",1);
    //行车记录仪
    OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_findcar_on=iniGetInt(sect,"dashcam_findcar_on",0);
    OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_rescue_on=iniGetInt(sect,"dashcam_rescue_on",0);

    //led
    OVD_DEMO_ConfigureInfo.led=iniGetInt(sect,"led",1);
    OVD_DEMO_ConfigureInfo.light_supplement_lamp_on=iniGetInt(sect,"light_supplement_lamp_on",1);
    //screen
    OVD_DEMO_ConfigureInfo.screen=iniGetInt(sect,"screen",1);
   
    //夜视模式
    OVD_DEMO_ConfigureInfo.normal_nightvision_mode=iniGetInt(sect,"normal_nightvision_mode",0);
    OVD_DEMO_ConfigureInfo.color_nightvision_mode=iniGetInt(sect,"color_nightvision_mode",2);
    OVD_DEMO_ConfigureInfo.nightvision_detect_mode=iniGetInt(sect,"nightvision_detect_mode",0);
    //补光灯
    OVD_DEMO_ConfigureInfo.fill_light.brightness=iniGetInt(sect,"fill_light_brightness",60);
    OVD_DEMO_ConfigureInfo.fill_light.on=iniGetInt(sect,"fill_light_on",0);
    OVD_DEMO_ConfigureInfo.fill_light.schedule.on=iniGetInt(sect,"fill_light_schedule_on",0);
    iniGetString(sect,"fill_light_start_time",OVD_DEMO_ConfigureInfo.fill_light.schedule.starttime,sizeof(OVD_DEMO_ConfigureInfo.fill_light.schedule.starttime),"00:00:00");
    iniGetString(sect,"fill_light_end_time",OVD_DEMO_ConfigureInfo.fill_light.schedule.endtime,sizeof(OVD_DEMO_ConfigureInfo.fill_light.schedule.endtime),"23:59:59");
    OVD_DEMO_ConfigureInfo.fill_light.schedule.repeat=iniGetInt(sect,"fill_light_repeat",127);
    //智能叠加显示
    OVD_DEMO_ConfigureInfo.AI_rules_mode_on=iniGetInt(sect,"AI_rules_mode_on",0);
    //autoreboot
    OVD_DEMO_ConfigureInfo.autorebootinfo.on=iniGetInt(sect,"autoreboot_on",1);
    OVD_DEMO_ConfigureInfo.autorebootinfo.cycle=iniGetDouble(sect, "autoreboot_cycle", 0);
    iniGetString(sect,"autoreboot_start",OVD_DEMO_ConfigureInfo.autorebootinfo.start,sizeof(OVD_DEMO_ConfigureInfo.autorebootinfo.start),NULL);
    iniGetString(sect,"autoreboot_end",OVD_DEMO_ConfigureInfo.autorebootinfo.end,sizeof(OVD_DEMO_ConfigureInfo.autorebootinfo.end),NULL);
    //庭院灯
    OVD_DEMO_ConfigureInfo.yardlightconfig.light_brightness=iniGetInt(sect,"light_brightness",50);
    OVD_DEMO_ConfigureInfo.yardlightconfig.control_mode=iniGetInt(sect,"control_mode",0);
    OVD_DEMO_ConfigureInfo.yardlightconfig.light_on=iniGetInt(sect,"light_on",0);
    iniGetString(sect,"start_time_0",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].start_time,sizeof(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].start_time),NULL);
    iniGetString(sect,"end_time_0",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].end_time,sizeof(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].end_time),NULL);
    iniGetString(sect,"start_time_1",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].start_time,sizeof(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[1].start_time),NULL);
    iniGetString(sect,"end_time_1",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].end_time,sizeof(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[1].end_time),NULL);

    // 枪球联动
    OVD_DEMO_ConfigureInfo.linkage_tracking_smart_on = iniGetInt(sect,"linkage_tracking_smart_on",0);
    OVD_DEMO_ConfigureInfo.linkage_tracking_manual_on = iniGetInt(sect,"linkage_tracking_manual_on",0);

    //feeder
    OVD_DEMO_Init_Feeder();

     //robot
    OVD_DEMO_Init_robot();

    //可移动摄像头
    OVD_DEMO_Init_mobocam();

    for (i = 0; i<OVD_DEMO_ConfigureInfo.channelsInfoCount; i++)
    {
        chn_info = &OVD_DEMO_ConfigureInfo.channelsInfo[i];
        //通道使能
        chn_info->on=iniGetInt(sect,"channel_on",0);
        chn_info->smart_mode_switch_2=iniGetInt(sect,"smart_mode_switch_2",1);
        chn_info->static_scence_ABR_on=iniGetInt(sect,"static_scence_ABR_on",1);
        chn_info->has_speek = iniGetInt(sect,"has_speek", -1);
        //chn_info->on = 0;
        //扬声器音量

        chn_info->audioOutValume=iniGetInt(sect,"audioOutValume",60);
        chn_info->doorbell_volume=iniGetInt(sect,"doorbellOutValume",60);
        chn_info->traceAbility=iniGetInt(sect,"traceAbility",0);
        chn_info->traces.tracetarget=iniGetInt(sect,"tracetarget",0);

        //图像翻转
        chn_info->flipInfo.horflip=iniGetInt(sect,"horflip",0);
        chn_info->flipInfo.verflip=iniGetInt(sect,"verflip",0);
        //音频 视频编码 质量 加密
        chn_info->videoinfo.codec=iniGetInt(sect,"video_codec",source_STREAM_CODEC_H264);
        chn_info->videoinfo.quality=iniGetInt(sect,"video_quality",2);
        chn_info->audioinfo.codec=iniGetInt(sect,"audio_codec",source_STREAM_CODEC_AAC_WITH_ADTS);
        //1.44 帧率、码率、gop设置
        chn_info->bitrate_mode=iniGetInt(sect,"bitrate_mode",0);
        chn_info->bitrate_average=iniGetInt(sect,"bitrate_average",0);
        chn_info->bitrate_peak=iniGetInt(sect,"bitrate_peak",0);
        chn_info->bitrate_statically=iniGetInt(sect,"bitrate_statically",0);
        chn_info->framerate=iniGetInt(sect,"framerate",15);
        //sd card
        OVD_DEMO_SdInfo.SDExist = iniGetInt(sect,"sd_state",0);
        OVD_DEMO_SdInfo.SDTotalSize = iniGetInt(sect,"sd_total_size",0);
        OVD_DEMO_SdInfo.SDFreeSize = iniGetInt(sect,"sd_free_size",0);

        //switch_schedule
        chn_info->switchschedule.on=iniGetInt(sect,"switch_schedule_on",0);
        iniGetString(sect,"switch_start_time",chn_info->switchschedule.start_time,sizeof(chn_info->switchschedule.start_time),"00:00:00");
        iniGetString(sect,"switch_shutdown_time",chn_info->switchschedule.shutdown_time,sizeof(chn_info->switchschedule.shutdown_time),"23:59:59");
        chn_info->switchschedule.repeat=iniGetInt(sect,"switch_repeat",127);

        //获取默认配置文件
        chn_info->dashcam_info.collision_alarm_sensitivity = iniGetInt(sect,"collision_alarm_sensitivity",60);
        chn_info->dashcam_info.alarm_duration.parking_notice = iniGetInt(sect,"parking_notice",10);
        chn_info->dashcam_info.alarm_duration.parking_collision = iniGetInt(sect,"parking_collision",10);
        chn_info->dashcam_info.alarm_duration.driving_collision.before = iniGetInt(sect,"driving_collision_before",10);
        chn_info->dashcam_info.alarm_duration.driving_collision.after = iniGetInt(sect,"driving_collision_after",10);
        chn_info->dashcam_info.alarm_duration.voice_capture.before = iniGetInt(sect,"voice_capture_before",10);
        chn_info->dashcam_info.alarm_duration.voice_capture.after = iniGetInt(sect,"voice_capture_after",10);
        // OSD
        OVD_DEMO_InitOSD(chn_info);

        //AI 人脸
        OVD_DEMO_Init_AI_Face_Config(&chn_info->aiset.AIface);

        //AI 厨房
        OVD_DEMO_Init_AI_Kitchen_Config(&chn_info->aiset.AIkitchen);

        //AI 机动车
        OVD_DEMO_Init_AI_Vehicle_Config(&chn_info->aiset.AIvehicle);

        //AI非机动车（梯控）
        OVD_DEMO_Init_AI_Nonvehicle_Config(&chn_info->aiset.AInonmotorvehicle);

        //客流统计
        OVD_DEMO_Init_AI_Passenger_Config(&chn_info->aiset.AIPassenger);

        //区域检测统计
        OVD_DEMO_InitRegionalPeopleStat(&chn_info->aiset.AIRegionalPeopleStat);

        //车道线
        OVD_DEMO_InitLaneLine(&chn_info->aiset.AILaneLine);

        //off duty
        OVD_DEMO_InitOffDuty(&chn_info->aiset.AIOffDuty);

        //头盔检测
        OVD_DEMO_InitHelmet(&chn_info->aiset.AIHelmet);

        //手势
        OVD_DEMO_InitGesture(&chn_info->aiset.AIGesture);

        //告警
        OVD_DEMO_Init_Alarm_Config(&OVD_DEMO_CapInfoV2.alarm.alertarea_params, &OVD_DEMO_CapInfoV2.alarm.transgression_params, &chn_info->alarms);

        //高空抛物
        OVD_DEMO_InitParabolicAerial(&OVD_DEMO_CapInfoV2.ai_parabolic, &chn_info->aiset.AIParabolicAerial);

        // 烟火检测
        OVD_DEMO_InitSmokefire(&OVD_DEMO_CapInfoV2.ai_smokefire, &chn_info->aiset.AISmokefire);

        //家人识别
        OVD_DEMO_InitFamily(&chn_info->aiset.AIFamilyRecognition);

        //隐私遮蔽
        OVD_DEMO_InitPrivacyMasking(&chn_info->aiset.AIPrivacyMasking);

        //跌倒检测
        OVD_DEMO_InitFallDetection(&chn_info->aiset.AIFallDetection);
        //睡眠检测
        OVD_DEMO_InitSleepMnitor(&chn_info->aiset.sleepAlarm);
        //表情识别
        OVD_DEMO_InitExpressionRecognition(&chn_info->aiset.AIExpressionRecognition);
    }

    iniFileFree();
    return 0;
}


int OVD_DEMOE_DeviceInit(char *path){
    int result = 0;

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","读取DeviceConf.ini\n");
    FILE *fp = NULL;
    char buff[255]={0};
    int subStrLenth = 0;
    char keys[255]={0};

    videoFPS = 0;
    bindId[0] = '\0';

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","whole file name %s\n", path);

    //加载配置文件
    iniFileLoad(path);

    result=OVD_DEMOE_InitDeviceInfo();
    if(0 != result){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->初始化DeviceInfo失败<-----\n");
        return -1;
    }
    OVD_DEMO_InitInterface();

    OVD_DEMOE_showDeviceInfo();

    result=OVD_DEMOE_InitClientInfo();
    if(0 != result){
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","----->初始化ClientInfo失败<-----\n");
        return -1;
    }
    OVD_DEMOE_showClientInfo();

    //先初始化能力集
    // OVD_DEMOE_InitDevCap();
    OVD_DEMOE_InitDevCapV2();

    //再初始化OVD_DEMO_ConfigureInfo
    OVD_DEMOE_InitDeviceConfInfo();

    OVD_DEMOE_InitSourceFile();
    OVD_DEMOE_InitLogConfInfo();

    iniFileFree();
    return 0;

}


void OVD_DEMOE_PrintDeviceInfo(){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","\nprint device info begin\n");
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","devId %s\n", OVD_DEMO_DeviceInfo.OVDDeviceID);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","hardwareModel %s\n", OVD_DEMO_DeviceInfo.OVDHardWareModel);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","firmware_model %s\n", OVD_DEMO_DeviceInfo.OVDSystemVersion);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","network_type %d\n", OVD_DEMO_DeviceInfo.networkType);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","DN_mode %d\n", OVD_DEMO_DeviceInfo.DN_mode);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","wifi_ssid %s\n", OVD_DEMO_DeviceInfo.wifi_ssid);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","wifi_signal %d\n", OVD_DEMO_DeviceInfo.wifi_signal);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","upBandwidth %d\n", OVD_DEMO_DeviceInfo.upBandwidth);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","downBandwidth %d\n", OVD_DEMO_DeviceInfo.downBandwidth);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","ipAddr %s\n", OVD_DEMO_DeviceInfo.ipAddr);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","macAddr %s\n", OVD_DEMO_DeviceInfo.macAddr);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","print device info over\n\n");
}

