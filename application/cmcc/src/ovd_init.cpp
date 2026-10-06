#include "OVD_define.h"
#include "OVD_OpenAPI.h"
#include "cjson/cJSON.h"
#include "ovd_data.h"
// #include "app_flash_otp.h"
#include "sys_version.h"
#include "su_base.h"
#include "ovd.h"
#include "wifi.h"
#include "sys_logger.h"
#include "st_koks_mediamanager.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>

#include <chrono>
#include <thread>

#define Device_DEBUG_TS 0
#define Device_IMAGE_BUFF_SIZE (256*1024) //默认截图图片初始大小为256KB
#define Device_AV_BUFF_DURATION 10 //默认设备上电后，音视频内容最长缓存时间为10s
#define Device_CSEG_DURATION 15 //默认单个云存切片时长为120s
#define Device_CSEG_MAXCOUNT 3 //默认云存模块最多切片为3个
#define Device_SNDDROPDELAY 4000 //默认底层流媒体srt连接缓存最大时延为4s
#define CMD_OVCCONNECTED 0x12
#define CMD_OVDCONFIG 0x13


#define OVD_MAX_JSON_REND_SIZE  (0x1000)
#define OVD_CHNANNEL_MAX_COUNT 1

OVDChannelsInfo OVD_ChannelsInfo[OVD_CHNANNEL_MAX_COUNT];
OVDConfigrationInfo OVD_ConfigureInfo;
static OVDLoginInfo OVD_LoginInfo;
OVDCallbackList g_callbackList;

static void writeConfigureInfo()
{
    int fd = open("/cmcc_fs/configure.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        LOG_ERROR("DEV/OVD open configure.bin failed\n");
        return;
    }

    int nBytes = write(fd, OVD_ChannelsInfo, OVD_CHNANNEL_MAX_COUNT * sizeof(OVDChannelsInfo));
    if(nBytes > 0) {
        nBytes = write(fd, &OVD_ConfigureInfo, sizeof(OVDConfigrationInfo));
    }
    if(nBytes < 0) {
        LOG_ERROR("DEV/OVD write configure.bin failed\n");
    }
}

static int readConfigureInfo()
{
    int fd = open("/cmcc_fs/configure.bin", O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("DEV/OVD open configure.bin failed\n");
        return -1;
    }
    int nbytes = read(fd, OVD_ChannelsInfo, OVD_CHNANNEL_MAX_COUNT * sizeof(OVDChannelsInfo));
    if(nbytes > 0) {
        nbytes = read(fd, &OVD_ConfigureInfo, sizeof(OVDConfigrationInfo));
    }
    if(nbytes > 0 ) {
        return 0;
    }

    return -1;
}

static void InitConfigureInfo() {
    memset(&OVD_ChannelsInfo, 0, OVD_CHNANNEL_MAX_COUNT * sizeof(OVDChannelsInfo));
    memset(&OVD_ConfigureInfo, 0, sizeof(OVDConfigrationInfo));
    if(readConfigureInfo() != 0) {
        OVDChannelsInfo *chn = OVD_ChannelsInfo;
        OVD_ConfigureInfo.channelsInfoCount = OVD_CHNANNEL_MAX_COUNT;
        OVD_ConfigureInfo.tz = 8;
        OVD_ConfigureInfo.autorebootinfo.on = 0;
        OVD_ConfigureInfo.autorebootinfo.cycle = 604800;
        OVD_ConfigureInfo.autorebootinfo.start[0] = '\0';
        OVD_ConfigureInfo.autorebootinfo.end[0] = '\0';
        OVD_ConfigureInfo.autorebootinfo.last_reboot[0] = '\0';
        OVD_ConfigureInfo.normal_nightvision_mode = 0;
        OVD_ConfigureInfo.channelsInfo = OVD_ChannelsInfo;
        OVD_ConfigureInfo.awake_on = true;
        for(int i = 0; i < OVD_ConfigureInfo.channelsInfoCount; i++)
        {
            char sect[256];
            memset(sect, 0, 256);
            snprintf(sect, 256, "channel%d", i);
            chn = &OVD_ConfigureInfo.channelsInfo[i];
            memset(chn, 0, sizeof(OVDChannelsInfo));
            chn->channel = i;
            chn->on = 1;
            chn->videoinfo.codec = source_STREAM_CODEC_H265;
            chn->videoinfo.framerate = 15;
            chn->videoinfo.frameInterval = 30;
            chn->videoinfo.quality = OVD_HDMODE;

            chn->audioinfo.codec = source_STREAM_CODEC_AAC;
            chn->audioinfo.samplesRate = 8000;
            chn->audioinfo.channelNumber = 1;

            chn->alarms.pirAlarm.on = 1;
            chn->alarms.pirAlarm.staymode_time = 15;
            chn->alarms.pirAlarm.sensitivity = 90;
            chn->alarms.lossLockAlarm.on = 1;
            chn->audioOutValume = 100;
        }
    }
    else{
        extern int g_normal_nightvision_mode;
        OVD_ConfigureInfo.channelsInfo = OVD_ChannelsInfo;
        g_normal_nightvision_mode = OVD_ConfigureInfo.normal_nightvision_mode;
    }
}

int OVD_InitCapbility(OVDCapInfoV2_t *capinfo)
{
    int i;
    OVD_cap_params_t  cap_param = {0};
    char buffer[256];

    cap_param.max_channel = 1;
    cap_param.product_function = OVD_PRODUCT_FUNCTION_LOWPOWER;
    cap_param.ai_function = 0; //NO AI function

    memset(capinfo, 0, sizeof(OVDCapInfoV2_t));
    OVD_CapGetV2(DEVICE_TYPE_DOORLOCK, &cap_param, capinfo);

    for (i = 0; i<capinfo->count; i++)
    {
        LOG_INFO("DEV/OVD cap setting, channel %d, type %d, size %d\n", capinfo->cap[i]->channel, capinfo->cap[i]->cap_type, capinfo->cap[i]->size);
        if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_BASE)
        {
            OVD_CapInfoBase *base = (OVD_CapInfoBase *)capinfo->cap[i]->cap_data;
            base->support_auto_reboot = OVD_TRUE;
            base->stream_encryption_mode = OVD_MEDIA_NO_ENCRYPT;
            base->support_ota_without_verify_header = OVD_TRUE;
        }
        else if(capinfo->cap[i]->cap_type == OVD_CAP_TYPE_DOORLOCK)
        {
            OVD_CapInfoDoorLock *doorlock = (OVD_CapInfoDoorLock *)capinfo->cap[i]->cap_data;
            doorlock->battery = OVD_LOCKBATTERYCAP_SINGLE;
            doorlock->tmppwd = OVD_TRUE;
            doorlock->unlockway = OVD_UNLOCKWAYCAP_FP | OVD_UNLOCKWAYCAP_PWD | OVD_UNLOCKWAYCAP_CARD | OVD_UNLOCKWAYCAP_WHITE_SIM | OVD_UNLOCKWAYCAP_ENCRY_SIM | OVD_UNLOCKWAYCAP_TMPPWD | OVD_UNLOCKWAYCAP_FACE;
            doorlock->usermanage = OVD_TRUE;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_VIDEO)
        {
            OVD_CapInfoVideo *video = (OVD_CapInfoVideo *)capinfo->cap[i]->cap_data;
            video->support_set_video_formats = OVD_FALSE;
            video->support_osd_text = OVD_FALSE;
            video->support_osd_logo = OVD_FALSE;
            video->resolution_supportlists = OVD_VIDEO_RESOLUTION_W640_H360 | OVD_VIDEO_RESOLUTION_W960_H540 | OVD_VIDEO_RESOLUTION_W1280_H720;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AUDIO)
        {
            OVD_CapInfoAudio *audio = (OVD_CapInfoAudio *)capinfo->cap[i]->cap_data;
            audio->audiosamplerate = OVD_AUDIO_SAMPLE_RATE_8K;
            audio->support_voiceout_volume = OVD_TRUE;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_SPECIAL)
        {
            OVD_CapInfoSpecial *special = (OVD_CapInfoSpecial *)capinfo->cap[i]->cap_data;
            special->support_4G = OVD_FALSE;
            special->support_hjkh = OVD_TRUE;
            special->hjkh_mode.support_not_disturb_mode = OVD_TRUE;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_CAMERA)
        {
            OVD_CapInfoCamera *camera = (OVD_CapInfoCamera *)capinfo->cap[i]->cap_data;
            camera->support_sd = OVD_FALSE;
            camera->AIGC_cloud_record = OVD_TRUE;
            camera->nightvision_mode = OVD_NIGHTVISION_MODE_NORMAL;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_ALARM)
        {
            OVD_CapInfoAlarm *alarm = (OVD_CapInfoAlarm*)capinfo->cap[i]->cap_data;
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoAlarm));
            alarm->support_alarms_pir = OVD_TRUE;
            alarm->support_alarms_pir_staymode = OVD_TRUE;
            alarm->support_alarms_lossLock = OVD_TRUE;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_LOW_POWER)
        {
            OVD_CapInfoLowPower *lowpower = (OVD_CapInfoLowPower *)capinfo->cap[i]->cap_data;
            lowpower->support_lowpower_mode = OVD_TRUE;
            lowpower->support_constant_power = OVD_FALSE;
            lowpower->support_awaked = OVD_TRUE;
            lowpower->support_awaked_switch = OVD_TRUE;
            lowpower->support_smart_mode = OVD_FALSE;
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_PTZ)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoPtz));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_LINKAGE_TRACKING)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoLinkageTracking));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_ROBOT)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoRobot));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_MOBOCAM)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoMobocam));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_DASH_CAM)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoDashCam));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_YARDLIGHT)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoYardLight));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_LINKAGE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVD_CapInfoLinkage));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FACE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIFaceCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_KITCHEN)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIKitchenCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_VEHICLE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIVehicleCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_NON_MOTOR)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAINonmotorvehicleCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PASSENGER)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIPassengerCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_REGIONAL_PEOPLE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIRegionalPeopleStatCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PARABOLIC_AERIAL)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIParabolicAerialCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_LANE_LINE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAILaneLineCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_OFF_DUTY)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIOffDutyCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_HELMET_WEARING)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIHelmetWearingCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_GESTURE_RECOGNITION)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIGestureRecognitionCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SMOKE_FIRE)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAISmokeFireCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FAMILY_RECOGNITION)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIFamilyRecognitionCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_PRIVACY_MASKING)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIPrivacyMaskingCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_FALL_DETECTION)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIFallDetectionCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SLEEP_MONITOR) //sleep_monitor
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAISleepMonitorCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_EXPRESSION_RECOGNITION)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIExpressionRecognitionCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_CLOUDAI)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDCloudAICapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_SPEECH)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAISpeechCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_LULLBY)
        {
            memset(capinfo->cap[i]->cap_data, 0,sizeof(OVDLullbyCapInfo));
        }
        else if (capinfo->cap[i]->cap_type == OVD_CAP_TYPE_IPC_AI_DIY)
        {
            memset(capinfo->cap[i]->cap_data, 0, sizeof(OVDAIDIYCapInfo));
        }
        else
        {
            LOG_WARNING("DEV/OVD ignore cap, channel %d, type %d\n", capinfo->cap[i]->channel, capinfo->cap[i]->cap_type);
            continue;
        }
    }

    return 0;
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

static void OVD_OnVCCConnectStatus(OVD_int32 status)
{
    const char *status_str[] = {"connected", "reconnecting", "disconnected"};
    if(status < 0 || status > 2){
        LOG_ERROR("DEV/OVD OVD_OnOVCConnectStatus, invalid status[%d]\n", status);
        return;
    }
    if(status == 0) {
        char cmdBuffer[512] = { 0 };
        OVD_UpdateChannelState(0, OVD_CHANNEL_ONLINE);
        OVD_updateOVDstate(OVD_ONLINE);
        sprintf(cmdBuffer, "custom_msg vnet0 30 %d", CMD_OVCCONNECTED);
        system(cmdBuffer);
        #define CMD_SETSLEEPSERVER 0x14
        ovd_hserverinfo_t hserverInfo;
        memset(&hserverInfo, 0, sizeof(hserverInfo));
        if(!GetHServerInfo(&hserverInfo)) {
            snprintf(cmdBuffer, sizeof(cmdBuffer), "custom_msg vnet0 30 %d %s,%d,%d,%s",
                CMD_SETSLEEPSERVER, hserverInfo.hdomain, hserverInfo.hport, hserverInfo.hb_interval, hserverInfo.hb_token);
            system(cmdBuffer);
        }
        #undef CMD_SETSLEEPSERVER
    }
    LOG_INFO("DEV/OVD OVD_OnOVCConnectStatus, status[%d]: %s\n", status, status_str[status]);
    extern int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);
    OVD_schedule_hibernate(0, 0, 20, OVD_HIBERNATE_OVC_NOTIFY);
}

void OVD_SetDeviceLoginInfo(char *deviceID, char *cmei, char *password, char* mac)
{
    int fd = -1;
    memset(&OVD_LoginInfo, 0, sizeof(OVDLoginInfo));
#if 0
    strncpy(OVD_LoginInfo.deviceid, "2009550000020768", sizeof(OVD_LoginInfo.deviceid) - 1);
    strncpy(OVD_LoginInfo.cmei, "111005510020778", sizeof(OVD_LoginInfo.cmei) - 1);
    strncpy(OVD_LoginInfo.password, "15l3XtAz", sizeof(OVD_LoginInfo.password) - 1);
    strncpy(OVD_LoginInfo.mac, "2C:6F:51:09:C9:45", sizeof(OVD_LoginInfo.mac) - 1);
#else
    strncpy(OVD_LoginInfo.deviceid, deviceID, sizeof(OVD_LoginInfo.deviceid) - 1);
    strncpy(OVD_LoginInfo.cmei, cmei, sizeof(OVD_LoginInfo.cmei) - 1);
    strncpy(OVD_LoginInfo.password, password, sizeof(OVD_LoginInfo.password) - 1);
    strncpy(OVD_LoginInfo.mac, mac, sizeof(OVD_LoginInfo.mac) - 1);
#endif
    fd = open("/cmcc_fs/LogInfo.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        LOG_ERROR("DEV/OVD OVD_SetDeviceLoginInfo, open login info file failed\n");
        return;
    }
    if (write(fd, &OVD_LoginInfo, sizeof(OVD_LoginInfo)) != sizeof(OVD_LoginInfo)) {
        LOG_ERROR("DEV/OVD OVD_SetDeviceLoginInfo, write login info failed\n");
    }
    close(fd);
}

static bool OVD_GetDeviceLoginInfo()
{
    int fd = open("/cmcc_fs/LogInfo.bin", O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("DEV/OVD OVD_GetDeviceInfo, open login info file failed\n");
        return false;
    }
    if (read(fd, &OVD_LoginInfo, sizeof(OVD_LoginInfo)) != sizeof(OVD_LoginInfo)) {
        LOG_ERROR("DEV/OVD OVD_GetDeviceInfo, read login info failed\n");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

static OVD_int32 OVD_GetDeviceInfo(OUT OVDDeviceInfo *deviceInfo)
{
    if(deviceInfo == nullptr) {
        LOG_ERROR("DEV/OVD OVD_GetDeviceInfo, invalid param\n");
        return -1;
    }

    memset(deviceInfo, 0, sizeof(OVDDeviceInfo));
    strncpy(deviceInfo->OVDDeviceID, OVD_LoginInfo.deviceid, sizeof(deviceInfo->OVDDeviceID) - 1);
    strncpy(deviceInfo->OVDChipModel, "592282", sizeof(deviceInfo->OVDChipModel) - 1);
    strncpy(deviceInfo->OVDHardWareModel, "S2FMAX", sizeof(deviceInfo->OVDHardWareModel) - 1);
    strncpy(deviceInfo->OVDSystemVersion, SYSTEM_VERSION, sizeof(deviceInfo->OVDSystemVersion) - 1);
    deviceInfo->networkType = 1;
    deviceInfo->DN_mode = QR_CONFIG;
    //TODO: ssid and ip address
}

void OVD_InitClientParam(OVDClientParam *clientParam)
{
    memset(clientParam, 0, sizeof(OVDClientParam));
    strncpy(clientParam->OVDDeviceID, OVD_LoginInfo.deviceid, sizeof(clientParam->OVDDeviceID) - 1);
    strncpy(clientParam->OVDHardWareModel, "S2FMAX", sizeof(clientParam->OVDHardWareModel) - 1);
    strncpy(clientParam->OVDSystemVersion, SYSTEM_VERSION, sizeof(clientParam->OVDSystemVersion) - 1);
    strncpy(clientParam->OVDModelId, "592282", sizeof(clientParam->OVDModelId) - 1);
    strncpy(clientParam->OVDDeviceCMEI, OVD_LoginInfo.cmei, sizeof(clientParam->OVDDeviceCMEI) - 1);
    strncpy(clientParam->OVDLoginPassword, OVD_LoginInfo.password, sizeof(clientParam->OVDLoginPassword) - 1);
    strncpy(clientParam->OVDMediaEncPassword, OVD_LoginInfo.password, sizeof(clientParam->OVDMediaEncPassword) - 1);
    strncpy(clientParam->OVDmacaddress, OVD_LoginInfo.mac, sizeof(clientParam->OVDmacaddress) - 1);
#ifdef CMCC_TEST_SERVER
    strncpy(clientParam->servicescheduleurl, "http://36.138.107.136:10000/ovp/getServerAddr", sizeof(clientParam->servicescheduleurl) - 1);
#else
    strncpy(clientParam->servicescheduleurl, "https://video.komect.com/ovp/getServerAddr", sizeof(clientParam->servicescheduleurl) - 1);
#endif
    strncpy(clientParam->ovd_data_path, "/cmcc_fs", sizeof(clientParam->ovd_data_path) - 1);
    strncpy(clientParam->ovd_log_path, "/cmcc_fs/log", sizeof(clientParam->ovd_log_path) - 1);
    clientParam->max_channel = 1;
}

static int OVD_GetInterfacesCallback(OUT char ifaces[][IFNAMSIZ], IN OVD_int32 max_count)
{
    LOG_INFO("DEV/OVD OVD_GetInterfacesCallback, max_count: %d\n", max_count);
    memset(ifaces, 0, sizeof(char) * IFNAMSIZ * max_count);
    strncpy(ifaces[0], "vnet0", IFNAMSIZ);
    return 1;
}

static int OVD_AlarmNotifyCalback(OVD_AlarmNotifyType_e type, const void* buf, const unsigned int len)
{
    int *value;
    OVD_AlarmNotifyMsg_t *msg;
    switch (type)
    {
    case ALARMNOTIFYTYPE_SETMINDURATION:
        value=(int*)buf;
        LOG_INFO("DEV/OVD ===设置状态告警最短时长：%ds====\n",*value);
        break;
    case ALARMNOTIFTYPE_SETMAXDURATION:
        value=(int*) buf;
        LOG_INFO("DEV/OVD ===设置状态告警最长时长：%ds===\n",*value);
        break;
    case ALARMNOTIFYTYPE_ALARMEXTEND:
        msg=(OVD_AlarmNotifyMsg_t *)buf;
        LOG_INFO("DEV/OVD ===通道:%d 告警类型:%d 因过早结束，而被sdk自动延长===\n",msg->channel,msg->type);
        break;
    case ALARMNOTIFYTYPE_ALARMTRUNCATE:
        msg=(OVD_AlarmNotifyMsg_t *)buf;
        LOG_INFO("DEV/OVD ===通道:%d 告警类型:%d 因告警过长，而被sdk自动截断===\n",msg->channel,msg->type);
        break;
    default:
        break;
    }
    return 0;
}

static int OVD_NotifyLoginFail(int err_code, char* err_msg)
{
    printf("[demo][login fail]err_code:%d,err_msg:%s",err_code,err_msg);
    return 0;
}

static cJSON* GetAlarmSentry(cJSON *channelentry, int i)
{
    cJSON *alarmsentry=NULL;
    cJSON *pirentry=NULL;

    //alarms
    cJSON_AddItemToObject(channelentry,"alarms",alarmsentry=cJSON_CreateObject());
    //pir
    pirentry = cJSON_CreateObject();
    cJSON_AddItemToObject(alarmsentry,"pir", pirentry);
    cJSON_AddBoolToObject(pirentry,"on",OVD_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.on);
    cJSON_AddNumberToObject(pirentry,"staymode_time",OVD_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.staymode_time);
    cJSON_AddNumberToObject(pirentry,"sensitivity",OVD_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.sensitivity);
    //losslock
    pirentry=cJSON_CreateObject();
    cJSON_AddItemToObject(alarmsentry,"losslock", pirentry);
    cJSON_AddBoolToObject(pirentry, "on", OVD_ConfigureInfo.channelsInfo[i].alarms.lossLockAlarm.on);

    cJSON_AddNumberToObject(channelentry,"audio_out_volume",OVD_ConfigureInfo.channelsInfo[i].audioOutValume);

    return NULL;
}

static cJSON *OVD_GetOVDChannelConfigureInfoJson(int channel)
{
    cJSON *video_encodingentry=NULL;
    cJSON *audio_encodingentry=NULL;

    cJSON *channelentry = cJSON_CreateObject();

    int i = channel;

    cJSON_AddBoolToObject(channelentry, "on", OVD_ConfigureInfo.channelsInfo[i].on);
    cJSON_AddNumberToObject(channelentry, "smart_mode_switch_2", OVD_ConfigureInfo.channelsInfo[i].smart_mode_switch_2);

    cJSON_AddItemToObject(channelentry,"video_encoding",video_encodingentry=cJSON_CreateObject());
    if (source_STREAM_CODEC_H265 == OVD_ConfigureInfo.channelsInfo[i].videoinfo.codec)
    {
        cJSON_AddStringToObject(video_encodingentry,"encoder","h265");
    }else
    {
        cJSON_AddStringToObject(video_encodingentry,"encoder","h264");
    }

    if(OVD_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_1DMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","ld");
    else if(OVD_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_SDMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","sd");
    else if(OVD_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_HDMODE )
        cJSON_AddStringToObject(video_encodingentry,"quality","hd");
    else if(OVD_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_FHDMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","fhd");

    cJSON_AddNumberToObject(video_encodingentry,"fps",OVD_ConfigureInfo.channelsInfo[i].videoinfo.framerate);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate",OVD_ConfigureInfo.channelsInfo[i].videoinfo.bitrate);
    cJSON_AddNumberToObject(video_encodingentry,"width",OVD_ConfigureInfo.channelsInfo[i].videoinfo.width);
    cJSON_AddNumberToObject(video_encodingentry,"height",OVD_ConfigureInfo.channelsInfo[i].videoinfo.height);
    cJSON_AddNumberToObject(video_encodingentry,"gop",OVD_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval);

    //1.44 帧率、码率设置
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_mode",OVD_ConfigureInfo.channelsInfo[i].bitrate_mode);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_average",OVD_ConfigureInfo.channelsInfo[i].bitrate_average);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_peak",OVD_ConfigureInfo.channelsInfo[i].bitrate_peak);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_statically",OVD_ConfigureInfo.channelsInfo[i].bitrate_statically);
    cJSON_AddNumberToObject(video_encodingentry,"framerate",OVD_ConfigureInfo.channelsInfo[i].framerate);
    cJSON_AddNumberToObject(video_encodingentry,"gop",OVD_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval);

    //audio_encoding
    cJSON_AddItemToObject(channelentry,"audio_encoding",audio_encodingentry=cJSON_CreateObject());
    cJSON_AddStringToObject(audio_encodingentry,"encoder","aac");
    cJSON_AddNumberToObject(audio_encodingentry,"sample_rate",OVD_ConfigureInfo.channelsInfo[i].audioinfo.samplesRate);
    cJSON_AddNumberToObject(audio_encodingentry,"bitrate",OVD_ConfigureInfo.channelsInfo[i].audioinfo.bitrate);
    cJSON_AddNumberToObject(audio_encodingentry,"bits_per_sample",OVD_ConfigureInfo.channelsInfo[i].audioinfo.bitsPerSample);
    cJSON_AddNumberToObject(audio_encodingentry,"sample_per_frame",OVD_ConfigureInfo.channelsInfo[i].audioinfo.sampleperframe);
    cJSON_AddNumberToObject(audio_encodingentry,"channel",OVD_ConfigureInfo.channelsInfo[i].audioinfo.channelNumber);

    GetAlarmSentry(channelentry,i);
    cJSON_AddNumberToObject(channelentry,"normal_nightvision_mode",OVD_ConfigureInfo.normal_nightvision_mode);

    return channelentry;
}

static int OVD_GetOVDConfigureInfo(char** output_ovdconfig, int *len)
{
    cJSON *root= cJSON_CreateObject();
    cJSON *channelsarray=NULL;
    cJSON *auto_rebootentry=NULL;
    cJSON *timed_schedule_entry=NULL;
	cJSON *support_softprobentry=NULL;
    cJSON *id=NULL;
    char *data=NULL;
    char *json_buffer = NULL;
    int ret;
    int i = 0;

     LOG_INFO("DEV/OVD %s start\n", __FUNCTION__);

    cJSON_AddBoolToObject(root, "awaked_on", OVD_ConfigureInfo.awake_on);
    cJSON_AddItemToObject(root, "channels", channelsarray=cJSON_CreateObject());
    cJSON_AddNumberToObject(root, "dutMTU", 1500);
    auto_rebootentry=cJSON_CreateObject();
    cJSON_AddItemToObject(root, "auto_reboot", auto_rebootentry);
    cJSON_AddBoolToObject(auto_rebootentry, "on", OVD_ConfigureInfo.autorebootinfo.on);
    cJSON_AddNumberToObject(auto_rebootentry, "cycle", OVD_ConfigureInfo.autorebootinfo.cycle);
    cJSON_AddStringToObject(auto_rebootentry, "start", OVD_ConfigureInfo.autorebootinfo.start);
    cJSON_AddStringToObject(auto_rebootentry, "end", OVD_ConfigureInfo.autorebootinfo.end);
    cJSON_AddStringToObject(auto_rebootentry, "last_reboot", OVD_ConfigureInfo.autorebootinfo.last_reboot);

    //ovd_config
    for(i = 0; i < OVD_ConfigureInfo.channelsInfoCount; i++)
    {
        cJSON *channelentry = OVD_GetOVDChannelConfigureInfoJson(OVD_ConfigureInfo.channelsInfo[i].channel);
        if (channelentry != NULL)
        {
            char channel_str[4];
            sprintf(channel_str,"%d", OVD_ConfigureInfo.channelsInfo[i].channel);
            cJSON_AddItemToObject(channelsarray, channel_str, channelentry);
        }

	}

    cJSON_AddNumberToObject(root,"tz",OVD_ConfigureInfo.tz);
    int buf_size = OVD_MAX_JSON_REND_SIZE * OVD_ConfigureInfo.channelsInfoCount;
    json_buffer = (char *)malloc(buf_size);
    ret = cJSON_PrintPreallocated(root, json_buffer, buf_size, 0);
    if (ret != 1)
    {
        LOG_ERROR("DEV/OVD Rend json object failed, maybe buffer is limited, current buffer size %d\n", buf_size);
        free(json_buffer);
        cJSON_Delete(root);
        *output_ovdconfig = NULL;
        return -1;
    }
    LOG_INFO("DEV/OVD output_ovdconfig:%s\n" ,json_buffer);
    cJSON_Delete(root);
    *len = strlen(json_buffer) + sizeof("");
    *output_ovdconfig = json_buffer;


    return 0;
}

int OVD_SetOVDConfigureInfo(char* in_ovdconfig, char* err_msg_info, int str_len)
{
    int ret = 0;
    if(in_ovdconfig == NULL){
        LOG_ERROR("DEV/OVD ovdconfig is NULL %d\n", ret);
        return -1;
    }

    LOG_INFO("DEV/OVD in_ovdconfig:%s\n", in_ovdconfig);
    char *buf = NULL;

    cJSON *root = cJSON_Parse(in_ovdconfig);
    if (!root) {
        LOG_ERROR("DEV/OVD JSON parse fail %d\n", ret);
        return -1;
    }

    cJSON *awaked = cJSON_GetObjectItem(root, "awaked_on");
    if(cJSON_IsBool(awaked)){
        OVD_ConfigureInfo.awake_on = awaked->valueint;

        char result_str[64] = {0};
        if (cJSON_IsTrue(awaked)) {
            snprintf(result_str, sizeof(result_str), "{\"awaked_on\":true}");
        } else if (cJSON_IsFalse(awaked)) {
            snprintf(result_str, sizeof(result_str), "{\"awaked_on\":false}");
        } else {
            LOG_ERROR("DEV/OVD awaked: no defined value\n");
        }

        char *escaped = new char[strlen(result_str) + 3];
        sprintf(escaped, "'%s'", result_str);
        vnet_send_msg(CMD_OVDCONFIG, "%s", escaped);
        delete[] escaped;
    }

    cJSON *channels = cJSON_GetObjectItem(root, "channels");
    if (!channels || !cJSON_IsObject(channels)) return -1;

    cJSON *channel0 = cJSON_GetObjectItem(channels, "0");
    if (!channel0) return -1;


    cJSON *alarms = cJSON_GetObjectItem(channel0, "alarms");
    if (alarms) {

        cJSON *pri_item = cJSON_GetObjectItem(alarms, "pir");
        if(pri_item)
        {
            cJSON *on_item = cJSON_GetObjectItem(pri_item, "on");
            if (cJSON_IsBool(on_item)) {
                OVD_ConfigureInfo.channelsInfo[0].alarms.pirAlarm.on = on_item->valueint;
            }

            cJSON *sensitivity_item = cJSON_GetObjectItem(pri_item, "sensitivity");
            if (cJSON_IsNumber(sensitivity_item)) {
                OVD_ConfigureInfo.channelsInfo[0].alarms.pirAlarm.sensitivity = sensitivity_item->valueint;
            }

            cJSON *staymode_time_item = cJSON_GetObjectItem(pri_item, "staymode_time");
            if (cJSON_IsNumber(staymode_time_item)) {
                OVD_ConfigureInfo.channelsInfo[0].alarms.pirAlarm.staymode_time = staymode_time_item->valueint;
            }
        }

        cJSON *losslock_item = cJSON_GetObjectItem(alarms, "losslock");
        if(losslock_item)
        {
            cJSON *on = cJSON_GetObjectItem(losslock_item, "on");
            if (cJSON_IsBool(on)) {
                OVD_ConfigureInfo.channelsInfo[0].alarms.lossLockAlarm.on = on->valueint;
            }
        }

        char *alarm_string = cJSON_PrintUnformatted(alarms);
        if (alarm_string == NULL) {
            LOG_ERROR("DEV/OVD ovdconfig is NULL %d\n", ret);
        }

        char *escaped = new char[strlen(alarm_string) + 3];
        sprintf(escaped, "'%s'", alarm_string);
        vnet_send_msg(CMD_OVDCONFIG, "%s", escaped);
        delete[] escaped;
    }

    cJSON *audio_out_volume = cJSON_GetObjectItem(channel0, "audio_out_volume");
    if(cJSON_IsNumber(audio_out_volume)){
        int audioOutVolume = audio_out_volume->valueint;
        if(audioOutVolume != OVD_ConfigureInfo.channelsInfo[0].audioOutValume)
        {
            OVD_ConfigureInfo.channelsInfo[0].audioOutValume = audioOutVolume;
            auto& media_manager = st::sense_koks::MediaManager::GetInstance();
            auto dev_audio_output = media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);
            dev_audio_output->SetVol(OVD_ConfigureInfo.channelsInfo[0].audioOutValume);
        }
    }

    cJSON *video_encoding = cJSON_GetObjectItem(channel0, "video_encoding");
    if(video_encoding)
    {
        cJSON *quality = cJSON_GetObjectItem(video_encoding, "quality");
        if (quality && cJSON_IsString(quality)) {
            char *quality_str = quality->valuestring;
            if(strcmp(quality_str, "sd") == 0)
            {
                OVD_ConfigureInfo.channelsInfo[0].videoinfo.quality = OVD_SDMODE;
                g_callbackList.updateVideoQuality((int)OVD_ConfigureInfo.channelsInfo[0].videoinfo.quality);
            }
            else if(strcmp(quality_str, "hd") == 0)
            {
                OVD_ConfigureInfo.channelsInfo[0].videoinfo.quality = OVD_HDMODE;
                g_callbackList.updateVideoQuality((int)OVD_ConfigureInfo.channelsInfo[0].videoinfo.quality);
            }
            else {
                LOG_ERROR("DEV/OVD quality_str is invalid %s\n", quality_str);
            }
        }
    }

    cJSON *nightvision = cJSON_GetObjectItem(channel0, "normal_nightvision_mode");
    if(nightvision && cJSON_IsNumber(nightvision))
    {
        extern int g_normal_nightvision_mode;
        OVD_ConfigureInfo.normal_nightvision_mode = nightvision->valueint;
        g_normal_nightvision_mode = OVD_ConfigureInfo.normal_nightvision_mode;
    }

    writeConfigureInfo();
    cJSON_Delete(root);
    return 0;
}

static int OVD_SetRebootInfo(OvdRebootParam *rebootinfo)
{
    LOG_INFO("DEV/OVD rebootinfo ts:%lu, reason:%d, message:%s\n", (unsigned long)rebootinfo->ts, rebootinfo->reason, rebootinfo->message);
    int fd = open("/cmcc_fs/rebootinfo.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
    {
        LOG_ERROR("DEV/OVD open rebootinfo.txt failed\n");
        return -1;
    }
    char buffer[512] = {0};
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "ts", rebootinfo->ts);
    cJSON_AddNumberToObject(json, "reason", rebootinfo->reason);
    cJSON_AddStringToObject(json, "message", rebootinfo->message);
    char *json_buffer = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    int ret = write(fd, json_buffer, strlen(json_buffer));
    free(json_buffer);
    close(fd);
    if (ret < 0)
    {
        LOG_ERROR("DEV/OVD write rebootinfo.txt failed\n");
        return -1;
    }

    return 0;
}

static int OVD_GetRebootInfoInternal(OvdRebootParam *rebootinfo)
{
    int fd = open("/cmcc_fs/rebootinfo.txt", O_RDONLY, 0666);
    if(fd < 0)
    {
        return -1;
    }

    char buffer[512] = {0};
    int ret = read(fd, buffer, sizeof(buffer));
    close(fd);
    if (ret < 0)
    {
        return -1;
    }
    LOG_INFO("DEV/OVD read rebootinfo.txt content:%s\n", buffer);
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        return -1;
    }
    cJSON *ts = cJSON_GetObjectItem(json, "ts");
    cJSON *reason = cJSON_GetObjectItem(json, "reason");
    cJSON *message = cJSON_GetObjectItem(json, "message");
    if (ts == NULL || reason == NULL || message == NULL)
    {
        cJSON_Delete(json);
        return -1;
    }

    rebootinfo->ts = (OVD_uint64)ts->valuedouble;
    rebootinfo->reason = (ovd_reboot_reason_e)reason->valueint;
    strncpy(rebootinfo->message, message->valuestring, sizeof(rebootinfo->message) - 1);
    cJSON_Delete(json);

    return 0;
}

static int OVD_GetRebootInfo(OvdRebootParam *rebootinfo)
{
    if(OVD_GetRebootInfoInternal(rebootinfo) < 0)
    {
        rebootinfo->ts = 0;
        rebootinfo->reason = OVD_REBOOT_REASON_DUT_COMMON;
        snprintf(rebootinfo->message, sizeof(rebootinfo->message), "Device rebooted by unknown reason");
    }
    return 0;
}

static int OVD_GetOVDChannelConfigureInfo(int channel, char** output_ovdconfig, int *len)
{
    int ret;
    cJSON *channelentry = OVD_GetOVDChannelConfigureInfoJson(channel);
    if (channelentry == NULL)
    {
        return -1;
    }

    int buf_size = OVD_MAX_JSON_REND_SIZE;
    char *json_buffer = (char *)malloc(buf_size);
    ret = cJSON_PrintPreallocated(channelentry, json_buffer, buf_size, 0);
    cJSON_Delete(channelentry);
    if (ret == 1)
    {
        *output_ovdconfig = json_buffer;
        *len = strlen(json_buffer) + sizeof("");
        return 0;
    }
    else
    {
        free(json_buffer);
        return -1;
    }
}

static void* OVD_DMEAPI_RecordOpen(int channel)
{
    LOG_INFO("DEV/OVD OVD_DEMO_DMEAPI_callback_RecordOpen, channel: %d\n",channel);

    return nullptr;
}

static int OVD_DMEAPI_RecordClose(void *ctx)
{
    LOG_INFO("DEV/OVD OVD_DEMO_DMEAPI_callback_RecordClose: %p\n", ctx);
    return 0;
}

static int OVD_DMEAPI_RecordSeek(void *ctx, int64_t seekTime)
{
    LOG_INFO("DEV/OVD OVD_DEMO_DMEAPI_callback_RecordSeek: %p, seekTime: %lld\n", ctx, seekTime);
    return 0;
}

static int OVD_Gettimestamp(OVD_uint64 *timestamp)
{
    *timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return 0;
}

static int OVD_Settime(const OVD_uint64 input_time, const OVD_int32 tolerance_value)
{
    return 0;
}

static int OVD_GetSimpleovdinfo(SimpleOVDinfo *out_simpleovdinfo) {
    out_simpleovdinfo->state = OVD_ONLINE;
    out_simpleovdinfo->channelcount = 1;
    out_simpleovdinfo->channelarray[0].channelnum = 0;
    out_simpleovdinfo->channelarray[0].state = OVD_CHANNEL_ONLINE;
    return 0;
}

static int OVD_ReBootDevice(ovd_reboot_reason_e reason)
{
    LOG_INFO("DEV/OVD Reboot device, reason: %d\n", reason);
    OvdRebootParam rebootinfo;
    rebootinfo.reason = reason;
    rebootinfo.ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    snprintf(rebootinfo.message, sizeof(rebootinfo.message), "Reboot because reason code %d", reason);
    OVD_SetRebootInfo(&rebootinfo);
    // system("reboot");
    return 0;
}

static int OVD_ResetConfiguration()
{
    return 0;
}

static int OVD_ForceIFrame(int channel, OVDCodeStream code_stream){
    LOG_INFO("DEV/OVD Force I frame, channel: %d, code_stream: %d\n", channel, code_stream);
    if(g_callbackList.requestIDR != nullptr)
    {
        return g_callbackList.requestIDR();
    }
    return -1;
}

static int OVD_Snapshot(IN OVD_int32 channel,OUT OVDImageInfo *imageInfo, OUT OVD_int32 maxImageSize)
{
    LOG_INFO("DEV/OVD Snapshot, channel: %d\n", channel);
    uint8_t* buffer = nullptr;

    int data_size = GetSnapshot(&buffer, 360, 640, callbackList.get_image_data_func);
    if(data_size > maxImageSize){
        LOG_ERROR("DEV/OVD Snapshot size too large\n");
        free(buffer);
        return -1;
    }

    memcpy(imageInfo->buf, buffer, data_size);
    imageInfo->size = data_size;
    free(buffer);

    return 0;
}

static int OVD_RunningStatusCallback(IN ovd_probe_exception_type_e status, IN OVD_int32 channel_id, IN void* data, IN void* user_data)
{
    LOG_INFO("DEV/OVD RunningStatusCallback, status: %d, channel_id: %d\n", status, channel_id);
    return 0;
}

static int OVD_GetDevRunningInfo(IN ovd_probe_devrunning_info_e info_e, OUT void *data)
{
    LOG_INFO("DEV/OVD GetDevRunningInfo, info_e: %d\n", info_e);
    return 0;
}

static int OVD_KeepAwakenUtilExpired(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason)
{
    extern int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);
    LOG_INFO("DEV/OVD KeepAwakenUtilExpired, channel: %d, notAllowHibernate: %d, expired: %d, reason: %d\n", channel, notAllowHibernate, expired, reason);

    extern void set_host_keepalive();
    set_host_keepalive();   
    return OVD_schedule_hibernate(channel, notAllowHibernate, 10, reason);
}

static int OVD_WifiSensingStatusQuery(IN char* wifi_status, IN char* linkage_status, IN char* desc)
{
    LOG_INFO("DEV/OVD WifiSensingStatusQuery, wifi_status: %s, linkage_status: %s, desc: %s\n", wifi_status, linkage_status, desc);
    return 0;
}

void OVD_InitCallbackList(OVD_CallBackFunList *callBackFunList)
{
    memset(callBackFunList, 0, sizeof(OVD_CallBackFunList));
    callBackFunList->OVD_OVCConnectStatus = OVD_OnVCCConnectStatus;
    callBackFunList->OVD_GetOVDDeviceInfo = OVD_GetDeviceInfo;
    callBackFunList->OVD_GetOVDConfigureInfo = OVD_GetOVDConfigureInfo;
    callBackFunList->OVC_SetOVDConfigureInfo = OVD_SetOVDConfigureInfo;
    callBackFunList->OVD_GetOVDChannelConfigureInfo = OVD_GetOVDChannelConfigureInfo;
    callBackFunList->OVD_GetInterfacesCallback = OVD_GetInterfacesCallback;
    callBackFunList->OVD_AlarmNotifyCallback_t = OVD_AlarmNotifyCalback;
    callBackFunList->OVD_NotifyLoginFail = OVD_NotifyLoginFail;
    callBackFunList->OVD_DMEAPI_callback_RecordOpen = OVD_DMEAPI_RecordOpen;
    callBackFunList->OVD_DMEAPI_callback_RecordClose = OVD_DMEAPI_RecordClose;
    callBackFunList->OVD_DMEAPI_callback_RecordSeek = OVD_DMEAPI_RecordSeek;
    callBackFunList->OVD_gettime = OVD_Gettimestamp;
    callBackFunList->OVD_settime = OVD_Settime;
    callBackFunList->OVD_getsimpleovdinfo = OVD_GetSimpleovdinfo;
    callBackFunList->OVD_ReBootDevice = OVD_ReBootDevice;
    callBackFunList->OVD_ResetConfiguration = OVD_ResetConfiguration;
    callBackFunList->OVD_ForceIFrame = OVD_ForceIFrame;
    callBackFunList->OVD_Snapshot = OVD_Snapshot;
    callBackFunList->ovd_running_status_cb = OVD_RunningStatusCallback;
    callBackFunList->OVD_GetDevRunningInfo = OVD_GetDevRunningInfo;
    callBackFunList->OVD_KeepAwakenUtilExpired = OVD_KeepAwakenUtilExpired;
    callBackFunList->OVD_WifiSensingStatusQuery_cb = OVD_WifiSensingStatusQuery;
}

static void free_cmcc_fs_space()
{
    const char *path = "/cmcc_fs";
    struct statvfs fs_info;

    if (statvfs(path, &fs_info) == -1) {
        perror("statvfs");
        return;
    }

    unsigned long long free_space = fs_info.f_bsize * fs_info.f_bavail;
    if(free_space < 128 * 1024) {
        LOG_INFO("DEV/OVD free space in /cmcc_fs is less than 128KB, clearing log files\n");
        system("rm -rf /cmcc_fs/log/*");
    }
}

bool ovd_sdk_init()
{
    OVDCapInfoV2_t ovd_capInfo;
    OVDClientParam ovd_clientParam;
    OVDLogParam OVD_LogParam;
    OvdRebootParam rebootinfo;
    OVD_CallBackFunList callBackFunList;
    char jsonParam[200];

    free_cmcc_fs_space();
    OVD_GetDeviceLoginInfo();
    InitConfigureInfo();
    memset(&g_callbackList, 0, sizeof(g_callbackList));

    OVD_LogParam.logSTD = OVD_LOGSTD_NO;
    OVD_LogParam.max_size = 1;
    OVD_LogParam.pOVDLogOutCallBack = nullptr;
    memset(&rebootinfo, 0, sizeof(OvdRebootParam));
    OVD_GetRebootInfo(&rebootinfo);
    memset(jsonParam, 0, sizeof(jsonParam));
    OVD_InitCapbility(&ovd_capInfo);
    OVD_InitClientParam(&ovd_clientParam);
    OVD_InitCallbackList(&callBackFunList);
    prepareJsonParam(jsonParam);
    return OVD_RET_SUCCESS == OVD_InitV2(&ovd_capInfo, &ovd_clientParam, &OVD_LogParam, &rebootinfo, &callBackFunList, jsonParam);
}