#include "OVD_Demo.h"
#include "OVD_define.h"
#include "inirw.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "malloc.h"
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "OVD_DemoTime.h"
#include "OVD_DEMO_FirmwareUpgrade.h"
#include <dirent.h>
#include "OVD_define.h"
#include "menuconfig.h"
#include "math.h"
#include "errno.h"
#include "sys/time.h"
#include "demoAVPushing_util.h"

#ifdef CONFIG_SUPPORT_curl
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#ifdef CONFIG_MODULE_LOCAL_STORAGE
#include "local_storage_api.h"
#endif
#ifdef CONFIG_MODULE_LOCAL_STORAGE2
#include "local_storage_api2.h"
#endif

#define OVD_MAX_JSON_REND_SIZE  (0x4000)

char g_aispeech_config_str[2048]={0};//临时存储AIspeech远程配置
//临时存储aispeech配置
int g_aispeech_status=1;
int g_aispeech_scheduleplan_on=0;
char g_aispeech_scheduleplan_startime[256]={0};
char g_aispeech_scheduleplan_endtime[256]={0};
int g_aispeech_scheduleplan_repeat=0;

// 枪球联动矫正进度
int g_linkage_tracking_fix_progress=0;

extern OVDConfigrationInfo OVD_DEMO_ConfigureInfo;
extern OVDDeviceInfo       OVD_DEMO_DeviceInfo;
extern OVDLogParam         OVD_DEMO_LogParam;
extern OVDClientParam      OVD_DEMO_Client;
extern int OVD_DEMO_status;
extern int TF_H264_H265_CTRL;
extern int g_sdcardstate;

extern int g_freedisk;

long long global_timeStamp;

char *Mp3PlayUrl;
OVDMp3PlayCtrl   Mp3PlayStatus;

pthread_t OPENAPI_DEMO_send_recordAV_pid=0;
pthread_mutex_t OPENAPI_DEMO_send_recordAV_mutex;

extern int demo_callback_changecodec2(void);

int OVD_DEMO_GetOVDDeviceInfo(OVDDeviceInfo *deviceInfo){
    OVD_DEMOE_PrintDeviceInfo();

    memcpy(deviceInfo, &OVD_DEMO_DeviceInfo, sizeof(OVDDeviceInfo) - sizeof(OVD_CameraLensesInfo));//结构体有变，
    //填入通道个数
    deviceInfo->channel_count = OVD_DEMO_ConfigureInfo.channelsInfoCount;
    //mock 测试
    //deviceInfo->channel_count=2;

    int i=0;
    for(i=0;i<deviceInfo->channel_count;i++)
    {
        deviceInfo->camera_lenses_info[i].channel_id=i;
        strncpy(deviceInfo->camera_lenses_info[i].focal_length_mm, "3.6", sizeof(deviceInfo->camera_lenses_info[i].focal_length_mm));
        
    }
    return 0;
}

int OVD_DEMO_OVCGetRepeat(cJSON* switchentry,int repeat){
	cJSON *resolutions = NULL;
	char *days[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int temp = 0;
    int j = 0;

	resolutions =cJSON_AddArrayToObject(switchentry,"repeat");
	//10进制转2进制 & 111 1111（7f） ->得出所有repeat日 ==7f则代表每天都repeat
    temp = repeat;
    while(temp){
        int i = temp%2;
        temp = temp/2;
        if (i==1){
			cJSON_AddItemToArray(resolutions, cJSON_CreateString(days[j]));
            //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","%d %s\n", i,days[j]);
        }
        j++;
    }
    return 0;
}

int OVD_DEMO_OVCRepeatCnt(cJSON *item){
	char *days[7]={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	const char repeat_value[7]={1, 2, 4, 8, 16, 32, 64};
  	int repeat_cnt=0, iCnt=0,i=0;
	int array_size = cJSON_GetArraySize(item);
	for(iCnt = 0; iCnt < array_size ; iCnt ++ ){
		cJSON * pSub = cJSON_GetArrayItem(item, iCnt);
		if(NULL == pSub )
        {
            continue;
        }

        if (cJSON_String != pSub->type)
        {
            continue;
        }

		//char * ivalue = pSub->valuestring ;
		for (i=0;i<7;i++){
			if (0==strcmp(pSub->valuestring,days[i]))
				repeat_cnt += repeat_value[i];
		}
	}
	return repeat_cnt;
}

//off duty
static cJSON* GetAIOffDutyEntry(cJSON *AI, int i)
{
    cJSON *AI_off_duty_detect=NULL;

    cJSON_AddItemToObject(AI,"off_duty_detection",AI_off_duty_detect=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_off_duty_detect,"detect_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.on);

    cJSON *detect_plans=NULL;
    detect_plans = cJSON_AddArrayToObject(AI_off_duty_detect,"detect_plans");

    cJSON * AI_off_duty =NULL;
    cJSON_AddItemToArray(detect_plans,AI_off_duty = cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_off_duty,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.on);
    cJSON_AddNumberToObject(AI_off_duty,"regional_id", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.regional_id);
    cJSON_AddStringToObject(AI_off_duty,"regional_name",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.regional_name);


    cJSON_AddNumberToObject(AI_off_duty,"on_duty_count", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.on_duty_people_count);

    cJSON_AddNumberToObject(AI_off_duty,"off_duty_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.off_duty_duration);

    cJSON *deteAlertTime=NULL;
    cJSON_AddItemToObject(AI_off_duty,"alerttime",deteAlertTime=cJSON_CreateObject());
    cJSON_AddBoolToObject(deteAlertTime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.schedule.on);
    cJSON_AddStringToObject(deteAlertTime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.schedule.starttime);
    cJSON_AddStringToObject(deteAlertTime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.schedule.endtime);
    int area_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(deteAlertTime,area_repeat);

    cJSON *zone=NULL;
    zone = cJSON_AddArrayToObject(AI_off_duty,"alarm_zone");
    cJSON * area =NULL;
    cJSON_AddItemToArray(zone,area = cJSON_CreateObject());
    cJSON_AddNumberToObject(area,"areaId",0);
    cJSON *pointarray=NULL;
    cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());
    cJSON *pointentry=NULL;

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",0);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",1);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",2);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.right_up_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",3);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan.zoneInfo.left_up_y);

    return NULL;
}


///高空检测
static cJSON* GetAIParabolicAerialEntry(cJSON *AI, int i)
{
    cJSON *AI_parabolic_aerial=NULL;
    cJSON *AI_floor_info=NULL;
    cJSON *AI_floor_entry =NULL;
    cJSON *floor_line = NULL;
    cJSON *area =NULL;
    cJSON *pointarray=NULL;
    cJSON *pointentry=NULL;
    cJSON *zone=NULL;

    OVDDemoAIParabolicAerialSet *pAIParabolicAerialSet = NULL;
    OVDAIParabolicAerialFloorConf* pFloorConfInfo = NULL;
    OVDAreasInfo * pDetectAreasInfo = NULL;
    OVDAreasInfo * pShieldAreasInfo = NULL;
    OVDSingleAreaInfo *pSingleAreaInfo = NULL;
    OVDAreaPointsInfo *pAreaPointsInfo = NULL;

    int k = 0;
    int j = 0;

    pAIParabolicAerialSet = &(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial);

    pFloorConfInfo = pAIParabolicAerialSet->pfloorInfo;

    pDetectAreasInfo = pAIParabolicAerialSet->pDetectZoneInfo;

    pShieldAreasInfo = pAIParabolicAerialSet->pShieldZoneInfo;


    cJSON_AddItemToObject(AI,"parabolic_aerial_detection",AI_parabolic_aerial=cJSON_CreateObject());

    cJSON_AddBoolToObject(AI_parabolic_aerial,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial.on);
    cJSON_AddNumberToObject(AI_parabolic_aerial,"sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial.sensitivity);



    AI_floor_info = cJSON_AddArrayToObject(AI_parabolic_aerial,"floor_configuration");

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","GetAIParabolicAerialEntry: floorNum[%d]\n", pFloorConfInfo->floorNum);

    for (k=0; k<pFloorConfInfo->floorNum; k++)
    {
        cJSON_AddItemToArray(AI_floor_info,AI_floor_entry = cJSON_CreateObject());

        cJSON_AddNumberToObject(AI_floor_entry,"floor_id",pFloorConfInfo->floorInfos[k].floor_id);
        cJSON_AddStringToObject(AI_floor_entry,"detail",  pFloorConfInfo->floorInfos[k].floor_detail);

        cJSON_AddItemToObject(AI_floor_entry,"floor_line",floor_line=cJSON_CreateObject());
        cJSON_AddNumberToObject(floor_line,"A_x",pFloorConfInfo->floorInfos[k].floor_line.point_a_x);
        cJSON_AddNumberToObject(floor_line,"A_y",pFloorConfInfo->floorInfos[k].floor_line.point_a_y);
        cJSON_AddNumberToObject(floor_line,"B_x",pFloorConfInfo->floorInfos[k].floor_line.point_b_x);
        cJSON_AddNumberToObject(floor_line,"B_y",pFloorConfInfo->floorInfos[k].floor_line.point_b_y);
    }

    //detect_zone
    zone = cJSON_AddArrayToObject(AI_parabolic_aerial,"detect_zone");
    for (k = 0; k<pDetectAreasInfo->areasNum; k++)
    {

        pSingleAreaInfo = pDetectAreasInfo->areas;

        cJSON_AddItemToArray(zone,area = cJSON_CreateObject());
        cJSON_AddNumberToObject(area,"areaId",k);
        cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());

        pAreaPointsInfo = pSingleAreaInfo[k].areaPoints;
        for(j= 0; j<4; j++)
        {
            pAreaPointsInfo = pSingleAreaInfo[k].areaPoints;
            cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
            cJSON_AddNumberToObject(pointentry,"pointId",j);
            if(j==0){
                pAreaPointsInfo->points[j].point_x = 0;
                pAreaPointsInfo->points[j].point_y = 0;
            }
            if(j==1){
                pAreaPointsInfo->points[j].point_x = 10000;
                pAreaPointsInfo->points[j].point_y = 0;
            }
            if(j==2){
                pAreaPointsInfo->points[j].point_x = 10000;
                pAreaPointsInfo->points[j].point_y = 10000;
            }
            if(j==3){
                pAreaPointsInfo->points[j].point_x = 0;
                pAreaPointsInfo->points[j].point_y = 10000;
            }
            cJSON_AddNumberToObject(pointentry,"x",      pAreaPointsInfo->points[j].point_x);
            cJSON_AddNumberToObject(pointentry,"y",      pAreaPointsInfo->points[j].point_y);
        }

    }

    //shield_zone
    zone = cJSON_AddArrayToObject(AI_parabolic_aerial,"shield_zone");
    if (pShieldAreasInfo != NULL)
    {
        for (k = 0; k<pShieldAreasInfo->areasNum; k++)
        {
            pSingleAreaInfo = pShieldAreasInfo->areas;

            cJSON_AddItemToArray(zone,area = cJSON_CreateObject());
            cJSON_AddNumberToObject(area,"areaId",k);
            cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());

            pAreaPointsInfo = pSingleAreaInfo[k].areaPoints;

            for(j= 0; j<pAreaPointsInfo->pointsNum; j++)
            {
                cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
                cJSON_AddNumberToObject(pointentry,"pointId",j);

                if(j==0){
                    pAreaPointsInfo->points[j].point_x = 0;
                    pAreaPointsInfo->points[j].point_y = 0;
                }
                if(j==1){
                    pAreaPointsInfo->points[j].point_x = 10000;
                    pAreaPointsInfo->points[j].point_y = 0;
                }
                if(j==2){
                    pAreaPointsInfo->points[j].point_x = 10000;
                    pAreaPointsInfo->points[j].point_y = 10000;
                }
                if(j==3){
                    pAreaPointsInfo->points[j].point_x = 0;
                    pAreaPointsInfo->points[j].point_y = 10000;
                }
                cJSON_AddNumberToObject(pointentry,"x",      pAreaPointsInfo->points[j].point_x);
                cJSON_AddNumberToObject(pointentry,"y",      pAreaPointsInfo->points[j].point_y);

            }

        }
    }

    return NULL;
}

//区域人数统计
static cJSON* GetAIRegionalPeopleStatEntry(cJSON *AI, int i)
{
    cJSON *AI_regional_people_statistics=NULL;

    cJSON_AddItemToObject(AI,"regional_people_statistics",AI_regional_people_statistics=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_regional_people_statistics,"detect_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.on);

    cJSON *detect_plans=NULL;
    detect_plans = cJSON_AddArrayToObject(AI_regional_people_statistics,"detect_plans");

    cJSON * AI_regional_people =NULL;
    cJSON_AddItemToArray(detect_plans,AI_regional_people = cJSON_CreateObject());

    cJSON_AddBoolToObject(AI_regional_people,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.on);
    cJSON_AddNumberToObject(AI_regional_people,"regional_id", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_id);
    cJSON_AddStringToObject(AI_regional_people,"regional_name",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_name);
    cJSON_AddNumberToObject(AI_regional_people,"regional_people_count", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_people_count);
    cJSON_AddNumberToObject(AI_regional_people,"alarm_report_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.alarm_report_duration);
    cJSON_AddNumberToObject(AI_regional_people,"detect_result_report_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.detect_result_report_duration);
    cJSON_AddBoolToObject(AI_regional_people,"osd_status", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.osd_status);


    cJSON *deteAlertTime=NULL;
    cJSON_AddItemToObject(AI_regional_people,"alerttime",deteAlertTime=cJSON_CreateObject());
    cJSON_AddBoolToObject(deteAlertTime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.on);
    cJSON_AddStringToObject(deteAlertTime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.starttime);
    cJSON_AddStringToObject(deteAlertTime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.endtime);
    int area_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(deteAlertTime,area_repeat);

    cJSON *linkage=NULL;
    cJSON_AddItemToObject(AI_regional_people,"linkage_strategy",linkage=cJSON_CreateObject());
    cJSON *speech=NULL;
    cJSON_AddItemToObject(linkage,"speech",speech=cJSON_CreateObject());
    cJSON_AddBoolToObject(speech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_on);
    cJSON_AddNumberToObject(speech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_vol);
    cJSON_AddNumberToObject(speech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_repeat);
    cJSON_AddBoolToObject(speech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_repeat_constant);
    cJSON_AddNumberToObject(speech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_interval);
    cJSON_AddStringToObject(speech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_url);
    cJSON *light=NULL;
    cJSON_AddItemToObject(linkage,"light",light=cJSON_CreateObject());
    cJSON_AddBoolToObject(light,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_on);
    cJSON_AddNumberToObject(light,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_mode);
    cJSON_AddNumberToObject(light,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_dur);
    cJSON_AddBoolToObject(light,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_dur_constant);

    cJSON *buz=NULL;
    cJSON_AddItemToObject(linkage,"buz",buz=cJSON_CreateObject());
    cJSON_AddBoolToObject(buz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.buz_on);
    cJSON_AddNumberToObject(buz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.buz_dur);



    cJSON *zone=NULL;
    zone = cJSON_AddArrayToObject(AI_regional_people,"alarm_zone");
    cJSON * area =NULL;
    cJSON_AddItemToArray(zone,area = cJSON_CreateObject());
    cJSON_AddNumberToObject(area,"areaId",0);
    cJSON *pointarray=NULL;
    cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());
    cJSON *pointentry=NULL;

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",0);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",1);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",2);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_up_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",3);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_up_y);

    return NULL;
}

//车道线
static cJSON* GetAILaneLineEntry(cJSON *AI, int i)
{
    int j = 0;

    cJSON *AI_lane_line_detection=NULL;
    cJSON_AddItemToObject(AI,"lane_line_detection",AI_lane_line_detection=cJSON_CreateObject());


    cJSON_AddBoolToObject(AI_lane_line_detection,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.on);
    cJSON_AddNumberToObject(AI_lane_line_detection,"lane_line_num", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.lane_line_num);

    cJSON *zone=NULL;
    zone = cJSON_AddArrayToObject(AI_lane_line_detection,"alarm_zone");
    cJSON * area =NULL;

    for (j=0; j<OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.lane_line_num; j++)
    {

        cJSON_AddItemToArray(zone,area = cJSON_CreateObject());

        cJSON_AddNumberToObject(area,"areaId",j);

        cJSON_AddNumberToObject(area,"direction",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.direction[j]);

        cJSON *pointarray=NULL;
        cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());
        cJSON *pointentry=NULL;

        cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(pointentry,"pointId",0);
        cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_bottom_x);
        cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_bottom_y);

        cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(pointentry,"pointId",1);
        cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_bottom_x);
        cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_bottom_y);

        cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(pointentry,"pointId",2);
        cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_up_x);
        cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_up_y);

        cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(pointentry,"pointId",3);
        cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_up_x);
        cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_up_y);
    }

    return NULL;
}

//头盔检测
static cJSON* GetAIHelmetEntry(cJSON *AI, int i)
{
    cJSON *AI_helmet_detect=NULL;
    cJSON_AddItemToObject(AI,"helmet_wearing_detection",AI_helmet_detect=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_helmet_detect,"detect_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.on);

    cJSON *DetectTime=NULL;
    cJSON_AddItemToObject(AI_helmet_detect,"detect_time",DetectTime=cJSON_CreateObject());
    cJSON_AddBoolToObject(DetectTime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.on);
    cJSON_AddStringToObject(DetectTime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime);
    cJSON_AddStringToObject(DetectTime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime);
    int helmet_repeat = OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(DetectTime, helmet_repeat);

    cJSON *pointarray=NULL;
    cJSON_AddItemToObject(AI_helmet_detect,"point",pointarray=cJSON_CreateArray());

    cJSON *pointentry=NULL;
    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",0);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",1);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",2);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",3);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_y);

    cJSON *linkage=NULL;
    cJSON_AddItemToObject(AI_helmet_detect,"linkage_strategy",linkage=cJSON_CreateObject());

    cJSON *speech=NULL;
    cJSON_AddItemToObject(linkage,"speech",speech=cJSON_CreateObject());
    cJSON_AddBoolToObject(speech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_on);
    cJSON_AddNumberToObject(speech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_vol);
    cJSON_AddNumberToObject(speech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat);
    cJSON_AddBoolToObject(speech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat_constant);
    cJSON_AddNumberToObject(speech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_interval);
    cJSON_AddStringToObject(speech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_url);

    cJSON *light=NULL;
    cJSON_AddItemToObject(linkage,"light",light=cJSON_CreateObject());
    cJSON_AddBoolToObject(light,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_on);
    cJSON_AddNumberToObject(light,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_mode);
    cJSON_AddNumberToObject(light,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur);
    cJSON_AddBoolToObject(light,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur_constant);

    cJSON *buz=NULL;
    cJSON_AddItemToObject(linkage,"buz",buz=cJSON_CreateObject());
    cJSON_AddBoolToObject(buz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_on);
    cJSON_AddNumberToObject(buz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_dur);

}

// 手势识别
static cJSON* GetAIGestureEntry(cJSON *AI, int i)
{
    cJSON *AI_gesture_recognition=NULL;
    cJSON_AddItemToObject(AI,"gesture_recognition",AI_gesture_recognition=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_gesture_recognition,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.on);
    cJSON_AddBoolToObject(AI_gesture_recognition,"gesture_call_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_on);
    cJSON_AddNumberToObject(AI_gesture_recognition,"gesture_call_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_duration);
    cJSON *AI_gesture_call_schedule=NULL;
    cJSON_AddItemToObject(AI_gesture_recognition,"gesture_call_switch_schedule",AI_gesture_call_schedule=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_gesture_call_schedule,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.on);
    cJSON_AddStringToObject(AI_gesture_call_schedule,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime);
    cJSON_AddStringToObject(AI_gesture_call_schedule,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime);
    int gesture_schedule_repeat = OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.repeat;
    OVD_DEMO_OVCGetRepeat(AI_gesture_call_schedule,gesture_schedule_repeat);
    cJSON_AddBoolToObject(AI_gesture_recognition,"gesture_help_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.help_on);
}

static cJSON* GetAISmokefireEntry(cJSON *AI, int i)
{
    cJSON *AI_smokefire=NULL;
    cJSON_AddItemToObject(AI,"smoke_fire_detection",AI_smokefire=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_smokefire,"detect_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.on);

    cJSON *DetectTime=NULL;
    cJSON_AddItemToObject(AI_smokefire,"detect_time",DetectTime=cJSON_CreateObject());
    cJSON_AddBoolToObject(DetectTime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.on);
    cJSON_AddStringToObject(DetectTime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.starttime);
    cJSON_AddStringToObject(DetectTime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.endtime);
    int smokefire_repeat = OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(DetectTime, smokefire_repeat);

    int j = 0;
    for (j = 0;
        OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas
        && j<OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas->areasNum;
        j++)
    {
        cJSON *zone = cJSON_AddArrayToObject(AI_smokefire,"zone");

        cJSON *area =NULL;
        cJSON_AddItemToArray(zone,area = cJSON_CreateObject());
        cJSON_AddNumberToObject(area,"areaId", j);
        cJSON_AddBoolToObject(area,"on", OVD_TRUE);

        cJSON *pointarray=NULL;
        cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());
        cJSON *pointentry=NULL;

        int k = 0;
        for (k = 0;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas->areas[j].areaPoints
            && k < OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas->areas[j].areaPoints->pointsNum;
            k++)
        {
            cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
            cJSON_AddNumberToObject(pointentry,"pointId",k);
            cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas->areas[j].areaPoints->points[k].point_x);
            cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.areas->areas[j].areaPoints->points[k].point_y);
        }
    }

    cJSON *linkage=NULL;
    cJSON_AddItemToObject(AI_smokefire,"linkage_strategy",linkage=cJSON_CreateObject());

    cJSON *speech=NULL;
    cJSON_AddItemToObject(linkage,"speech",speech=cJSON_CreateObject());
    cJSON_AddBoolToObject(speech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_on);
    cJSON_AddNumberToObject(speech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_vol);
    cJSON_AddNumberToObject(speech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_repeat);
    cJSON_AddNumberToObject(speech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_interval);
    cJSON_AddStringToObject(speech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_url);

    cJSON *light=NULL;
    cJSON_AddItemToObject(linkage,"light",light=cJSON_CreateObject());
    cJSON_AddBoolToObject(light,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_on);
    cJSON_AddNumberToObject(light,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_mode);
    cJSON_AddNumberToObject(light,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_dur);

    cJSON *buz=NULL;
    cJSON_AddItemToObject(linkage,"buz",buz=cJSON_CreateObject());
    cJSON_AddBoolToObject(buz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.buz_on);
    cJSON_AddNumberToObject(buz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.buz_dur);
}

//家人识别
static cJSON* GetAIFamilyRecognition(cJSON *AI, int i)
{
    int j;
    cJSON *AI_family_recognition=NULL;
    cJSON_AddItemToObject(AI,"family_recognition",AI_family_recognition=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_family_recognition,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.on);

    cJSON *DetectTime=NULL;
    cJSON_AddItemToObject(AI_family_recognition,"schedule_time",DetectTime=cJSON_CreateObject());
    cJSON_AddBoolToObject(DetectTime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.on);
    cJSON_AddStringToObject(DetectTime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.starttime);
    cJSON_AddStringToObject(DetectTime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.endtime);
    int family_repeat = OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(DetectTime, family_repeat);

    cJSON *face_management=NULL;
    cJSON_AddItemToObject(AI_family_recognition,"face_management",face_management=cJSON_CreateArray());
    int AI_family_face_num = OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.facesNum;
    for(j= 0; j<AI_family_face_num; j++)
    {
        cJSON *faceinfo=NULL;
        cJSON_AddItemToArray(face_management,faceinfo=cJSON_CreateObject());
        cJSON_AddStringToObject(faceinfo,"name",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[j].name);
        cJSON_AddStringToObject(faceinfo,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[j].url);
    }


    cJSON *linkage=NULL;
    cJSON_AddItemToObject(AI_family_recognition,"linkage_strategy",linkage=cJSON_CreateObject());
    cJSON *speech=NULL;
    cJSON_AddItemToObject(linkage,"speech",speech=cJSON_CreateObject());
    cJSON_AddBoolToObject(speech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_on);
    cJSON_AddNumberToObject(speech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_vol);
    cJSON_AddNumberToObject(speech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_repeat);
    cJSON_AddBoolToObject(speech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_repeat_constant);
    cJSON_AddNumberToObject(speech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_interval);
    cJSON_AddStringToObject(speech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_url);
    cJSON *light=NULL;
    cJSON_AddItemToObject(linkage,"light",light=cJSON_CreateObject());
    cJSON_AddBoolToObject(light,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_on);
    cJSON_AddNumberToObject(light,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_mode);
    cJSON_AddNumberToObject(light,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_dur);
    cJSON_AddBoolToObject(light,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_dur_constant);
    cJSON *buz=NULL;
    cJSON_AddItemToObject(linkage,"buz",buz=cJSON_CreateObject());
    cJSON_AddBoolToObject(buz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.buz_on);
    cJSON_AddNumberToObject(buz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.buz_dur);

}

//隐私遮蔽
static cJSON* GetAIPrivacyMasking(cJSON *AI, int i)
{
    cJSON *AI_privacy_masking=NULL;
    cJSON_AddItemToObject(AI,"privacy_masking",AI_privacy_masking=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_privacy_masking,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.on);

    //区域
    cJSON *zone=NULL;
    zone = cJSON_AddArrayToObject(AI_privacy_masking,"zone");

    cJSON * area =NULL;
    cJSON_AddItemToArray(zone,area = cJSON_CreateObject());

    cJSON *pointarray=NULL;
    cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());

    cJSON *pointentry=NULL;
    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",0);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",1);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",2);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",3);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_y);
}

//跌倒检测
static cJSON* GetAIFallDetection(cJSON *AI, int i)
{
    cJSON *AI_fall_detection=NULL;
    cJSON_AddItemToObject(AI,"fall_detection",AI_fall_detection=cJSON_CreateObject());

    cJSON_AddBoolToObject(AI_fall_detection,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.on);
    cJSON_AddNumberToObject(AI_fall_detection,"sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.sensitivity);

    //区域
    cJSON *zone=NULL;
    zone = cJSON_AddArrayToObject(AI_fall_detection,"zone");

    cJSON * area =NULL;
    cJSON_AddItemToArray(zone,area = cJSON_CreateObject());

    cJSON *pointarray=NULL;
    cJSON_AddItemToObject(area,"point",pointarray=cJSON_CreateArray());

    cJSON *pointentry=NULL;
    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",0);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",1);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",2);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_y);

    cJSON_AddItemToArray(pointarray,pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pointentry,"pointId",3);
    cJSON_AddNumberToObject(pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_y);

    //时间段
    cJSON *schedule=NULL;
    cJSON_AddItemToObject(AI_fall_detection,"schedule_time",schedule=cJSON_CreateObject());

    cJSON_AddBoolToObject(schedule,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.on);
    cJSON_AddStringToObject(schedule,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime);
    cJSON_AddStringToObject(schedule,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime);
    int repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(schedule,repeat);

    //联动
    cJSON *linkage=NULL;
    cJSON_AddItemToObject(AI_fall_detection,"linkage_strategy",linkage=cJSON_CreateObject());

    cJSON *speech=NULL;
    cJSON_AddItemToObject(linkage,"speech",speech=cJSON_CreateObject());

    cJSON_AddBoolToObject(speech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_on);
    cJSON_AddNumberToObject(speech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_vol);
    cJSON_AddNumberToObject(speech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_repeat);
    cJSON_AddBoolToObject(speech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_repeat_constant);
    cJSON_AddNumberToObject(speech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_interval);
    cJSON_AddStringToObject(speech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_url);

    cJSON *light=NULL;
    cJSON_AddItemToObject(linkage,"light",light=cJSON_CreateObject());

    cJSON_AddBoolToObject(light,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_on);
    cJSON_AddNumberToObject(light,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_mode);
    cJSON_AddNumberToObject(light,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_dur);
    cJSON_AddBoolToObject(light,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_dur_constant);

    cJSON *buz=NULL;
    cJSON_AddItemToObject(linkage,"buz",buz=cJSON_CreateObject());
    cJSON_AddBoolToObject(buz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_on);
    cJSON_AddNumberToObject(buz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_dur);
}

//睡眠检测-sleep_monitor
static cJSON* GetAISleepMonitor(cJSON *AI, int i)
{
    cJSON *sleepentry=NULL;
    cJSON_AddItemToObject(AI,"sleep_monitor",sleepentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(sleepentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.on);
    cJSON *sleep_detection=NULL;
    cJSON_AddItemToObject(sleepentry,"sleep_detection",sleep_detection=cJSON_CreateObject());
    cJSON_AddBoolToObject(sleep_detection,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.on);
    cJSON_AddNumberToObject(sleep_detection,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.sensitivity);
    cJSON *face_cover=NULL;
    cJSON_AddItemToObject(sleepentry,"face_cover",face_cover=cJSON_CreateObject());
    cJSON_AddBoolToObject(face_cover,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.on);
    cJSON_AddNumberToObject(face_cover,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.sensitivity);
    cJSON_AddNumberToObject(face_cover,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.mode);
    cJSON *wakeup_detection=NULL;
    cJSON_AddItemToObject(sleepentry,"wakeup_detection",wakeup_detection=cJSON_CreateObject());
    cJSON_AddBoolToObject(wakeup_detection,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.on);
    cJSON_AddNumberToObject(wakeup_detection,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.sensitivity);
}

//表情识别 CSWID-2025-000FE7
static cJSON* GetAIExpressionRecognition(cJSON *AI, int channel_index)
{
    if (!AI || channel_index < 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR, "DEV/DEMO", "Invalid input parameters");
        return NULL;
    }

    cJSON *expression_recognition_entry = cJSON_GetObjectItem(AI, "expression_recognition");
    if (!expression_recognition_entry) {
        expression_recognition_entry = cJSON_CreateObject();
        cJSON_AddItemToObject(AI, "expression_recognition", expression_recognition_entry);
    }

    cJSON_AddBoolToObject(expression_recognition_entry, "on", 
                          OVD_DEMO_ConfigureInfo.channelsInfo[channel_index].aiset.AIExpressionRecognition.on);

    return expression_recognition_entry;
}


static cJSON* GetAISentry(cJSON *channelentry,int i)
{
	//AI
	cJSON *AI=NULL;
	cJSON_AddItemToObject(channelentry,"AI",AI=cJSON_CreateObject());
	//AI-face
	cJSON *AI_face=NULL;
	cJSON_AddItemToObject(AI,"face",AI_face=cJSON_CreateObject());
	cJSON_AddBoolToObject(AI_face,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.on);
	cJSON_AddBoolToObject(AI_face,"exposure_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_on);
	cJSON_AddNumberToObject(AI_face,"exposure_adjust",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_adjust);
	cJSON_AddStringToObject(AI_face,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.mode);
	cJSON_AddNumberToObject(AI_face,"quality_value",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.quality_value);
	cJSON_AddNumberToObject(AI_face,"alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.sensitivity);
	cJSON *facezone=NULL;
	facezone = cJSON_AddArrayToObject(AI_face,"alarm_zone");
	cJSON * face_area =NULL;
	cJSON_AddItemToArray(facezone,face_area = cJSON_CreateObject());
	cJSON_AddNumberToObject(face_area,"areaId",0);
	cJSON *face_pointarray=NULL;
	cJSON_AddItemToObject(face_area,"point",face_pointarray=cJSON_CreateArray());
	cJSON *face_pointentry=NULL;

	cJSON_AddItemToArray(face_pointarray,face_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(face_pointentry,"pointId",0);
	cJSON_AddNumberToObject(face_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_bottom_x);
	cJSON_AddNumberToObject(face_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_bottom_y);

	cJSON_AddItemToArray(face_pointarray,face_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(face_pointentry,"pointId",1);
	cJSON_AddNumberToObject(face_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_x);
	cJSON_AddNumberToObject(face_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_y);

	cJSON_AddItemToArray(face_pointarray,face_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(face_pointentry,"pointId",2);
	cJSON_AddNumberToObject(face_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_up_x);
	cJSON_AddNumberToObject(face_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_up_y);

    cJSON_AddItemToArray(face_pointarray,face_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(face_pointentry,"pointId",3);
	cJSON_AddNumberToObject(face_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_x);
	cJSON_AddNumberToObject(face_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_y);

	//AI-face-mask_detection
    cJSON *mask_detection=NULL;
	cJSON_AddItemToObject(AI_face,"mask_detection",mask_detection=cJSON_CreateObject());
	cJSON_AddBoolToObject(mask_detection,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.on);

	cJSON *maskDetetime=NULL;
	cJSON_AddItemToObject(mask_detection,"alerttime",maskDetetime=cJSON_CreateObject());
	cJSON_AddBoolToObject(maskDetetime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.on);
	cJSON_AddStringToObject(maskDetetime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime);
	cJSON_AddStringToObject(maskDetetime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime);
	int area_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.repeat;
	OVD_DEMO_OVCGetRepeat(maskDetetime,area_repeat);

	cJSON *maskDetelinkage=NULL;
	cJSON_AddItemToObject(mask_detection,"linkage_strategy",maskDetelinkage=cJSON_CreateObject());
	cJSON *maskDetespeech=NULL;
	cJSON_AddItemToObject(maskDetelinkage,"speech",maskDetespeech=cJSON_CreateObject());
	cJSON_AddBoolToObject(maskDetespeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_on);
	cJSON_AddNumberToObject(maskDetespeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_vol);
	cJSON_AddNumberToObject(maskDetespeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat);
    cJSON_AddBoolToObject(maskDetespeech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat_constant);
    cJSON_AddNumberToObject(maskDetespeech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_interval);
	cJSON_AddStringToObject(maskDetespeech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_url);
	cJSON *maskDetelight=NULL;
	cJSON_AddItemToObject(maskDetelinkage,"light",maskDetelight=cJSON_CreateObject());
	cJSON_AddBoolToObject(maskDetelight,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_on);
	cJSON_AddNumberToObject(maskDetelight,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_mode);
	cJSON_AddNumberToObject(maskDetelight,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur);
    cJSON_AddBoolToObject(maskDetelight,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur_constant);

	cJSON *maskDetebuz=NULL;
	cJSON_AddItemToObject(maskDetelinkage,"buz",maskDetebuz=cJSON_CreateObject());
	cJSON_AddBoolToObject(maskDetebuz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_on);
	cJSON_AddNumberToObject(maskDetebuz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_dur);


	//AI-kitchen_detection
	cJSON *AI_kitchen=NULL;
	cJSON_AddItemToObject(AI,"kitchen_detection",AI_kitchen=cJSON_CreateObject());
	cJSON_AddBoolToObject(AI_kitchen,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.on);
	cJSON_AddBoolToObject(AI_kitchen,"mask_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.mask_on);
	cJSON_AddBoolToObject(AI_kitchen,"cap_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.cap_on);
	cJSON_AddBoolToObject(AI_kitchen,"clothes_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_on);
	cJSON_AddStringToObject(AI_kitchen,"clothes_color",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_color);
	cJSON_AddNumberToObject(AI_kitchen,"alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.sensitivity);
	cJSON *kitchenzone=NULL;
	kitchenzone = cJSON_AddArrayToObject(AI_kitchen,"alarm_zone");
	cJSON * kitchen_area =NULL;
	cJSON_AddItemToArray(kitchenzone,kitchen_area = cJSON_CreateObject());
	cJSON_AddNumberToObject(kitchen_area,"areaId",0);
	cJSON *kitchen_pointarray=NULL;
	cJSON_AddItemToObject(kitchen_area,"point",kitchen_pointarray=cJSON_CreateArray());
	cJSON *kitchen_pointentry=NULL;

	cJSON_AddItemToArray(kitchen_pointarray,kitchen_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(kitchen_pointentry,"pointId",0);
	cJSON_AddNumberToObject(kitchen_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_bottom_x);
	cJSON_AddNumberToObject(kitchen_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_bottom_y);

	cJSON_AddItemToArray(kitchen_pointarray,kitchen_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(kitchen_pointentry,"pointId",1);
	cJSON_AddNumberToObject(kitchen_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_x);
	cJSON_AddNumberToObject(kitchen_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_y);

	cJSON_AddItemToArray(kitchen_pointarray,kitchen_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(kitchen_pointentry,"pointId",2);
	cJSON_AddNumberToObject(kitchen_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_up_x);
	cJSON_AddNumberToObject(kitchen_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_up_y);

    cJSON_AddItemToArray(kitchen_pointarray,kitchen_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(kitchen_pointentry,"pointId",3);
	cJSON_AddNumberToObject(kitchen_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_x);
	cJSON_AddNumberToObject(kitchen_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_y);

	//AI-vehicle_detection
	cJSON *AI_vehicle=NULL;
	cJSON_AddItemToObject(AI,"vehicle_detection",AI_vehicle=cJSON_CreateObject());
	cJSON_AddBoolToObject(AI_vehicle,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.on);
    cJSON_AddNumberToObject(AI_vehicle,"alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.sensitivity);
	cJSON_AddNumberToObject(AI_vehicle,"quality", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.quality);
	cJSON_AddNumberToObject(AI_vehicle,"capture_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.capture_mode);
	cJSON_AddNumberToObject(AI_vehicle,"scheduled_capture_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.scheduled_capture_time);
	cJSON_AddNumberToObject(AI_vehicle,"detect_site",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.detect_site);
	cJSON_AddNumberToObject(AI_vehicle,"exit_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.exit_mode);


	cJSON *vehlinkage=NULL;
	cJSON_AddItemToObject(AI_vehicle,"linkage_strategy",vehlinkage=cJSON_CreateObject());
	cJSON *vehspeech=NULL;
	cJSON_AddItemToObject(vehlinkage,"speech",vehspeech=cJSON_CreateObject());
	cJSON_AddBoolToObject(vehspeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_on);
	cJSON_AddNumberToObject(vehspeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_vol);
	cJSON_AddNumberToObject(vehspeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat);
    cJSON_AddBoolToObject(vehspeech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat_constant);
    cJSON_AddNumberToObject(vehspeech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_interval);
	cJSON_AddStringToObject(vehspeech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_url);
	cJSON *vehlight=NULL;
	cJSON_AddItemToObject(vehlinkage,"light",vehlight=cJSON_CreateObject());
	cJSON_AddBoolToObject(vehlight,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_on);
	cJSON_AddNumberToObject(vehlight,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_mode);
	cJSON_AddNumberToObject(vehlight,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur);
    cJSON_AddBoolToObject(vehlight,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur_constant);

	cJSON *vehbuz=NULL;
	cJSON_AddItemToObject(vehlinkage,"buz",vehbuz=cJSON_CreateObject());
	cJSON_AddBoolToObject(vehbuz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_on);
	cJSON_AddNumberToObject(vehbuz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_dur);

	cJSON *vehiclezone=NULL;
	vehiclezone = cJSON_AddArrayToObject(AI_vehicle,"alarm_zone");
	cJSON * vehicle_area =NULL;
	cJSON_AddItemToArray(vehiclezone,vehicle_area = cJSON_CreateObject());
	cJSON_AddNumberToObject(vehicle_area,"areaId",0);
	cJSON *vehicle_pointarray=NULL;
	cJSON_AddItemToObject(vehicle_area,"point",vehicle_pointarray=cJSON_CreateArray());
	cJSON *vehicle_pointentry=NULL;

	cJSON_AddItemToArray(vehicle_pointarray,vehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(vehicle_pointentry,"pointId",0);
	cJSON_AddNumberToObject(vehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_bottom_x);
	cJSON_AddNumberToObject(vehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_bottom_y);

	cJSON_AddItemToArray(vehicle_pointarray,vehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(vehicle_pointentry,"pointId",1);
	cJSON_AddNumberToObject(vehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_x);
	cJSON_AddNumberToObject(vehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_y);

	cJSON_AddItemToArray(vehicle_pointarray,vehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(vehicle_pointentry,"pointId",2);
	cJSON_AddNumberToObject(vehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_up_x);
	cJSON_AddNumberToObject(vehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_up_y);

    cJSON_AddItemToArray(vehicle_pointarray,vehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(vehicle_pointentry,"pointId",3);
	cJSON_AddNumberToObject(vehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_x);
	cJSON_AddNumberToObject(vehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_y);

    //AI-nonmotorvehicle_detection(梯控)
    cJSON *AI_nonmotorvehicle=NULL;
	cJSON_AddItemToObject(AI,"nonmotorvehicle_detection",AI_nonmotorvehicle=cJSON_CreateObject());
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","nonmotorvehicle_detection_on: %d\n",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.on);
    cJSON_AddBoolToObject(AI_nonmotorvehicle,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.on);
	cJSON_AddStringToObject(AI_nonmotorvehicle,"detect_mode_list",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.detect_mode_list);
    cJSON_AddNumberToObject(AI_nonmotorvehicle,"alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.sensitivity);
    cJSON_AddNumberToObject(AI_nonmotorvehicle,"quality",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.quality);
    cJSON_AddNumberToObject(AI_nonmotorvehicle,"capture_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.capture_mode);
    cJSON_AddNumberToObject(AI_nonmotorvehicle,"scheduled_capture_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.scheduled_capture_time);


	cJSON *nonvehlinkage=NULL;
	cJSON_AddItemToObject(AI_nonmotorvehicle,"linkage_strategy",nonvehlinkage=cJSON_CreateObject());
	cJSON *nonvehspeech=NULL;
	cJSON_AddItemToObject(nonvehlinkage,"speech",nonvehspeech=cJSON_CreateObject());
	cJSON_AddBoolToObject(nonvehspeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_on);
	cJSON_AddNumberToObject(nonvehspeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_vol);
	cJSON_AddNumberToObject(nonvehspeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat);
    cJSON_AddBoolToObject(nonvehspeech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat_constant);
    cJSON_AddNumberToObject(nonvehspeech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_interval);
	cJSON_AddStringToObject(nonvehspeech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_url);
	cJSON *nonvehlight=NULL;
	cJSON_AddItemToObject(nonvehlinkage,"light",nonvehlight=cJSON_CreateObject());
	cJSON_AddBoolToObject(nonvehlight,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_on);
	cJSON_AddNumberToObject(nonvehlight,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_mode);
	cJSON_AddNumberToObject(nonvehlight,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur);
    cJSON_AddBoolToObject(nonvehlight,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur_constant);

	cJSON *nonvehbuz=NULL;
	cJSON_AddItemToObject(nonvehlinkage,"buz",nonvehbuz=cJSON_CreateObject());
	cJSON_AddBoolToObject(nonvehbuz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_on);
	cJSON_AddNumberToObject(nonvehbuz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_dur);

	cJSON *nonvehiclezone=NULL;
	nonvehiclezone = cJSON_AddArrayToObject(AI_nonmotorvehicle,"alarm_zone");
	cJSON * nonvehicle_area =NULL;
	cJSON_AddItemToArray(nonvehiclezone,nonvehicle_area = cJSON_CreateObject());
	cJSON_AddNumberToObject(nonvehicle_area,"areaId",0);
	cJSON *nonvehicle_pointarray=NULL;
	cJSON_AddItemToObject(nonvehicle_area,"point",nonvehicle_pointarray=cJSON_CreateArray());
	cJSON *nonvehicle_pointentry=NULL;

	cJSON_AddItemToArray(nonvehicle_pointarray,nonvehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(nonvehicle_pointentry,"pointId",0);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_bottom_x);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_bottom_y);

	cJSON_AddItemToArray(nonvehicle_pointarray,nonvehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(nonvehicle_pointentry,"pointId",1);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_x);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_y);

	cJSON_AddItemToArray(nonvehicle_pointarray,nonvehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(nonvehicle_pointentry,"pointId",2);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_up_x);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_up_y);

	cJSON_AddItemToArray(nonvehicle_pointarray,nonvehicle_pointentry=cJSON_CreateObject());
	cJSON_AddNumberToObject(nonvehicle_pointentry,"pointId",3);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_x);
	cJSON_AddNumberToObject(nonvehicle_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_y);


    //客流统计
    cJSON *AI_passenger=NULL;
    cJSON_AddItemToObject(AI,"passenger_flow_statistics",AI_passenger=cJSON_CreateObject());
    cJSON_AddBoolToObject(AI_passenger,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.on);
    cJSON_AddBoolToObject(AI_passenger,"exposure_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_on);
    cJSON_AddBoolToObject(AI_passenger,"osd_status",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.osd_status);
    cJSON_AddNumberToObject(AI_passenger,"exposure_adjust",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_adjust);
    cJSON_AddStringToObject(AI_passenger,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.mode);
    cJSON_AddNumberToObject(AI_passenger,"quality_value",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.quality_value);
    cJSON_AddNumberToObject(AI_passenger,"alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.sensitivity);
    cJSON_AddNumberToObject(AI_passenger,"statistics_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.statistics_mode);

    cJSON *AI_welcome=NULL;
    cJSON_AddItemToObject(AI_passenger,"welcome_promotion",AI_welcome=cJSON_CreateObject());
    cJSON *welcomespeech=NULL;
    cJSON_AddItemToObject(AI_welcome,"speech",welcomespeech=cJSON_CreateObject());
	cJSON_AddBoolToObject(welcomespeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_on);
	cJSON_AddNumberToObject(welcomespeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_vol);
	cJSON_AddNumberToObject(welcomespeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_repeat);
	cJSON_AddStringToObject(welcomespeech,"url_enter",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_enter);
    cJSON_AddStringToObject(welcomespeech,"url_leave",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_leave);
    cJSON *welcomealerttime=NULL;
    cJSON_AddItemToObject(AI_welcome,"alerttime",welcomealerttime=cJSON_CreateObject());
    cJSON_AddStringToObject(welcomealerttime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time);
    cJSON_AddStringToObject(welcomealerttime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time);
    int passenger_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.repeat;
    OVD_DEMO_OVCGetRepeat(welcomealerttime,passenger_repeat);

    cJSON *statistcs_line=NULL;
    cJSON_AddItemToObject(AI_passenger,"statistics_line",statistcs_line=cJSON_CreateObject());
    cJSON_AddNumberToObject(statistcs_line,"A_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_x);
    cJSON_AddNumberToObject(statistcs_line,"A_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_y);
    cJSON_AddNumberToObject(statistcs_line,"B_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_x);
    cJSON_AddNumberToObject(statistcs_line,"B_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_y);

    cJSON *passengerzone=NULL;
    passengerzone = cJSON_AddArrayToObject(AI_passenger,"alarm_zone");
    cJSON * passenger_area =NULL;
    cJSON_AddItemToArray(passengerzone,passenger_area = cJSON_CreateObject());
    cJSON_AddNumberToObject(passenger_area,"areaId",0);
    cJSON *passenger_pointarray=NULL;
    cJSON_AddItemToObject(passenger_area,"point",passenger_pointarray=cJSON_CreateArray());
    cJSON *passenger_pointentry=NULL;

    cJSON_AddItemToArray(passenger_pointarray,passenger_pointentry=cJSON_CreateObject());

    cJSON_AddNumberToObject(passenger_pointentry,"pointId",0);
    cJSON_AddNumberToObject(passenger_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(passenger_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_bottom_y);

    cJSON_AddItemToArray(passenger_pointarray,passenger_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(passenger_pointentry,"pointId",1);
    cJSON_AddNumberToObject(passenger_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(passenger_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(passenger_pointarray,passenger_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(passenger_pointentry,"pointId",2);
    cJSON_AddNumberToObject(passenger_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(passenger_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_up_y);

    cJSON_AddItemToArray(passenger_pointarray,passenger_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(passenger_pointentry,"pointId",3);
    cJSON_AddNumberToObject(passenger_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(passenger_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_y);

    //高空抛物
    GetAIParabolicAerialEntry(AI, 0);

    //区域人数统计
    GetAIRegionalPeopleStatEntry(AI, 0);

    //车道线
    GetAILaneLineEntry(AI, 0);

    //off duty
    GetAIOffDutyEntry(AI, 0);

    //头盔检测
    GetAIHelmetEntry(AI, 0);

    //手势识别
    GetAIGestureEntry(AI, 0);

    //烟火检测
    GetAISmokefireEntry(AI, 0);

    //家人识别
    GetAIFamilyRecognition(AI, 0);

    //隐私遮蔽
    GetAIPrivacyMasking(AI, 0);

    //跌倒检测
    GetAIFallDetection(AI, 0);

    //睡眠检测-sleep_monitor
    GetAISleepMonitor(AI, 0);

    //表情识别
    GetAIExpressionRecognition(AI, 0);
    return NULL;

}

static cJSON* GetAlarmSentry(cJSON *channelentry,int i)
{
    cJSON *alarmsentry=NULL;
    cJSON *ioentry=NULL;
    cJSON *voiceentry=NULL;
    cJSON *cryentry=NULL;
    cJSON *cryingpacifyentry=NULL;
    cJSON *motionentry=NULL;
    cJSON *alertareaentry=NULL;
    cJSON *crossentry=NULL;
    cJSON *bodyentry=NULL;
    cJSON *pirentry=NULL;
    cJSON *sendor_3d=NULL;

    //alarms
    cJSON_AddItemToObject(channelentry,"alarms",alarmsentry=cJSON_CreateObject());
    //io
    cJSON_AddItemToObject(alarmsentry,"io",ioentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(ioentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.on);
    cJSON_AddNumberToObject(ioentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.sensitivity);
    //cry
    cJSON_AddItemToObject(alarmsentry,"cry",cryentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(cryentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.on);
    cJSON_AddNumberToObject(cryentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.sensitivity);
    cJSON_AddItemToObject(cryentry,"crying_pacify",cryingpacifyentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(cryingpacifyentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_on);
    cJSON_AddNumberToObject(cryingpacifyentry,"audio_playing_count",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_count);
    cJSON_AddNumberToObject(cryingpacifyentry,"audio_playing_volumn",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_volumn);
    cJSON_AddStringToObject(cryingpacifyentry,"audio_playing_url",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_url);


    //voice
    cJSON_AddItemToObject(alarmsentry,"voice",voiceentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(voiceentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.on);
    cJSON_AddNumberToObject(voiceentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.sensitivity);
    //motion
    cJSON_AddItemToObject(alarmsentry,"motion",motionentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(motionentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.on);
    cJSON_AddNumberToObject(motionentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.sensitivity);

    //alertarea
    cJSON_AddItemToObject(alarmsentry,"alertarea",alertareaentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(alertareaentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.on);
    cJSON_AddBoolToObject(alertareaentry,"expel",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.expel);
    cJSON_AddNumberToObject(alertareaentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.sensitivity);
    cJSON *alertareatime=NULL;
    cJSON_AddItemToObject(alertareaentry,"alerttime",alertareatime=cJSON_CreateObject());
    cJSON_AddBoolToObject(alertareatime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.on);
    cJSON_AddStringToObject(alertareatime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.starttime);
    cJSON_AddStringToObject(alertareatime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.endtime);
    int area_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.repeat;
    OVD_DEMO_OVCGetRepeat(alertareatime,area_repeat);
    cJSON *areazone=NULL;
    areazone = cJSON_AddArrayToObject(alertareaentry,"zone");
    cJSON * alarmarea =NULL;
    int area_count=0;
    int zone_size=OVD_DEMO_CapInfoV2.alarm.alertarea_params.alertarea_num;
    for (area_count=0; area_count<zone_size; area_count++)
    {
        cJSON_AddItemToArray(areazone,alarmarea = cJSON_CreateObject());
        cJSON_AddStringToObject(alarmarea,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].on);  // 1.41
        cJSON_AddNumberToObject(alarmarea,"areaId",area_count);

        cJSON *area_pointarray=NULL;
        cJSON_AddItemToObject(alarmarea,"point",area_pointarray=cJSON_CreateArray());

        cJSON *area_pointentry=NULL;
        cJSON_AddItemToArray(area_pointarray,area_pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(area_pointentry,"pointId",0);
        cJSON_AddNumberToObject(area_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].left_bottom_x);
        cJSON_AddNumberToObject(area_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].left_bottom_y);

        cJSON_AddItemToArray(area_pointarray,area_pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(area_pointentry,"pointId",1);
        cJSON_AddNumberToObject(area_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].right_bottom_x);
        cJSON_AddNumberToObject(area_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].right_bottom_y);

        cJSON_AddItemToArray(area_pointarray,area_pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(area_pointentry,"pointId",2);
        cJSON_AddNumberToObject(area_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].right_up_x);
        cJSON_AddNumberToObject(area_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].right_up_y);

        cJSON_AddItemToArray(area_pointarray,area_pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(area_pointentry,"pointId",3);
        cJSON_AddNumberToObject(area_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].left_up_x);
        cJSON_AddNumberToObject(area_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[area_count].left_up_y);
    }

    cJSON *arealinkage=NULL;
    cJSON_AddItemToObject(alertareaentry,"linkage_strategy",arealinkage=cJSON_CreateObject());
    cJSON *areaspeech=NULL;
    cJSON_AddItemToObject(arealinkage,"speech",areaspeech=cJSON_CreateObject());
    cJSON_AddBoolToObject(areaspeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_on);
    cJSON_AddNumberToObject(areaspeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_vol);
    cJSON_AddNumberToObject(areaspeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat);
    cJSON_AddBoolToObject(areaspeech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat_constant);
    cJSON_AddNumberToObject(areaspeech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_interval);
    cJSON_AddStringToObject(areaspeech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_url);
    cJSON *arealight=NULL;
    cJSON_AddItemToObject(arealinkage,"light",arealight=cJSON_CreateObject());
    cJSON_AddBoolToObject(arealight,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_on);
    cJSON_AddNumberToObject(arealight,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_mode);
    cJSON_AddNumberToObject(arealight,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur);
    cJSON_AddBoolToObject(arealight,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur_constant);

    cJSON *areabuz=NULL;
    cJSON_AddItemToObject(arealinkage,"buz",areabuz=cJSON_CreateObject());
    cJSON_AddBoolToObject(areabuz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_on);
    cJSON_AddNumberToObject(areabuz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_dur);

    cJSON_AddNumberToObject(alertareaentry,"targetType",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.targetType);
    cJSON_AddNumberToObject(alertareaentry,"staymode_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.staymode_time);

    //越界侦测
    cJSON *transgressionentry=NULL;
    cJSON_AddItemToObject(alarmsentry,"transgression",transgressionentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(transgressionentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.on);
    cJSON_AddNumberToObject(transgressionentry,"targetType",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.targetType);
    cJSON *transgressiontime=NULL;
    cJSON_AddItemToObject(transgressionentry,"alerttime",transgressiontime=cJSON_CreateObject());
    cJSON_AddBoolToObject(transgressiontime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.on);
    cJSON_AddStringToObject(transgressiontime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.starttime);
    cJSON_AddStringToObject(transgressiontime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.endtime);
    int transgression_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.repeat;
    OVD_DEMO_OVCGetRepeat(transgressiontime,transgression_repeat);
    //第一条规则线
    cJSON *statistics_line_entry=NULL;
    cJSON_AddItemToObject(transgressionentry,"statistics_line",statistics_line_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(statistics_line_entry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.on);
    cJSON_AddNumberToObject(statistics_line_entry,"A_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_x);
    cJSON_AddNumberToObject(statistics_line_entry,"A_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_x);
    cJSON_AddNumberToObject(statistics_line_entry,"A_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_y);
    cJSON_AddNumberToObject(statistics_line_entry,"B_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_x);
    cJSON_AddNumberToObject(statistics_line_entry,"B_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_y);
    cJSON_AddNumberToObject(transgressionentry,"statistics_mode_style",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_style);
    cJSON_AddNumberToObject(transgressionentry,"statistics_mode_direction",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_direction);
    //额外的规则线
    cJSON *statistics_line_another_entry=NULL;
    statistics_line_another_entry = cJSON_AddArrayToObject(transgressionentry,"statistics_line_another");
    int line_another_count=0;
    int line_another_size=OVD_DEMO_CapInfoV2.alarm.transgression_params.statistics_line_another_num;
    cJSON *statisticsline=NULL;
    for (line_another_count=0; line_another_count<line_another_size; line_another_count++)
    {
        cJSON_AddItemToArray(statistics_line_another_entry, statisticsline = cJSON_CreateObject());
        cJSON_AddBoolToObject(statisticsline,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].on);
        cJSON_AddNumberToObject(statisticsline,"lineId",line_another_count);
        cJSON *line_entry=NULL;
        cJSON_AddItemToObject(statisticsline,"line",line_entry=cJSON_CreateObject());
        cJSON_AddNumberToObject(line_entry,"A_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_line_A_x);
        cJSON_AddNumberToObject(line_entry,"A_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_line_A_y);
        cJSON_AddNumberToObject(line_entry,"B_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_line_B_x);
        cJSON_AddNumberToObject(line_entry,"B_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_line_B_y);
        cJSON_AddNumberToObject(statisticsline,"statistics_mode_style",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_mode_style);
        cJSON_AddNumberToObject(statisticsline,"statistics_mode_direction",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[line_another_count].statistics_mode_direction);
    }
    cJSON *transgreslinkage=NULL;
    cJSON_AddItemToObject(transgressionentry,"linkage_strategy",transgreslinkage=cJSON_CreateObject());
    cJSON *transgresspeech=NULL;
    cJSON_AddItemToObject(transgreslinkage,"speech",transgresspeech=cJSON_CreateObject());
    cJSON_AddBoolToObject(transgresspeech,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_on);
    cJSON_AddNumberToObject(transgresspeech,"vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_vol);
    cJSON_AddNumberToObject(transgresspeech,"repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat);
    cJSON_AddBoolToObject(transgresspeech,"repeat_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat_constant);
    cJSON_AddNumberToObject(transgresspeech,"interval",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_interval);
    cJSON_AddStringToObject(transgresspeech,"url",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_url);
    cJSON *transgreslight=NULL;
    cJSON_AddItemToObject(transgreslinkage,"light",transgreslight=cJSON_CreateObject());
    cJSON_AddBoolToObject(transgreslight,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_on);
    cJSON_AddNumberToObject(transgreslight,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_mode);
    cJSON_AddNumberToObject(transgreslight,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur);
    cJSON_AddBoolToObject(transgreslight,"dur_constant",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur_constant);

    cJSON *transgresbuz=NULL;
    cJSON_AddItemToObject(transgreslinkage,"buz",transgresbuz=cJSON_CreateObject());
    cJSON_AddBoolToObject(transgresbuz,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_on);
    cJSON_AddNumberToObject(transgresbuz,"dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_dur);

    //WIFI感知联动
    cJSON *wifisensingentry=NULL;
    cJSON_AddItemToObject(alarmsentry,"wifi_sensing",wifisensingentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(wifisensingentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.on);
    cJSON_AddBoolToObject(wifisensingentry,"linkage_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.linkage_on);
    cJSON_AddNumberToObject(wifisensingentry,"sensing_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.sensing_mode);
    cJSON *wifisensingtime=NULL;
    cJSON_AddItemToObject(wifisensingentry,"schedule_time",wifisensingtime=cJSON_CreateObject());
    cJSON_AddBoolToObject(wifisensingtime,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.on);
    cJSON_AddStringToObject(wifisensingtime,"starttime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.starttime);
    cJSON_AddStringToObject(wifisensingtime,"endtime",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.endtime);
    int wifisensing_repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.repeat;
    OVD_DEMO_OVCGetRepeat(wifisensingtime,wifisensing_repeat);
    int entire_dev_num = OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_dev_num;
    int j_dev_num = 0;
    cJSON *entire_dev_management_array = NULL;
    entire_dev_management_array = cJSON_AddArrayToObject(wifisensingentry,"entire_dev_management");
    cJSON * entire_dev_management =NULL;
    for (j_dev_num = 0; j_dev_num < entire_dev_num; j_dev_num++)
    {
        cJSON_AddItemToArray(entire_dev_management_array, entire_dev_management = cJSON_CreateObject());
        cJSON_AddStringToObject(entire_dev_management, "name", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].name);
        cJSON_AddStringToObject(entire_dev_management, "option60", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].option60);
        cJSON_AddStringToObject(entire_dev_management, "ip", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].ip);
        cJSON_AddStringToObject(entire_dev_management, "mac", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].mac);
        cJSON_AddBoolToObject(entire_dev_management, "on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].on);
    }
    int added_dev_num = OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_dev_num;
    cJSON *added_dev_management_array = NULL;
    added_dev_management_array = cJSON_AddArrayToObject(wifisensingentry,"added_dev_management");
    cJSON * added_dev_management =NULL;
    for (j_dev_num = 0; j_dev_num < added_dev_num; j_dev_num++)
    {
        cJSON_AddItemToArray(added_dev_management_array, added_dev_management = cJSON_CreateObject());
        cJSON_AddStringToObject(added_dev_management, "name", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].name);
        cJSON_AddStringToObject(added_dev_management, "option60", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].option60);
        cJSON_AddStringToObject(added_dev_management, "ip", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].ip);
        cJSON_AddStringToObject(added_dev_management, "mac", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].mac);
    }


    //cross
    cJSON_AddItemToObject(alarmsentry,"cross",crossentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(crossentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.on);
    cJSON_AddNumberToObject(crossentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.sensitivity);

    //body
    cJSON_AddItemToObject(alarmsentry,"body",bodyentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(bodyentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.on);
    cJSON_AddNumberToObject(bodyentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.sensitivity);

    cJSON *bodyzone=NULL;
    bodyzone= cJSON_AddArrayToObject(bodyentry,"zone");
    int body_count=0;
    cJSON * bodyarea =NULL;
    cJSON_AddItemToArray(bodyzone,bodyarea=cJSON_CreateObject());
    cJSON_AddNumberToObject(bodyarea,"areaId",body_count);
    cJSON *body_pointarray=NULL;
    cJSON_AddItemToObject(bodyarea,"point",body_pointarray=cJSON_CreateArray());
    cJSON *body_pointentry = NULL;
    cJSON_AddItemToArray(body_pointarray,body_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(body_pointentry,"pointId",0);
    cJSON_AddNumberToObject(body_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(body_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_bottom_y);


    cJSON_AddItemToArray(body_pointarray,body_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(body_pointentry,"pointId",1);
    cJSON_AddNumberToObject(body_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(body_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_y);

    cJSON_AddItemToArray(body_pointarray,body_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(body_pointentry,"pointId",2);
    cJSON_AddNumberToObject(body_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(body_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_up_y);

    cJSON_AddItemToArray(body_pointarray,body_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(body_pointentry,"pointId",3);
    cJSON_AddNumberToObject(body_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(body_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_y);

    //pir
    cJSON_AddItemToObject(alarmsentry,"pir", pirentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(pirentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.on);
    cJSON_AddNumberToObject(pirentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.sensitivity);
    //losslock
    cJSON_AddItemToObject(alarmsentry,"losslock", pirentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(pirentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.lossLockAlarm.on);

    cJSON_AddNumberToObject(channelentry,"audio_out_volume",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioOutValume);
    cJSON_AddNumberToObject(channelentry,"doorbell_volume",OVD_DEMO_ConfigureInfo.channelsInfo[i].doorbell_volume);
    cJSON_AddBoolToObject(channelentry,"trace",OVD_DEMO_ConfigureInfo.channelsInfo[i].traceAbility);
    cJSON *motion_tracking=NULL;
    cJSON_AddItemToObject(channelentry,"motion_tracking",motion_tracking=cJSON_CreateObject());
    cJSON_AddNumberToObject(motion_tracking,"targetType",OVD_DEMO_ConfigureInfo.channelsInfo[i].traces.tracetarget);

    //3d sendor
    cJSON_AddItemToObject(alarmsentry,"3d_light_sensor",sendor_3d=cJSON_CreateObject());
    cJSON_AddBoolToObject(sendor_3d,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.on);
    cJSON *fall_detect =NULL;
    cJSON_AddItemToObject(sendor_3d,"falldown_detect",fall_detect=cJSON_CreateObject());
    cJSON_AddBoolToObject(fall_detect,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.on);
    cJSON_AddNumberToObject(fall_detect,"time",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.time);
    cJSON *wc_trapped =NULL;
    cJSON_AddItemToObject(sendor_3d,"wc_trapped",wc_trapped=cJSON_CreateObject());
    cJSON_AddBoolToObject(wc_trapped,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.on);
    cJSON_AddNumberToObject(wc_trapped,"time",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.time);

    //pet
    cJSON *petentry=NULL;
    cJSON_AddItemToObject(alarmsentry,"pet",petentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(petentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.on);
    cJSON_AddNumberToObject(petentry,"sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.sensitivity);
    cJSON *petzone=NULL;
    petzone= cJSON_AddArrayToObject(petentry,"zone");
    int pet_count=0;
    cJSON * petarea =NULL;
    cJSON_AddItemToArray(petzone,petarea=cJSON_CreateObject());
    cJSON_AddNumberToObject(petarea,"areaId",pet_count);
    cJSON *pet_pointarray=NULL;
    cJSON_AddItemToObject(petarea,"point",pet_pointarray=cJSON_CreateArray());
    cJSON *pet_pointentry = NULL;
    cJSON_AddItemToArray(pet_pointarray,pet_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pet_pointentry,"pointId",0);
    cJSON_AddNumberToObject(pet_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_bottom_x);
    cJSON_AddNumberToObject(pet_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_bottom_y);
    cJSON_AddItemToArray(pet_pointarray,pet_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pet_pointentry,"pointId",1);
    cJSON_AddNumberToObject(pet_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_x);
    cJSON_AddNumberToObject(pet_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_y);
    cJSON_AddItemToArray(pet_pointarray,pet_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pet_pointentry,"pointId",2);
    cJSON_AddNumberToObject(pet_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_up_x);
    cJSON_AddNumberToObject(pet_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_up_y);
    cJSON_AddItemToArray(pet_pointarray,pet_pointentry=cJSON_CreateObject());
    cJSON_AddNumberToObject(pet_pointentry,"pointId",3);
    cJSON_AddNumberToObject(pet_pointentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_x);
    cJSON_AddNumberToObject(pet_pointentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_y);


    return NULL;
}


int dutMTU=1500;


cJSON *OVD_DEMO_GetOVDChannelConfigureInfoJson(int channel)
{
    cJSON *video_encodingentry=NULL;
    cJSON *audio_encodingentry=NULL;
    cJSON *switchentry=NULL;
	cJSON *imageentry=NULL;

	cJSON *osdentry=NULL;
	cJSON *osdposentry=NULL;
	cJSON *osdcolorentry=NULL;
	cJSON *osdlogoentry=NULL;

    cJSON *dashcam_info_entry = NULL;
    cJSON *alarm_duration_entry = NULL;
    cJSON *driving_collision_entry = NULL;
    cJSON *voice_capture_entry = NULL;
    cJSON *osdtemp=NULL;

    cJSON *channelentry = cJSON_CreateObject();

    int i = channel;
    //cJSON_AddNumberToObject(channelentry,"channel",OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
    /*
    if(strlen(g_aispeech_config_str)!=0)
    {
        printf("get g_aispeech_config_str:%s\n",g_aispeech_config_str);
        cJSON *AISpeech=cJSON_Parse(g_aispeech_config_str);
        if(AISpeech)
        {
            cJSON_AddItemToObject(channelentry,"AISpeech",AISpeech);
        }
    }
    */

    printf("===wjj==get aispeech==status:%d plan:%d %s %s %d====\n",
        g_aispeech_status,g_aispeech_scheduleplan_on,g_aispeech_scheduleplan_startime,g_aispeech_scheduleplan_endtime,g_aispeech_scheduleplan_repeat);

    cJSON *aispeech_entry=NULL;
    cJSON_AddItemToObject(channelentry,"AISpeech",aispeech_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(aispeech_entry,"status",g_aispeech_status);
   
    cJSON *aispeech_scheduleplan_entry=NULL;
    cJSON_AddItemToObject(aispeech_entry,"scheduledplan",aispeech_scheduleplan_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(aispeech_scheduleplan_entry,"on",g_aispeech_scheduleplan_on);
    cJSON_AddStringToObject(aispeech_scheduleplan_entry,"starttime",g_aispeech_scheduleplan_startime);
    cJSON_AddStringToObject(aispeech_scheduleplan_entry,"endtime",g_aispeech_scheduleplan_endtime);
   
    OVD_DEMO_OVCGetRepeat(aispeech_scheduleplan_entry, g_aispeech_scheduleplan_repeat);

    cJSON_AddBoolToObject(channelentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].on);
    cJSON_AddNumberToObject(channelentry,"smart_mode_switch_2",OVD_DEMO_ConfigureInfo.channelsInfo[i].smart_mode_switch_2);


    cJSON_AddItemToObject(channelentry,"video_encoding",video_encodingentry=cJSON_CreateObject());

    //cJSON_AddStringToObject(video_encodingentry,"encoder",OVD_DEMO_ConfigureInfo->channelsInfo[i].videoinfo.codec);
    //当前只支持h264
    //cJSON_AddStringToObject(video_encodingentry,"encoder","h264");

    if (source_STREAM_CODEC_H265 == OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec)
    {
        cJSON_AddStringToObject(video_encodingentry,"encoder","h265");
    }else
    {
        cJSON_AddStringToObject(video_encodingentry,"encoder","h264");
    }
    cJSON_AddBoolToObject(video_encodingentry,"static_scence_ABR_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].static_scence_ABR_on);



    if(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_1DMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","ld");
    else if(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_SDMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","sd");
    else if(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_HDMODE )
        cJSON_AddStringToObject(video_encodingentry,"quality","hd");
    else if(OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality == OVD_FHDMODE)
        cJSON_AddStringToObject(video_encodingentry,"quality","fhd");

    cJSON_AddNumberToObject(video_encodingentry,"fps",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.framerate);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.bitrate);
    cJSON_AddNumberToObject(video_encodingentry,"width",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.width);
    cJSON_AddNumberToObject(video_encodingentry,"height",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.height);
    cJSON_AddNumberToObject(video_encodingentry,"gop",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval);

    //1.44 帧率、码率设置
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_mode);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_average",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_average);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_peak",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_peak);
    cJSON_AddNumberToObject(video_encodingentry,"bitrate_statically",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_statically);
    cJSON_AddNumberToObject(video_encodingentry,"framerate",OVD_DEMO_ConfigureInfo.channelsInfo[i].framerate);
    cJSON_AddNumberToObject(video_encodingentry,"gop",OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval);

    //cJSON_AddBoolToObject(video_encodingentry)

    //audio_encoding
    cJSON_AddItemToObject(channelentry,"audio_encoding",audio_encodingentry=cJSON_CreateObject());
    cJSON_AddStringToObject(audio_encodingentry,"encoder","aac");
    cJSON_AddNumberToObject(audio_encodingentry,"sample_rate",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.samplesRate);
    cJSON_AddNumberToObject(audio_encodingentry,"bitrate",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.bitrate);
    cJSON_AddNumberToObject(audio_encodingentry,"bits_per_sample",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.bitsPerSample);
    cJSON_AddNumberToObject(audio_encodingentry,"sample_per_frame",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.sampleperframe);
    cJSON_AddNumberToObject(audio_encodingentry,"channel",OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.channelNumber);

    // 是否有音柱设备
    cJSON_AddNumberToObject(channelentry, "Speeker", OVD_DEMO_ConfigureInfo.channelsInfo[i].has_speek);

    //image
    cJSON_AddItemToObject(channelentry,"image",imageentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(imageentry,"horflip",OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.horflip);
    cJSON_AddBoolToObject(imageentry,"verflip",OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.verflip);

    GetAISentry(channelentry,i);
    GetAlarmSentry(channelentry,i);

    //switch_schedule
    cJSON_AddItemToObject(channelentry,"switch_schedule",switchentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(switchentry,"on",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.on);
    cJSON_AddStringToObject(switchentry,"start_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.start_time);
    cJSON_AddStringToObject(switchentry,"shutdown_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.shutdown_time);
    int repeat =OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.repeat;
    OVD_DEMO_OVCGetRepeat(switchentry,repeat);
    //行车记录仪
    cJSON_AddItemToObject(channelentry,"driving_recorder",dashcam_info_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(dashcam_info_entry,"collision_alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.collision_alarm_sensitivity);
    cJSON_AddItemToObject(dashcam_info_entry,"alarm_duration",alarm_duration_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(alarm_duration_entry,"parking_notice",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_notice);
    cJSON_AddNumberToObject(alarm_duration_entry,"parking_collision",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_collision);
    cJSON_AddItemToObject(alarm_duration_entry,"driving_collision",driving_collision_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(driving_collision_entry,"before",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.before);
    cJSON_AddNumberToObject(driving_collision_entry,"after",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.after);
    cJSON_AddItemToObject(alarm_duration_entry,"voice_capture",voice_capture_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(voice_capture_entry,"before",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.before);
    cJSON_AddNumberToObject(voice_capture_entry,"after",OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.after);
    //osd
    cJSON_AddItemToObject(channelentry,"osd_text",osdtemp=cJSON_CreateArray());
    int osd_num=0;
    int osd_count=OVD_DEMO_ConfigureInfo.channelsInfo[i].osdcount;
    for (osd_num=0;osd_num<osd_count;osd_num++) {
        osdentry = cJSON_CreateObject();
        cJSON_AddBoolToObject(osdentry,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].on);
        cJSON_AddStringToObject(osdentry,"text",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].text);
        cJSON_AddNumberToObject(osdentry,"fontsize",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdfont);
        cJSON_AddNumberToObject(osdentry,"mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].mode);
        cJSON_AddNumberToObject(osdentry,"id",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].id);

        cJSON_AddItemToObject(osdentry,"pos",osdposentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(osdposentry,"x",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.x);
        cJSON_AddNumberToObject(osdposentry,"y",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.y);

        cJSON_AddItemToObject(osdentry,"color",osdcolorentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(osdcolorentry,"Red",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.red);
        cJSON_AddNumberToObject(osdcolorentry,"Green",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.green);
        cJSON_AddNumberToObject(osdcolorentry,"Blue",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.blue);
        cJSON_AddItemToArray(osdtemp, osdentry);
    }
    cJSON_AddNumberToObject(channelentry,"normal_nightvision_mode",OVD_DEMO_ConfigureInfo.normal_nightvision_mode);
    cJSON_AddNumberToObject(channelentry,"color_nightvision_mode",OVD_DEMO_ConfigureInfo.color_nightvision_mode);
    cJSON_AddNumberToObject(channelentry,"nightvision_detect_mode",OVD_DEMO_ConfigureInfo.nightvision_detect_mode);

    cJSON *fill_light_entry=NULL;
    cJSON_AddItemToObject(channelentry,"fill_light",fill_light_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(fill_light_entry,"on",OVD_DEMO_ConfigureInfo.fill_light.on);
    cJSON_AddNumberToObject(fill_light_entry,"brightness",OVD_DEMO_ConfigureInfo.fill_light.brightness);
    cJSON *fill_light_schedule_time=NULL;
    cJSON_AddItemToObject(fill_light_entry,"schedule_time",fill_light_schedule_time=cJSON_CreateObject());
    cJSON_AddBoolToObject(fill_light_schedule_time,"on",OVD_DEMO_ConfigureInfo.fill_light.schedule.on);
    cJSON_AddStringToObject(fill_light_schedule_time,"starttime",OVD_DEMO_ConfigureInfo.fill_light.schedule.starttime);
    cJSON_AddStringToObject(fill_light_schedule_time,"endtime",OVD_DEMO_ConfigureInfo.fill_light.schedule.endtime);
    int fill_light_repeat = OVD_DEMO_ConfigureInfo.fill_light.schedule.repeat;
    OVD_DEMO_OVCGetRepeat(fill_light_schedule_time, fill_light_repeat);

    cJSON_AddBoolToObject(channelentry,"AI_rules_mode_on",OVD_DEMO_ConfigureInfo.AI_rules_mode_on);

    cJSON_AddItemToObject(channelentry,"osd_logo",osdlogoentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(osdlogoentry,"on", OVD_DEMO_ConfigureInfo.channelsInfo[i].osd_logo);

    cJSON *channel_rebot=NULL;
    cJSON_AddItemToObject(channelentry,"robotKeeper",channel_rebot=cJSON_CreateObject());
    cJSON *channel_extra=NULL;
    cJSON_AddItemToObject(channel_rebot,"extraInfo",channel_extra=cJSON_CreateObject());
    cJSON *channel_remind=NULL;
    cJSON_AddItemToObject(channel_extra,"remindConfig",channel_remind=cJSON_CreateArray());
    cJSON *pointentry=NULL;
    int rebot_num = 0;
    for(rebot_num=0;rebot_num<OVD_DEMO_ConfigureInfo.robotcount;rebot_num++){

        cJSON_AddItemToArray(channel_remind,pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(pointentry,"bizType",OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].bizType);
        cJSON_AddStringToObject(pointentry,"startTime",OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].start_time);
        cJSON_AddStringToObject(pointentry,"endTime",OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].end_time);
    }

    return channelentry;
}

int OVD_DEMO_GetOVDChannelConfigureInfo(int channel, char** output_ovdconfig, int *len)
{
    int ret;
    cJSON *channelentry = OVD_DEMO_GetOVDChannelConfigureInfoJson(channel);
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

int OVD_DEMO_GetOVDConfigureInfo(char** output_ovdconfig, int *len){
   //服务器来这边取
    //给我一个json序列化的代码
    //OVDConfigrationInfo OVD_DEMO_ConfigureInfo;

   //in_size 用于厂商检验字符串是否超出范围；

    cJSON *root= cJSON_CreateObject();
    cJSON *channelsarray=NULL;
    cJSON *channelentry=NULL;
    cJSON *screenentry=NULL;
    //cJSON *resp=NULL ;

    cJSON *auto_rebootentry=NULL;
    cJSON *yard_light_config_entry=NULL;
    cJSON *EBO_config_entry=NULL;
    cJSON *rebot_ctrl_entry=NULL;
    cJSON *OWL_config_cruise_entry=NULL;
    cJSON *EBO_config_cruise_entry=NULL;
    cJSON *dashcam_entry=NULL;
    cJSON *timed_schedule_entry=NULL;
	cJSON *support_softprobentry=NULL;
    cJSON *id=NULL;
    char *data=NULL;
    char *json_buffer = NULL;
    int ret;
    int i=0;
	//OVD_DEMOE_InitSetableKeys();
    cJSON_AddItemToObject(root,"channels",channelsarray=cJSON_CreateObject());
	cJSON_AddBoolToObject(root,"led",OVD_DEMO_ConfigureInfo.led);
    cJSON_AddItemToObject(root,"screen",screenentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(screenentry,"on",OVD_DEMO_ConfigureInfo.screen);

    cJSON_AddBoolToObject(root,"light_supplement_lamp_on",OVD_DEMO_ConfigureInfo.light_supplement_lamp_on);
    cJSON_AddNumberToObject(root,"dutMTU",dutMTU);

    
	cJSON_AddItemToObject(root,"auto_reboot",auto_rebootentry=cJSON_CreateObject());
    cJSON_AddBoolToObject(auto_rebootentry,"on",OVD_DEMO_ConfigureInfo.autorebootinfo.on);
    cJSON_AddNumberToObject(auto_rebootentry,"cycle",OVD_DEMO_ConfigureInfo.autorebootinfo.cycle);
    cJSON_AddStringToObject(auto_rebootentry,"start",OVD_DEMO_ConfigureInfo.autorebootinfo.start);
    cJSON_AddStringToObject(auto_rebootentry,"end",OVD_DEMO_ConfigureInfo.autorebootinfo.end);
	cJSON_AddStringToObject(auto_rebootentry,"last_reboot",OVD_DEMO_ConfigureInfo.autorebootinfo.last_reboot);
    //庭院灯
    cJSON_AddItemToObject(root,"yard_light_config",yard_light_config_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(yard_light_config_entry,"light_brightness",OVD_DEMO_ConfigureInfo.yardlightconfig.light_brightness);
    cJSON_AddNumberToObject(yard_light_config_entry,"control_mode",OVD_DEMO_ConfigureInfo.yardlightconfig.control_mode);
    cJSON_AddBoolToObject(yard_light_config_entry,"light_on",OVD_DEMO_ConfigureInfo.yardlightconfig.light_on);
    cJSON_AddItemToObject(yard_light_config_entry,"timed_schedule",timed_schedule_entry=cJSON_CreateArray());
    int timed_schedule_num = 0;
    int timed_schedule_size = 0;
    cJSON *timed_schedule_tmp_entry = NULL;
    if(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule_size){
        timed_schedule_size=OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule_size;
    }else{
        timed_schedule_size=2;
    }
    for(timed_schedule_num=0;timed_schedule_num < timed_schedule_size; timed_schedule_num++){
        timed_schedule_tmp_entry = cJSON_CreateObject();
        cJSON_AddStringToObject(timed_schedule_tmp_entry,"start_time",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[timed_schedule_num].start_time);
        cJSON_AddStringToObject(timed_schedule_tmp_entry,"end_time",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[timed_schedule_num].end_time);
        cJSON_AddItemToArray(timed_schedule_entry, timed_schedule_tmp_entry);
    }
    cJSON_AddItemToObject(root,"EBO",EBO_config_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(EBO_config_entry,"EBO_nofall_on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on);
    cJSON_AddBoolToObject(EBO_config_entry,"EBO_laser_on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_laser_on);
    cJSON_AddBoolToObject(EBO_config_entry,"EBO_track_on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_on);
    cJSON_AddNumberToObject(EBO_config_entry,"EBO_track_type",OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_type);
    cJSON_AddBoolToObject(EBO_config_entry,"EBO_collision_on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_collision_on);
    cJSON_AddBoolToObject(EBO_config_entry,"EBO_reminder_on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_reminder_on);
    cJSON_AddNumberToObject(EBO_config_entry,"EBO_speed_adjust",OVD_DEMO_ConfigureInfo.eboconfig.EBO_speed_adjust);
    cJSON_AddItemToObject(EBO_config_entry,"EBO_cruise_on",EBO_config_cruise_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(EBO_config_cruise_entry,"on",OVD_DEMO_ConfigureInfo.eboconfig.EBO_cruise_on);
    cJSON_AddStringToObject(EBO_config_cruise_entry,"start_time",OVD_DEMO_ConfigureInfo.eboconfig.start_time);
    cJSON_AddNumberToObject(EBO_config_cruise_entry,"time_long",OVD_DEMO_ConfigureInfo.eboconfig.time_long);
	OVD_DEMO_OVCGetRepeat(EBO_config_cruise_entry,OVD_DEMO_ConfigureInfo.eboconfig.repeat);

    cJSON_AddItemToObject(root,"OWL",rebot_ctrl_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(rebot_ctrl_entry,"OWL_food_on",OVD_DEMO_ConfigureInfo.robot_config.OWL_food_on);
    cJSON_AddBoolToObject(rebot_ctrl_entry,"OWL_track_mode",OVD_DEMO_ConfigureInfo.robot_config.OWL_track_mode);
    cJSON_AddBoolToObject(rebot_ctrl_entry,"OWL_touch_mode",OVD_DEMO_ConfigureInfo.robot_config.OWL_touch_mode);
    cJSON_AddBoolToObject(rebot_ctrl_entry,"OWL_sound_on",OVD_DEMO_ConfigureInfo.robot_config.OWL_sound_on);
    cJSON_AddNumberToObject(rebot_ctrl_entry,"OWL_speed_switch",OVD_DEMO_ConfigureInfo.robot_config.OWL_speed_switch);
    cJSON_AddItemToObject(rebot_ctrl_entry,"OWL_cruise_on",OWL_config_cruise_entry=cJSON_CreateObject());
    cJSON_AddStringToObject(OWL_config_cruise_entry,"start_time",OVD_DEMO_ConfigureInfo.robot_config.start_time);
	OVD_DEMO_OVCGetRepeat(OWL_config_cruise_entry,OVD_DEMO_ConfigureInfo.robot_config.repeat);
    //可移动摄像头
    cJSON *mobocam_entry=NULL;
    cJSON_AddItemToObject(root,"mobocam",mobocam_entry=cJSON_CreateObject());
    //可移动摄像头-速度调节
    cJSON *speed_entry=NULL;
    cJSON_AddItemToObject(mobocam_entry,"speedSwitch",speed_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(speed_entry,"cmd",OVD_DEMO_ConfigureInfo.mobocam_config.speedswith);
    //可移动摄像头-自动巡航
    cJSON *channel_cruise=NULL;
    cJSON_AddItemToObject(mobocam_entry,"cruiseControl",channel_cruise=cJSON_CreateArray());
    cJSON *cruise_pointentry=NULL;
    int robot_num = 0;
    for(robot_num=0;robot_num< 3 ;robot_num++){

        cJSON_AddItemToArray(channel_cruise,cruise_pointentry=cJSON_CreateObject());
        cJSON_AddNumberToObject(cruise_pointentry,"cruiseId",OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].cruiseId);
        cJSON_AddNumberToObject(cruise_pointentry,"time_long",OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].time_long);
        cJSON_AddStringToObject(cruise_pointentry,"start_time",OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].start_time);
        cJSON_AddStringToObject(cruise_pointentry,"end_time",OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].end_time);
        OVD_DEMO_OVCGetRepeat(cruise_pointentry,OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].repeat);
    }
    //可移动摄像头-自动追踪
    cJSON *track_entry=NULL;
    cJSON_AddItemToObject(mobocam_entry,"track",track_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(track_entry,"on",OVD_DEMO_ConfigureInfo.mobocam_config.track_on);
    cJSON_AddNumberToObject(track_entry,"type",OVD_DEMO_ConfigureInfo.mobocam_config.track_type);
    

    //1.45 行车记录仪
    cJSON_AddItemToObject(root,"dashcam",dashcam_entry=cJSON_CreateObject());
    cJSON_AddBoolToObject(dashcam_entry,"dashcam_findcar_on",OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_findcar_on);
    cJSON_AddBoolToObject(dashcam_entry,"dashcam_rescue_on",OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_rescue_on);


    cJSON *feeder_config_entry=NULL;
    cJSON *feeder_immed_config_entry=NULL;
    cJSON_AddItemToObject(root,"petfeeder",feeder_config_entry=cJSON_CreateObject());
    cJSON_AddItemToObject(feeder_config_entry,"feeder_immed",feeder_immed_config_entry=cJSON_CreateObject());
    cJSON_AddNumberToObject(feeder_immed_config_entry,"feeder_quantity",OVD_DEMO_ConfigureInfo.feeder_immed_quantity);
    cJSON *feeder_plan_config_entry=NULL;
    cJSON_AddItemToObject(feeder_config_entry,"feeder_plan",feeder_plan_config_entry=cJSON_CreateArray());
    cJSON *pointentry=NULL;
    int feeder_num = 0;
    int feeder_count = OVD_DEMO_ConfigureInfo.feedercount;
    for(feeder_num=0;feeder_num<feeder_count;feeder_num++){

        cJSON_AddItemToArray(feeder_plan_config_entry,pointentry=cJSON_CreateObject());
        cJSON_AddBoolToObject(pointentry,"on",OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].on);
        cJSON_AddNumberToObject(pointentry,"feeder_quantity",OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].quantity);
        cJSON_AddStringToObject(pointentry,"feeder_name",OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name);
        cJSON_AddStringToObject(pointentry,"feeder_time",OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time);
        OVD_DEMO_OVCGetRepeat(pointentry,OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].repeat);
    }

    // 枪球联动
    cJSON *linkage_tracking_entry = NULL;
    cJSON_AddItemToObject(root, "linkage_tracking", linkage_tracking_entry = cJSON_CreateObject());
    cJSON *linkage_tracking_smart_entry = NULL;
    cJSON_AddItemToObject(linkage_tracking_entry, "linkage_tracking_smart", linkage_tracking_smart_entry = cJSON_CreateObject());
    cJSON_AddBoolToObject(linkage_tracking_smart_entry, "on", OVD_DEMO_ConfigureInfo.linkage_tracking_smart_on);
    cJSON *linkage_tracking_manual_entry = NULL;
    cJSON_AddItemToObject(linkage_tracking_entry, "linkage_tracking_manual", linkage_tracking_manual_entry = cJSON_CreateObject());
    cJSON_AddBoolToObject(linkage_tracking_manual_entry, "on", OVD_DEMO_ConfigureInfo.linkage_tracking_manual_on);

    //ovd_config
    for(i=0;i< OVD_DEMO_ConfigureInfo.channelsInfoCount;i++)
    {
        cJSON *channelentry = OVD_DEMO_GetOVDChannelConfigureInfoJson(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
        if (channelentry != NULL)
        {
            char channel_str[4];
            sprintf(channel_str,"%d", OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
            cJSON_AddItemToObject(channelsarray, channel_str, channelentry);
        }

	}

    cJSON_AddNumberToObject(root,"tz",OVD_DEMO_ConfigureInfo.tz);
    int buf_size = OVD_MAX_JSON_REND_SIZE * OVD_DEMO_ConfigureInfo.channelsInfoCount;
    json_buffer = (char *)malloc(buf_size);
    ret = cJSON_PrintPreallocated(root, json_buffer, buf_size, 0);
    if (ret != 1)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Rend json object failed, maybe buffer is limited, current buffer size %d\n", buf_size);
        free(json_buffer);
        cJSON_Delete(root);
        *output_ovdconfig = NULL;
        return -1;
    }
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","output_ovdconfig:%s\n" ,json_buffer);
    cJSON_Delete(root);
    *len = strlen(json_buffer) + sizeof("");
    *output_ovdconfig = json_buffer;

    return 0;
}

//off duty
static cJSON* SetAIOffDutyEntry(cJSON* AI_entry, int i)
{
    int rv = 0;
    cJSON *item = NULL;

    cJSON* AI_off_duty_detection = cJSON_GetObjectItem(AI_entry,"off_duty_detection");
    if (AI_off_duty_detection!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        if (!cJSON_IsObject(AI_off_duty_detection))
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
            return NULL;
        }

        cJSON * AI_off_duty_detection_on = cJSON_GetObjectItem(AI_off_duty_detection, "detect_on");
        if(AI_off_duty_detection_on!=NULL)
        {
            if (!cJSON_IsBool(AI_off_duty_detection_on))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            int onValue=AI_off_duty_detection_on->valueint;

            //下发配置
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.on = onValue;
        }

        cJSON *AI_off_duty_detect_plans=cJSON_GetObjectItem(AI_off_duty_detection,"detect_plans");
        if(AI_off_duty_detect_plans !=NULL)
        {
            OVDDemoAIOffDutyPlan detePlan;
            memset(&detePlan, 0, sizeof(OVDDemoAIOffDutyPlan));

            if(cJSON_Array != AI_off_duty_detect_plans->type)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            int plan_nums = cJSON_GetArraySize(AI_off_duty_detect_plans);

            if (1!=plan_nums)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","plan_nums[%d] error!\n", plan_nums);
                return NULL;
            }

            cJSON * detect_plan_0 = cJSON_GetArrayItem(AI_off_duty_detect_plans, 0);
            if(detect_plan_0 !=NULL)
            {
                if (!cJSON_IsObject(detect_plan_0))
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }

                cJSON *AI_on=cJSON_GetObjectItem(detect_plan_0,"on");
                if(AI_on!=NULL)
                {
                    if (!cJSON_IsBool(AI_on))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.on=AI_on->valueint;
                }

                cJSON *AI_regional_id=cJSON_GetObjectItem(detect_plan_0,"regional_id");
                if(AI_regional_id !=NULL)
                {
                    if(cJSON_Number != AI_regional_id->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }



                    detePlan.regional_id=AI_regional_id->valueint;

                    if (0!=detePlan.regional_id)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_id[%d] error\n", detePlan.regional_id);
                        return NULL;
                    }

                }

                cJSON *AI_regional_name=cJSON_GetObjectItem(detect_plan_0,"regional_name");
                if(AI_regional_name !=NULL)
                {
                    if(cJSON_String == AI_regional_name->type)
                    {
                        memset(detePlan.regional_name, 0, sizeof(detePlan.regional_name));
                        strncpy(detePlan.regional_name, AI_regional_name->valuestring,sizeof(detePlan.regional_name)-1);

                        if (strlen(detePlan.regional_name)>127)
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_name[%s] exceed the length[%d] limited!\n", detePlan.regional_name, strlen(detePlan.regional_name));
                            return NULL;

                        }

                        if ((strstr(detePlan.regional_name, "~"))
                            ||(strstr(detePlan.regional_name, "!"))
                            ||(strstr(detePlan.regional_name, "@"))
                            ||(strstr(detePlan.regional_name, "#"))
                            ||(strstr(detePlan.regional_name, "$"))
                            ||(strstr(detePlan.regional_name, "%"))
                            ||(strstr(detePlan.regional_name, "^"))
                            ||(strstr(detePlan.regional_name, "&"))
                            ||(strstr(detePlan.regional_name, "*")))
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_name[%s] include special label!\n", detePlan.regional_name);
                            return NULL;
                        }

                        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","regional_name[%s]\n", detePlan.regional_name);

                    }
                    else
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                }

                cJSON *AI_on_duty_count=cJSON_GetObjectItem(detect_plan_0,"on_duty_count");
                if(AI_on_duty_count!=NULL)
                {
                    if(cJSON_Number != AI_on_duty_count->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.on_duty_people_count=AI_on_duty_count->valueint;
                    if ((detePlan.on_duty_people_count < 1) || (detePlan.on_duty_people_count > OVD_DEMO_CapInfoV2.ai_offduty.on_duty_count))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","on_duty_count[%d] exceed the limited!\n", detePlan.on_duty_people_count);
                        return NULL;
                    }

                }

                cJSON *AI_off_duty_duration=cJSON_GetObjectItem(detect_plan_0,"off_duty_duration");
                if(AI_off_duty_duration!=NULL)
                {
                    if(cJSON_Number != AI_off_duty_duration->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.off_duty_duration=AI_off_duty_duration->valueint;
                    if ((detePlan.off_duty_duration < 1) || (detePlan.off_duty_duration > OVD_DEMO_CapInfoV2.ai_offduty.off_duty_durtion))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","off_duty_duration[%d] exceed the limited!\n", detePlan.off_duty_duration);
                        return NULL;
                    }

                }

                cJSON *maskDeteTime=cJSON_GetObjectItem(detect_plan_0,"alerttime");
                if(maskDeteTime!=NULL)
                {
                    item=cJSON_GetObjectItem(maskDeteTime,"on");
                    if(item!=NULL)
                    {
                        if (!cJSON_IsBool(item))
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                            return NULL;
                        }
                        detePlan.schedule.on=item->valueint;

                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"starttime");
                    if(item!=NULL)
                    {
                        if(cJSON_String == item->type)
                        {
                            memset(detePlan.schedule.starttime, 0, sizeof(detePlan.schedule.starttime));
                            strncpy(detePlan.schedule.starttime,item->valuestring, sizeof(detePlan.schedule.starttime)-1);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }

                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"endtime");
                    if(item!=NULL)
                    {
                        if(cJSON_String == item->type)
                        {
                            memset(detePlan.schedule.endtime, 0, sizeof(detePlan.schedule.endtime));
                            strncpy(detePlan.schedule.endtime,item->valuestring, sizeof(detePlan.schedule.endtime)-1);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }
                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"repeat");
                    if(item!=NULL)
                    {
                        if(cJSON_Array == item->type)
                        {
                            detePlan.schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }

                    }

                }

                cJSON *AI_alarm_zone=cJSON_GetObjectItem(detect_plan_0,"alarm_zone");
                if(AI_alarm_zone!=NULL)
                {
                    int zone_size = cJSON_GetArraySize(AI_alarm_zone);

                    if (1!=zone_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","zone size[%d] error!\n", zone_size);
                        return NULL;
                    }
                    //有多少个区域要记录到本地
                    int zone = 0;
                    for(zone = 0;zone < zone_size ;zone++)
                    {

                        cJSON * area = cJSON_GetArrayItem(AI_alarm_zone, zone);
                        cJSON * pointCjson = cJSON_GetObjectItem(area,"point");
                        if(pointCjson!=NULL)
                        {
                            int point_num = cJSON_GetArraySize(pointCjson);

                            if (4!=point_num)
                            {
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","point_num [%d] error!\n", point_num);
                                return NULL;
                            }

                            //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                            cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
                            cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
                            cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
                            cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

                            //获取这两个点的具体x，y坐标值
                            cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
                            cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
                            cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
                            cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
                            cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
                            cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
                            cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
                            cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

                            detePlan.zoneInfo.left_up_x = point_left_up_x->valueint;
                            detePlan.zoneInfo.left_up_y = point_left_up_y->valueint;
                            detePlan.zoneInfo.left_bottom_x = point_left_bottom_x->valueint;
                            detePlan.zoneInfo.left_bottom_y = point_left_bottom_y->valueint;
                            detePlan.zoneInfo.right_bottom_x = point_right_bottom_x->valueint;
                            detePlan.zoneInfo.right_bottom_y = point_right_bottom_y->valueint;
                            detePlan.zoneInfo.right_up_x = point_right_up_x->valueint;
                            detePlan.zoneInfo.right_up_y = point_right_up_y->valueint;

                        }

                    }
                }


                 //下发配置
                 memcpy(&(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIOffDuty.detect_plan), &detePlan, sizeof(OVDDemoAIOffDutyPlan));

            }

        }
    }
    return NULL;
}

//头盔检测
static cJSON* SetAIHelmetEntry(cJSON* AI_entry, int i)
{
    cJSON *item=NULL;

    OVDDemoAIHelmet HelmetDetect;
    memset(&HelmetDetect, 0, sizeof(OVDDemoAIHelmet));

    cJSON* AI_helmet_wearing_detection = cJSON_GetObjectItem(AI_entry,"helmet_wearing_detection");
    if (AI_helmet_wearing_detection!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        if (!cJSON_IsObject(AI_helmet_wearing_detection))
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
            return NULL;
        }

        cJSON * AI_helmet_wearing_detection_on = cJSON_GetObjectItem(AI_helmet_wearing_detection, "detect_on");
        if(AI_helmet_wearing_detection_on!=NULL)
        {
            if (!cJSON_IsBool(AI_helmet_wearing_detection_on))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.on =AI_helmet_wearing_detection_on->valueint;
        }

        cJSON *linkage_strategy=cJSON_GetObjectItem(AI_helmet_wearing_detection,"linkage_strategy");
        if(linkage_strategy!=NULL)
        {
            cJSON *maskDeteSpeech=cJSON_GetObjectItem(linkage_strategy,"speech");
            if(maskDeteSpeech!=NULL)
            {
                item=cJSON_GetObjectItem(maskDeteSpeech,"on");
                if(item!=NULL)
                {
                    if (!cJSON_IsBool(item))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_on=item->valueint;
                }
                item=cJSON_GetObjectItem(maskDeteSpeech,"vol");
                if(item!=NULL)
                {
                    if(cJSON_Number == item->type)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_vol=item->valueint;
                    }
                    else
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
                    }

                }
                item=cJSON_GetObjectItem(maskDeteSpeech,"repeat");
                if(item!=NULL)
                {
                    if(cJSON_Number == item->type)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat=item->valueint;
                    }
                    else
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
                    }
                }
                item=cJSON_GetObjectItem(maskDeteSpeech,"repeat_constant");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat_constant=item->valueint;
                }
                item=cJSON_GetObjectItem(maskDeteSpeech,"interval");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_interval=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteSpeech,"url");
                if(item!=NULL)
                {

                    if(cJSON_String == item->type)
                    {
                        memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_url, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_url));
                        strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_url,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_url)-1);
                    }
                    else
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
                    }

                }

            }
            cJSON *maskDeteLight=cJSON_GetObjectItem(linkage_strategy,"light");
            if(maskDeteLight!=NULL)
            {
                item=cJSON_GetObjectItem(maskDeteLight,"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_on=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteLight,"mode");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_mode=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteLight,"dur");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur=item->valueint;
                }
                item=cJSON_GetObjectItem(maskDeteLight,"dur_constant");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur_constant=item->valueint;
                }
            }

            cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
            if(maskDeteBuz!=NULL)
            {
                item=cJSON_GetObjectItem(maskDeteBuz,"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_on=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_dur=item->valueint;
                }
            }
        }

        cJSON *maskDeteTime=cJSON_GetObjectItem(AI_helmet_wearing_detection,"detect_time");
        if(maskDeteTime!=NULL)
        {
            item=cJSON_GetObjectItem(maskDeteTime,"on");
            if(item!=NULL)
            {
                if (!cJSON_IsBool(item))
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.on=item->valueint;

            }

            item=cJSON_GetObjectItem(maskDeteTime,"starttime");
            if(item!=NULL)
            {
                if(cJSON_String == item->type)
                {
                    memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime));
                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime)-1);
                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                    return NULL;
                }

            }

            item=cJSON_GetObjectItem(maskDeteTime,"endtime");
            if(item!=NULL)
            {
                if(cJSON_String == item->type)
                {
                    memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime));
                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime)-1);
                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                    return NULL;
                }
            }

            item=cJSON_GetObjectItem(maskDeteTime,"repeat");
            if(item!=NULL)
            {
                if(cJSON_Array == item->type)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                    return NULL;
                }

            }

        }

        cJSON * pointCjson = cJSON_GetObjectItem(AI_helmet_wearing_detection,"point");
        if(pointCjson!=NULL)
        {
            int point_num = cJSON_GetArraySize(pointCjson);

            if (4!=point_num)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","point_num [%d] error!\n", point_num);
                return NULL;
            }

            //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
            cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
            cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
            cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
            cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

            //获取这两个点的具体x，y坐标值
            cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
            cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
            cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
            cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
            cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
            cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
            cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
            cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_x = point_left_up_x->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_y = point_left_up_y->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_x = point_left_bottom_x->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_y = point_left_bottom_y->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_x = point_right_bottom_x->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_y = point_right_bottom_y->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_x = point_right_up_x->valueint;
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_y = point_right_up_y->valueint;
        }

    }
    return NULL;

}

// 烟火检测
static cJSON* SetAiSmokefireEntry(cJSON* AI_entry, int i){
    OVDDemoAISmokefire *smokefire = &OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire;
    cJSON *AI_smokefile = cJSON_GetObjectItem(AI_entry,"smoke_fire_detection");
    if(AI_smokefile != NULL){
        cJSON *smokefile_on = cJSON_GetObjectItem(AI_smokefile,"detect_on");
        if(smokefile_on != NULL){
            smokefire->on = smokefile_on->valueint;
        }

        cJSON *maskDeteTime=cJSON_GetObjectItem(AI_smokefile,"detect_time");
        if(maskDeteTime!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(maskDeteTime,"on");
            if(item!=NULL)
            {
                smokefire->schedule.on=item->valueint;
            }

            item=cJSON_GetObjectItem(maskDeteTime,"starttime");
            if(item!=NULL)
            {
                memset(smokefire->schedule.starttime, 0, sizeof(smokefire->schedule.starttime));
                strncpy(smokefire->schedule.starttime, item->valuestring, sizeof(smokefire->schedule.starttime)-1);
            }

            item=cJSON_GetObjectItem(maskDeteTime,"endtime");
            if(item!=NULL)
            {
                memset(smokefire->schedule.endtime, 0, sizeof(smokefire->schedule.endtime));
                strncpy(smokefire->schedule.endtime, item->valuestring, sizeof(smokefire->schedule.endtime)-1);
            }

            item=cJSON_GetObjectItem(maskDeteTime,"repeat");
            if(item!=NULL)
            {
                smokefire->schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
            }
        }

        int i;
        cJSON *detect_zone_array = cJSON_GetObjectItem(AI_smokefile,"zone");
        if(detect_zone_array != NULL && cJSON_GetArraySize(detect_zone_array) <= smokefire->areas->areasNum){
            for (i = 0; i< cJSON_GetArraySize(detect_zone_array); i++){
                cJSON *detect_zone = cJSON_GetArrayItem(detect_zone_array, i);
                if(detect_zone != NULL)
                {
                    OVDSingleAreaInfo *single_area = NULL;
                    cJSON *areaId = cJSON_GetObjectItem(detect_zone,"areaId");
                    if(areaId != NULL && areaId->valueint < smokefire->areas->areasNum){
                        single_area = &smokefire->areas->areas[areaId->valueint];
                    }else{
                        continue;
                    }

                    int j;
                    cJSON *point_array = cJSON_GetObjectItem(detect_zone,"point");
                    if(point_array != NULL && cJSON_GetArraySize(point_array) <= single_area->areaPoints->pointsNum){
                        for (j=0;j<cJSON_GetArraySize(point_array);j++){
                            cJSON *point = cJSON_GetArrayItem(point_array,j);
                            if(point != NULL){
                                OVDSinglePointInfo *single_point = NULL;

                                cJSON *pointId = cJSON_GetObjectItem(point,"pointId");
                                cJSON *point_x = cJSON_GetObjectItem(point,"x");
                                cJSON *point_y = cJSON_GetObjectItem(point,"y");
                                if(pointId != NULL && pointId->valueint < single_area->areaPoints->pointsNum){
                                    single_point = &single_area->areaPoints->points[pointId->valueint];
                                }
                                else {
                                    continue;
                                }
                                if (point_x != NULL){
                                    single_point->point_x = point_x->valueint;
                                }
                                if (point_y != NULL){
                                    single_point->point_y = point_y->valueint;
                                }
                            }
                        }

                    }
                }
            }
        }

        cJSON *linkage_strategy = cJSON_GetObjectItem(AI_smokefile,"linkage_strategy");
        if(linkage_strategy!=NULL)
        {
            cJSON *speech=cJSON_GetObjectItem(linkage_strategy,"speech");
            if(speech!=NULL)
            {
                cJSON *item = cJSON_GetObjectItem(speech,"on");
                if(item!=NULL){
                    smokefire->linkage.speech_on=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"vol");
                if(item!=NULL){
                    smokefire->linkage.speech_vol=item->valueint;
                }
                item=cJSON_GetObjectItem(speech,"repeat");
                if(item!=NULL) {
                    smokefire->linkage.speech_repeat=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"interval");
                if(item!=NULL){
                    smokefire->linkage.speech_interval=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"url");
                if(item!=NULL){
                    memset(smokefire->linkage.speech_url, 0, sizeof(smokefire->linkage.speech_url));
                    strncpy(smokefire->linkage.speech_url, item->valuestring, sizeof(smokefire->linkage.speech_url)-1);
                }

            }

            cJSON *light=cJSON_GetObjectItem(linkage_strategy,"light");
            if(light!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(light,"on");
                if(item!=NULL) {
                    smokefire->linkage.light_on=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"mode");
                if(item!=NULL) {
                    smokefire->linkage.light_mode=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"dur");
                if(item!=NULL){
                    smokefire->linkage.light_dur=item->valueint;
                }
            }

            cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
            if(maskDeteBuz!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(maskDeteBuz,"on");
                if(item!=NULL) {
                    smokefire->linkage.buz_on=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                if(item!=NULL) {
                    smokefire->linkage.buz_dur=item->valueint;
                }
            }
        }
    }
    return NULL;
}

//手势识别
static cJSON* SetAIGestureEntry(cJSON* AI_entry, int i)
{
    cJSON *item=NULL;

    OVDDemoAIGesture GestureRecognition;
    memset(&GestureRecognition, 0, sizeof(OVDDemoAIGesture));

    cJSON* AI_gesture_recognition = cJSON_GetObjectItem(AI_entry,"gesture_recognition");
    if (AI_gesture_recognition!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        if (!cJSON_IsObject(AI_gesture_recognition))
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
            return NULL;
        }

        cJSON * AI_gesture_recognition_on = cJSON_GetObjectItem(AI_gesture_recognition, "on");
        if(AI_gesture_recognition_on!=NULL)
        {
            if (!cJSON_IsBool(AI_gesture_recognition_on))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.on =AI_gesture_recognition_on->valueint;
        }

        cJSON * AI_gesture_call_on = cJSON_GetObjectItem(AI_gesture_recognition, "gesture_call_on");
        if(AI_gesture_call_on!=NULL)
        {
            if (cJSON_IsBool(AI_gesture_call_on))
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_on =AI_gesture_call_on->valueint;
            }

        }

        cJSON * AI_gesture_call_duration = cJSON_GetObjectItem(AI_gesture_recognition, "gesture_call_duration");
        if(AI_gesture_call_duration!=NULL)
        {
            if (cJSON_IsNumber(AI_gesture_call_duration))
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_duration = AI_gesture_call_duration->valueint;
            }

        }

        cJSON* AI_gesture_call_switch_schedule = cJSON_GetObjectItem(AI_gesture_recognition,"gesture_call_switch_schedule");
        if(AI_gesture_call_switch_schedule!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(AI_gesture_call_switch_schedule,"on");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.on=item->valueint;
            }

            item=cJSON_GetObjectItem(AI_gesture_call_switch_schedule,"starttime");
            if(item!=NULL)
            {
                memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime));
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime)-1);
            }

            item=cJSON_GetObjectItem(AI_gesture_call_switch_schedule,"endtime");
            if(item!=NULL)
            {
                memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime));
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime)-1);
            }

            item=cJSON_GetObjectItem(AI_gesture_call_switch_schedule,"repeat");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
            }
        }

        cJSON * AI_gesture_help_on = cJSON_GetObjectItem(AI_gesture_recognition, "gesture_help_on");
        if(AI_gesture_help_on!=NULL)
        {
            if (cJSON_IsBool(AI_gesture_help_on))
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.help_on =AI_gesture_help_on->valueint;
            }

        }
    }
}

//家人识别
static cJSON* SetAiFamilyrecognitionEntry(cJSON* AI_entry, int i)
{
    OVDDemoAIFamilyRecognition *family = &OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition;
    cJSON *AI_family = cJSON_GetObjectItem(AI_entry,"family_recognition");
    if(AI_family != NULL){
        cJSON *on = cJSON_GetObjectItem(AI_family,"on");
        if(on != NULL){
            family->on = on->valueint;
        }

        int i;
        cJSON *face_array = cJSON_GetObjectItem(AI_family,"face_management");
        if(face_array != NULL && cJSON_GetArraySize(face_array) <= family->face_management.facesNum)
        {
            for (i = 0; i< cJSON_GetArraySize(face_array); i++)
            {
                cJSON *face_info = cJSON_GetArrayItem(face_array, i);
                if(face_info != NULL)
                {
                    cJSON *name=cJSON_GetObjectItem(face_info,"name");
                    if(name != NULL)
                    {
                        memset(family->face_management.face_info[i].name, 0, sizeof(family->face_management.face_info[i].name));
                        strncpy(family->face_management.face_info[i].name, name->valuestring, sizeof(family->face_management.face_info[i].name)-1);
                    }
                    cJSON *url=cJSON_GetObjectItem(face_info,"url");
                    if(url != NULL)
                    {
                        memset(family->face_management.face_info[i].url, 0, sizeof(family->face_management.face_info[i].url));
                        strncpy(family->face_management.face_info[i].url, url->valuestring, sizeof(family->face_management.face_info[i].url)-1);
                    }
                }
            }
        }

        cJSON *maskDeteTime=cJSON_GetObjectItem(AI_family,"schedule_time");
        if(maskDeteTime!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(maskDeteTime,"on");
            if(item!=NULL)
            {
                family->schedule.on=item->valueint;
            }

            item=cJSON_GetObjectItem(maskDeteTime,"starttime");
            if(item!=NULL)
            {
                memset(family->schedule.starttime, 0, sizeof(family->schedule.starttime));
                strncpy(family->schedule.starttime, item->valuestring, sizeof(family->schedule.starttime)-1);
            }

            item=cJSON_GetObjectItem(maskDeteTime,"endtime");
            if(item!=NULL)
            {
                memset(family->schedule.endtime, 0, sizeof(family->schedule.endtime));
                strncpy(family->schedule.endtime, item->valuestring, sizeof(family->schedule.endtime)-1);
            }

            item=cJSON_GetObjectItem(maskDeteTime,"repeat");
            if(item!=NULL)
            {
                family->schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
            }
        }

        cJSON *linkage_strategy = cJSON_GetObjectItem(AI_family,"linkage_strategy");
        if(linkage_strategy!=NULL)
        {
            cJSON *speech=cJSON_GetObjectItem(linkage_strategy,"speech");
            if(speech!=NULL)
            {
                cJSON *item = cJSON_GetObjectItem(speech,"on");
                if(item!=NULL){
                    family->linkage.speech_on=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"vol");
                if(item!=NULL){
                    family->linkage.speech_vol=item->valueint;
                }
                item=cJSON_GetObjectItem(speech,"repeat");
                if(item!=NULL) {
                    family->linkage.speech_repeat=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"interval");
                if(item!=NULL){
                    family->linkage.speech_interval=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"url");
                if(item!=NULL){
                    memset(family->linkage.speech_url, 0, sizeof(family->linkage.speech_url));
                    strncpy(family->linkage.speech_url, item->valuestring, sizeof(family->linkage.speech_url)-1);
                }

            }

            cJSON *light=cJSON_GetObjectItem(linkage_strategy,"light");
            if(light!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(light,"on");
                if(item!=NULL) {
                    family->linkage.light_on=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"mode");
                if(item!=NULL) {
                    family->linkage.light_mode=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"dur");
                if(item!=NULL){
                    family->linkage.light_dur=item->valueint;
                }
            }

            cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
            if(maskDeteBuz!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(maskDeteBuz,"on");
                if(item!=NULL) {
                    family->linkage.buz_on=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                if(item!=NULL) {
                    family->linkage.buz_dur=item->valueint;
                }
            }
        }
    }
    return NULL;
}

//隐私遮蔽
static cJSON* SetAIPrivacyMaskingEntry(cJSON* AI_entry, int i)
{
    int j = 0;
    cJSON* AI_privacy_masking = cJSON_GetObjectItem(AI_entry,"privacy_masking");
    if (AI_privacy_masking!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");
        cJSON * AI_privacy_masking_on = cJSON_GetObjectItem(AI_privacy_masking, "on");
        if(AI_privacy_masking_on!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.on =AI_privacy_masking_on->valueint;
        }


        cJSON *AI_alarm_zone=cJSON_GetObjectItem(AI_privacy_masking, "zone");
        if(AI_alarm_zone!=NULL)
        {
            int zone_size = cJSON_GetArraySize(AI_alarm_zone);

            if (1!=zone_size)
            {
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","zone size[%d] error!\n", zone_size);
                return NULL;
            }
            //有多少个区域要记录到本地
            int zone = 0;
            for(zone = 0;zone < zone_size ;zone++)
            {

                cJSON * area = cJSON_GetArrayItem(AI_alarm_zone, zone);
                cJSON * pointCjson = cJSON_GetObjectItem(area,"point");
                if(pointCjson!=NULL)
                {
                    int point_num = cJSON_GetArraySize(pointCjson);

                    if (4!=point_num)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","point_num [%d] error!\n", point_num);
                        return NULL;
                    }

                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
                    cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
                    cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
                    cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

                    //获取这两个点的具体x，y坐标值
                    cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
                    cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
                    cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
                    cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
                    cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
                    cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
                    cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
                    cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_up_x = point_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_up_y = point_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_bottom_x = point_left_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_bottom_y = point_left_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_bottom_x = point_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_bottom_y = point_right_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_up_x = point_right_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_up_y = point_right_up_y->valueint;

                }

            }
        }
    }
}

//睡眠检测
static cJSON* SetAISleepMonitorEntry(cJSON* AI_entry, int i)
{
    int j = 0;
    cJSON* AI_sleep_monitor = cJSON_GetObjectItem(AI_entry,"sleep_monitor");
    if (AI_sleep_monitor!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");
        cJSON * AI_sleep_monitor_on = cJSON_GetObjectItem(AI_sleep_monitor, "on");
        if(AI_sleep_monitor_on!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.on =AI_sleep_monitor_on->valueint;
        }
        cJSON *sleep_detection=cJSON_GetObjectItem(AI_sleep_monitor,"sleep_detection");
        if(sleep_detection!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(sleep_detection,"on");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.on=item->valueint;
            }
            item=cJSON_GetObjectItem(sleep_detection,"sensitivity");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.sensitivity=item->valueint;
            }
        }
        cJSON *face_cover=cJSON_GetObjectItem(AI_sleep_monitor,"face_cover");
        if(face_cover!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(face_cover,"on");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.on=item->valueint;
            }
            item=cJSON_GetObjectItem(face_cover,"sensitivity");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.sensitivity=item->valueint;
            }
            item=cJSON_GetObjectItem(face_cover,"mode");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.mode=item->valueint;
            }
        }
        cJSON *wakeup_detection=cJSON_GetObjectItem(AI_sleep_monitor,"wakeup_detection");
        if(wakeup_detection!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(wakeup_detection,"on");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.on=item->valueint;
            }
            item=cJSON_GetObjectItem(wakeup_detection,"sensitivity");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.sensitivity=item->valueint;
            }
        }
    }
}

//表情识别 CSWID-2025-000FEX
static cJSON* SetAIExpressionRecognitionEntry(cJSON* AI_entry, int channel_index)
{
    if (!AI_entry || channel_index < 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR, "DEV/DEMO", "Invalid input parameters");
        return NULL;
    }

    cJSON* expression_recognition = cJSON_GetObjectItem(AI_entry, "expression_recognition");
    if (!expression_recognition) {
        return NULL;
    }

    OVD_LOG(OVD_LOGLEVEL_INFO, "DEV/DEMO", "Processing expression recognition entry for channel %d", channel_index);

    cJSON* on_switch = cJSON_GetObjectItem(expression_recognition, "on");
    if (on_switch && cJSON_IsBool(on_switch)) {
        OVD_DEMO_ConfigureInfo.channelsInfo[channel_index].aiset.AIExpressionRecognition.on = on_switch->valueint;
    } else {
        OVD_LOG(OVD_LOGLEVEL_DEBUG, "DEV/DEMO", "Missing or invalid 'on' field in expression recognition config");
    }

    return expression_recognition;
}


//跌倒检测
static cJSON* SetAIFallDetectionEntry(cJSON* AI_entry, int i)
{
    int j = 0;
    cJSON* AI_fall_detection = cJSON_GetObjectItem(AI_entry,"fall_detection");
    if (AI_fall_detection!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");
        cJSON * AI_fall_detection_on = cJSON_GetObjectItem(AI_fall_detection, "on");
        if(AI_fall_detection_on!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.on =AI_fall_detection_on->valueint;
        }
        cJSON * AI_fall_detection_sensitivity = cJSON_GetObjectItem(AI_fall_detection, "sensitivity");
        if(AI_fall_detection_sensitivity!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.sensitivity =AI_fall_detection_sensitivity->valueint;
        }

        //区域
        cJSON *AI_alarm_zone=cJSON_GetObjectItem(AI_fall_detection, "zone");
        if(AI_alarm_zone!=NULL)
        {
            int zone_size = cJSON_GetArraySize(AI_alarm_zone);

            if (1!=zone_size)
            {
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","zone size[%d] error!\n", zone_size);
                return NULL;
            }
            //有多少个区域要记录到本地
            int zone = 0;
            for(zone = 0;zone < zone_size ;zone++)
            {

                cJSON * area = cJSON_GetArrayItem(AI_alarm_zone, zone);
                cJSON * pointCjson = cJSON_GetObjectItem(area,"point");
                if(pointCjson!=NULL)
                {
                    int point_num = cJSON_GetArraySize(pointCjson);

                    if (4!=point_num)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","point_num [%d] error!\n", point_num);
                        return NULL;
                    }

                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
                    cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
                    cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
                    cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

                    //获取这两个点的具体x，y坐标值
                    cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
                    cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
                    cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
                    cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
                    cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
                    cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
                    cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
                    cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_x = point_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_y = point_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_x = point_left_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_y = point_left_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_x = point_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_y = point_right_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_x = point_right_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_y = point_right_up_y->valueint;
                }

            }
        }

        //时间段
        cJSON *schedule_time=cJSON_GetObjectItem(AI_fall_detection,"schedule_time");
        if(schedule_time!=NULL)
        {
            cJSON *item=cJSON_GetObjectItem(schedule_time,"on");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.on=item->valueint;
            }

            item=cJSON_GetObjectItem(schedule_time,"starttime");
            if(item!=NULL)
            {
                memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime));
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime)-1);
            }

            item=cJSON_GetObjectItem(schedule_time,"endtime");
            if(item!=NULL)
            {
                memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime));
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime)-1);
            }

            item=cJSON_GetObjectItem(schedule_time,"repeat");
            if(item!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
            }
        }

        //联动
        cJSON *linkage_strategy = cJSON_GetObjectItem(AI_fall_detection,"linkage_strategy");
        if(linkage_strategy!=NULL)
        {
            cJSON *speech=cJSON_GetObjectItem(linkage_strategy,"speech");
            if(speech!=NULL)
            {
                cJSON *item = cJSON_GetObjectItem(speech,"on");
                if(item!=NULL){
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_on=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"vol");
                if(item!=NULL){
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_vol=item->valueint;
                }
                item=cJSON_GetObjectItem(speech,"repeat");
                if(item!=NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_repeat=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"interval");
                if(item!=NULL){
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_interval=item->valueint;
                }

                item=cJSON_GetObjectItem(speech,"url");
                if(item!=NULL){
                    memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_url, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_url));
                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_url, item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_url)-1);
                }

            }

            cJSON *light=cJSON_GetObjectItem(linkage_strategy,"light");
            if(light!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(light,"on");
                if(item!=NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_on=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"mode");
                if(item!=NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_mode=item->valueint;
                }

                item=cJSON_GetObjectItem(light,"dur");
                if(item!=NULL){
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_dur=item->valueint;
                }
            }

            cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
            if(maskDeteBuz!=NULL)
            {
                cJSON *item=cJSON_GetObjectItem(maskDeteBuz,"on");
                if(item!=NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_on=item->valueint;
                }

                item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                if(item!=NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_dur=item->valueint;
                }
            }
        }
    }
}

//车道线
static cJSON* SetAILaneLineEntry(cJSON* AI_entry, int i)
{
    int j = 0;

    cJSON* AI_lane_line = cJSON_GetObjectItem(AI_entry,"lane_line_detection");
    if (AI_lane_line!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        cJSON * AI_lane_line_on = cJSON_GetObjectItem(AI_lane_line, "on");
        if(AI_lane_line_on!=NULL)
        {
            if (!cJSON_IsBool(AI_lane_line_on))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.on =AI_lane_line_on->valueint;


        }

        cJSON *AI_lane_line_num=cJSON_GetObjectItem(AI_lane_line,"lane_line_num");
        if(AI_lane_line_num !=NULL)
        {
            if(cJSON_Number != AI_lane_line_num->type)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            if (AI_lane_line_num->valueint > OVD_DEMO_CapInfoV2.ai_laneline.lane_line_num)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","capability not support!\n");
                return NULL;
            }

            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.lane_line_num =AI_lane_line_num->valueint;

        }

        cJSON *AI_alarm_zone=cJSON_GetObjectItem(AI_lane_line,"alarm_zone");
        if(AI_alarm_zone!=NULL)
        {
            if(cJSON_Array != AI_alarm_zone->type)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            int zone_size = cJSON_GetArraySize(AI_alarm_zone);

            if ((zone_size >2) || (zone_size < 1))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","zone size[%d] error!\n", zone_size);
                return NULL;
            }

            if (zone_size!=OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.lane_line_num)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","zone size[%d] not equal with lane line num[%d]!\n", zone_size, OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.lane_line_num);
                return NULL;
            }

            //有多少个区域要记录到本地
            for(j = 0;j < zone_size ;j++)
            {

                cJSON * area = cJSON_GetArrayItem(AI_alarm_zone, j);

                cJSON * areaId = cJSON_GetObjectItem(area,"areaId");
                if(areaId!=NULL)
                {
                    if(cJSON_Number != areaId->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }
                }

                cJSON * direction = cJSON_GetObjectItem(area,"direction");
                if(direction!=NULL)
                {
                    if(cJSON_Number != direction->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.direction[j] = direction->valueint;
                }

                cJSON * pointCjson = cJSON_GetObjectItem(area,"point");
                if(pointCjson!=NULL)
                {

                    if(cJSON_Array != pointCjson->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    int point_num = cJSON_GetArraySize(pointCjson);

                    if (4!=point_num)
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","point_num [%d] error!\n", point_num);
                        return NULL;
                    }

                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
                    cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
                    cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
                    cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

                    //获取这两个点的具体x，y坐标值
                    cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
                    cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
                    cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
                    cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
                    cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
                    cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
                    cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
                    cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

                    if((cJSON_Number != point_left_up_x->type)
                        &&(cJSON_Number != point_left_up_y->type)
                        &&(cJSON_Number != point_left_bottom_x->type)
                        &&(cJSON_Number != point_left_bottom_y->type)
                        &&(cJSON_Number != point_right_bottom_x->type)
                        &&(cJSON_Number != point_right_bottom_y->type)
                        &&(cJSON_Number != point_right_up_x->type)
                        &&(cJSON_Number != point_right_up_y->type))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_up_x = point_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_up_y = point_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_bottom_x = point_left_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].left_bottom_y = point_left_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_bottom_x = point_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_bottom_y = point_right_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_up_x = point_right_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AILaneLine.zoneInfo[j].right_up_y = point_right_up_y->valueint;

                }

            }

        }
    }


    return 0;
}

//高空检测
static cJSON* SetAIParabolicAerialEntry(cJSON* AI_entry, int i)
{
    cJSON *item=NULL;
    int k = 0;
    int j = 0;

    cJSON * pointInfo = NULL;
    cJSON * singlePoint = NULL;
    cJSON * singlePoint_x = NULL;
    cJSON * singlePoint_y = NULL;

    OVDDemoAIParabolicAerialSet *pAIParabolicAerialSet = NULL;
    OVDAIParabolicAerialFloorConf* pFloorConfInfo = NULL;
    OVDAreasInfo * pDetectAreasInfo = NULL;
    OVDAreasInfo * pShieldAreasInfo = NULL;
    OVDSingleAreaInfo *pSingleAreaInfo = NULL;
    OVDAreaPointsInfo *pAreaPointsInfo = NULL;

    pAIParabolicAerialSet = &(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial);

    pFloorConfInfo = pAIParabolicAerialSet->pfloorInfo;

    pDetectAreasInfo = pAIParabolicAerialSet->pDetectZoneInfo;

    pShieldAreasInfo = pAIParabolicAerialSet->pShieldZoneInfo;


    cJSON* AI_parabolic_aerial_detection = cJSON_GetObjectItem(AI_entry,"parabolic_aerial_detection");
    if(AI_parabolic_aerial_detection!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        cJSON *AI_on=cJSON_GetObjectItem(AI_parabolic_aerial_detection,"on");
        if(AI_on!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial.on=AI_on->valueint;
        }

        cJSON *AI_sensitivity=cJSON_GetObjectItem(AI_parabolic_aerial_detection,"sensitivity");
        if(AI_sensitivity !=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial.sensitivity=AI_sensitivity->valueint;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","SetAIParabolicAerialEntry: sensitivity[%d]\n", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIParabolicAerial.sensitivity);

        }

        //floor configuration
        cJSON *AI_floor=cJSON_GetObjectItem(AI_parabolic_aerial_detection,"floor_configuration");
        if(AI_floor!=NULL)
        {
            int floor_num = cJSON_GetArraySize(AI_floor);
            if (floor_num > OVD_DEMO_CapInfoV2.ai_parabolic.floor_conf_num)
            {
                floor_num = OVD_DEMO_CapInfoV2.ai_parabolic.floor_conf_num;
            }

            pFloorConfInfo->floorNum = floor_num;

            for (k = 0; k< floor_num; k++)
            {
                cJSON *AI_single_floor = cJSON_GetArrayItem(AI_floor, k);
                if (AI_single_floor!=NULL)
                {
                    cJSON * floor_id = cJSON_GetObjectItem(AI_single_floor,"floor_id");
                    if (floor_id!=NULL)
                    {
                        pFloorConfInfo->floorInfos[k].floor_id = floor_id->valueint;
                    }

                    cJSON * floor_detail = cJSON_GetObjectItem(AI_single_floor,"detail");
                    if (floor_detail!=NULL)
                    {
                        if(cJSON_String == floor_detail->type)
                        {
                            memset(pFloorConfInfo->floorInfos[k].floor_detail, 0, sizeof(pFloorConfInfo->floorInfos[k].floor_detail));
                            strncpy(pFloorConfInfo->floorInfos[k].floor_detail, floor_detail->valuestring,sizeof(pFloorConfInfo->floorInfos[k].floor_detail)-1);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","floor_detail Json type error!\n");
                            return NULL;
                        }
                    }

                    cJSON * floor_line = cJSON_GetObjectItem(AI_single_floor,"floor_line");
                    if (floor_line!=NULL)
                    {
                        cJSON * floor_line_A_x = cJSON_GetObjectItem(floor_line,"A_x");
                        if (floor_line_A_x!=NULL)
                        {
                            pFloorConfInfo->floorInfos[k].floor_line.point_a_x = floor_line_A_x->valueint;
                        }

                        cJSON * floor_line_A_y = cJSON_GetObjectItem(floor_line,"A_y");
                        if (floor_line_A_y!=NULL)
                        {
                            pFloorConfInfo->floorInfos[k].floor_line.point_a_y = floor_line_A_y->valueint;
                        }

                        cJSON * floor_line_B_x = cJSON_GetObjectItem(floor_line,"B_x");
                        if (floor_line_B_x!=NULL)
                        {
                            pFloorConfInfo->floorInfos[k].floor_line.point_b_x = floor_line_B_x->valueint;
                        }

                        cJSON * floor_line_B_y = cJSON_GetObjectItem(floor_line,"B_y");
                        if (floor_line_B_y!=NULL)
                        {
                            pFloorConfInfo->floorInfos[k].floor_line.point_b_y = floor_line_B_y->valueint;
                        }

                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","k=[%d] A_x[%d] A_y[%d] B_x[%d] B_y[%d]\n", k, pFloorConfInfo->floorInfos[k].floor_line.point_a_x, pFloorConfInfo->floorInfos[k].floor_line.point_a_y, pFloorConfInfo->floorInfos[k].floor_line.point_b_x, pFloorConfInfo->floorInfos[k].floor_line.point_b_y);
                    }
                }
            }
        }



        //检测区域
        cJSON *AI_detect_zone=cJSON_GetObjectItem(AI_parabolic_aerial_detection,"detect_zone");
        if(AI_detect_zone!=NULL)
        {
            int zone_size = cJSON_GetArraySize(AI_detect_zone);
            if (zone_size > OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone_num)
            {
                zone_size = OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone_num;
            }

            pDetectAreasInfo->areasNum = zone_size;

            //有多少个区域要记录到本地
            for(k = 0;k < zone_size ;k++)
            {
                pSingleAreaInfo = pDetectAreasInfo->areas;

                pAreaPointsInfo = pSingleAreaInfo[k].areaPoints;

                cJSON * area = cJSON_GetArrayItem(AI_detect_zone, k);
                if (area!=NULL)
                {
                     cJSON *AI_detect_zone_areaId=cJSON_GetObjectItem(area, "areaId");
                     if (AI_detect_zone_areaId!=NULL)
                     {
                         pSingleAreaInfo[k].areaId=AI_detect_zone_areaId->valueint;
                     }

                     cJSON *AI_detect_zone_point=cJSON_GetObjectItem(area, "point");

                     int point_num = cJSON_GetArraySize(AI_detect_zone_point);
                     if (point_num > OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone)
                     {
                         point_num = OVD_DEMO_CapInfoV2.ai_parabolic.detect_zone;
                     }

                     pAreaPointsInfo->pointsNum = point_num;


                     for (j = 0;j < point_num ;j++)
                     {
                          pointInfo = cJSON_GetArrayItem(AI_detect_zone_point, j);
                          if(pointInfo!=NULL)
                          {
                              singlePoint=cJSON_GetObjectItem(pointInfo, "pointId");
                              if (singlePoint!=NULL)
                              {
                                  pAreaPointsInfo->points[j].pointId = singlePoint->valueint;
                              }

                              singlePoint_x=cJSON_GetObjectItem(pointInfo, "x");
                              if (singlePoint_x!=NULL)
                              {
                                  pAreaPointsInfo->points[j].point_x = singlePoint_x->valueint;
                              }

                              singlePoint_y=cJSON_GetObjectItem(pointInfo, "y");
                              if (singlePoint_y!=NULL)
                              {
                                  pAreaPointsInfo->points[j].point_y = singlePoint_y->valueint;
                              }

                              OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","detect zone, k=[%d] j[%d] pointId[%d] point_x[%d] point_y[%d]\n", k, j,pAreaPointsInfo->points[j].pointId, pAreaPointsInfo->points[j].point_x, pAreaPointsInfo->points[j].point_y);

                          }
                     }
                }

            }
        }

        //屏蔽区域
        cJSON *AI_shield_zone=cJSON_GetObjectItem(AI_parabolic_aerial_detection,"shield_zone");
        if(AI_shield_zone!=NULL)
        {
            int shield_zone_size = cJSON_GetArraySize(AI_shield_zone);
            if (shield_zone_size > OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone_num)
            {
                shield_zone_size = OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone_num;
            }

            pShieldAreasInfo->areasNum = shield_zone_size;

            //有多少个区域要记录到本地
            for(k = 0;k < shield_zone_size ;k++)
            {
                pSingleAreaInfo = pShieldAreasInfo->areas;

                pAreaPointsInfo = pSingleAreaInfo[k].areaPoints;

                cJSON * shield_area = cJSON_GetArrayItem(AI_shield_zone, k);
                if (shield_area!=NULL)
                {
                     cJSON *AI_shield_zone_areaId=cJSON_GetObjectItem(shield_area, "areaId");
                     if (AI_shield_zone_areaId!=NULL)
                     {
                         pSingleAreaInfo[k].areaId=AI_shield_zone_areaId->valueint;
                     }

                     cJSON *AI_shield_zone_point=cJSON_GetObjectItem(shield_area, "point");

                     int shield_point_num = cJSON_GetArraySize(AI_shield_zone_point);
                     if (shield_point_num > OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone)
                     {
                         shield_point_num = OVD_DEMO_CapInfoV2.ai_parabolic.shield_zone;
                     }

                     pAreaPointsInfo->pointsNum = shield_point_num;

                     for (j = 0;j < shield_point_num ;j++)
                     {
                          pointInfo = cJSON_GetArrayItem(AI_shield_zone_point, j);
                          if(pointInfo!=NULL)
                          {
                              singlePoint=cJSON_GetObjectItem(pointInfo, "pointId");
                              if (singlePoint!=NULL)
                              {
                                  pAreaPointsInfo->points[j].pointId = singlePoint->valueint;
                              }

                              singlePoint_x=cJSON_GetObjectItem(pointInfo, "x");
                              if (singlePoint_x!=NULL)
                              {
                                  pAreaPointsInfo->points[j].point_x = singlePoint_x->valueint;
                              }

                              singlePoint_y=cJSON_GetObjectItem(pointInfo, "y");
                              if (singlePoint_y!=NULL)
                              {
                                  pAreaPointsInfo->points[j].point_y = singlePoint_y->valueint;
                              }

                              OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","shield zone, k=[%d] j[%d] pointId[%d] point_x[%d] point_y[%d]\n", k, j,pAreaPointsInfo->points[j].pointId, pAreaPointsInfo->points[j].point_x, pAreaPointsInfo->points[j].point_y);


                          }
                     }
                }

            }
        }

    }

    return NULL;
}

//区域人数统计, 目前只支持一个检测计划
static cJSON* SetAIRegionalPeopleStatEntry(cJSON* AI_entry, int i)
{
    int rv = 0;
    cJSON *item = NULL;

    cJSON* AI_regional_people_statistics = cJSON_GetObjectItem(AI_entry,"regional_people_statistics");
    if (AI_regional_people_statistics!=NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Entry!\n");

        if (!cJSON_IsObject(AI_regional_people_statistics))
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
            return NULL;
        }

        cJSON * AI_detection_on = cJSON_GetObjectItem(AI_regional_people_statistics, "detect_on");
        if(AI_detection_on!=NULL)
        {
            if (!cJSON_IsBool(AI_detection_on))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            int onValue=AI_detection_on->valueint;

            //下发配置
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.on = onValue;
        }

        cJSON *AI_detect_plans=cJSON_GetObjectItem(AI_regional_people_statistics,"detect_plans");
        if(AI_detect_plans !=NULL)
        {
            OVDDemoAIRegionalPeopleStatPlan detePlan;
            memset(&detePlan, 0, sizeof(OVDDemoAIRegionalPeopleStatPlan));

            if(cJSON_Array != AI_detect_plans->type)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            int plan_nums = cJSON_GetArraySize(AI_detect_plans);

            if (1!=plan_nums)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","plan_nums[%d] error!\n", plan_nums);
                return NULL;
            }

            cJSON * detect_plan_0 = cJSON_GetArrayItem(AI_detect_plans, 0);
            if(detect_plan_0 !=NULL)
            {
                if (!cJSON_IsObject(detect_plan_0))
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }

                cJSON *AI_on=cJSON_GetObjectItem(detect_plan_0,"on");
                if(AI_on!=NULL)
                {
                    if (!cJSON_IsBool(AI_on))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.on=AI_on->valueint;
                }

                cJSON *AI_regional_id=cJSON_GetObjectItem(detect_plan_0,"regional_id");
                if(AI_regional_id !=NULL)
                {
                    if(cJSON_Number != AI_regional_id->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.regional_id=AI_regional_id->valueint;

                    if (0!=detePlan.regional_id)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_id[%d] error\n", detePlan.regional_id);
                        return NULL;
                    }

                }

                cJSON *AI_regional_name=cJSON_GetObjectItem(detect_plan_0,"regional_name");
                if(AI_regional_name !=NULL)
                {
                    if(cJSON_String == AI_regional_name->type)
                    {
                        memset(detePlan.regional_name, 0, sizeof(detePlan.regional_name));
                        strncpy(detePlan.regional_name, AI_regional_name->valuestring,sizeof(detePlan.regional_name)-1);

                        if (strlen(detePlan.regional_name)>127)
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_name[%s] exceed the length[%d] limited!\n", detePlan.regional_name, strlen(detePlan.regional_name));
                            return NULL;

                        }

                        if ((strstr(detePlan.regional_name, "~"))
                            ||(strstr(detePlan.regional_name, "!"))
                            ||(strstr(detePlan.regional_name, "@"))
                            ||(strstr(detePlan.regional_name, "#"))
                            ||(strstr(detePlan.regional_name, "$"))
                            ||(strstr(detePlan.regional_name, "%"))
                            ||(strstr(detePlan.regional_name, "^"))
                            ||(strstr(detePlan.regional_name, "&"))
                            ||(strstr(detePlan.regional_name, "*")))
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","regional_name[%s] include special label!\n", detePlan.regional_name);
                            return NULL;
                        }

                        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","regional_name[%s]\n", detePlan.regional_name);

                    }
                    else
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                }

                cJSON *AI_regional_people_count=cJSON_GetObjectItem(detect_plan_0,"regional_people_count");
                if(AI_regional_people_count!=NULL)
                {
                    if(cJSON_Number != AI_regional_people_count->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.regional_people_count=AI_regional_people_count->valueint;
                    if ((detePlan.regional_people_count < 1) || (detePlan.regional_people_count > OVD_DEMO_CapInfoV2.ai_regional_people.regional_people_count))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","on_duty_count[%d] exceed the limited!\n", detePlan.regional_people_count);
                        return NULL;
                    }

                }

                cJSON *AI_alarm_report_duration=cJSON_GetObjectItem(detect_plan_0,"alarm_report_duration");
                if(AI_alarm_report_duration!=NULL)
                {
                    if(cJSON_Number != AI_alarm_report_duration->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.alarm_report_duration=AI_alarm_report_duration->valueint;
                    if ((detePlan.alarm_report_duration < 1) || (detePlan.alarm_report_duration > OVD_DEMO_CapInfoV2.ai_regional_people.alarm_report_duration))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","off_duty_duration[%d] exceed the limited!\n", detePlan.alarm_report_duration);
                        return NULL;
                    }

                }

                cJSON *maskDeteTime=cJSON_GetObjectItem(detect_plan_0,"alerttime");
                if(maskDeteTime!=NULL)
                {
                    item=cJSON_GetObjectItem(maskDeteTime,"on");
                    if(item!=NULL)
                    {
                        if (!cJSON_IsBool(item))
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                            return NULL;
                        }
                        detePlan.schedule.on=item->valueint;

                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"starttime");
                    if(item!=NULL)
                    {
                        if(cJSON_String == item->type)
                        {
                            memset(detePlan.schedule.starttime, 0, sizeof(detePlan.schedule.starttime));
                            strncpy(detePlan.schedule.starttime,item->valuestring, sizeof(detePlan.schedule.starttime)-1);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }

                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"endtime");
                    if(item!=NULL)
                    {
                        if(cJSON_String == item->type)
                        {
                            memset(detePlan.schedule.endtime, 0, sizeof(detePlan.schedule.endtime));
                            strncpy(detePlan.schedule.endtime,item->valuestring, sizeof(detePlan.schedule.endtime)-1);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }
                    }

                    item=cJSON_GetObjectItem(maskDeteTime,"repeat");
                    if(item!=NULL)
                    {
                        if(cJSON_Array == item->type)
                        {
                            detePlan.schedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                        }
                        else
                        {
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","Json type error!\n");
                            return NULL;
                        }

                    }

                }

                cJSON *linkage_strategy=cJSON_GetObjectItem(detect_plan_0,"linkage_strategy");
                if(linkage_strategy!=NULL)
                {
                    cJSON *maskDeteSpeech=cJSON_GetObjectItem(linkage_strategy,"speech");
                    if(maskDeteSpeech!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteSpeech,"on");
                        if(item!=NULL)
                        {
                            if (!cJSON_IsBool(item))
                            {
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                                return NULL;
                            }

                            detePlan.linkage.speech_on=item->valueint;

                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"vol");
                        if(item!=NULL)
                        {
                            if(cJSON_Number == item->type)
                            {
                                detePlan.linkage.speech_vol=item->valueint;
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }

                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"repeat");
                        if(item!=NULL)
                        {
                            if(cJSON_Number == item->type)
                            {
                                detePlan.linkage.speech_repeat=item->valueint;
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"repeat_constant");
                        if(item!=NULL)
                        {
                            detePlan.linkage.speech_repeat_constant=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"interval");
                        if(item!=NULL)
                        {
                            detePlan.linkage.speech_interval=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"url");
                        if(item!=NULL)
                        {

                            if(cJSON_String == item->type)
                            {
                                memset(detePlan.linkage.speech_url, 0, sizeof(detePlan.linkage.speech_url));
                                strncpy(detePlan.linkage.speech_url,item->valuestring, sizeof(detePlan.linkage.speech_url)-1);
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }

                        }

                    }
                    cJSON *maskDeteLight=cJSON_GetObjectItem(linkage_strategy,"light");
                    if(maskDeteLight!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteLight,"on");
                        if(item!=NULL)
                        {
                            detePlan.linkage.light_on=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteLight,"mode");
                        if(item!=NULL)
                        {
                            detePlan.linkage.light_mode=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteLight,"dur");
                        if(item!=NULL)
                        {
                            detePlan.linkage.light_dur=item->valueint;
                        }
                        item=cJSON_GetObjectItem(maskDeteLight,"dur_constant");
                        if(item!=NULL)
                        {
                            detePlan.linkage.light_dur_constant=item->valueint;
                        }
                    }

                    cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
                    if(maskDeteBuz!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteBuz,"on");
                        if(item!=NULL)
                        {
                            detePlan.linkage.buz_on=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                        if(item!=NULL)
                        {
                            detePlan.linkage.buz_dur=item->valueint;
                        }
                    }
                }

                cJSON *AI_detect_result_report_duration=cJSON_GetObjectItem(detect_plan_0,"detect_result_report_duration");
                if(AI_detect_result_report_duration!=NULL)
                {
                    if(cJSON_Number != AI_detect_result_report_duration->type)
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.detect_result_report_duration=AI_detect_result_report_duration->valueint;
                    if ((detePlan.detect_result_report_duration < 1) || (detePlan.detect_result_report_duration > OVD_DEMO_CapInfoV2.ai_regional_people.detect_result_report_duration))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","detect_result_report_duration[%d] exceed the limited!\n", detePlan.detect_result_report_duration);
                        return NULL;
                    }

                }

                cJSON *AI_osd_status=cJSON_GetObjectItem(detect_plan_0,"osd_status");
                if(AI_osd_status!=NULL)
                {
                    if (!cJSON_IsBool(AI_osd_status))
                    {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                    }

                    detePlan.osd_status=AI_osd_status->valueint;
                }

                cJSON *AI_alarm_zone=cJSON_GetObjectItem(detect_plan_0,"alarm_zone");
                if(AI_alarm_zone!=NULL)
                {
                    int zone_size = cJSON_GetArraySize(AI_alarm_zone);

                    if (1!=zone_size)
                    {
                        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","zone size[%d] error!\n", zone_size);
                        return NULL;
                    }
                    //有多少个区域要记录到本地
                    int zone = 0;
                    for(zone = 0;zone < zone_size ;zone++)
                    {

                        cJSON * area = cJSON_GetArrayItem(AI_alarm_zone, zone);
                        cJSON * pointCjson = cJSON_GetObjectItem(area,"point");
                        if(pointCjson!=NULL)
                        {
                            int point_num = cJSON_GetArraySize(pointCjson);

                            if (4!=point_num)
                            {
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","point_num [%d] error!\n", point_num);
                                return NULL;
                            }

                            //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                            cJSON * point_left_up = cJSON_GetArrayItem(pointCjson, 0);
                            cJSON * point_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
                            cJSON * point_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
                            cJSON * point_right_up = cJSON_GetArrayItem(pointCjson, 3);

                            //获取这两个点的具体x，y坐标值
                            cJSON * point_left_up_x = cJSON_GetObjectItem(point_left_up,"x");
                            cJSON * point_left_up_y = cJSON_GetObjectItem(point_left_up,"y");
                            cJSON * point_left_bottom_x = cJSON_GetObjectItem(point_left_bottom,"x");
                            cJSON * point_left_bottom_y = cJSON_GetObjectItem(point_left_bottom,"y");
                            cJSON * point_right_bottom_x = cJSON_GetObjectItem(point_right_bottom,"x");
                            cJSON * point_right_bottom_y = cJSON_GetObjectItem(point_right_bottom,"y");
                            cJSON * point_right_up_x = cJSON_GetObjectItem(point_right_up,"x");
                            cJSON * point_right_up_y = cJSON_GetObjectItem(point_right_up,"y");

                            detePlan.zoneInfo.left_up_x = point_left_up_x->valueint;
                            detePlan.zoneInfo.left_up_y = point_left_up_y->valueint;
                            detePlan.zoneInfo.left_bottom_x = point_left_bottom_x->valueint;
                            detePlan.zoneInfo.left_bottom_y = point_left_bottom_y->valueint;
                            detePlan.zoneInfo.right_bottom_x = point_right_bottom_x->valueint;
                            detePlan.zoneInfo.right_bottom_y = point_right_bottom_y->valueint;
                            detePlan.zoneInfo.right_up_x = point_right_up_x->valueint;
                            detePlan.zoneInfo.right_up_y = point_right_up_y->valueint;

                        }

                    }
                }

              }

            //下发配置
            memcpy(&(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan), &detePlan, sizeof(OVDDemoAIRegionalPeopleStatPlan));

            }
        }
        return NULL;
}

static cJSON* SetAIPassengerEntry(cJSON* AI_entry, int i)
{

    //客流检测
    cJSON* AI_passenger = cJSON_GetObjectItem(AI_entry,"passenger_flow_statistics");
    if(AI_passenger!=NULL)
    {
        cJSON *AI_passenger_on=cJSON_GetObjectItem(AI_passenger,"on");
        if(AI_passenger_on!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.on=AI_passenger_on->valueint;
        }

        cJSON *AI_passenger_exposureon=cJSON_GetObjectItem(AI_passenger,"exposure_on");
        if(AI_passenger_exposureon !=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_on=AI_passenger_exposureon->valueint;

        }

        cJSON *AI_passenger_osdstatus=cJSON_GetObjectItem(AI_passenger,"osd_status");
        if(AI_passenger_osdstatus !=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.osd_status=AI_passenger_osdstatus->valueint;
        }

        cJSON *AI_passenger_exposureadjust=cJSON_GetObjectItem(AI_passenger,"exposure_adjust");
        if(AI_passenger_exposureadjust !=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_adjust=AI_passenger_exposureadjust->valueint;

        }

        cJSON *AI_passenger_mode=cJSON_GetObjectItem(AI_passenger,"mode");
        if(AI_passenger_mode!=NULL)
        {
            if(cJSON_String == AI_passenger_mode->type)
            {
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.mode, AI_passenger_mode->valuestring,sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.mode));
            }
            else
            {
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                return NULL;
            }
        }

        cJSON *AI_passenger_qualityvalue=cJSON_GetObjectItem(AI_passenger,"quality_value");
        if(AI_passenger_qualityvalue!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.quality_value=AI_passenger_qualityvalue->valueint;
        }

        cJSON *AI_passenger_sensitivity=cJSON_GetObjectItem(AI_passenger,"alarm_sensitivity");
        if(AI_passenger_sensitivity!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.sensitivity=AI_passenger_sensitivity->valueint;
        }


        cJSON* AI_passenger_statistcs_line = cJSON_GetObjectItem(AI_passenger,"statistics_line");
        if(AI_passenger_statistcs_line!=NULL)
        {
            cJSON* statistcs_line_A_x = cJSON_GetObjectItem(AI_passenger_statistcs_line, "A_x");
            if(statistcs_line_A_x!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_x=statistcs_line_A_x->valueint;
            }

            cJSON* statistcs_line_A_y = cJSON_GetObjectItem(AI_passenger_statistcs_line, "A_y");
            if(statistcs_line_A_y!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_y=statistcs_line_A_y->valueint;
            }

            cJSON* statistcs_line_B_x = cJSON_GetObjectItem(AI_passenger_statistcs_line, "B_x");
            if(statistcs_line_B_x!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_x=statistcs_line_B_x->valueint;
            }

            cJSON* statistcs_line_B_y = cJSON_GetObjectItem(AI_passenger_statistcs_line, "B_y");
            if(statistcs_line_B_y!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_y=statistcs_line_B_y->valueint;
            }

        }

        cJSON* AI_passenger_statistcs_mode = cJSON_GetObjectItem(AI_passenger,"statistics_mode");
        if(AI_passenger_statistcs_mode!=NULL)
        {
            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.statistics_mode=AI_passenger_statistcs_mode->valueint;
        }

        cJSON *AI_passenger_zone=cJSON_GetObjectItem(AI_passenger,"alarm_zone");
        if(AI_passenger_zone!=NULL)
        {
            int zone_size = cJSON_GetArraySize(AI_passenger_zone);
                    //有多少个区域要记录到本地
            int facezone = 0;
            for(facezone = 0;facezone < zone_size ;facezone++)
            {

                cJSON * area = cJSON_GetArrayItem(AI_passenger_zone, facezone);
                cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

                //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                cJSON * facePoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
                cJSON * facePoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);

                //获取这两个点的具体x，y坐标值
                cJSON * facePoint_left_up_x = cJSON_GetObjectItem(facePoint_left_up,"x");
                cJSON * facePoint_left_up_y = cJSON_GetObjectItem(facePoint_left_up,"y");
                cJSON * facePoint_right_bottom_x = cJSON_GetObjectItem(facePoint_right_bottom,"x");
                        cJSON * facePoint_right_bottom_y = cJSON_GetObjectItem(facePoint_right_bottom,"y");

                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_x = facePoint_left_up_x->valueint;
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_y = facePoint_left_up_y->valueint;
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_x = facePoint_right_bottom_x->valueint;
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_y = facePoint_right_bottom_y->valueint;

            }
        }

        cJSON* AI_welcome = cJSON_GetObjectItem(AI_passenger,"welcome_promotion");
        if(AI_welcome!=NULL)
        {
            cJSON* AI_welcome_speech = cJSON_GetObjectItem(AI_welcome,"speech");
            if(AI_welcome_speech!=NULL)
            {
                cJSON *AI_welcome_speech_on = cJSON_GetObjectItem(AI_welcome_speech,"on");
                if(AI_welcome_speech_on != NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_on = AI_welcome_speech_on->valueint;
                }
                cJSON *AI_welcome_speech_vol = cJSON_GetObjectItem(AI_welcome_speech,"vol");
                if(AI_welcome_speech_vol != NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_vol = AI_welcome_speech_vol->valueint;
                }
                cJSON *AI_welcome_speech_repeat = cJSON_GetObjectItem(AI_welcome_speech,"repeat");
                if(AI_welcome_speech_repeat != NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_repeat = AI_welcome_speech_repeat->valueint;
                }
                cJSON *AI_welcome_speech_url_enter = cJSON_GetObjectItem(AI_welcome_speech,"url_enter");
                if(AI_welcome_speech_url_enter != NULL)
                {
                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_enter,AI_welcome_speech_url_enter->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_enter)-1);
                }
                cJSON *AI_welcome_speech_url_leave = cJSON_GetObjectItem(AI_welcome_speech,"url_leave");
                if(AI_welcome_speech_url_leave != NULL)
                {
                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_leave,AI_welcome_speech_url_leave->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_leave)-1);
                }
            }
            cJSON* AI_welcome_alerttime = cJSON_GetObjectItem(AI_welcome,"alerttime");
            if(AI_welcome_alerttime!=NULL)
            {
                cJSON *item=NULL;
                item=cJSON_GetObjectItem(AI_welcome_alerttime,"starttime");
				if(item!=NULL)
				{
				    if(cJSON_String == item->type)
				    {
				        memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time));
                        strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time)-1);
					}
					else
					{
						OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
					}
				}

				item=cJSON_GetObjectItem(AI_welcome_alerttime,"endtime");
				if(item!=NULL)
				{
					if(cJSON_String == item->type)
				    {
				        memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time));
                        strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time)-1);
					}
					else
					{
						OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
					}
				}

				item=cJSON_GetObjectItem(AI_welcome_alerttime,"repeat");
				if(item!=NULL)
				{
				    if(cJSON_Array == item->type)
				    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
					}
					else
					{
						OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
				    }
                }
            }
        }

    }

    return NULL;

}

static cJSON* SetAISentry(cJSON *channelentry,int i)
{

	cJSON* AI_entry= cJSON_GetObjectItem(channelentry,"AI");
    if(AI_entry !=NULL)
    {
    	//face
        cJSON* AI_face = cJSON_GetObjectItem(AI_entry,"face");
        if(AI_face!=NULL)
        {
            cJSON *AI_face_on=cJSON_GetObjectItem(AI_face,"on");
            if(AI_face_on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.on=AI_face_on->valueint;
            }

            cJSON *AI_face_exposureon=cJSON_GetObjectItem(AI_face,"exposure_on");
            if(AI_face_exposureon !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_on=AI_face_exposureon->valueint;

            }

             cJSON *AI_face_exposureadjust=cJSON_GetObjectItem(AI_face,"exposure_adjust");
            if(AI_face_exposureadjust !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_adjust=AI_face_exposureadjust->valueint;

            }

            cJSON *AI_face_mode=cJSON_GetObjectItem(AI_face,"mode");
            if(AI_face_mode!=NULL)
            {
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.mode, AI_face_mode->valuestring,sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.mode));
            }

            cJSON *AI_face_qualityvalue=cJSON_GetObjectItem(AI_face,"quality_value");
            if(AI_face_qualityvalue!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.quality_value=AI_face_qualityvalue->valueint;
            }
			cJSON *AI_face_sensitivity=cJSON_GetObjectItem(AI_face,"alarm_sensitivity");
            if(AI_face_sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.sensitivity=AI_face_sensitivity->valueint;
            }
			cJSON *zone=cJSON_GetObjectItem(AI_face,"alarm_zone");
			if(zone!=NULL)
			{
				int zone_size = cJSON_GetArraySize(zone);
				//有多少个区域要记录到本地
				int facezone = 0;
				for(facezone = 0;facezone < zone_size ;facezone++)
				{

					cJSON * area = cJSON_GetArrayItem(zone, facezone);
					cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

					//每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
					cJSON * facePoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
					cJSON * facePoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);

					//获取这两个点的具体x，y坐标值
					cJSON * facePoint_left_up_x = cJSON_GetObjectItem(facePoint_left_up,"x");
					cJSON * facePoint_left_up_y = cJSON_GetObjectItem(facePoint_left_up,"y");
					cJSON * facePoint_right_bottom_x = cJSON_GetObjectItem(facePoint_right_bottom,"x");
					cJSON * facePoint_right_bottom_y = cJSON_GetObjectItem(facePoint_right_bottom,"y");

					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_x = facePoint_left_up_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_y = facePoint_left_up_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_x = facePoint_right_bottom_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_y = facePoint_right_bottom_y->valueint;

				}
			}

			//口罩检测内容解析
			cJSON *mask_detection=cJSON_GetObjectItem(AI_face,"mask_detection");
			cJSON *item=NULL;
			if (mask_detection != NULL)
			{
                cJSON *mask_detection_on=cJSON_GetObjectItem(mask_detection,"on");
                if(mask_detection_on!=NULL)
                {
                    if(cJSON_Number == mask_detection_on->type)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.on = mask_detection_on->valueint;
					}
					else
					{
						OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                        return NULL;
					}

                }

                cJSON *maskDeteTime=cJSON_GetObjectItem(mask_detection,"alerttime");
			    if(maskDeteTime!=NULL)
			    {
				    item=cJSON_GetObjectItem(maskDeteTime,"on");
				    if(item!=NULL)
				    {
				        if(cJSON_Number == item->type)
				        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.on=item->valueint;
						}
						else
					    {
						    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                            return NULL;
					    }

				    }

				    item=cJSON_GetObjectItem(maskDeteTime,"starttime");
				    if(item!=NULL)
				    {
				        if(cJSON_String == item->type)
				        {
				            memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime));
                            strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime)-1);
						}
						else
						{
							OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                            return NULL;
						}

				    }

				    item=cJSON_GetObjectItem(maskDeteTime,"endtime");
				    if(item!=NULL)
				    {
						if(cJSON_String == item->type)
				        {
				            memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime));
                            strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime)-1);
						}
						else
						{
							OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                            return NULL;
						}
					}

				    item=cJSON_GetObjectItem(maskDeteTime,"repeat");
				    if(item!=NULL)
				    {
				        if(cJSON_Array == item->type)
				        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
						}
						else
						{
							OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                            return NULL;
						}

				    }

			    }

                cJSON *linkage_strategy=cJSON_GetObjectItem(mask_detection,"linkage_strategy");
                if(linkage_strategy!=NULL)
                {
                    cJSON *maskDeteSpeech=cJSON_GetObjectItem(linkage_strategy,"speech");
                    if(maskDeteSpeech!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteSpeech,"on");
                        if(item!=NULL)
                        {
                            if(cJSON_Number == item->type)
                            {
                                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_on=item->valueint;
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }

                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"vol");
                        if(item!=NULL)
                        {
                            if(cJSON_Number == item->type)
                            {
                                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_vol=item->valueint;
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }

                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"repeat");
                        if(item!=NULL)
                        {
                            if(cJSON_Number == item->type)
                            {
                                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat=item->valueint;
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"repeat_constant");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat_constant=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"interval");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_interval=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteSpeech,"url");
                        if(item!=NULL)
                        {

                            if(cJSON_String == item->type)
                            {
                                memset(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_url, 0, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_url));
                                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_url,item->valuestring, sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_url)-1);
                            }
                            else
                            {
                                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Json type error!\n");
                                return NULL;
                            }

                        }

                    }
                    cJSON *maskDeteLight=cJSON_GetObjectItem(linkage_strategy,"light");
                    if(maskDeteLight!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteLight,"on");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_on=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteLight,"mode");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_mode=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteLight,"dur");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur=item->valueint;
                        }
                        item=cJSON_GetObjectItem(maskDeteLight,"dur_constant");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur_constant=item->valueint;
                        }
                    }

                    cJSON *maskDeteBuz=cJSON_GetObjectItem(linkage_strategy,"buz");
                    if(maskDeteBuz!=NULL)
                    {
                        item=cJSON_GetObjectItem(maskDeteBuz,"on");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_on=item->valueint;
                        }

                        item=cJSON_GetObjectItem(maskDeteBuz,"dur");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_dur=item->valueint;
                        }
                    }
                }
            }

        }

		//AI_kitchen
		cJSON* AI_kitchen = cJSON_GetObjectItem(AI_entry,"kitchen_detection");
        if(AI_kitchen!=NULL)
        {
            cJSON *AI_kitchen_on=cJSON_GetObjectItem(AI_kitchen,"on");
            if(AI_kitchen_on!=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.on=AI_kitchen_on->valueint;

            cJSON *AI_kitchen_maskon=cJSON_GetObjectItem(AI_kitchen,"mask_on");
            if(AI_kitchen_maskon !=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.mask_on=AI_kitchen_maskon->valueint;

			cJSON *AI_kitchen_capon=cJSON_GetObjectItem(AI_kitchen,"cap_on");
			if(AI_kitchen_capon !=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.cap_on=AI_kitchen_capon->valueint;

			cJSON *AI_kitchen_clotheson=cJSON_GetObjectItem(AI_kitchen,"clothes_on");
			if(AI_kitchen_clotheson !=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_on=AI_kitchen_clotheson->valueint;

            cJSON *AI_kitchen_color=cJSON_GetObjectItem(AI_kitchen,"clothes_color");
            if(AI_kitchen_color!=NULL)
            {
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_color, AI_kitchen_color->valuestring,sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_color));
            }
			cJSON *AI_kitchen_sensitivity=cJSON_GetObjectItem(AI_kitchen,"alarm_sensitivity");
            if(AI_kitchen_sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.sensitivity=AI_kitchen_sensitivity->valueint;
            }
			cJSON *zone=cJSON_GetObjectItem(AI_kitchen,"alarm_zone");
			if(zone!=NULL){
				int zone_size = cJSON_GetArraySize(zone);
				//有多少个区域要记录到本地
				int kitchenzone = 0;
				for(kitchenzone = 0;kitchenzone < zone_size ;kitchenzone++){

					cJSON * area = cJSON_GetArrayItem(zone, kitchenzone);
					cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

					//每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
					cJSON * kitchenPoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
					cJSON * kitchenPoint_left_bottom = cJSON_GetArrayItem(pointCjson, 1);
					cJSON * kitchenPoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);
					cJSON * kitchenPoint_right_up = cJSON_GetArrayItem(pointCjson, 3);

					//获取这两个点的具体x，y坐标值
					cJSON * kitchenPoint_left_up_x = cJSON_GetObjectItem(kitchenPoint_left_up,"x");
					cJSON * kitchenPoint_left_up_y = cJSON_GetObjectItem(kitchenPoint_left_up,"y");
					cJSON * kitchenPoint_left_bottom_x = cJSON_GetObjectItem(kitchenPoint_left_bottom,"x");
					cJSON * kitchenPoint_left_bottom_y = cJSON_GetObjectItem(kitchenPoint_left_bottom,"y");
					cJSON * kitchenPoint_right_bottom_x = cJSON_GetObjectItem(kitchenPoint_right_bottom,"x");
					cJSON * kitchenPoint_right_bottom_y = cJSON_GetObjectItem(kitchenPoint_right_bottom,"y");
					cJSON * kitchenPoint_right_up_x = cJSON_GetObjectItem(kitchenPoint_right_up,"x");
					cJSON * kitchenPoint_right_up_y = cJSON_GetObjectItem(kitchenPoint_right_up,"y");

					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_x = kitchenPoint_left_up_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_y = kitchenPoint_left_up_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_bottom_x = kitchenPoint_left_bottom_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_bottom_y = kitchenPoint_left_bottom_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_x = kitchenPoint_right_bottom_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_y = kitchenPoint_right_bottom_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_up_x = kitchenPoint_right_up_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_up_y = kitchenPoint_right_up_y->valueint;


				}
			}
        }

		//AI_vehicle
		cJSON* AI_vehicle = cJSON_GetObjectItem(AI_entry,"vehicle_detection");
        if(AI_vehicle!=NULL)
        {
            cJSON *AI_vehicle_on=cJSON_GetObjectItem(AI_vehicle,"on");
            if(AI_vehicle_on!=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.on=AI_vehicle_on->valueint;

			cJSON *AI_vehicle_sensitivity=cJSON_GetObjectItem(AI_vehicle,"alarm_sensitivity");
            if(AI_vehicle_sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.sensitivity=AI_vehicle_sensitivity->valueint;
            }

            cJSON *AI_vehicle_quality=cJSON_GetObjectItem(AI_vehicle,"quality");
            if(AI_vehicle_quality !=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.quality=AI_vehicle_quality->valueint;

			cJSON *AI_vehicle_capture_mode=cJSON_GetObjectItem(AI_vehicle,"capture_mode");
			if(AI_vehicle_capture_mode !=NULL)
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.capture_mode=AI_vehicle_capture_mode->valueint;

			cJSON *AI_vehicle_scheduled_capture_time=cJSON_GetObjectItem(AI_vehicle,"scheduled_capture_time");
            if(AI_vehicle_scheduled_capture_time!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.scheduled_capture_time=AI_vehicle_scheduled_capture_time->valueint;
            }

			cJSON *AI_vehicle_detect_site=cJSON_GetObjectItem(AI_vehicle,"detect_site");
            if(AI_vehicle_detect_site!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.detect_site=AI_vehicle_detect_site->valueint;
            }

			cJSON *AI_vehicle_exit_mode=cJSON_GetObjectItem(AI_vehicle,"exit_mode");
            if(AI_vehicle_exit_mode!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.exit_mode=AI_vehicle_exit_mode->valueint;
            }

			cJSON *linkage_strategy=cJSON_GetObjectItem(AI_vehicle,"linkage_strategy");
			cJSON *item=NULL;
			if(linkage_strategy!=NULL)
        	{
        		cJSON *vehspeech=cJSON_GetObjectItem(linkage_strategy,"speech");
				if(vehspeech!=NULL)
            	{
            		item=cJSON_GetObjectItem(vehspeech,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_on=item->valueint;
					item=cJSON_GetObjectItem(vehspeech,"vol");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_vol=item->valueint;

					item=cJSON_GetObjectItem(vehspeech,"repeat");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat=item->valueint;

                    item=cJSON_GetObjectItem(vehspeech,"repeat_constant");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat_constant=item->valueint;

                    item=cJSON_GetObjectItem(vehspeech,"interval");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_interval=item->valueint;

					item=cJSON_GetObjectItem(vehspeech,"url");
					if(item!=NULL)
						strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_url,item->valuestring);

				}
				cJSON *vehlight=cJSON_GetObjectItem(linkage_strategy,"light");
				if(vehlight!=NULL)
            	{
            		item=cJSON_GetObjectItem(vehlight,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_on=item->valueint;

					item=cJSON_GetObjectItem(vehlight,"mode");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_mode=item->valueint;

					item=cJSON_GetObjectItem(vehlight,"dur");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur=item->valueint;

                    item=cJSON_GetObjectItem(vehlight,"dur_constant");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur_constant=item->valueint;
				}
				cJSON *vehbuz=cJSON_GetObjectItem(linkage_strategy,"buz");
				if(vehbuz!=NULL)
            	{
            		item=cJSON_GetObjectItem(vehbuz,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_on=item->valueint;

					item=cJSON_GetObjectItem(vehbuz,"dur");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_dur=item->valueint;
				}

			}


			cJSON *zone=cJSON_GetObjectItem(AI_vehicle,"alarm_zone");
			if(zone!=NULL){
				int zone_size = cJSON_GetArraySize(zone);
				//有多少个区域要记录到本地
				int vehiclezone = 0;
				for(vehiclezone = 0;vehiclezone < zone_size ;vehiclezone++){

					cJSON * area = cJSON_GetArrayItem(zone, vehiclezone);
					cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

					//每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
					cJSON * vehiclePoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
					cJSON * vehiclePoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);

					//获取这两个点的具体x，y坐标值
					cJSON * vehiclePoint_left_up_x = cJSON_GetObjectItem(vehiclePoint_left_up,"x");
					cJSON * vehiclePoint_left_up_y = cJSON_GetObjectItem(vehiclePoint_left_up,"y");
					cJSON * vehiclePoint_right_bottom_x = cJSON_GetObjectItem(vehiclePoint_right_bottom,"x");
					cJSON * vehiclePoint_right_bottom_y = cJSON_GetObjectItem(vehiclePoint_right_bottom,"y");

					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_x = vehiclePoint_left_up_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_y = vehiclePoint_left_up_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_x = vehiclePoint_right_bottom_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_y = vehiclePoint_right_bottom_y->valueint;
				}
			}
        }

        //AI_nonmotorvehicle(梯控)

		cJSON* AI_nonmotorvehicle = cJSON_GetObjectItem(AI_entry,"nonmotorvehicle_detection");
        if(AI_nonmotorvehicle!=NULL)
        {
            cJSON *AI_nonmotorvehicle_on=cJSON_GetObjectItem(AI_nonmotorvehicle,"on");
            if(AI_nonmotorvehicle_on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.on=AI_nonmotorvehicle_on->valueint;
            }

            cJSON *AI_nonmotorvehicle_detect_mode_list=cJSON_GetObjectItem(AI_nonmotorvehicle,"detect_mode_list");
            if(AI_nonmotorvehicle_detect_mode_list !=NULL)
            {
                strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.detect_mode_list, AI_nonmotorvehicle_detect_mode_list->valuestring,sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.detect_mode_list));
            }

            cJSON *AI_nonmotorvehicle_sensitivity=cJSON_GetObjectItem(AI_nonmotorvehicle,"alarm_sensitivity");
            if(AI_nonmotorvehicle_sensitivity !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.sensitivity=AI_nonmotorvehicle_sensitivity->valueint;
            }

            cJSON *AI_nonmotorvehicle_quality=cJSON_GetObjectItem(AI_nonmotorvehicle,"quality");
            if(AI_nonmotorvehicle_quality !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.quality=AI_nonmotorvehicle_quality->valueint;
            }

            cJSON *AI_nonmotorvehicle_capture_mode=cJSON_GetObjectItem(AI_nonmotorvehicle,"capture_mode");
            if(AI_nonmotorvehicle_capture_mode !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.capture_mode=AI_nonmotorvehicle_capture_mode->valueint;
            }

            cJSON *AI_nonmotorvehicle_scheduled_capture_time=cJSON_GetObjectItem(AI_nonmotorvehicle,"scheduled_capture_time");
            if(AI_nonmotorvehicle_scheduled_capture_time !=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.scheduled_capture_time=AI_nonmotorvehicle_scheduled_capture_time->valueint;
            }

            cJSON *nonvehlinkage_strategy=cJSON_GetObjectItem(AI_nonmotorvehicle,"linkage_strategy");
            cJSON *item=NULL;
            if(nonvehlinkage_strategy!=NULL)
            {
                cJSON *nonvehspeech=cJSON_GetObjectItem(nonvehlinkage_strategy,"speech");
                if(nonvehspeech!=NULL)
                {
                	item=cJSON_GetObjectItem(nonvehspeech,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_on=item->valueint;
					item=cJSON_GetObjectItem(nonvehspeech,"vol");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_vol=item->valueint;

					item=cJSON_GetObjectItem(nonvehspeech,"repeat");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat=item->valueint;

                    item=cJSON_GetObjectItem(nonvehspeech,"repeat_constant");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat_constant=item->valueint;

                    item=cJSON_GetObjectItem(nonvehspeech,"interval");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_interval=item->valueint;

					item=cJSON_GetObjectItem(nonvehspeech,"url");
					if(item!=NULL)
						strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_url,item->valuestring);

				}
                cJSON *nonvehlight=cJSON_GetObjectItem(nonvehlinkage_strategy,"light");
                if(nonvehlight!=NULL)
                {
            		item=cJSON_GetObjectItem(nonvehlight,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_on=item->valueint;

					item=cJSON_GetObjectItem(nonvehlight,"mode");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_mode=item->valueint;

					item=cJSON_GetObjectItem(nonvehlight,"dur");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur=item->valueint;

                    item=cJSON_GetObjectItem(nonvehlight,"dur_constant");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur_constant=item->valueint;
				}

                cJSON *nonvehbuz=cJSON_GetObjectItem(nonvehlinkage_strategy,"buz");
                if(nonvehbuz!=NULL)
                {
            		item=cJSON_GetObjectItem(nonvehbuz,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_on=item->valueint;

					item=cJSON_GetObjectItem(nonvehbuz,"dur");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_dur=item->valueint;
                }

			}


            cJSON *nonvehzone=cJSON_GetObjectItem(AI_nonmotorvehicle,"alarm_zone");
			if(nonvehzone!=NULL){
				int nonveh_zone_size = cJSON_GetArraySize(nonvehzone);
				//有多少个区域要记录到本地
				int nonvehiclezone = 0;
				for(nonvehiclezone = 0;nonvehiclezone < nonveh_zone_size ;nonvehiclezone++){

					cJSON * nonveharea = cJSON_GetArrayItem(nonvehzone, nonvehiclezone);
					cJSON * nonvehpointCjson = cJSON_GetObjectItem(nonveharea,"point");

					//每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
					cJSON * nonvehiclePoint_left_up = cJSON_GetArrayItem(nonvehpointCjson, 0);
					cJSON * nonvehiclePoint_right_bottom = cJSON_GetArrayItem(nonvehpointCjson, 2);

					//获取这两个点的具体x，y坐标值
					cJSON * nonvehiclePoint_left_up_x = cJSON_GetObjectItem(nonvehiclePoint_left_up,"x");
					cJSON * nonvehiclePoint_left_up_y = cJSON_GetObjectItem(nonvehiclePoint_left_up,"y");
					cJSON * nonvehiclePoint_right_bottom_x = cJSON_GetObjectItem(nonvehiclePoint_right_bottom,"x");
					cJSON * nonvehiclePoint_right_bottom_y = cJSON_GetObjectItem(nonvehiclePoint_right_bottom,"y");

					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_x = nonvehiclePoint_left_up_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_y = nonvehiclePoint_left_up_y->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_x = nonvehiclePoint_right_bottom_x->valueint;
					OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_y = nonvehiclePoint_right_bottom_y->valueint;
				}
			}
        }

        SetAIPassengerEntry(AI_entry, i);

       // SetAIParabolicAerialEntry(AI_entry, i);

        SetAIRegionalPeopleStatEntry(AI_entry, i);

        SetAILaneLineEntry(AI_entry, i);

        SetAIOffDutyEntry(AI_entry, i);

        SetAIHelmetEntry(AI_entry, i);

        SetAIGestureEntry(AI_entry, i);

        SetAiSmokefireEntry(AI_entry, i);

        SetAiFamilyrecognitionEntry(AI_entry, i);

        SetAIPrivacyMaskingEntry(AI_entry, i);

        SetAIFallDetectionEntry(AI_entry, i);

        SetAISleepMonitorEntry(AI_entry, i);
        
        SetAIExpressionRecognitionEntry(AI_entry, i);
    }

    return NULL;
}

static cJSON* SetAlarmSentry(cJSON *channelentry,int i)
{
    cJSON* alarms = cJSON_GetObjectItem(channelentry,"alarms");
    if(alarms!=NULL)
    {
        //io
        cJSON *io=cJSON_GetObjectItem(alarms,"io");
        if(io !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(io,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(io,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.sensitivity=sensitivity->valueint;
            }
        }
        //pir
        cJSON *pir=cJSON_GetObjectItem(alarms,"pir");
        if(pir !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(pir,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(pir,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.sensitivity=sensitivity->valueint;
            }
        }
        //lossLock
        cJSON *lossLock=cJSON_GetObjectItem(alarms,"lossLock");
        if(lossLock !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(lossLock,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.lossLockAlarm.on=on->valueint;
            }

        }

        //cry
        cJSON *cry=cJSON_GetObjectItem(alarms,"cry");
        if(cry !=NULL)
        {
            if (!cJSON_IsObject(cry))
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                return NULL;
            }

            cJSON *on=cJSON_GetObjectItem(cry,"on");
            if(on!=NULL)
            {
                if (!cJSON_IsBool(on))
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }

                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(cry,"sensitivity");
            if(sensitivity!=NULL)
            {
                if(cJSON_Number != sensitivity->type)
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }

                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.sensitivity=sensitivity->valueint;
            }

            cJSON *crying_pacify=cJSON_GetObjectItem(cry,"crying_pacify");
            if(crying_pacify!=NULL)
            {
                if (!cJSON_IsObject(crying_pacify))
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                    return NULL;
                }

                cJSON *crying_pacify_on=cJSON_GetObjectItem(crying_pacify,"on");
                if(crying_pacify_on!=NULL)
                {
                     if (!cJSON_IsBool(crying_pacify_on))
                     {
                         OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                         return NULL;
                     }

                     OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_on=crying_pacify_on->valueint;
                }

                cJSON *crying_pacify_play_count=cJSON_GetObjectItem(crying_pacify,"audio_playing_count");
                if(crying_pacify_play_count!=NULL)
                {
                     if(cJSON_Number != crying_pacify_play_count->type)
                     {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                     }

                     OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_count=crying_pacify_play_count->valueint;
                }

                cJSON *crying_pacify_play_volumn=cJSON_GetObjectItem(crying_pacify,"audio_playing_volumn");
                if(crying_pacify_play_volumn!=NULL)
                {
                     if(cJSON_Number != crying_pacify_play_volumn->type)
                     {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                     }

                     OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_volumn=crying_pacify_play_volumn->valueint;
                }

                cJSON *crying_pacify_play_url=cJSON_GetObjectItem(crying_pacify,"audio_playing_url");
                if(crying_pacify_play_url!=NULL)
                {
                    if(cJSON_String != crying_pacify_play_url->type)
                     {
                        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","json type error!\n");
                        return NULL;
                     }

                    strncpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_url, crying_pacify_play_url->valuestring,sizeof(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_url));
                }
            }

        }

        //voice
        cJSON *voice=cJSON_GetObjectItem(alarms,"voice");
        if(voice !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(voice,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(voice,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.sensitivity=sensitivity->valueint;
            }
        }

        //motion
        cJSON *motion=cJSON_GetObjectItem(alarms,"motion");
        if(motion !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(motion,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(motion,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.sensitivity=sensitivity->valueint;
            }
        }

        //alertarea
        cJSON *alertarea=cJSON_GetObjectItem(alarms,"alertarea");
        cJSON *item=NULL;
        if(alertarea !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(alertarea,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.on=on->valueint;
            }
            cJSON *expel=cJSON_GetObjectItem(alertarea,"expel");
            if(expel!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.expel=expel->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(alertarea,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.sensitivity=sensitivity->valueint;
            }
            cJSON *alertareatime=cJSON_GetObjectItem(alertarea,"alerttime");
            if(alertareatime!=NULL)
            {
                item=cJSON_GetObjectItem(alertareatime,"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.on=item->valueint;
                }
                item=cJSON_GetObjectItem(alertareatime,"starttime");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.starttime,item->valuestring);
                }

                item=cJSON_GetObjectItem(alertareatime,"endtime");
                if(item!=NULL)
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.endtime,item->valuestring);

                item=cJSON_GetObjectItem(alertareatime,"repeat");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                }

            }

            cJSON *area_zone=cJSON_GetObjectItem(alertarea,"zone");
            if(area_zone!=NULL)
            {

                int zone_size = OVD_DEMO_CapInfoV2.alarm.alertarea_params.alertarea_num;
                //有多少个区域要记录到本地
                int k_zone = 0;
                for(k_zone = 0;k_zone < zone_size ;k_zone++)
                {
                    cJSON *area_a = cJSON_GetArrayItem(area_zone, k_zone);

                    cJSON * area_zone_on = cJSON_GetObjectItem(area_a,"on");  // 1.41
                    if(area_zone_on!=NULL)
                    {
                        strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].on,area_zone_on->valuestring);
                    }

                    cJSON * point = cJSON_GetObjectItem(area_a,"point");
                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * areaPoint_left_up = cJSON_GetArrayItem(point, 0);
                    cJSON * areaPoint_left_bottom = cJSON_GetArrayItem(point, 1);
                    cJSON * areaPoint_right_bottom = cJSON_GetArrayItem(point, 2);
                    cJSON * areaPoint_right_up = cJSON_GetArrayItem(point, 3);
                    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","areaPoint_left_up[%d] areaPoint_right_bottom:%s\n",areaPoint_left_up,areaPoint_right_bottom);
                    //获取这两个点的具体x，y坐标值
                    cJSON * areaPoint_left_up_x = cJSON_GetObjectItem(areaPoint_left_up,"x");
                    cJSON * areaPoint_left_up_y = cJSON_GetObjectItem(areaPoint_left_up,"y");
                    cJSON * areaPoint_left_bottom_x = cJSON_GetObjectItem(areaPoint_left_bottom,"x");
                    cJSON * areaPoint_left_bottom_y = cJSON_GetObjectItem(areaPoint_left_bottom,"y");
                    cJSON * areaPoint_right_bottom_x = cJSON_GetObjectItem(areaPoint_right_bottom,"x");
                    cJSON * areaPoint_right_bottom_y = cJSON_GetObjectItem(areaPoint_right_bottom,"y");
                    cJSON * areaPoint_right_up_x = cJSON_GetObjectItem(areaPoint_right_up,"x");
                    cJSON * areaPoint_right_up_y = cJSON_GetObjectItem(areaPoint_right_up,"y");

                    //全屏默认是10000，所以需要用x，y值乘以次码流的分辨率640x360然后除以10000得出真正的坐标

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_up_x = areaPoint_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_up_y = areaPoint_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_bottom_x = areaPoint_left_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_bottom_y = areaPoint_left_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_bottom_x = areaPoint_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_bottom_y = areaPoint_right_bottom_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_up_x = areaPoint_right_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_up_y = areaPoint_right_up_y->valueint;

                }

            }
            cJSON *linkage_strategy=cJSON_GetObjectItem(alertarea,"linkage_strategy");
            if(linkage_strategy!=NULL)
            {
                cJSON *areaspeech=cJSON_GetObjectItem(linkage_strategy,"speech");
                if(areaspeech!=NULL)
                {
                    item=cJSON_GetObjectItem(areaspeech,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_on=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"vol");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_vol=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"repeat");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"repeat_constant");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat_constant=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"interval");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_interval=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"url");
                    if(item!=NULL)
                        strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_url,item->valuestring);

                }
                cJSON *arealight=cJSON_GetObjectItem(linkage_strategy,"light");
                if(arealight!=NULL)
                {
                    item=cJSON_GetObjectItem(arealight,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_on=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"mode");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_mode=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"dur");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"dur_constant");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur_constant=item->valueint;
                }
                cJSON *areabuz=cJSON_GetObjectItem(linkage_strategy,"buz");
                if(areabuz!=NULL)
                {
                    item=cJSON_GetObjectItem(areabuz,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_on=item->valueint;

                    item=cJSON_GetObjectItem(areabuz,"dur");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_dur=item->valueint;
                }

            }
            cJSON *targetType=cJSON_GetObjectItem(alertarea,"targetType");
            if(targetType!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.targetType=targetType->valueint;
            }

            cJSON *staymode_time=cJSON_GetObjectItem(alertarea,"staymode_time");
            if(staymode_time!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.staymode_time=staymode_time->valueint;
            }
        }

        //越界侦测
        cJSON *transgressionentry=cJSON_GetObjectItem(alarms,"transgression");
        if(transgressionentry !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(transgressionentry,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.on=on->valueint;
            }
            cJSON *targetType=cJSON_GetObjectItem(transgressionentry,"targetType");
            if(targetType!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.targetType=targetType->valueint;
            }
            cJSON *alertareatime=cJSON_GetObjectItem(transgressionentry,"alerttime");
            if(alertareatime!=NULL)
            {
                cJSON * item=cJSON_GetObjectItem(alertareatime,"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.on=item->valueint;
                }
                item=cJSON_GetObjectItem(alertareatime,"starttime");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.starttime,item->valuestring);
                }

                item=cJSON_GetObjectItem(alertareatime,"endtime");
                if(item!=NULL)
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.endtime,item->valuestring);

                item=cJSON_GetObjectItem(alertareatime,"repeat");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                }
            }
            //第一条规则线
            cJSON *statistics_line=cJSON_GetObjectItem(transgressionentry,"statistics_line");
            if(statistics_line !=NULL)
            {

                cJSON * statistics_line_on = cJSON_GetObjectItem(statistics_line,"on");
                if(statistics_line_on!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.on=statistics_line_on->valueint;
                }
                item = cJSON_GetObjectItem(statistics_line,"A_x");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_x=item->valueint;
                }
                item=cJSON_GetObjectItem(statistics_line,"A_y");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_y=item->valueint;
                }
                item=cJSON_GetObjectItem(statistics_line,"B_x");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_x=item->valueint;
                }
                item=cJSON_GetObjectItem(statistics_line,"B_y");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_y=item->valueint;
                }
            }
            cJSON *statistics_mode_style=cJSON_GetObjectItem(transgressionentry,"statistics_mode_style");
            if(statistics_mode_style!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_style=statistics_mode_style->valueint;
            }
            cJSON *statistics_mode_direction=cJSON_GetObjectItem(transgressionentry,"statistics_mode_direction");
            if(statistics_mode_direction!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_direction=statistics_mode_direction->valueint;
            }
            //额外的规则线
            cJSON *statistics_line_another=cJSON_GetObjectItem(transgressionentry,"statistics_line_another");
            if(statistics_line_another!=NULL)
            {
                int line_another_size = OVD_DEMO_CapInfoV2.alarm.transgression_params.statistics_line_another_num;
                int k_line = 0;
                for(k_line = 0;k_line < line_another_size ;k_line++)
                {
                    cJSON *line_a = cJSON_GetArrayItem(statistics_line_another, k_line);

                    cJSON * statistics_line_on = cJSON_GetObjectItem(line_a,"on");
                    if(statistics_line_on!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].on=statistics_line_on->valueint;
                    }

                    cJSON *statistics_line=cJSON_GetObjectItem(line_a,"line");
                    if(statistics_line !=NULL)
                    {
                        item = cJSON_GetObjectItem(statistics_line,"A_x");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_A_x=item->valueint;
                        }
                        item=cJSON_GetObjectItem(statistics_line,"A_y");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_A_y=item->valueint;
                        }
                        item=cJSON_GetObjectItem(statistics_line,"B_x");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_B_x=item->valueint;
                        }
                        item=cJSON_GetObjectItem(statistics_line,"B_y");
                        if(item!=NULL)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_B_y=item->valueint;
                        }
                    }
                    cJSON *statistics_mode_style=cJSON_GetObjectItem(line_a,"statistics_mode_style");
                    if(statistics_mode_style!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_mode_style=statistics_mode_style->valueint;
                    }
                    cJSON *statistics_mode_direction=cJSON_GetObjectItem(line_a,"statistics_mode_direction");
                    if(statistics_mode_direction!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_mode_direction=statistics_mode_direction->valueint;
                    }
                }
            }

            cJSON *linkage_strategy=cJSON_GetObjectItem(transgressionentry,"linkage_strategy");
            if(linkage_strategy!=NULL)
            {
                cJSON *areaspeech=cJSON_GetObjectItem(linkage_strategy,"speech");
                if(areaspeech!=NULL)
                {
                    item=cJSON_GetObjectItem(areaspeech,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_on=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"vol");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_vol=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"repeat");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"repeat_constant");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat_constant=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"interval");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_interval=item->valueint;

                    item=cJSON_GetObjectItem(areaspeech,"url");
                    if(item!=NULL)
                        strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_url,item->valuestring);

                }
                cJSON *arealight=cJSON_GetObjectItem(linkage_strategy,"light");
                if(arealight!=NULL)
                {
                    item=cJSON_GetObjectItem(arealight,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_on=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"mode");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_mode=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"dur");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur=item->valueint;

                    item=cJSON_GetObjectItem(arealight,"dur_constant");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur_constant=item->valueint;
                }
                cJSON *areabuz=cJSON_GetObjectItem(linkage_strategy,"buz");
                if(areabuz!=NULL)
                {
                    item=cJSON_GetObjectItem(areabuz,"on");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_on=item->valueint;

                    item=cJSON_GetObjectItem(areabuz,"dur");
                    if(item!=NULL)
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_dur=item->valueint;
                }

            }
        }

        //WIFI感知联动
        cJSON *wifisensingentry=cJSON_GetObjectItem(alarms,"wifi_sensing");
        if(wifisensingentry !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(wifisensingentry,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.on=on->valueint;
            }
            cJSON *linkage_on=cJSON_GetObjectItem(wifisensingentry,"linkage_on");
            if(linkage_on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.linkage_on=linkage_on->valueint;
            }
            cJSON *sensing_mode=cJSON_GetObjectItem(wifisensingentry,"sensing_mode");
            if(sensing_mode!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.sensing_mode=sensing_mode->valueint;
            }
            cJSON *alertareatime=cJSON_GetObjectItem(wifisensingentry,"schedule_time");
            if(alertareatime!=NULL)
            {
                cJSON * item=cJSON_GetObjectItem(alertareatime,"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.on=item->valueint;
                }
                item=cJSON_GetObjectItem(alertareatime,"starttime");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.starttime,item->valuestring);
                }

                item=cJSON_GetObjectItem(alertareatime,"endtime");
                if(item!=NULL)
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.endtime,item->valuestring);

                item=cJSON_GetObjectItem(alertareatime,"repeat");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.repeat=OVD_DEMO_OVCRepeatCnt(item);
                }
            }
            cJSON *entire_dev_management=cJSON_GetObjectItem(wifisensingentry,"entire_dev_management");
            int entire_dev_num = cJSON_GetArraySize(entire_dev_management);
            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_dev_num= entire_dev_num;
            int j_dev_num = 0;
            for(j_dev_num = 0;j_dev_num < entire_dev_num; j_dev_num++)
            {
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(entire_dev_management,j_dev_num),"name");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].name, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(entire_dev_management,j_dev_num),"option60");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].option60, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(entire_dev_management,j_dev_num),"ip");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].ip, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(entire_dev_management,j_dev_num),"mac");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].mac, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(entire_dev_management,j_dev_num),"on");
                if(item!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[j_dev_num].on=item->valueint;
                }
            }
            cJSON *added_dev_management=cJSON_GetObjectItem(wifisensingentry,"added_dev_management");
            int added_dev_num = cJSON_GetArraySize(added_dev_management);
            OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_dev_num= added_dev_num;
            for(j_dev_num = 0;j_dev_num < added_dev_num; j_dev_num++)
            {
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(added_dev_management,j_dev_num),"name");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].name, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(added_dev_management,j_dev_num),"option60");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].option60, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(added_dev_management,j_dev_num),"ip");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].ip, item->valuestring);
                }
                item = cJSON_GetObjectItem(cJSON_GetArrayItem(added_dev_management,j_dev_num),"mac");
                if(item!=NULL)
                {
                    strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[j_dev_num].mac, item->valuestring);
                }
            }

        }

        //cross
        cJSON *cross=cJSON_GetObjectItem(alarms,"cross");
        if(cross !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(cross,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(motion,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.sensitivity=sensitivity->valueint;
            }
        }

        //body
        cJSON *body=cJSON_GetObjectItem(alarms,"body");
        if(body !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(body,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(body,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.sensitivity=sensitivity->valueint;
            }
            cJSON *zone=cJSON_GetObjectItem(body,"zone");
            if(zone!=NULL)
            {
                int zone_size = cJSON_GetArraySize(zone);
                //有多少个区域要记录到本地
                int m = 0;
                for(m = 0;m < zone_size ;m++)
                {

                    cJSON * area = cJSON_GetArrayItem(zone, m);
                    cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * areaPoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
                    cJSON * areaPoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);

                    //获取这两个点的具体x，y坐标值
                    cJSON * areaPoint_left_up_x = cJSON_GetObjectItem(areaPoint_left_up,"x");
                    cJSON * areaPoint_left_up_y = cJSON_GetObjectItem(areaPoint_left_up,"y");
                    cJSON * areaPoint_right_bottom_x = cJSON_GetObjectItem(areaPoint_right_bottom,"x");
                    cJSON * areaPoint_right_bottom_y = cJSON_GetObjectItem(areaPoint_right_bottom,"y");

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_x = areaPoint_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_y = areaPoint_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_x = areaPoint_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_y = areaPoint_right_bottom_y->valueint;
                }
            }
        }

        /* 3D传感器告警配置解析 */
        cJSON* sensor_3d = cJSON_GetObjectItem(alarms, "3d_light_sensor");
        if (sensor_3d != NULL) {
            // 解析主开关
            cJSON* on = cJSON_GetObjectItem(sensor_3d, "on");
            if (on != NULL) {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.on = on->valueint;
            }

            // 解析跌倒检测配置
            cJSON* detect = cJSON_GetObjectItem(sensor_3d, "falldown_detect");
            if (detect != NULL) {
                cJSON* detect_on = cJSON_GetObjectItem(detect, "on");
                if (detect_on != NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.on = detect_on->valueint;
                }

                cJSON* time = cJSON_GetObjectItem(detect, "time");
                if (time != NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.time = time->valueint;
                }
            }

            // 解析跌倒检测配置
            cJSON* trapped = cJSON_GetObjectItem(sensor_3d, "wc_trapped");
            if (trapped != NULL) {
                cJSON* on = cJSON_GetObjectItem(trapped, "on");
                if (on != NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.on = on->valueint;
                }

                cJSON* time = cJSON_GetObjectItem(trapped, "time");
                if (time != NULL) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.time = time->valueint;
                }
            }
        }
        //pet
        cJSON *pet=cJSON_GetObjectItem(alarms,"pet");
        if(pet !=NULL)
        {
            cJSON *on=cJSON_GetObjectItem(pet,"on");
            if(on!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.on=on->valueint;
            }
            cJSON *sensitivity=cJSON_GetObjectItem(pet,"sensitivity");
            if(sensitivity!=NULL)
            {
                OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.sensitivity=sensitivity->valueint;
            }
            cJSON *zone=cJSON_GetObjectItem(pet,"zone");
            if(zone!=NULL)
            {
                int zone_size = cJSON_GetArraySize(zone);
                //有多少个区域要记录到本地
                int m = 0;
                for(m = 0;m < zone_size ;m++)
                {

                    cJSON * area = cJSON_GetArrayItem(zone, m);
                    cJSON * pointCjson = cJSON_GetObjectItem(area,"point");

                    //每个area 都有4个点,算法只需要使用左上的点坐标和右下的点坐标
                    cJSON * areaPoint_left_up = cJSON_GetArrayItem(pointCjson, 0);
                    cJSON * areaPoint_right_bottom = cJSON_GetArrayItem(pointCjson, 2);

                    //获取这两个点的具体x，y坐标值
                    cJSON * areaPoint_left_up_x = cJSON_GetObjectItem(areaPoint_left_up,"x");
                    cJSON * areaPoint_left_up_y = cJSON_GetObjectItem(areaPoint_left_up,"y");
                    cJSON * areaPoint_right_bottom_x = cJSON_GetObjectItem(areaPoint_right_bottom,"x");
                    cJSON * areaPoint_right_bottom_y = cJSON_GetObjectItem(areaPoint_right_bottom,"y");

                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_x = areaPoint_left_up_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_y = areaPoint_left_up_y->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_x = areaPoint_right_bottom_x->valueint;
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_y = areaPoint_right_bottom_y->valueint;
                }
            }
        }
    }
    return NULL;
}



int OVD_DEMO_SetOVDConfigureInfo(char* in_ovdconfig,char* err_msg_info,int str_len){

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_SetOVDConfigureInfo,str_len %d \n",str_len);
    //给我一个json序列化的代码
    //服务器过来设置
    cJSON *root=NULL;
    cJSON *channelsarray=NULL;
    cJSON *channelentry=NULL;
    cJSON *screenentry=NULL;
    cJSON *autorebootentry=NULL;
    cJSON *yard_light_config_entry=NULL;
    cJSON *eboentry=NULL;
    cJSON *owlentry=NULL;
    cJSON *owlentry_cruise=NULL;
    cJSON *eboentry_cruise=NULL;
    cJSON *dashcamentry=NULL;
    cJSON *linkage_tracking=NULL;
    cJSON *item=NULL;
    int ret = 0;
    int i;
  	int repeat_cnt=0;
    int osdarraySize=0,osd_num=0;
    int timed_schedule_array_size=0;
    int timed_schedule_array_num=0;

	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","input [%s] \n", in_ovdconfig);

    root=cJSON_Parse(in_ovdconfig);
    if(root ==NULL)
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","!!!==cjson parse failed\n");

    item=cJSON_GetObjectItem(root,"tz");
    if(item!=NULL)
        OVD_DEMO_ConfigureInfo.tz = item->valueint;

	item= cJSON_GetObjectItem(root,"led");
	if(item!=NULL)
		OVD_DEMO_ConfigureInfo.led=item->valueint;

    item= cJSON_GetObjectItem(root,"light_supplement_lamp_on");
    if(item!=NULL)
		OVD_DEMO_ConfigureInfo.light_supplement_lamp_on=item->valueint;

    screenentry=cJSON_GetObjectItem(root,"screen");
    if(screenentry != NULL)
    {
        item= cJSON_GetObjectItem(screenentry,"on");
	    if(item!=NULL)
		    OVD_DEMO_ConfigureInfo.screen=item->valueint;
    }

    item= cJSON_GetObjectItem(root,"dutMTU");
    if(item!=NULL)
        dutMTU=item->valueint;


    do
    {
    eboentry=cJSON_GetObjectItem(root,"EBO");
    if(eboentry!=NULL)
    {

        item= cJSON_GetObjectItem(eboentry,"EBO_nofall_on");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","EBO_nofall_on %d\n",OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on);
            }
            else
            {
                ret = -1;
                break;
            }
        }
        item= cJSON_GetObjectItem(eboentry,"EBO_laser_on");
        if(item!=NULL){
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_laser_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","EBO_laser_on %d\n",OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on);
            }
            else
            {
                ret = -1;
                break;
            }

        }
        item= cJSON_GetObjectItem(eboentry,"EBO_track_on");
        if(item!=NULL){

            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","EBO_track_on %d\n",OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_on);
            }
            else
            {
                ret = -1;
                break;
            }

        }
        item= cJSON_GetObjectItem(eboentry,"EBO_track_type");
        if(item!=NULL){
            if(cJSON_Number != item->type)
            {
                ret = -1;
                break;
            }
            if(1 == item->valueint || 2 == item->valueint)
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_type=item->valueint;
            else
            {
                ret = -1;
                break;
            }
        }
        item= cJSON_GetObjectItem(eboentry,"EBO_collision_on");
        if(item!=NULL)
        {
            if(cJSON_False == item->type || cJSON_True == item->type)
            {
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_collision_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","EBO_collision_on %d\n",OVD_DEMO_ConfigureInfo.eboconfig.EBO_collision_on);
            }
            else{
                ret = -1;
                break;
            }

        }
        else
        {
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","have no EBO_collision_on\n");
        }

        item= cJSON_GetObjectItem(eboentry,"EBO_reminder_on");
        if(item!=NULL)
        {
            if(cJSON_False == item->type || cJSON_True == item->type)
            {
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_reminder_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","EBO_reminder_on %d\n",OVD_DEMO_ConfigureInfo.eboconfig.EBO_reminder_on);
            }
            else{
                ret = -1;
                break;
            }

        }
        else
        {
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","have no EBO_reminder_on\n");
        }

        item= cJSON_GetObjectItem(eboentry,"EBO_speed_adjust");
        if(item!=NULL)
        {
            if(cJSON_Number == item->type && item->valueint >= 0 && item->valueint <= 4)
                OVD_DEMO_ConfigureInfo.eboconfig.EBO_speed_adjust=item->valueint;
            else
            {
                ret = -1;
                break;
            }
        }

        eboentry_cruise=cJSON_GetObjectItem(eboentry,"EBO_cruise_on");
        if(eboentry_cruise !=NULL)
        {
            item = cJSON_GetObjectItem(eboentry_cruise,"on");
            if(item!=NULL)
            {

                if(cJSON_False == item->type  || cJSON_True == item->type )
                {
                    OVD_DEMO_ConfigureInfo.eboconfig.EBO_cruise_on=item->valueint;
                }
                else{
                    ret = -1;
                    break;
                }
            }

            cJSON * start_time = cJSON_GetObjectItem(eboentry_cruise, "start_time");
            if(NULL != start_time){

                if(cJSON_IsString(start_time)){

                    strncpy(OVD_DEMO_ConfigureInfo.eboconfig.start_time, start_time->valuestring,sizeof(OVD_DEMO_ConfigureInfo.eboconfig.start_time));
                }
                else{
                    ret = -1;
                    break;
                }
            }
            item = cJSON_GetObjectItem(eboentry_cruise,"time_long");
            if(item!=NULL){
                if(cJSON_Number == item->type && item->valueint > 0)
                {
                    OVD_DEMO_ConfigureInfo.eboconfig.time_long=item->valueint;
                }
                else{
                    ret = -1;
                    break;
                }
            }
            item=cJSON_GetObjectItem(eboentry_cruise,"repeat");
            if(item!=NULL){
                repeat_cnt=OVD_DEMO_OVCRepeatCnt(item);
                OVD_DEMO_ConfigureInfo.eboconfig.repeat=repeat_cnt;

            }
        }
    }

    owlentry=cJSON_GetObjectItem(root,"OWL");
    if(owlentry != NULL)
    {
        item= cJSON_GetObjectItem(owlentry,"OWL_food_on");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.robot_config.OWL_food_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OWL_food_on %d\n",OVD_DEMO_ConfigureInfo.robot_config.OWL_food_on);
            }
            else{
                ret = -1;
                break;
            }
        }
        item= cJSON_GetObjectItem(owlentry,"OWL_track_mode");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.robot_config.OWL_track_mode=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OWL_track_mode %d\n",OVD_DEMO_ConfigureInfo.robot_config.OWL_track_mode);
            }
            else{
                ret = -1;
                break;
            }
        }
        item= cJSON_GetObjectItem(owlentry,"OWL_touch_mode");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.robot_config.OWL_touch_mode=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OWL_touch_mode %d\n",OVD_DEMO_ConfigureInfo.robot_config.OWL_touch_mode);
            }
            else{
                ret = -1;
                break;
            }

        }
        item= cJSON_GetObjectItem(owlentry,"OWL_sound_on");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.robot_config.OWL_sound_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OWL_sound_on %d\n",OVD_DEMO_ConfigureInfo.robot_config.OWL_sound_on);
            }
            else{
                ret = -1;
                break;
            }

        }
        item= cJSON_GetObjectItem(owlentry,"OWL_speed_switch");
        if(item!=NULL)
        {
            if(cJSON_Number == item->type && item->valueint >= 0 && item->valueint < 3)
            {
                OVD_DEMO_ConfigureInfo.robot_config.OWL_speed_switch=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OWL_speed_switch %d\n",OVD_DEMO_ConfigureInfo.robot_config.OWL_speed_switch);
            }
            else{
                ret = -1;
                break;
            }
        }
        owlentry_cruise=cJSON_GetObjectItem(owlentry,"OWL_cruise_on");
        if(owlentry_cruise !=NULL)
        {
            cJSON * start_time = cJSON_GetObjectItem(owlentry_cruise, "start_time");
            if(NULL != start_time){
                if(cJSON_IsString(start_time)){
                    strncpy(OVD_DEMO_ConfigureInfo.robot_config.start_time, start_time->valuestring,sizeof(OVD_DEMO_ConfigureInfo.robot_config.start_time));
                }
                else{
                    ret = -1;
                    break;
                    }
                }
            item=cJSON_GetObjectItem(owlentry_cruise,"repeat");
            if(item!=NULL){
                repeat_cnt=OVD_DEMO_OVCRepeatCnt(item);
                OVD_DEMO_ConfigureInfo.robot_config.repeat=repeat_cnt;
            }
        }
    }

    cJSON *moboentry=NULL;
    moboentry=cJSON_GetObjectItem(root,"mobocam");
    if(moboentry != NULL)
    {
        item= cJSON_GetObjectItem(moboentry,"speedSwitch");
        if(item!=NULL)
        {
            cJSON *cmd_entry=NULL;
            cmd_entry = cJSON_GetObjectItem(item,"cmd");
            if(cJSON_Number == cmd_entry->type && cmd_entry->valueint >= 0)
            {
                OVD_DEMO_ConfigureInfo.mobocam_config.speedswith=cmd_entry->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","mobocam_speed_switch %d\n",OVD_DEMO_ConfigureInfo.mobocam_config.speedswith);
            }
            else{
                ret = -1;
                break;
            }
        }
        cJSON *moboentry_cruise=NULL;
        moboentry_cruise=cJSON_GetObjectItem(moboentry,"cruiseControl");
        OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount =cJSON_GetArraySize(moboentry_cruise);
        if(moboentry_cruise !=NULL)
        {
            cJSON *pointentry=NULL;
            int rebot_num = 0;
            for(rebot_num=0;rebot_num<OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount;rebot_num++){
                pointentry=cJSON_GetArrayItem(moboentry_cruise,rebot_num);
                item=cJSON_GetObjectItem(pointentry,"cruiseId");
                OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].cruiseId = item->valueint;
                item=cJSON_GetObjectItem(pointentry,"time_long");
                OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].time_long = item->valueint;
                item=cJSON_GetObjectItem(pointentry,"start_time");
                if(item!=NULL)
                {
                strncpy(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].start_time, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].start_time));
                }
                item=cJSON_GetObjectItem(pointentry,"end_time");
                if(item!=NULL)
                {
                strncpy(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].end_time, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].end_time));
                }           
                item=cJSON_GetObjectItem(pointentry,"repeat");
                if(item!=NULL){
                    OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[rebot_num].repeat=OVD_DEMO_OVCRepeatCnt(item);
    
                }

            }
        }
        item= cJSON_GetObjectItem(moboentry,"track");
        if(item!=NULL)
        {
            cJSON *type_entry=NULL;
            type_entry = cJSON_GetObjectItem(item,"type");
            OVD_DEMO_ConfigureInfo.mobocam_config.track_type=type_entry->valueint;
        
            cJSON *on_entry=NULL;
            on_entry = cJSON_GetObjectItem(item,"on");
            OVD_DEMO_ConfigureInfo.mobocam_config.track_on=on_entry->valueint;

        }
    }

    dashcamentry=cJSON_GetObjectItem(root,"dashcam");
    if(dashcamentry != NULL)
    {
        item= cJSON_GetObjectItem(dashcamentry,"dashcam_findcar_on");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_findcar_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","dashcam_findcar_on %d\n",OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_findcar_on);
            }
            else{
                ret = -1;
                break;
            }
        }
        item= cJSON_GetObjectItem(dashcamentry,"dashcam_rescue_on");
        if(item!=NULL)
        {
            if(cJSON_True == item->type || cJSON_False == item->type)
            {
                OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_rescue_on=item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","dashcam_rescue_on %d\n",OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_rescue_on);
            }
            else{
                ret = -1;
                break;
            }
        }
    }

    cJSON *feeder_config_entry=NULL;
    cJSON *feeder_immed_config_entry=NULL;
    feeder_config_entry=cJSON_GetObjectItem(root,"petfeeder");
    if(feeder_config_entry!=NULL)
    {

        feeder_immed_config_entry= cJSON_GetObjectItem(feeder_config_entry,"feeder_immed");
        if(feeder_immed_config_entry!=NULL)
        {
            item= cJSON_GetObjectItem(feeder_immed_config_entry,"feeder_quantity");
            if(item!=NULL)
            {
                if(cJSON_Number == item->type && item->valueint > 0 && item->valueint < 11)
                {
                    OVD_DEMO_ConfigureInfo.feeder_immed_quantity=item->valueint;
                    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","feeder_immed_quantity %d\n",OVD_DEMO_ConfigureInfo.feeder_immed_quantity);
                }
                else{
                    ret = -1;
                    break;
                }
            }
            else{
                ret = -1;
                break;
            }

        }
        cJSON *feeder_plan_config_entry=NULL;
        int feeder_num = 0;
        cJSON *pointentry=NULL;
        feeder_plan_config_entry = cJSON_GetObjectItem(feeder_config_entry,"feeder_plan");
        if(feeder_plan_config_entry != NULL){
            OVD_DEMO_ConfigureInfo.feedercount =cJSON_GetArraySize(feeder_plan_config_entry);
            if(OVD_DEMO_ConfigureInfo.feedercount >10)
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","feeder_plan > 10,ERROR!\n");
            for(feeder_num=0;feeder_num<OVD_DEMO_ConfigureInfo.feedercount;feeder_num++){
                pointentry=cJSON_GetArrayItem(feeder_plan_config_entry,feeder_num);
                item=cJSON_GetObjectItem(pointentry,"on");
                if(item != NULL){
                    if(cJSON_False== item->type || cJSON_True == item->type)
                    {
                        OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].on=item->valueint;
                    }
                    else{
                        ret = -1;
                        break;
                    }

                }
                item=cJSON_GetObjectItem(pointentry,"feeder_quantity");
                if(item != NULL){
                    if(cJSON_Number == item->type && item->valueint > 0 && item->valueint < 11)
                        OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].quantity=item->valueint;
                    else{
                        ret = -1;
                        break;
                    }
                }
                item=cJSON_GetObjectItem(pointentry,"feeder_name");
                if(item != NULL){
                    if(cJSON_String == item->type)
                        strncpy(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name));
                    else{
                        ret = -1;
                        break;
                    }
                }
                item=cJSON_GetObjectItem(pointentry,"feeder_time");
                if(item != NULL){
                    if(cJSON_String == item->type)
                        strncpy(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time));
                    else{
                        ret = -1;
                        break;
                    }
                }
                item=cJSON_GetObjectItem(pointentry,"repeat");
                if(item!=NULL){
                    repeat_cnt=OVD_DEMO_OVCRepeatCnt(item);
                    OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].repeat=repeat_cnt;

                }
            }

        }
    }
    } while (0);

    autorebootentry=cJSON_GetObjectItem(root,"auto_reboot");
    if(autorebootentry!=NULL)
    {
        item=cJSON_GetObjectItem(autorebootentry,"light_brightness");
        if(item!=NULL)
            OVD_DEMO_ConfigureInfo.autorebootinfo.on=item->valueint;

        item=cJSON_GetObjectItem(autorebootentry,"cycle");
        if(item!=NULL)
            OVD_DEMO_ConfigureInfo.autorebootinfo.cycle=(long long)item->valuedouble;

        item=cJSON_GetObjectItem(autorebootentry,"start");
        if(item!=NULL)
            strcpy(OVD_DEMO_ConfigureInfo.autorebootinfo.start,item->valuestring);

        item=cJSON_GetObjectItem(autorebootentry,"end");
        if(item!=NULL)
            strcpy(OVD_DEMO_ConfigureInfo.autorebootinfo.end,item->valuestring);
    }

    yard_light_config_entry=cJSON_GetObjectItem(root,"yard_light_config");
    if(yard_light_config_entry!=NULL)
    {
        item=cJSON_GetObjectItem(yard_light_config_entry,"light_brightness");
        if(item!=NULL)
            OVD_DEMO_ConfigureInfo.yardlightconfig.light_brightness=item->valueint;

        item=cJSON_GetObjectItem(yard_light_config_entry,"control_mode");
        if(item!=NULL)
            OVD_DEMO_ConfigureInfo.yardlightconfig.control_mode=item->valueint;

        item=cJSON_GetObjectItem(yard_light_config_entry,"light_on");
        if(item!=NULL)
            OVD_DEMO_ConfigureInfo.yardlightconfig.light_on=item->valueint;

        cJSON *timed_schedule_entry=NULL;
        cJSON *timed_schedule_array = cJSON_GetObjectItem(yard_light_config_entry, "timed_schedule");
        if (timed_schedule_array != NULL){
            timed_schedule_array_size = cJSON_GetArraySize(timed_schedule_array);
            OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule_size = timed_schedule_array_size;
            for (timed_schedule_array_num = 0; timed_schedule_array_num < timed_schedule_array_size; timed_schedule_array_num++){
                timed_schedule_entry=cJSON_GetArrayItem(timed_schedule_array, timed_schedule_array_num);
                cJSON * start_time = cJSON_GetObjectItem(timed_schedule_entry, "start_time");
                if(NULL != start_time){
                    if(cJSON_IsString(start_time)){
                        strcpy(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[timed_schedule_array_num].start_time, start_time->valuestring);
                    }
                }
                cJSON *end_time = cJSON_GetObjectItem(timed_schedule_entry, "end_time");
                if(NULL != end_time){
                    if(cJSON_IsString(end_time)){
                        strcpy(OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[timed_schedule_array_num].end_time, end_time->valuestring);
                    }
                }
            }
        }
    }

    // 枪球联动
    linkage_tracking = cJSON_GetObjectItem(root, "linkage_tracking");
    if (linkage_tracking != NULL) {
        cJSON *linkage_tracking_smart = cJSON_GetObjectItem(linkage_tracking, "linkage_tracking_smart");
        if (linkage_tracking_smart != NULL) {
            item = cJSON_GetObjectItem(linkage_tracking_smart, "on");
            if (item != NULL) {
                OVD_DEMO_ConfigureInfo.linkage_tracking_smart_on = item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO, "DEV/DEMO", "linkage_tracking_smart_on %d\n", OVD_DEMO_ConfigureInfo.linkage_tracking_smart_on);
            }
        }
        cJSON *linkage_tracking_manual = cJSON_GetObjectItem(linkage_tracking, "linkage_tracking_manual");
        if (linkage_tracking_manual != NULL) {
            item = cJSON_GetObjectItem(linkage_tracking_manual, "on");
            if (item != NULL) {
                OVD_DEMO_ConfigureInfo.linkage_tracking_manual_on = item->valueint;
                OVD_LOG(OVD_LOGLEVEL_INFO, "DEV/DEMO", "linkage_tracking_manual_on %d\n", OVD_DEMO_ConfigureInfo.linkage_tracking_manual_on);
            }
        }
    }

    channelsarray=cJSON_GetObjectItem(root,"channels");
    if(channelsarray != NULL)
    {
        for(i=0;i<OVD_DEMO_ConfigureInfo.channelsInfoCount;i++)
        {
            char channel_str[4];
            sprintf(channel_str,"%d",OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
            channelentry=cJSON_GetObjectItem(channelsarray,channel_str);

            if(channelentry!=NULL)
            {
                cJSON *channel_rebot=NULL;
                channel_rebot = cJSON_GetObjectItem(channelentry,"robotKeeper");
                cJSON *channel_extra=NULL;
                channel_extra = cJSON_GetObjectItem(channel_rebot,"extraInfo");
                cJSON *channel_remind=NULL;
                channel_remind = cJSON_GetObjectItem(channel_extra,"remindConfig");
                OVD_DEMO_ConfigureInfo.robotcount =cJSON_GetArraySize(channel_remind);
                cJSON *pointentry=NULL;
                int rebot_num = 0;

                for(rebot_num=0;rebot_num<OVD_DEMO_ConfigureInfo.robotcount;rebot_num++){
                    pointentry=cJSON_GetArrayItem(channel_remind,rebot_num);
                    item=cJSON_GetObjectItem(pointentry,"bizType");
                    OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].bizType = item->valueint;
                    item=cJSON_GetObjectItem(pointentry,"startTime");
                    strncpy(OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].start_time, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].start_time));
                    item=cJSON_GetObjectItem(pointentry,"endTime");
                    strncpy(OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].end_time, item->valuestring,sizeof(OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].end_time));

                }
                

                cJSON *channel_on=NULL;
                channel_on = cJSON_GetObjectItem(channelentry,"on");
                if(channel_on != NULL)
                {
                    //to do:
                    //设备侧需要做的动作
                    //如果设置为0，（从1到0）设备要调用OVD_AVPushEnd 停止喂流
                    //如果设置为1，（从0到1）设备要调用OVD_AVPushData 开始喂流
                    if (channel_on->valueint)
                    {
                        OVD_Demo_init_channel(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
                        OVD_Demo_start_push(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
                    }
                    else
                    {
                        OVD_Demo_deinit_channel(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
                        OVD_Demo_stop_push(OVD_DEMO_ConfigureInfo.channelsInfo[i].channel);
                    }
                }
                
                cJSON *AISpeech=NULL;
                AISpeech = cJSON_GetObjectItem(channelentry,"AISpeech");
                if(!AISpeech || !cJSON_IsObject(AISpeech))
                {
                    printf("AISpeech not exist\n");
                }
                else
                {
                    char *detail_str=cJSON_PrintUnformatted(AISpeech);
                    printf("wjj receive AISpeech detail: %s\n",detail_str);
                    strcpy(g_aispeech_config_str,detail_str);
                    free(detail_str);

                    item=cJSON_GetObjectItem(AISpeech,"status");
                    if(item!=NULL)
                    {
                        g_aispeech_status=item->valueint;
                    }

                    cJSON* aispeech_scheduleplan=cJSON_GetObjectItem(AISpeech,"scheduledplan");
                    item=cJSON_GetObjectItem(aispeech_scheduleplan,"on");
                    if(item!=NULL)
                        g_aispeech_scheduleplan_on=item->valueint;

                    item=cJSON_GetObjectItem(aispeech_scheduleplan,"starttime");
                    if(item!=NULL)
                    {
                        memset(g_aispeech_scheduleplan_startime,0,sizeof(g_aispeech_scheduleplan_startime));
                        strcpy(g_aispeech_scheduleplan_startime,item->valuestring);
                    }

                    item=cJSON_GetObjectItem(aispeech_scheduleplan,"endtime");
                    if(item!=NULL)
                    {
                        memset(g_aispeech_scheduleplan_endtime,0,sizeof(g_aispeech_scheduleplan_endtime));
                        strcpy(g_aispeech_scheduleplan_endtime,item->valuestring);
                    }

                    item=cJSON_GetObjectItem(aispeech_scheduleplan,"repeat");
                    if(item!=NULL)
                    {
                        g_aispeech_scheduleplan_repeat=OVD_DEMO_OVCRepeatCnt(item);
                    }

                    printf("===wjj==aispeech==status:%d plan:%d %s %s %d====\n",
                        g_aispeech_status,g_aispeech_scheduleplan_on,g_aispeech_scheduleplan_startime,g_aispeech_scheduleplan_endtime,g_aispeech_scheduleplan_repeat);
                }

                cJSON *video_encoding=NULL;
                video_encoding = cJSON_GetObjectItem(channelentry,"video_encoding");
                if(video_encoding!=NULL)
                {

                    cJSON *channel_static_scence_ABR_on =NULL;
                    channel_static_scence_ABR_on=cJSON_GetObjectItem(video_encoding,"static_scence_ABR_on");
                    if(channel_static_scence_ABR_on != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].static_scence_ABR_on=channel_static_scence_ABR_on->valueint;
                    }

                    cJSON *channel_bitrate_mode =NULL;
                    channel_bitrate_mode=cJSON_GetObjectItem(video_encoding,"bitrate_mode");
                    if(channel_bitrate_mode != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_mode=channel_bitrate_mode->valueint;
                    }

                    cJSON *channel_bitrate_average =NULL;
                    channel_bitrate_average=cJSON_GetObjectItem(video_encoding,"bitrate_average");
                    if(channel_bitrate_average != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_average=channel_bitrate_average->valueint;
                    }

                    cJSON *channel_bitrate_peak =NULL;
                    channel_bitrate_peak=cJSON_GetObjectItem(video_encoding,"bitrate_peak");
                    if(channel_bitrate_peak != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_peak=channel_bitrate_peak->valueint;
                    }

                    cJSON *channel_bitrate_statically =NULL;
                    channel_bitrate_statically=cJSON_GetObjectItem(video_encoding,"bitrate_statically");
                    if(channel_bitrate_statically != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_statically=channel_bitrate_statically->valueint;
                    }

                    cJSON *channel_framerate =NULL;
                    channel_framerate=cJSON_GetObjectItem(video_encoding,"framerate");
                    if(channel_framerate != NULL)
                    {
                        if(channel_framerate->valueint >=15)
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].framerate=channel_framerate->valueint;
                        }
                        else
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].framerate=15;
                        }
                    }

                    cJSON *channel_gop = cJSON_GetObjectItem(video_encoding,"gop");
                    if(channel_gop != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval=channel_gop->valueint;
                    }

                    cJSON *width = cJSON_GetObjectItem(video_encoding,"width");
                    if(width != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.width=width->valueint;
                    }

                    cJSON *height = cJSON_GetObjectItem(video_encoding,"height");
                    if(height != NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.height=height->valueint;
                    }

                    cJSON *encoder=cJSON_GetObjectItem(video_encoding,"encoder");
                    if(encoder !=NULL)
                    {
                        demo_callback_changecodec2();
                        //to do
                        /*if(0==strcmp(encoder->valuestring,"h264"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec=source_STREAM_CODEC_H264;
                        }
                        else if(0==strcmp(encoder->valuestring,"h265"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec=source_STREAM_CODEC_H265;
                        }*/
                    }
                    cJSON *quality=cJSON_GetObjectItem(video_encoding,"quality");
                    if(quality!=NULL)
                    {
                        if(0==strcmp(quality->valuestring,"ld"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality=0;
                        }
                        else if(0==strcmp(quality->valuestring,"sd"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality=1;
                        }
                        else if(0==strcmp(quality->valuestring,"hd"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality=2;
                        }
                        else if(0==strcmp(quality->valuestring,"fhd"))
                        {
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality=3;
                        }
                    }


                }

                cJSON *audio_encoding=NULL;
                audio_encoding = cJSON_GetObjectItem(channelentry,"audio_encoding");
                if(audio_encoding!=NULL)
                {
                    cJSON *encoder=cJSON_GetObjectItem(audio_encoding,"encoder");
                    if(encoder !=NULL)
                    {
                        // to do
                    }


                }


                cJSON* image = cJSON_GetObjectItem(channelentry,"image");
                if(image!=NULL)
                {
                    cJSON *horflip=cJSON_GetObjectItem(image,"horflip");
                    if(horflip !=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.horflip=horflip->valueint;
                    }

                    cJSON *verflip=cJSON_GetObjectItem(image,"verflip");
                    if(verflip!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.verflip=verflip->valueint;
                    }
                }
				SetAISentry(channelentry,i);
                SetAlarmSentry(channelentry,i);

				cJSON *osd_entry=NULL;
				cJSON *osd_array = cJSON_GetObjectItem(channelentry, "osd_text");
				if (osd_array != NULL){
					osdarraySize=cJSON_GetArraySize(osd_array);
					OVD_DEMO_ConfigureInfo.channelsInfo[i].osdcount=osdarraySize;
					for (osd_num=0; osd_num<osdarraySize; osd_num++) {
						osd_entry=cJSON_GetArrayItem(osd_array, osd_num);
						cJSON *on=cJSON_GetObjectItem(osd_entry, "on");
						if(on != NULL){
							OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].on=on->valueint;
						}
						cJSON *text=cJSON_GetObjectItem(osd_entry, "text");
						if(text != NULL) {
							strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].text, text->valuestring);
						}
						cJSON *font=cJSON_GetObjectItem(osd_entry, "fontsize");
						if (font != NULL) {
							OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdfont=font->valueint;
						}
						cJSON *pos=cJSON_GetObjectItem(osd_entry, "pos");
						if (pos != NULL) {
							cJSON *x = cJSON_GetObjectItem(pos, "x");
							if (x!=NULL) {
								OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.x = x->valueint;
							}
							cJSON *y = cJSON_GetObjectItem(pos, "y");
							if (y!=NULL) {
								OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.y = y->valueint;
							}
						}
						cJSON *color=cJSON_GetObjectItem(osd_entry, "color");
						if (color != NULL) {
							cJSON *red = cJSON_GetObjectItem(color, "Red");
							if (red != NULL) {
								OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.red = red->valueint;
							}
							cJSON *green = cJSON_GetObjectItem(color, "Green");
							if (green != NULL) {
								OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.green = green->valueint;
							}
							cJSON *blue = cJSON_GetObjectItem(color, "Blue");
							if (blue != NULL) {
								OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.blue = blue->valueint;
							}
						}
						cJSON *mode=cJSON_GetObjectItem(osd_entry, "mode");
						if (mode != NULL) {
							OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].mode=mode->valueint;
						}

						cJSON *id=cJSON_GetObjectItem(osd_entry, "id");
						if (id != NULL) {
							OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].id=id->valueint;
						}
					}
				}

				item= cJSON_GetObjectItem(channelentry,"normal_nightvision_mode");
				if(item!=NULL)
					OVD_DEMO_ConfigureInfo.normal_nightvision_mode=item->valueint;

				item= cJSON_GetObjectItem(channelentry,"color_nightvision_mode");
				if(item!=NULL)
					OVD_DEMO_ConfigureInfo.color_nightvision_mode=item->valueint;

				item= cJSON_GetObjectItem(channelentry,"nightvision_detect_mode");
				if(item!=NULL)
					OVD_DEMO_ConfigureInfo.nightvision_detect_mode=item->valueint;
                // 补光灯
                cJSON *fill_light =cJSON_GetObjectItem(channelentry, "fill_light");
                if(fill_light!=NULL)
                {
                    item = cJSON_GetObjectItem(fill_light, "on");
                    if(item!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.fill_light.on=item->valueint;
                    }
                    item = cJSON_GetObjectItem(fill_light, "brightness");
                    if(item!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.fill_light.brightness=item->valueint;
                    }
                    cJSON* fill_light_schedule_time=cJSON_GetObjectItem(fill_light,"schedule_time");
                    if(fill_light_schedule_time!=NULL)
                    {
                        item=cJSON_GetObjectItem(fill_light_schedule_time,"on");
                        if(item!=NULL)
                            OVD_DEMO_ConfigureInfo.fill_light.schedule.on=item->valueint;

                        item=cJSON_GetObjectItem(fill_light_schedule_time,"starttime");
                        if(item!=NULL)
                            strcpy(OVD_DEMO_ConfigureInfo.fill_light.schedule.starttime,item->valuestring);

                        item=cJSON_GetObjectItem(fill_light_schedule_time,"endtime");
                        if(item!=NULL)
                            strcpy(OVD_DEMO_ConfigureInfo.fill_light.schedule.endtime,item->valuestring);

                        item=cJSON_GetObjectItem(fill_light_schedule_time,"repeat");
                        if(item!=NULL)
                        {
                            repeat_cnt=OVD_DEMO_OVCRepeatCnt(item);
                            OVD_DEMO_ConfigureInfo.fill_light.schedule.repeat=repeat_cnt;
                        }
                    }
                }

                item=cJSON_GetObjectItem(channelentry, "AI_rules_mode_on");
				if(item!= NULL)
                {
                    if(cJSON_False== item->type || cJSON_True == item->type)
                    {
                        OVD_DEMO_ConfigureInfo.AI_rules_mode_on=item->valueint;
                    }
                    else
                    {
                        ret = -1;
                        break;
                    }
				}


				cJSON *osd_logo = cJSON_GetObjectItem(channelentry, "osd_logo");
				if (osd_logo != NULL){
					cJSON *on=cJSON_GetObjectItem(osd_logo, "on");
					if(on != NULL) {
						OVD_DEMO_ConfigureInfo.channelsInfo[i].osd_logo=on->valueint;
					}
				}



                //audio_out_volume
                cJSON* audio_out_volume = cJSON_GetObjectItem(channelentry,"audio_out_volume");
                if(audio_out_volume!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].audioOutValume=audio_out_volume->valueint;
                }

                //doorbell_out_volume
                cJSON* doorbell_out_volume = cJSON_GetObjectItem(channelentry,"doorbell_volume");
                if(doorbell_out_volume!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].doorbell_volume=doorbell_out_volume->valueint;
                }

				cJSON* switchentry=cJSON_GetObjectItem(channelentry,"switch_schedule");
				if(switchentry!=NULL)
				{
					item=cJSON_GetObjectItem(switchentry,"on");
					if(item!=NULL)
						OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.on=item->valueint;

					item=cJSON_GetObjectItem(switchentry,"start_time");
					if(item!=NULL)
						strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.start_time,item->valuestring);

					item=cJSON_GetObjectItem(switchentry,"shutdown_time");
					if(item!=NULL)
						strcpy(OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.shutdown_time,item->valuestring);

					item=cJSON_GetObjectItem(switchentry,"repeat");
					if(item!=NULL){
						repeat_cnt=OVD_DEMO_OVCRepeatCnt(item);
						OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.repeat=repeat_cnt;

					}
				}

                //行车记录仪
                do
                {
                cJSON* dashcam_entry=cJSON_GetObjectItem(channelentry, "driving_recorder");
                if(dashcam_entry != NULL){
                    item=cJSON_GetObjectItem(dashcam_entry, "collision_alarm_sensitivity");
                    if(item != NULL){
                        if(!cJSON_IsNumber(item)){
                            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "collsion alarm sensitivity non number");
                            ret = -1;
                            break;
                        }else{
                            if(item->valueint < 0 || item->valueint > 100){
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","item->valueint is %d", item->valueint);
                                ret = -1;
                                break;
                            }
                            OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.collision_alarm_sensitivity = item->valueint;
                        }
                    }
                    cJSON* alarm_duration_entry=cJSON_GetObjectItem(dashcam_entry, "alarm_duration");
                    if(alarm_duration_entry != NULL){
                        //停车通知时长
                        item=cJSON_GetObjectItem(alarm_duration_entry, "parking_notice");
                        if(item != NULL){
                            if(!cJSON_IsNumber(item)){
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "parking_notice non number");
                                ret = -1;
                                break;
                            }else{
                                OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_notice = item->valueint;
                            }
                        }
                        //停车碰撞时长
                        item=cJSON_GetObjectItem(alarm_duration_entry, "parking_collision");
                        if(item != NULL){
                            if(!cJSON_IsNumber(item)){
                                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "parking_collsion non number");
                                ret = -1;
                                break;
                            }else{
                                OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_collision = item->valueint;
                            }
                        }
                        //行车碰撞
                        cJSON* driving_collision_entry=cJSON_GetObjectItem(alarm_duration_entry, "driving_collision");
                        if(driving_collision_entry != NULL){
                            item=cJSON_GetObjectItem(driving_collision_entry, "before");
                            if(item != NULL)
                            {
                                if(!cJSON_IsNumber(item)){
                                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "driving_collision before non number");
                                    ret = -1;
                                    break;
                                }else{
                                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.before = item->valueint;
                                }
                            }
                            item=cJSON_GetObjectItem(driving_collision_entry, "after");
                            if(item != NULL)
                            {
                                if(!cJSON_IsNumber(item)){
                                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "driving_collision after non number");
                                    ret = -1;
                                    break;
                                }else{
                                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.after = item->valueint;
                                }
                            }
                        }
                        //语音事件
                        cJSON* voice_capture_entry=cJSON_GetObjectItem(alarm_duration_entry, "voice_capture");
                        if(voice_capture_entry != NULL){
                            item=cJSON_GetObjectItem(voice_capture_entry, "before");
                            if(item != NULL)
                            {
                                if(!cJSON_IsNumber(item)){
                                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "voice_capture before non number");
                                    ret = -1;
                                    break;
                                }else{
                                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.before = item->valueint;
                                }
                            }
                            item=cJSON_GetObjectItem(voice_capture_entry, "after");
                            if(item != NULL)
                            {
                                if(!cJSON_IsNumber(item)){
                                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "voice_capture after non number");
                                    ret = -1;
                                    break;
                                }else{
                                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.after = item->valueint;
                                }
                            }
                        }
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO", "driving_recorder params: collsion_alarm_sensitivity..%d, parking_notice..%d, parking_collsion..%d,driving_collision.before..%d,driving_collision.after..%d, voice_capture.before..%d,voice_capture.after..%d\n",
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.collision_alarm_sensitivity,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_notice,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_collision,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.before,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.after,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.before,
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.after);
                    }

                }
                } while (0);

                cJSON* trace= cJSON_GetObjectItem(channelentry,"trace");
                if(trace!=NULL)
                {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].traceAbility=trace->valueint;
                }

                cJSON* pSmartModeSwitch2 = cJSON_GetObjectItem(channelentry, "smart_mode_switch_2");
                if (pSmartModeSwitch2 && cJSON_IsNumber(pSmartModeSwitch2)) {
                    OVD_DEMO_ConfigureInfo.channelsInfo[i].smart_mode_switch_2 = pSmartModeSwitch2->valueint;
                }


                cJSON* motion_tracking= cJSON_GetObjectItem(channelentry,"motion_tracking");
                if(motion_tracking!=NULL)
                {
                    cJSON* targetType= cJSON_GetObjectItem(motion_tracking,"targetType");
                    if(targetType!=NULL)
                    {
                        OVD_DEMO_ConfigureInfo.channelsInfo[i].traces.tracetarget=targetType->valueint;
                    }
                }
            }
        }
    }

    OVD_DEMO_SetConfig();//set
    cJSON_Delete(root);
    return ret;
}

void OVD_DEMO_OVCConnectStatus(int connectStatus){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","当前与ovc的连接状态是 %d\n",connectStatus);
    return;
}


int OVD_DEMO_ReBootChannel(int channel){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","channel重启 %d\n",channel);
    return 0;
}

extern int OVD_DEMOE_SetRebootInfo(OvdRebootParam *rebootinfo);

int OVD_DEMO_ReBootDevice(ovd_reboot_reason_e reason){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","设备重启... %d sleep 2s\n",reason);
    OvdRebootParam rebootinfo={0};
    rebootinfo.reason=reason;
     struct timeval tv = {0};

    gettimeofday(&tv, NULL);
    rebootinfo.ts= tv.tv_sec;
    strncpy(rebootinfo.message,"sdk call", sizeof(rebootinfo.message)-1);
    OVD_DEMOE_SetRebootInfo(&rebootinfo);

	exit(0);
    return 0;
}

int OVD_DEMO_GetGpsInfo(OvdGpsInfo *gpsinfo){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","get gps info\n");
    gpsinfo->latitude = 13362244.451255787;
    gpsinfo->longitude = 3520505.895809378;

    return 0;
}


#if !defined(__ANDROID__) && !defined(__LITEOS__) && !defined(__RTTHREAD_OS__) && !defined(__ALIOS__)
typedef struct __dirstream DIR;
#endif

int OVD_DEMO_QueryRecordPage(unsigned int channelmask1,unsigned int channelmask2,int recordType, unsigned long long StartStamp,unsigned long long EndStamp,int Page,int numInPage,OVDRecordFileListPerPage *FilePage){
		OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","打开sd卡文件列表\n");
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","1 channelmask1=%u,channelmask2=%u,recordType=%d,StartStamp=%llu,EndStamp=%llu,Page=%d,numInPage=%d \n",channelmask1,channelmask2,recordType,StartStamp,EndStamp,Page,numInPage);

/*
    DIR *dir;
	struct dirent *ptr;
	struct stat stat_buf;
    //char *path="./video/";
	char *path="./configure/resources/";
	char *ret;

	if((dir = opendir(path))==NULL)
	{
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","cannot open dir:%s\n", path);
		return -1;
	}
	int channelCount = 0;
    int bit = 1;
    int channelNum = 0;
    int bitzero = (channelmask1 & 1);

    while(((ptr=readdir(dir))!=NULL)&&(channelCount<16)){
		if(strcmp(ptr->d_name,".")==0|| strcmp(ptr->d_name,"..")==0)
    		continue;
		char tmp[100];
    	strcpy(tmp,path);
    	strcat(tmp,ptr->d_name);
		lstat(tmp,&stat_buf);
    	int d_size=stat_buf.st_size;

        channelCount++;
        channelNum = bitzero + (2 * (bit - 1));

		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","path: %s size:%d\n", tmp,d_size);
        FilePage->fileinfo[channelCount].channel = channelNum;
        FilePage->fileinfo[channelCount].FileType = 0;
        FilePage->fileinfo[channelCount].FileSize = d_size;
        memcpy(FilePage->fileinfo[channelCount].FileName, ptr->d_name, 10);
		time_t rawtime;
        time(&rawtime);
        FilePage->fileinfo[channelCount].FileStartStamp = rawtime-360;
        FilePage->fileinfo[channelCount].FileEndStamp = rawtime;
        FilePage->fileinfo[channelCount].RecordDuration = 10;
		bit++;
    }

    FilePage->fileCount = channelCount;
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","fileCount=%d \n",FilePage->fileCount);
	closedir(dir);
	*/

    FilePage->fileinfo[0].channel=0;
    FilePage->fileinfo[0].FileType=0;
    strcpy(FilePage->fileinfo[0].FileName,"demotftest");
    FilePage->fileinfo[0].FileSize=10240;
    FilePage->fileinfo[0].FileStartStamp=StartStamp;
    FilePage->fileinfo[0].FileEndStamp=EndStamp;
    FilePage->fileinfo[0].RecordDuration=EndStamp-StartStamp;
    FilePage->fileCount=1;

    return 0;
}


void getOVDVideoDataFormat(int channel,OVDVideoDataFormat* videoInfo){
    int index=0;
    for(index=0; index < OVD_DEMO_ConfigureInfo.channelsInfoCount; index++){
        if(channel == OVD_DEMO_ConfigureInfo.channelsInfo[index].channel){
            *videoInfo = OVD_DEMO_ConfigureInfo.channelsInfo[index].videoinfo;
        }
    }
}

void getOVDAudioDataFormat(int channel, OVDAudioDataFormat* audioInfo){
    int index=0;
    for(index=0; index < OVD_DEMO_ConfigureInfo.channelsInfoCount; index++){
        if(channel == OVD_DEMO_ConfigureInfo.channelsInfo[index].channel){
            *audioInfo = OVD_DEMO_ConfigureInfo.channelsInfo[index].audioinfo;
        }
    }
}

struct VideoData		   // 待发送的测试数据结构
{
    unsigned char *buffer; // 测试数据缓冲区
    int bufLen;            // 缓冲区长度
    int frameCount;        // 记录总帧数
    int framePos[100000];   // 记录每一帧帧头的起始位置
    int frameLen[100000];   // 记录每一帧的长度
    int IframeFlag[100000]; // 记录每一帧是否是I帧

};

struct AudioData		   //
{
    unsigned char *buffer; // 测试数据缓冲区
    int bufLen;            // 缓冲区长度
    int frameCount;        // 记录总帧数
    long long timestamp;//当前播放音频时间戳
    int frame_index;       //记录当前帧
    int framePos[100000];   // 记录每一帧帧头的起始位置
    int frameLen[100000];   // 记录每一帧的长度

};
// static struct AudioData p2p_stAudioData; // 待发送的测试数据

static struct VideoData p2p_stVideoData; // 待发送的测试数据
int p2p_switch=0;
int p2p_sd_stop=0;

void p2p_ReadH264File()
{
    memset(&p2p_stVideoData,0,sizeof(struct VideoData));
    struct VideoData* p = &p2p_stVideoData;
    FILE *fp = NULL;
    fp = fopen(p2ptfh264filepath, "rb+");
    if (fp == NULL)
    {
        return;
    }
    fseek(fp, 0, SEEK_END);
    p->bufLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    p->buffer = malloc(p->bufLen);
    memset(p->buffer, 0, p->bufLen);
    fread(p->buffer, p->bufLen, 1, fp);

    int iLenCount = 0;
    int FrameCount = 0;
	int i = 0;
    for (i = 0; i < p->bufLen - 4;)
    {
        if ((p->buffer[i] == 0x00)
         && (p->buffer[i + 1] == 0x00)
         && (p->buffer[i + 2] == 0x00)
         && (p->buffer[i + 3] == 0x01))
        {
            if (p->buffer[i + 4] == 0x65)
            {
                p->IframeFlag[FrameCount] = 1;
            }

            if (p->frameCount > 0)
            {
                p->frameLen[iLenCount++] = i - p->framePos[p->frameCount - 1];
            }
            p->framePos[p->frameCount++] = i;
            i += 4;
            FrameCount++;
            continue;
        }
        else if ((p->buffer[i] == 0x00)
               &&(p->buffer[i + 1] == 0x00)
               &&(p->buffer[i + 2] == 0x01))
        {
            if (p->buffer[i + 3] == 0x65)
            {
                p->IframeFlag[FrameCount] = 1;
            }

            if (p->frameCount > 0)
            {
                p->frameLen[iLenCount++] = i - p->framePos[p->frameCount - 1];
            }
            p->framePos[p->frameCount++] = i;
            i += 3;
            FrameCount++;
            continue;
        }
        i++;
    }

    p->frameLen[iLenCount++] = p->bufLen - p->framePos[p->frameCount - 1];

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.bufLen=%d \n", p->bufLen);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.frameCount=%d \n", p->frameCount);
    fclose(fp);

}

void p2p_ReadH265File()
{
    memset(&p2p_stVideoData, 0, sizeof(struct VideoData));
    struct VideoData* p = &p2p_stVideoData;
    FILE *fp = NULL;
    fp = fopen(p2ptfh265filepath, "rb+");
    if (fp == NULL)
    {
        return;
    }
    fseek(fp, 0, SEEK_END);
    p->bufLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    p->buffer = malloc(p->bufLen);
    memset(p->buffer, 0, p->bufLen);
    fread(p->buffer, p->bufLen, 1, fp);

    int iLenCount = 0;
    int FrameCount = 0;
    int i = 0;
    unsigned char tmp = 0;
    for (i = 0; i < p->bufLen - 4;)
    {
        if ((p->buffer[i] == 0x00)
         && (p->buffer[i + 1] == 0x00)
         && (p->buffer[i + 2] == 0x00)
         && (p->buffer[i + 3] == 0x01))
        {
            tmp = (p->buffer[i + 4]) >> 1;
            if (tmp == 19 || tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39)
            {
                p->IframeFlag[FrameCount] = 1;
            }

            if (p->frameCount > 0)
            {
                p->frameLen[iLenCount++] = i - p->framePos[p->frameCount - 1];
            }
            p->framePos[p->frameCount++] = i;
            i += 4;
            FrameCount++;
            continue;
        }
        else if ((p->buffer[i] == 0x00)
               &&(p->buffer[i + 1] == 0x00)
               &&(p->buffer[i + 2] == 0x01))
        {
            tmp = (p->buffer[i + 3]) >> 1;
            if (tmp == 19 || tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39)
            {
                p->IframeFlag[FrameCount] = 1;
            }

            if (p->frameCount > 0)
            {
                p->frameLen[iLenCount++] = i - p->framePos[p->frameCount - 1];
            }
            p->framePos[p->frameCount++] = i;
            i += 3;
            FrameCount++;
            continue;
        }
        i++;
    }

    p->frameLen[iLenCount++] = p->bufLen - p->framePos[p->frameCount - 1];

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.bufLen=%d \n", p->bufLen);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.frameCount=%d \n", p->frameCount);
    fclose(fp);
}

#ifdef CONFIG_MODULE_CardPlayback
void* pthread_OVD_DemoSendRecordFile(void *args){
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","pthread_OVD_DemoSendRecordFile\r\n");
    prctl(PR_SET_NAME,"Demo_sdthread");
    if(TF_H264_H265_CTRL==1)
    {
        p2p_ReadH265File();
    }
    else
    {
        p2p_ReadH264File();
    }
    /*
    if (0 == TF_H264_H265_CTRL % 2)
    {
        p2p_ReadH264File();
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","P2P Read H264 File\n");
    } else {
        p2p_ReadH265File();
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","P2P Read H265 File\n");
    }
    TF_H264_H265_CTRL += 1;
    */
    //p2p循环播放有问题，故不用循环播放
   //while(p2p_sd_stop==0)
   {
        int j=0;
        for ( j = 0; j < p2p_stVideoData.frameCount; j++)
        {
            usleep(1000*1000/15);
			//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","p2p_sd_stop=%d \n",p2p_sd_stop);

			if(p2p_sd_stop==0){
				OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","push tf frame: %d / %d\n",j,p2p_stVideoData.frameCount);
				OVD_SendRecordAVContent(0,OVD_Video,p2p_stVideoData.IframeFlag[j],p2p_stVideoData.buffer + p2p_stVideoData.framePos[j],p2p_stVideoData.frameLen[j],20*j);
			}
            else if(p2p_sd_stop==1)
            {
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","push tf pause\n");
                sleep(1);
            }
			else if(p2p_sd_stop==2){
				break;
			}

			//OVD_SendRecordAVContent(0,OVD_Video,p2p_stVideoData.IframeFlag[j],p2p_stVideoData.buffer + p2p_stVideoData.framePos[j],p2p_stVideoData.frameLen[j],20*j);
			//usleep(50*1000);
       }
    }

    OPENAPI_DEMO_send_recordAV_pid=0;
   return NULL;
}

//srt 卡录像方案

static char demo_tfrecord_sessionid[256];
static long long demo_tfrecord_starttime=0;
static long long demo_tfrecord_playtime=0;

//弃用
#if 0
void* pthread_OVDDME_DemoSendRecordFile(void *args){
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pthread_OVDDME_DemoSendRecordFile\r\n");
    prctl(PR_SET_NAME,"Demo_DME_sdthread");
    //demo_tfrecord_playtime=demo_tfrecord_starttime;

    p2p_ReadH264File();
    /*
    if(TF_H264_H265_CTRL==1)
    {
        p2p_ReadH265File();
    }
    else
    {
        p2p_ReadH264File();
    }
    */
    //需要告知卡录像模块音视频格式
    OVDVideoDataFormat vf;
    OVDAudioDataFormat af;
    memset(&vf,0,sizeof(OVDVideoDataFormat));
    memset(&af,0,sizeof(OVDAudioOutDataFormat));
    vf.codec=source_STREAM_CODEC_H264;
    af.codec=source_STREAM_CODEC_AAC_WITH_ADTS;
    OVD_tfRecord_SetAVFormat(demo_tfrecord_sessionid, & vf, NULL);
    p2p_sd_stop=0;

    /*
    if (0 == TF_H264_H265_CTRL % 2)
    {
        p2p_ReadH264File();
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","P2P Read H264 File\n");
    } else {
        p2p_ReadH265File();
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","P2P Read H265 File\n");
    }
    TF_H264_H265_CTRL += 1;
    */
    //p2p循环播放有问题，故不用循环播放
   while(p2p_sd_stop!=2)
   {
        int j=0;
        for ( j = 0; j < p2p_stVideoData.frameCount; )
        {
            usleep(1000*1000/25);
			//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","p2p_sd_stop=%d \n",p2p_sd_stop);

			if(p2p_sd_stop==0){
				OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","push tf frame: %d / %d ts:%llu\n",j,p2p_stVideoData.frameCount,demo_tfrecord_playtime);
				//OVD_SendRecordAVContent(0,OVD_Video,p2p_stVideoData.IframeFlag[j],p2p_stVideoData.buffer + p2p_stVideoData.framePos[j],p2p_stVideoData.frameLen[j],20*j);
				demo_tfrecord_playtime+=40;
                //该接口
               // OVD_tfRecord_PushData(demo_tfrecord_sessionid, OVD_Video, p2p_stVideoData.IframeFlag[j], p2p_stVideoData.buffer + p2p_stVideoData.framePos[j], p2p_stVideoData.frameLen[j], demo_tfrecord_playtime);
                j++;
            }
            else if(p2p_sd_stop==1)
            {
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","push tf pause\n");
                sleep(1);

            }
			else if(p2p_sd_stop==2){
				break;
			}

			//OVD_SendRecordAVContent(0,OVD_Video,p2p_stVideoData.IframeFlag[j],p2p_stVideoData.buffer + p2p_stVideoData.framePos[j],p2p_stVideoData.frameLen[j],20*j);
			//usleep(50*1000);

       }
    }
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","pthread_OVDDME_DemoSendRecordFile exit\r\n");
    p2p_sd_stop=0;
    OPENAPI_DEMO_send_recordAV_pid=0;
}
#endif

    /*
   ###接口描述：
       RecordSearch
       OVC可以通过该指令后去获取指定时间区间的卡录像记录
       RCP OVC-->OVD
   ### 参数说明
   [in] channel: 通道
   [in] starttime: 查询的起始时间
   [in] endtime: 查询的结束时间
   [in] page: 查询的页面索引，若一次查询区间不够，sdk会回调第二次
   [in] numinpage: 该页面最大的区间数量
   [out] fileinpage: 厂商回填区间信息

   //demo 模拟整天都有录像

   */
int OVD_DEMO_DMEAPI_callback_RecordSearch(int channel,long long starttime, long long endtime, int page, int numInPage, OVD_DMERecordFileListPerPage *fileinpage  )
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel: %d, starttime:%llu, endtime:%llu, page:%d, numInPage:%d \n",channel, starttime,endtime,page,numInPage);
    fileinpage->Count=1;
    fileinpage->sectioninfo[0].StartStamp=starttime;
    fileinpage->sectioninfo[0].EndStamp=endtime;
    demo_tfrecord_starttime=starttime*1000;
    return 0;
}


#if 0
/*
### 接口描述：
    RecordPublish
    OVC可以通过该方法请求OVD建立一条媒体流通道到指定URL，用于后续卡录像视频传输
    厂商需知道某一channel下出现一个卡录像会话。
### 参数说明
    [in] channel: 通道
    [
    [in] session_id: 字符串，用来标识这串流
*/

int OVD_DEMO_DMEAPI_callback_RecordPublish(int channel,  char *session_id)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","channel: %d session_id:%s\n",channel, session_id);
    //厂商要保存channel 和session_id的对应关系
    strncpy(demo_tfrecord_sessionid,session_id,sizeof(demo_tfrecord_sessionid)-1);
    return 0;
}



/*
### 接口描述：
    RecordPlay
控制设备卡录像播放指令
### 参数说明
[in] session_id: 字符串，用来标识流ID
[in] timestamp: 64位长整数，播放的绝对时间戳
*/
//demo 不演示seek场景
int OVD_DEMO_DMEAPI_callback_RecordPlay(char *session_id, long long timestamp)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","session_id:%s, timestamp:%llu\n",session_id,timestamp);
    if(timestamp!=0)
    {
        demo_tfrecord_playtime=timestamp*1000;
    }
    if(OPENAPI_DEMO_send_recordAV_pid==0)
    {

	    int result = pthread_create(&OPENAPI_DEMO_send_recordAV_pid,NULL,pthread_OVDDME_DemoSendRecordFile,NULL);

        pthread_detach(OPENAPI_DEMO_send_recordAV_pid);
    }
    else
    {

        //如果已经在推，只不过暂停了
        p2p_sd_stop=0;
    }

    return 0;
}


/*
### 接口描述：
RecordPause
控制设备卡录像暂停播放
### 参数说明
[in] session_id: 字符串
*/
int OVD_DEMO_DMEAPI_callback_RecordPause(char *session_id)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","session_id:%s\n",session_id);
    p2p_sd_stop=1;
    return 0;
}


/*
 ### 接口描述：
 RecordStop
 OVC可以通过该方法请求OVD 关闭卡录像媒体传输流
 ### 参数说明
 [in] session_id: 字符串，用来标识这串流
 */
 int OVD_DEMO_DMEAPI_callback_RecordStop(char* session_id)
 {
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","session_id:%s\n",session_id);
    p2p_sd_stop=2;
    return 0;

 }
#endif

//第二期卡录像机制


 struct DEMO_SRTRECORD_CONTENT{
       int h264_or_h265;   //0表示h264,1表示265
       int flag; //是否已经输入过内容
       long long timestamp;//当前播放时间戳
       int frame_index; //当前帧序列
       int frame_type;  //当前帧类型
       struct VideoData srtRecord_data;
       struct AudioData srtRecord_Audiodata;
       long long starttime; //此上下文的开始时间戳
       long long endtime; //此上下文的结束时间戳
 };

//demo是否支持多文件卡回放，打开此宏表示支持多个文件
//#define SUPPORT_MANY_TF_FILES

struct DEMO_SRTRECORD_CONTENT demo_srtrecord_content_all[3] = {0};
unsigned int  demo_srtrecord_content_num_cur = 0;  //当前支持的个数
unsigned int  demo_tf_audio_sampling = 44100; //16000;  //音频采样频率
unsigned int  demo_tf_vedio_fps = 15; //25; //视频帧率


 void demo_srt_ReadH264File(struct DEMO_SRTRECORD_CONTENT* ptr_srtrecord_content_h264)
 {
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","enter demo_srt_ReadH264File:%s.\n", p2ptfh264filepath);

    if (ptr_srtrecord_content_h264->srtRecord_data.buffer != NULL) {
        free(ptr_srtrecord_content_h264->srtRecord_data.buffer);
        ptr_srtrecord_content_h264->srtRecord_data.buffer = NULL;
    }
    if (ptr_srtrecord_content_h264->srtRecord_Audiodata.buffer != NULL) {
        free(ptr_srtrecord_content_h264->srtRecord_Audiodata.buffer);
        ptr_srtrecord_content_h264->srtRecord_Audiodata.buffer = NULL;
    }
    memset(&(ptr_srtrecord_content_h264->srtRecord_data),0,sizeof(struct VideoData));

     ptr_srtrecord_content_h264->frame_type = OVD_Video;//默认为视频

     struct VideoData* p = &(ptr_srtrecord_content_h264->srtRecord_data);
     FILE *fp = NULL;
     fp = fopen(p2ptfh264filepath, "rb+");
     if (fp == NULL)
     {
         return;
     }
     fseek(fp, 0, SEEK_END);
     p->bufLen = ftell(fp);
     fseek(fp, 0, SEEK_SET);

     p->buffer = malloc(p->bufLen);
     memset(p->buffer, 0, p->bufLen);
     fread(p->buffer, p->bufLen, 1, fp);

     int FrameCount = 0;
     int i = 0;
     for (i = 0; i < p->bufLen - 4;)
     {
         if ((p->buffer[i] == 0x00)
          && (p->buffer[i + 1] == 0x00)
          && (p->buffer[i + 2] == 0x00)
          && (p->buffer[i + 3] == 0x01))
         {
             unsigned char tmpType = ((p->buffer[i + 4]) & 0x1f);
             //if (tmpType == 0x05) {
             if (tmpType == 0x07) {
                 p->IframeFlag[FrameCount] = 1; //I帧
             } else if ((tmpType == 0x01) || (tmpType == 0x02) || (tmpType == 0x03) || (tmpType == 0x04)) {
                p->IframeFlag[FrameCount] = 0;  //P帧
             } else {
                i += 4;//其他帧,直接跳过
                continue;
             }

             if (FrameCount > 0)
             {
                 //计算上一帧的长度，当前位置减去上一帧的位置
                 p->frameLen[FrameCount-1] = i - p->framePos[FrameCount - 1];
                 //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","4 FrameCount=%d, frameLen:%d, framePos:%d. \n", FrameCount-1, p->frameLen[FrameCount-1], p->framePos[FrameCount - 1]);
             }
             p->framePos[FrameCount] = i;
             FrameCount++;
             i += 4;
             continue;
         }
         else if ((p->buffer[i] == 0x00)
                &&(p->buffer[i + 1] == 0x00)
                &&(p->buffer[i + 2] == 0x01))
         {
             unsigned char tmpType = ((p->buffer[i + 3]) & 0x1f);
             //if (tmpType == 0x05) {
             if (tmpType == 0x07) {
                 p->IframeFlag[FrameCount] = 1;
             }else if ((tmpType == 0x01) || (tmpType == 0x02) || (tmpType == 0x03) || (tmpType == 0x04)) {
                p->IframeFlag[FrameCount] = 0;  //P帧
             } else {
                i += 3;//其他帧,直接跳过
                continue;
             }

             if (FrameCount > 0)
             {
                 p->frameLen[FrameCount-1] = i - p->framePos[FrameCount - 1];
                 //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","3 FrameCount=%d, frameLen:%d, framePos:%d. \n", FrameCount-1, p->frameLen[FrameCount-1], p->framePos[FrameCount - 1]);
             }
             p->framePos[FrameCount] = i;
             FrameCount++;
             i += 3;
             continue;
         }
         i++;
     }

     p->frameCount = FrameCount;
     //计算最后一帧的长度
     p->frameLen[FrameCount-1] = p->bufLen - p->framePos[FrameCount - 1];

     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.bufLen=%d \n", p->bufLen);
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.frameCount=%d \n", p->frameCount);
     fclose(fp);

 }


 void demo_srt_ReadH265File(struct DEMO_SRTRECORD_CONTENT* ptr_srtrecord_content_h265)
 {
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","enter demo_srt_ReadH265File:%d.\n", p2ptfh265filepath);

     if (ptr_srtrecord_content_h265->srtRecord_data.buffer != NULL) {
        free(ptr_srtrecord_content_h265->srtRecord_data.buffer);
        ptr_srtrecord_content_h265->srtRecord_data.buffer = NULL;
     }
     if (ptr_srtrecord_content_h265->srtRecord_Audiodata.buffer != NULL) {
        free(ptr_srtrecord_content_h265->srtRecord_Audiodata.buffer);
        ptr_srtrecord_content_h265->srtRecord_Audiodata.buffer = NULL;
     }
     memset(&(ptr_srtrecord_content_h265->srtRecord_data),0,sizeof(struct VideoData));

     struct VideoData* p = &(ptr_srtrecord_content_h265->srtRecord_data);

     ptr_srtrecord_content_h265->frame_type = OVD_Video;//默认为视频

     FILE *fp = NULL;
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","p2ptfh265filepath %s\n",p2ptfh265filepath);
     fp = fopen(p2ptfh265filepath, "rb+");
     if (fp == NULL)
     {
         return;
     }
     fseek(fp, 0, SEEK_END);
     p->bufLen = ftell(fp);
     fseek(fp, 0, SEEK_SET);

     p->buffer = malloc(p->bufLen);
     memset(p->buffer, 0, p->bufLen);
     fread(p->buffer, p->bufLen, 1, fp);

     int FrameCount = 0;
     int i = 0;
     unsigned char tmp = 0;
     for (i = 0; i < p->bufLen - 4;)
     {
         if ((p->buffer[i] == 0x00)
          && (p->buffer[i + 1] == 0x00)
          && (p->buffer[i + 2] == 0x00)
          && (p->buffer[i + 3] == 0x01))
         {
             tmp = (p->buffer[i + 4]) >> 1;
             //if (tmp == 19 || tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39)
             if (32 == tmp) {
                 p->IframeFlag[FrameCount] = 1;  //I帧
             } else if (1 == tmp) {
                p->IframeFlag[FrameCount] = 0;   //P帧
             } else {
                i += 4;
                continue;
             }

             if (FrameCount > 0)
             {
                 //计算上一帧的长度，当前位置减去上一帧的位置
                 p->frameLen[FrameCount-1] = i - p->framePos[FrameCount - 1];
                 //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","4 FrameCount=%d, frameLen:%d, framePos:%d. \n", FrameCount-1, p->frameLen[FrameCount-1], p->framePos[FrameCount - 1]);
             }
             p->framePos[FrameCount] = i;
             i += 4;
             FrameCount++;
             continue;
         }
         else if ((p->buffer[i] == 0x00)
                &&(p->buffer[i + 1] == 0x00)
                &&(p->buffer[i + 2] == 0x01))
         {
             tmp = (p->buffer[i + 3]) >> 1;
             //if (tmp == 19 || tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39)
             if (32 == tmp) {
                 p->IframeFlag[FrameCount] = 1;
             } else if (1 == tmp) {
                p->IframeFlag[FrameCount] = 0;
             } else {
                i += 3;
                continue;
             }

             if (FrameCount > 0)
             {
                 p->frameLen[FrameCount-1] = i - p->framePos[FrameCount - 1];
                 //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","3 FrameCount=%d, frameLen:%d, framePos:%d. \n", FrameCount-1, p->frameLen[FrameCount-1], p->framePos[FrameCount - 1]);
             }
             p->framePos[FrameCount] = i;
             i += 3;
             FrameCount++;
             continue;
         }
         i++;
     }

     p->frameCount = FrameCount;
     //计算最后一帧的长度
     p->frameLen[FrameCount-1] = p->bufLen - p->framePos[FrameCount - 1];

     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.bufLen=%d \n", p->bufLen);
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","stVideoData.frameCount=%d \n", p->frameCount);
     fclose(fp);
 }

//demo 读取音频文件
 void demo_srt_ReadAudioFile(struct DEMO_SRTRECORD_CONTENT* ptr_srtrecord_content)
 {

     struct AudioData* p = &(ptr_srtrecord_content->srtRecord_Audiodata);
     FILE *fp = NULL;
     char *AudioFILEPATH=audioFILEPATH;//"./configure/resources/audio.aac";
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AudioFILEPATH %s\n",AudioFILEPATH);
     fp = fopen(AudioFILEPATH, "rb+");
     if (fp == NULL)
     {
         return;
     }
     fseek(fp, 0, SEEK_END);
     p->bufLen = ftell(fp);
     if (p->bufLen < 7)
     {
         //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","demo util %d: 数据源过小 BB buffer size=%d\n",__LINE__,p->bufLen);
         fclose(fp);
         return;
     }
     fseek(fp, 0, SEEK_SET);

     p->buffer = malloc(p->bufLen);
     memset(p->buffer, 0, p->bufLen);
     fread(p->buffer, p->bufLen, 1, fp);

     int FrameCount = 0;
     int i = 0;
     //unsigned char tmp = 0;
	 //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","demo util %d: %p;%d\n",__LINE__,p->buffer,p->bufLen);

    int date_size = 0;
    for (i = 0; i < p->bufLen - 7; )
	{
        if ((p->buffer[i] == 0xff) && ((p->buffer[i+1] & 0xf0) == 0xf0))
		{
            date_size |= ((p->buffer[i+3] & 0x03) << 11);     //high 2 bit
            date_size |= p->buffer[i+4]<<3;                   //middle 8 bit
            date_size |= ((p->buffer[i+5] & 0xe0) >> 5);      //low 3 bitt

            p->frameLen[FrameCount] = date_size;
            p->framePos[FrameCount] = i;
            //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","FrameCount=%d, i=%d, date_size=%d \n", FrameCount, i, date_size);
            FrameCount++;
            i += date_size;
            date_size = 0;
            continue;
        }

        i++;
    }

     p->frameCount = FrameCount;

     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AudioData.bufLen=%d \n", p->bufLen);
     OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AudioData.frameCount=%d \n", p->frameCount);

     //free(p->buffer);
     fclose(fp);
 }

void OVD_DEMO_CONFIG_TF_RecordFile()
{
     //配置tf卡回放文件
#ifdef SUPPORT_MANY_TF_FILES
    demo_srtrecord_content_num_cur = 3;
    demo_srtrecord_content_all[0].h264_or_h265 = 0;
    demo_srtrecord_content_all[1].h264_or_h265 = 1;
    demo_srtrecord_content_all[2].h264_or_h265 = 0;
#else
    demo_srtrecord_content_num_cur = 1;
    if(TF_H264_H265_CTRL==0) {
        demo_srtrecord_content_all[0].h264_or_h265 = 0; //h264
    } else {
        demo_srtrecord_content_all[0].h264_or_h265 = 1; //h265
    }
#endif

    //读取文件内容
    int i = 0;
    struct DEMO_SRTRECORD_CONTENT *ptr_content = NULL;
    for (i = 0; i < demo_srtrecord_content_num_cur; i++) {

        ptr_content = &demo_srtrecord_content_all[i];
        if (0 == ptr_content->h264_or_h265) {

            if (0 == ptr_content->flag) {
                demo_srt_ReadH264File(ptr_content);
                demo_srt_ReadAudioFile(ptr_content);
                ptr_content->flag = 1;
            }
        } else {

            if (0 == ptr_content->flag) {
                demo_srt_ReadH265File(ptr_content);
                demo_srt_ReadAudioFile(ptr_content);
                ptr_content->flag = 1;
            }
        }
    }

    return;
}

 /*
###接口描述：
    RecordSearch
    OVC可以通过该指令后去获取指定时间区间的卡录像记录
    RCP OVC-->OVD
### 参数说明
[in] channel: 通道
[in] starttime: 查询的起始时间 单位为秒
[in] endtime: 查询的结束时间
[in] page: 查询的页面索引，若一次查询区间不够，sdk会回调第二次
[in] numinpage: 该页面最大的区间数量
[out] fileinpage: 厂商回填区间信息

//demo 模拟整天都有录像

*/
OVD_int32 OVD_DEMO_DMEAPI_callback_RecordSearch2(OVD_int32 channel, OVD_uint64 starttime, OVD_uint64 endtime, OVD_int32 page,
            OVD_int32 numInPage, OVD_DMERecordFileListPerPage *fileinpage)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_DMEAPI_callback_RecordSearch, channel: %d, starttime:%llu, endtime:%llu, page:%d, numInPage:%d \n",channel, starttime,endtime,page,numInPage);

    //TF卡文件配置
    OVD_DEMO_CONFIG_TF_RecordFile();

    //生成起始时间戳，要求视频帧率都是15，不然时间戳计算存在错误误差
    OVD_uint64 localStartTime = starttime*1000;  //ms
    struct DEMO_SRTRECORD_CONTENT *ptr_content = NULL;
    int i = 0;
    for (i = 0; i < demo_srtrecord_content_num_cur; i++) {

        ptr_content = &demo_srtrecord_content_all[i];
        ptr_content->timestamp = localStartTime;
        ptr_content->starttime = localStartTime;
        ptr_content->endtime = ptr_content->starttime + (ptr_content->srtRecord_data.frameCount+(demo_tf_vedio_fps-1))/demo_tf_vedio_fps*1000;

        fileinpage->sectioninfo[i].StartStamp = ptr_content->starttime/1000;
        fileinpage->sectioninfo[i].EndStamp = ptr_content->endtime/1000;
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","i: %d, return starttime:%llu, endtime:%llu,  \n", i, fileinpage->sectioninfo[i].StartStamp, fileinpage->sectioninfo[i].EndStamp);

        //下一个文件的起始时间为当前文件末尾+5000ms
        localStartTime = ptr_content->endtime + 5000;
    }
    fileinpage->Count = demo_srtrecord_content_num_cur;

    return 0;
 }


 /*

    ### 回调接口描述：

       OVD_DMEAPI_callback_RecordOpen

       通知厂商打开一条某一channel下的卡录像会话。

    ### 参数说明

       [in] channel: 通道



    ### 返回值：

         失败： NULL

          成功： 返回卡录像会话厂商侧的上下文（context）标识，ctx

   */
 void* OVD_DEMO_DMEAPI_callback_RecordOpen(int channel)
 {
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_DEMO_DMEAPI_callback_RecordOpen, channel: %d\n",channel);

    //TF卡文件配置
    OVD_DEMO_CONFIG_TF_RecordFile();

    return &demo_srtrecord_content_all;
 }


  /*

    ### 回调接口描述：

     OVD_DMEAPI_callback_RecordSeek

     将指定卡录像会话游标定位到某一时间点，

     注：该调用后，sdk下一次读取音视频数据帧时，厂商需从相近时间点的I帧（优先向前）开始返回数据。

       若对应时间点没有录像数据则返回错误。

    ### 参数说明

     [in] ctx:厂商卡录像会话上下文标识

     [in] timestamp: 绝对时间戳，seek的绝对时间点

    ###返回值：

     成功：0

     失败：-1 通用错误

          -2  对应时间点没有录像数据

     */

 int OVD_DEMO_DMEAPI_callback_RecordSeek(void* ctx, long long timestamp)
 {
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","demo seek timestamp: %llu\n",timestamp);

    if(timestamp*1000 < demo_srtrecord_content_all[0].starttime
        || (timestamp*1000 > demo_srtrecord_content_all[demo_srtrecord_content_num_cur -1].endtime))
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","seek time out of range[%lld--%lld]\n",
            demo_srtrecord_content_all[0].starttime, demo_srtrecord_content_all[demo_srtrecord_content_num_cur -1].endtime);
        return OVD_RET_SEEK_NODATA;
    }

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","seektime in range\n");

    double inter_video = (double)1000.0/demo_tf_vedio_fps;//视频帧间隔
    double inter_audio = (double)1024*1000.0/demo_tf_audio_sampling;//音频帧间隔

    int i = 0;
    int j = 0;
    OVD_bool bfind = OVD_FALSE;
    int target=0;//目标I帧
    long long target_time=0;
    struct DEMO_SRTRECORD_CONTENT *ptr_content = NULL;
    for (j = 0; j < demo_srtrecord_content_num_cur; j++) {

        ptr_content = &demo_srtrecord_content_all[j];
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","j:%d, starttime:%lld endtime:%lld", j, ptr_content->starttime,ptr_content->endtime);

        ptr_content->frame_type = OVD_Video;//seek后先传I帧视频

        for(i=0; i<ptr_content->srtRecord_data.frameCount; i++){

            if ((ptr_content->starttime+i*inter_video) > (timestamp*1000)) {
                bfind = OVD_TRUE;
                break;
            } else if (ptr_content->srtRecord_data.IframeFlag[i] == 1) {
                target=i;
                target_time=ptr_content->starttime+i*inter_video;
            }
        }


        if (bfind == OVD_TRUE) {

            ptr_content->frame_index=target;
            ptr_content->timestamp=target_time;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","j:%d, vedio frame index:%d time:%llu", j, target,target_time);

            int audioseekframe = 0;
            audioseekframe = (target_time - ptr_content->starttime) / inter_audio;
            ptr_content->srtRecord_Audiodata.timestamp = ptr_content->starttime + (long long)audioseekframe*inter_audio;
            ptr_content->srtRecord_Audiodata.frame_index = audioseekframe;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","j:%d, audio frame index:%d time:%llu", j, ptr_content->srtRecord_Audiodata.frame_index, ptr_content->srtRecord_Audiodata.timestamp);

            break;
        }

        ptr_content->frame_index=ptr_content->srtRecord_data.frameCount;
        ptr_content->timestamp=ptr_content->endtime;
        ptr_content->srtRecord_Audiodata.frame_index = ptr_content->srtRecord_Audiodata.frameCount;
        ptr_content->srtRecord_Audiodata.timestamp = ptr_content->starttime + ptr_content->srtRecord_Audiodata.frame_index*inter_audio;

    }

    for (j = j+1; j < demo_srtrecord_content_num_cur; j++) {

        ptr_content = &demo_srtrecord_content_all[j];
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","j:%d, starttime:%lld endtime:%lld", j, ptr_content->starttime,ptr_content->endtime);

        ptr_content->frame_type = OVD_Video;//seek后先传I帧视频
        ptr_content->frame_index=0;
        ptr_content->timestamp=ptr_content->starttime;
        ptr_content->srtRecord_Audiodata.frame_index = 0;
        ptr_content->srtRecord_Audiodata.timestamp = ptr_content->starttime;

    }


    return 0;
 }


 /*
  ###接口描述:
       OVD_DMEAPI_callback_RecordReadFrame
  ###参数说明:
       [in] ctx: 厂商卡录像会话上下文
       [in] pframe_info： 帧数据信息
           注：pframe_info 结构体内存由sdk分配, 由sdk 负责释放
               pframe_info里的frame_buf 由厂商分配，也由厂商释放;建议厂商在下次read或者会话结束释放内存；
   ##返回值：
          OVD_RET_COMMON_ERROR = -1,    //通用错误码
               OVD_RET_SUCCESS = 0,           //成功
               OVD_RET_READ_FRAME_RETRY=201, //读帧重试错误码
               OVD_RET_READ_FRAME_EOF=202,   //卡录像读取完毕，厂商反馈EOF

         若收到通用错误，则sdk会调用销毁卡录像会话资源机制
   */
int OVD_DEMO_DMEAPI_callback_RecordReadFrame(void* ctx, OVD_FrameInfo *pframe_info)
{
    int j = 0;
    OVD_bool bFind = OVD_FALSE;
    struct DEMO_SRTRECORD_CONTENT *ptr_content = NULL;
    for (j = 0; j < demo_srtrecord_content_num_cur; j++) {

        ptr_content = &demo_srtrecord_content_all[j];

        if (!(ptr_content->frame_index<ptr_content->srtRecord_data.frameCount) && !(ptr_content->srtRecord_Audiodata.frame_index<ptr_content->srtRecord_Audiodata.frameCount))
        {
            OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "j:%d, read video and audio over.\n", j);//音视频帧传完
            ptr_content->timestamp = ptr_content->endtime;
            continue;
        }

        bFind = OVD_TRUE;
        break;
    }

    if (bFind != OVD_TRUE) {
        return OVD_RET_READ_FRAME_EOF;
    }
    OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "j:%d, timestamp:%lld  frame_index:%d  frame_type:%d. \n", j, ptr_content->timestamp, ptr_content->frame_index, ptr_content->frame_type);

    if (!(ptr_content->frame_index<ptr_content->srtRecord_data.frameCount) && !(ptr_content->srtRecord_Audiodata.frame_index<ptr_content->srtRecord_Audiodata.frameCount))
    {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","read video and audio over\n");//音视频帧传完
        return OVD_RET_READ_FRAME_EOF;
    }
    if (ptr_content->frame_index < ptr_content->srtRecord_data.frameCount && ptr_content->srtRecord_Audiodata.frame_index == ptr_content->srtRecord_Audiodata.frameCount)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read video.\n");
        ptr_content->frame_type=OVD_Video;//音频帧先传完
    }
    if (ptr_content->frame_index == ptr_content->srtRecord_data.frameCount && ptr_content->srtRecord_Audiodata.frame_index < ptr_content->srtRecord_Audiodata.frameCount)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read audio.\n");
        ptr_content->frame_type=OVD_Audio;//视频帧先传完
        //return OVD_RET_READ_FRAME_EOF;
    }

    double inter_video = (double)1000.0/demo_tf_vedio_fps;//视频帧间隔
    double inter_audio = (double)1024*1000/demo_tf_audio_sampling;//音频帧间隔

     pframe_info->frame_type = ptr_content->frame_type;
     if(pframe_info->frame_type==OVD_Video)
     {
      //demo 视频帧
      //读帧
        int codec_enc=0;

        if(ptr_content->h264_or_h265==0)
         {
            pframe_info->codec_type=source_STREAM_CODEC_H264;
		    codec_enc=1;
         }
        else
         {
            pframe_info->codec_type=source_STREAM_CODEC_H265;
	    	codec_enc=2;
         }

        pframe_info->timestamp=ptr_content->starttime+ptr_content->frame_index*inter_video;
        ptr_content->timestamp=pframe_info->timestamp;
        pframe_info->video_info.iskey_frame=ptr_content->srtRecord_data.IframeFlag[ptr_content->frame_index];
        pframe_info->video_info.framerate=demo_tf_vedio_fps;
        pframe_info->video_info.gop=25;//demogop

        pframe_info->framebuf_len=ptr_content->srtRecord_data.frameLen[ptr_content->frame_index];
        pframe_info->frame_buf=ptr_content->srtRecord_data.buffer+ptr_content->srtRecord_data.framePos[ptr_content->frame_index];
        (ptr_content->frame_index)++;

        if ((ptr_content->timestamp+inter_video) < (ptr_content->srtRecord_Audiodata.timestamp+inter_audio)){
            ptr_content->frame_type = OVD_Video;//下一帧传视频
        } else{
            ptr_content->frame_type = OVD_Audio;//下一帧传音频
        }

        OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "[Video] type:%d,codec:%d, ts:%llu, gap_v:%f,gap_a:%f,ts_a:%llu,next_type:%d, iskey:%d, framerate:%d,frame_len:%d\n",
            pframe_info->frame_type,pframe_info->codec_type,pframe_info->timestamp,
            inter_video, inter_audio, ptr_content->srtRecord_Audiodata.timestamp, ptr_content->frame_type,
            pframe_info->video_info.iskey_frame,pframe_info->video_info.framerate,pframe_info->framebuf_len);
        return 0;
     }

     if(pframe_info->frame_type==OVD_Audio)
     {

        pframe_info->codec_type=source_STREAM_CODEC_AAC_WITH_ADTS;
        pframe_info->audio_info.samplesRate=demo_tf_audio_sampling;
        pframe_info->audio_info.channelNumber=1;
        pframe_info->audio_info.bitsPerSample=16;
        pframe_info->audio_info.samplePerFrame=1024;

        unsigned int audiobufflen = ptr_content->srtRecord_Audiodata.frameLen[ptr_content->srtRecord_Audiodata.frame_index];
        pframe_info->frame_buf=ptr_content->srtRecord_Audiodata.buffer+
        ptr_content->srtRecord_Audiodata.framePos[ptr_content->srtRecord_Audiodata.frame_index];

        pframe_info->timestamp=ptr_content->starttime+ptr_content->srtRecord_Audiodata.frame_index*inter_audio;
        ptr_content->srtRecord_Audiodata.timestamp = pframe_info->timestamp;

        if ((ptr_content->timestamp+inter_video) < (ptr_content->srtRecord_Audiodata.timestamp+inter_audio))
        {
            ptr_content->frame_type = OVD_Video;//下一帧传视频
        }
        else{
            ptr_content->frame_type = OVD_Audio;//下一帧传音频
        }

        pframe_info->framebuf_len = audiobufflen;
        ptr_content->srtRecord_Audiodata.frame_index ++;

        OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "[Audio] type:%d,codec:%d, ts:%llu, gap_v:%f, gap_a:%f, ts_v:%llu,next_type:%d,samplesRate:%d, chanNum:%d,frame_len:%d\n",
            pframe_info->frame_type,pframe_info->codec_type,pframe_info->timestamp,
            inter_audio,inter_audio,ptr_content->timestamp,ptr_content->frame_type,
            pframe_info->audio_info.samplesRate,pframe_info->audio_info.channelNumber, pframe_info->framebuf_len);

        return 0;
     }
     return 0;
 }



 int OVD_DEMO_DMEAPI_callback_RecordSpeedReadFrame(void* ctx, ovd_playback_speed_times_e speed, OVD_FrameInfo *pframe_info)
 {
     int j = 0;
     OVD_bool bFind = OVD_FALSE;
     struct DEMO_SRTRECORD_CONTENT *ptr_content = NULL;
     for (j = 0; j < demo_srtrecord_content_num_cur; j++) {

         ptr_content = &demo_srtrecord_content_all[j];

         if (!(ptr_content->frame_index<ptr_content->srtRecord_data.frameCount) && !(ptr_content->srtRecord_Audiodata.frame_index<ptr_content->srtRecord_Audiodata.frameCount))
         {
             OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "j:%d, read video and audio over.\n", j);//音视频帧传完
             ptr_content->timestamp = ptr_content->endtime;
             continue;
         }

         bFind = OVD_TRUE;
         break;
     }

     if (bFind != OVD_TRUE) {
         return OVD_RET_READ_FRAME_EOF;
     }
     OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "j:%d, timestamp:%lld  frame_index:%d  frame_type:%d. \n", j, ptr_content->timestamp, ptr_content->frame_index, ptr_content->frame_type);

     if (!(ptr_content->frame_index<ptr_content->srtRecord_data.frameCount) && !(ptr_content->srtRecord_Audiodata.frame_index<ptr_content->srtRecord_Audiodata.frameCount))
     {
         OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","read video and audio over\n");//音视频帧传完
         return OVD_RET_READ_FRAME_EOF;
     }
     if (ptr_content->frame_index < ptr_content->srtRecord_data.frameCount && ptr_content->srtRecord_Audiodata.frame_index == ptr_content->srtRecord_Audiodata.frameCount)
     {
         OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read video.\n");
         ptr_content->frame_type=OVD_Video;//音频帧先传完
     }
     if (ptr_content->frame_index == ptr_content->srtRecord_data.frameCount && ptr_content->srtRecord_Audiodata.frame_index < ptr_content->srtRecord_Audiodata.frameCount)
     {
         OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read audio.\n");
         ptr_content->frame_type=OVD_Audio;//视频帧先传完
         //return OVD_RET_READ_FRAME_EOF;
     }

     float time = 1.0;
     if (OVD_PLAYBACK_SPEED_QUARTER == speed)
     {
        time = 0.25;
     }
     else if (OVD_PLAYBACK_SPEED_HALF == speed)
     {
        time = 0.50;
     }
     else if (OVD_PLAYBACK_SPEED_1TIMES == speed)
     {
        time = 1.0;
     }
     else if (OVD_PLAYBACK_SPEED_2TIMES == speed)
     {
        time = 2.0;
     }
     else if (OVD_PLAYBACK_SPEED_4TIMES == speed)
     {
        time = 4.0;
     }
     else if (OVD_PLAYBACK_SPEED_8TIMES == speed)
     {
        time = 8.0;
     }
     else if (OVD_PLAYBACK_SPEED_16TIMES == speed)
     {
        time = 16.0;
     }


     double inter_video = (double)1000.0/demo_tf_vedio_fps; //视频帧间隔
     double inter_audio = (double)1024*1000/demo_tf_audio_sampling; //音频帧间隔

      pframe_info->frame_type = ptr_content->frame_type;
      if(pframe_info->frame_type==OVD_Video)
      {
       //demo 视频帧
       //读帧
         int codec_enc=0;

         if(ptr_content->h264_or_h265==0)
          {
             pframe_info->codec_type=source_STREAM_CODEC_H264;
             codec_enc=1;
          }
         else
          {
             pframe_info->codec_type=source_STREAM_CODEC_H265;
             codec_enc=2;
          }

         pframe_info->timestamp=ptr_content->starttime+ptr_content->frame_index*(inter_video/time);
         ptr_content->timestamp=pframe_info->timestamp;
         pframe_info->video_info.iskey_frame=ptr_content->srtRecord_data.IframeFlag[ptr_content->frame_index];
         pframe_info->video_info.framerate=demo_tf_vedio_fps*time;
         pframe_info->video_info.gop=25;//demogop

         pframe_info->framebuf_len=ptr_content->srtRecord_data.frameLen[ptr_content->frame_index];
         pframe_info->frame_buf=ptr_content->srtRecord_data.buffer+ptr_content->srtRecord_data.framePos[ptr_content->frame_index];
         (ptr_content->frame_index)++;

         if ((ptr_content->timestamp+inter_video) < (ptr_content->srtRecord_Audiodata.timestamp+inter_audio)){
             ptr_content->frame_type = OVD_Video;//下一帧传视频
         } else{
             ptr_content->frame_type = OVD_Audio;//下一帧传音频
         }

         OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "[Video] type:%d,codec:%d, ts:%llu, gap_v:%f,gap_a:%f,ts_a:%llu,next_type:%d, iskey:%d, framerate:%d,frame_len:%d\n",
             pframe_info->frame_type,pframe_info->codec_type,pframe_info->timestamp,
             inter_video, inter_audio, ptr_content->srtRecord_Audiodata.timestamp, ptr_content->frame_type,
             pframe_info->video_info.iskey_frame,pframe_info->video_info.framerate,pframe_info->framebuf_len);
         return 0;
      }

      if(pframe_info->frame_type==OVD_Audio)
      {

         pframe_info->codec_type=source_STREAM_CODEC_AAC_WITH_ADTS;
         pframe_info->audio_info.samplesRate=demo_tf_audio_sampling;
         pframe_info->audio_info.channelNumber=1;
         pframe_info->audio_info.bitsPerSample=16;
         pframe_info->audio_info.samplePerFrame=1024;

         unsigned int audiobufflen = ptr_content->srtRecord_Audiodata.frameLen[ptr_content->srtRecord_Audiodata.frame_index];
         pframe_info->frame_buf=ptr_content->srtRecord_Audiodata.buffer+
         ptr_content->srtRecord_Audiodata.framePos[ptr_content->srtRecord_Audiodata.frame_index];

         pframe_info->timestamp=ptr_content->starttime+ptr_content->srtRecord_Audiodata.frame_index*inter_audio;
         ptr_content->srtRecord_Audiodata.timestamp = pframe_info->timestamp;

         if ((ptr_content->timestamp+inter_video) < (ptr_content->srtRecord_Audiodata.timestamp+inter_audio))
         {
             ptr_content->frame_type = OVD_Video;//下一帧传视频
         }
         else{
             ptr_content->frame_type = OVD_Audio;//下一帧传音频
         }

         pframe_info->framebuf_len = audiobufflen;
         ptr_content->srtRecord_Audiodata.frame_index++;

         OVD_LOG(OVD_LOGLEVEL_TRACE,"DEV/DEMO", "[Audio] type:%d,codec:%d, ts:%llu, gap_v:%f, gap_a:%f, ts_v:%llu,next_type:%d,samplesRate:%d, chanNum:%d,frame_len:%d\n",
             pframe_info->frame_type,pframe_info->codec_type,pframe_info->timestamp,
             inter_audio,inter_audio,ptr_content->timestamp,ptr_content->frame_type,
             pframe_info->audio_info.samplesRate,pframe_info->audio_info.channelNumber, pframe_info->framebuf_len);

         return 0;
      }
      return 0;
  }


 /*
 ###接口描述：
      OVD_DMEAPI_callback_RecordClose
 ###参数说明：
      [in] ctx： 厂商卡录像会话上下文
 ### 返回值：
      0：代表成功
      其他：代表失败
  */
int OVD_DEMO_DMEAPI_callback_RecordClose(void *ctx)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_DEMO_DMEAPI_callback_RecordClose: %p\n", ctx);
    return 0;
}


//end of 第二期卡录像机制







int OVD_DEMO_OpenRecordFile(int channel,char* recordname,OVDVideoDataFormat* videoInfo,OVDAudioDataFormat* audioInfo,int* fileTotalTime){
	int s32Ret;
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","打开sd卡文件 H264_H265flag:%d\r\n", TF_H264_H265_CTRL);
    *fileTotalTime = 360;
    if(TF_H264_H265_CTRL==1)
        videoInfo->codec=source_STREAM_CODEC_H265;

    else
        videoInfo->codec=source_STREAM_CODEC_H264;

    p2p_sd_stop=0;
/*
    if (0 == TF_H264_H265_CTRL % 2)
    {
        videoInfo->codec = 0;
    }
    else
    {
        videoInfo->codec = 4;
    }
*/
#if 1
    if(OPENAPI_DEMO_send_recordAV_pid==0)
    {
#ifdef __ARCH_RTOS__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 128*1024);
    s32Ret = pthread_create(&OPENAPI_DEMO_send_recordAV_pid,&attr,pthread_OVD_DemoSendRecordFile,(void *)recordname);
#else
	s32Ret = pthread_create(&OPENAPI_DEMO_send_recordAV_pid,NULL,pthread_OVD_DemoSendRecordFile,(void *)recordname);
#endif
    if (s32Ret != 0)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Error # Create thread pthread_OVD_DemoSendRecordFile failed\n");
    }
    pthread_detach(OPENAPI_DEMO_send_recordAV_pid);
    }
#endif
    return 0;
}


int OVD_DEMO_VedioSwitchQuality(int channel, OVDEncodeQuality quality, OVDVideoDataFormat *vedioInfo){
    //服务器端设置了，但设备端不一定会采用
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_VedioSwitchQuality\r\n");
    getOVDVideoDataFormat(channel, vedioInfo);

    return 0;
}

int OVD_DEMO_RecordCotrol(int channel,OVDCONTROLTYPE controlType,int value){
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_RecordCotrol chanel: %d  type:%d \r\n",channel,controlType);
    if(controlType==0)//继续播放
    {
        p2p_sd_stop=0;
    }
    if(controlType==1) //暂停
    {
        //p2p_sd_stop=1;
    }
    if(controlType==2) //停止
    {
    	p2p_sd_stop=2;
		OVD_RecordAVContentSendOver(channel);
    }

    return 0;
}

#endif


int OVD_DEMO_SyncTime(char *datetime,int tz, int offset){

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","厂商同步时间， sync time,datetime:%s tz:%d, offset:%d ",datetime,tz, offset);

    //厂商将时间同步到设备里

    return 0;
}

int OVD_DEMO_QueryTime(char* out_datetime, int time_len, int *tz){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_QueryTime %lld\r\n", global_timeStamp);

    strncpy(out_datetime,"2016-12-05T02:15:32",time_len);
    *tz=9;
    return 0;
}


int OVD_DEMO_GetSDInfo(OVDSDInfo *sdInfo){
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_GetSDInfo\n");
    //sdInfo->SDExist = 1;
    //sdInfo->SDTotalSize = 256;
    //sdInfo->SDFreeSize = 100;
    //sdInfo->EarlyFileName = {"testsdfile.avi"};


	sdInfo->SDExist = OVD_DEMO_SdInfo.SDExist;
    sdInfo->SDTotalSize = OVD_DEMO_SdInfo.SDTotalSize;
    sdInfo->SDFreeSize = OVD_DEMO_SdInfo.SDFreeSize;

    strncpy(sdInfo->EarlyFileName, "testsdfile.avi", MAX_LEN_24);


    //1:buffer is accessed out of bounds.
    return 0;
}

int OVD_DEMO_SetSDCardFormat(){
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_SetSDCardFormat, sd card formated\n");
    return 0;
}


int g_presetlist[256]={0};

int OVD_DEMO_PTZCmd(int channel,OVCPTZControlCmd ptzcmd,int speed){
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_PTZCmd\n");

    if(OVD_DEMO_ConfigureInfo.channelsInfo[channel].on == 0)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","dut channel %d disable\n",channel);
        return 0;
    }

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","demo device contrl channel %d, cmd %d, speed %d\r\n", channel, ptzcmd, speed);
    if(OVC_PTZ_SET_PRESET == ptzcmd )
    {
        g_presetlist[speed] =1;
    }
    if(OVC_PTZ_SET_433_PRESET == ptzcmd )
    {
        g_presetlist[speed] =1;
    }

    if(OVC_PTZ_CLEAR_PRESET == ptzcmd)
    {
        g_presetlist[speed]=0;
    }

    return 0;
}

int OVD_DEMO_GetPresetList(int channel,int *presetList,int array_num, int *count){
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_GetPresetList\n");
    int i=0;

    int num=0;
    for(i=0;i<256;i++)
    {
        if(g_presetlist[i]==1)
        {
            presetList[num]=i;
            num++;
        }
    }

    *count = num;


    return 0;
}

int OVD_DEMO_EBOCmd(OVCEBOControlCmd cmd,void* data){
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_EBOCmd:%d\n",cmd);
    ovd_ebo_content_t* ebo_data = (ovd_ebo_content_t*)data;
    int ret = OVD_RET_SUCCESS;
    switch (cmd)
    {
        case OVC_EBO_MV_STEP: //步进指令
        {

            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVC_EBO_MV_STEP move to position seq:%d [%d %d]\n",ebo_data->ebo_space.seq, ebo_data->ebo_space.pan, ebo_data->ebo_space.tilt);
            break;
        }
        case OVC_EBO_MV_CONT: //持续移动指令
        {

            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVC_EBO_MV_CONT move to position seq:%d [%d %d]\n",ebo_data->ebo_space.seq, ebo_data->ebo_space.pan, ebo_data->ebo_space.tilt);
            break;
        }
        case OVC_EBO_NOFALL_TEST:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","防跌落检测\n");
            break;
        }
        case OVC_EBO_LED_TEST:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","补光灯检测\n");
            break;
        }
        case OVC_EBO_MV_FORWARD:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","向前猛冲\n");
            break;
        }
        case OVC_EBO_RECHARGE:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","回充\n");
            break;
        }
        case OVC_EBO_CIRCLE:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","转圈\n");
            break;
        }
        case OVC_EBO_SHAKE:
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","抖动\n");
            break;
        case OVC_EBO_SELF_TEST:
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","启动自检\n");
            break;
        case OVC_EBO_REPORT_LEN:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","报告长度查看 \n");
            ebo_data->ebo_data.size = 64;
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","报告长度为 %d\n",ebo_data->ebo_data.size);
            break;
        }
        case OVC_EBO_REPORT:
        {

            cJSON *root= cJSON_CreateObject();
            cJSON *EBO_config_entry=NULL;
            cJSON_AddStringToObject(root,"sn","012AAHY219S0024");
            cJSON_AddStringToObject(root,"devid","8955112000000022");
            cJSON_AddStringToObject(root,"cmei","111071780000023");
            cJSON_AddStringToObject(root,"spec","cn");
            cJSON_AddStringToObject(root,"firmware_ipc_version","v2.5.1-7");
            cJSON_AddItemToObject(root,"check_data",EBO_config_entry=cJSON_CreateObject());
            cJSON_AddNumberToObject(EBO_config_entry,"dB",10);
	        cJSON_AddNumberToObject(EBO_config_entry,"power",273.000000);
            cJSON_AddNumberToObject(EBO_config_entry,"left_motor_speed",10);
            cJSON_AddBoolToObject(EBO_config_entry,"left_motor_status",1);
            cJSON_AddNumberToObject(EBO_config_entry,"distance_measuring_range",5000.000000);
            cJSON_AddNumberToObject(EBO_config_entry,"infrared_reception_L_value",4800.000000);
            cJSON_PrintPreallocated(root, ebo_data->ebo_data.buf, ebo_data->ebo_data.size, 0);
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","报告查看 data:%s,len:%d\n",ebo_data->ebo_data.buf, ebo_data->ebo_data.size);
            break;
        }
        case OVC_EBO_MV_START:
        {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVC_EBO_MV_STEP move to position seq:%d [%d %d]\n", ebo_data->ebo_space.seq,ebo_data->ebo_space.pan, ebo_data->ebo_space.tilt);
            break;
        }
        default:
            ret = OVD_RET_RPC_UNIMPLEMENT;
            break;
    }

    return ret;
}

int OVD_DEMO_ROBOTCmd(OvdRobotContent* cmd,void* data)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","action:%s,type:%s,data:%s\n",cmd->action,cmd->type,cmd->data);
    ovd_common_info_t *rebot_data = (ovd_common_info_t *)data;
    if(strcmp(cmd->action, "getSurplusGrain") == 0)
    {
        cJSON *root= cJSON_CreateObject();
        cJSON_AddNumberToObject(root,"SurplusGrain",0);
        cJSON_PrintPreallocated(root, rebot_data->buf, rebot_data->size, 0);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","rebot_data data:%s,len:%d\n", rebot_data->buf, rebot_data->size);
    }

    if((strcmp(cmd->action, "motioncal") == 0) ||(strcmp(cmd->action, "petTrack") == 0) ||(strcmp(cmd->action, "recharge") == 0))
    {
        cJSON *root= cJSON_CreateObject();
        cJSON_AddBoolToObject(root,"return",0);
        cJSON_PrintPreallocated(root, rebot_data->buf, rebot_data->size, 0);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","rebot_data data:%s,len:%d\n", rebot_data->buf, rebot_data->size);
    }

    return 0;
}

int fd = -1;
char filename[25]={0};
int OVD_DEMO_AudioPlayStart(int channel, OVDAudioOutDataFormat andioFormat){
	//OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_AudioPlayStart\r\n");
	//snprintf(filename,sizeof(filename),"%d-m.pcm",channel);
	//O_WRONLY(只写模式) O_CREAT(如果指定文件不存在，则创建这个文件) O_APPEND(每次写操作都写入文件的末尾)
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","demo.%d: audioplay_file %s\n",__LINE__, audioplayFILEPATH);
	fd = open(audioplayFILEPATH,O_WRONLY | O_CREAT | O_APPEND, 0);
	if(fd<0)
	{
		OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","get file failed,open file error. \n");
	}
    return 0;
}

int OVD_DEMO_AudioPlayProGress(int channel,unsigned char* buf, int size){
    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_AudioPlayProGress, channel %d, size %d\r\n", channel, size);
	write(fd,buf,size);
	return 0;
}

int OVD_DEMO_AudioPlayStop(int channel){
	OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","OVD_DEMO_AudioPlayStop\r\n");
	close(fd);
    return 0;
}

int OVD_DEMO_ForceIFrame(int channel, OVDCodeStream code_stream){
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","demo device force i frame,channel:%d,code_stream:%d\r\n",channel,code_stream);
    return 0;
}



int OVD_DEMO_Snapshot(int channel,OVDImageInfo *imageInfo, int maxImageSize){
	OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO","OVD_DEMO_Snapshot\r\n");

#if 0
    //需要提供获取图片接口
    FILE *in;
    in = fopen(snapshotFILEPATH,"r");
    if(NULL==in){
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","open snapshotPic.bmp fail\n");
        return -1;
    }

    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","begin snapshot process\r\n");
    unsigned int width;
    unsigned int height;
    unsigned int bitCount;
    unsigned char buf[128];
    unsigned int  size;
    unsigned char* praw=get_data(in,&width,&height,&bitCount,&size);
    if(NULL==praw){
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read image is fail\r\n");
        return -1;
    }else{
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","read image is success\r\n");
    }
#endif
    int ret = 0;
    int size = 0;
    int buf_size = 0;
    char *buf = NULL;
    struct stat stat_buf;

    ret = stat(snapshotFILEPATH, &stat_buf);
    if(ret){

        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","%s %d: [demo] stat pic filepath %s fail\n",__func__,__LINE__,snapshotFILEPATH);
        return -1;

    }
    size =(int)stat_buf.st_size;

    buf = (char *)malloc(size);

    FILE *f = fopen(snapshotFILEPATH, "rb");
    if(!f){
        ret = -1;
        goto _exit1;
    }

   buf_size = fread(buf, 1, size, f);
   OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","%s %d: [demo]=== buf_size:%d ===\n",__func__,__LINE__,buf_size);

    if(size > maxImageSize){
        imageInfo->size = size;
        ret = -2;
        goto _exit2;
    }

    memcpy(imageInfo->buf, buf, size);
    imageInfo->size = size;

_exit2:
    fclose(f);
_exit1:
    free(buf);
    return ret;

}

//推流状态通知回调
int OVD_DEMO_StreamNotifyCalback(OVD_StreamNotifyType_e type,const unsigned int channel)
{
    switch (type)
    {
    case STREAMNOTIFYTYPE_LIVEPULISH:
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===channel[%d] 收到直播开启指令====\n",channel);
        break;
    case STREAMNOTIFYTYPE_LIVESTOP:
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===channel[%d] 收到直播关闭指令====\n",channel);
        break;
    case STREAMNOTIFYTYPE_EVENTCSEGSTART:
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===channel[%d] 收到事件直存套餐开启指令===\n",channel);
        break;

    case STREAMNOTIFYTYPE_FULLCSEGSTART:
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===channel[%d] 收到全天录像直存套餐开启指令===\n",channel);
        break;

    case STREAMNOTIFYTYPE_CSEGSTOP:
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===channel[%d] 收到录像直存套餐关闭指令===\n",channel);
        break;

    default:
        break;
    }
    return 0;

}

int OVD_DEMO_NotifyLoginFail(int err_code, char* err_msg)
{
    printf("[demo][login fail]err_code:%d,err_msg:%s",err_code,err_msg);
    return 0;
}

//告警状态通知回调
int OVD_DEMO_AlarmNotifyCalback(OVD_AlarmNotifyType_e type, const void* buf, const unsigned int len)
{
    int *value;
    OVD_AlarmNotifyMsg_t *msg;
    switch (type)
    {
    case ALARMNOTIFYTYPE_SETMINDURATION:
        value=(int*)buf;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===设置状态告警最短时长：%ds====\n",*value);
        break;
    case ALARMNOTIFTYPE_SETMAXDURATION:
        value=(int*) buf;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===设置状态告警最长时长：%ds===\n",*value);
        break;
    case ALARMNOTIFYTYPE_ALARMEXTEND:
        msg=(OVD_AlarmNotifyMsg_t *)buf;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===通道:%d 告警类型:%d 因过早结束，而被sdk自动延长===\n",msg->channel,msg->type);
        break;

    case ALARMNOTIFYTYPE_ALARMTRUNCATE:
        msg=(OVD_AlarmNotifyMsg_t *)buf;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","===通道:%d 告警类型:%d 因告警过长，而被sdk自动截断===\n",msg->channel,msg->type);
        break;


    default:
        break;
    }
    return 0;

}

int OVD_DEMO_SetMp3Url(int channel,const char *url,int repeat, int volume){
	OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_SetAudioOutPlay channel:%d url:%s repeat:%d volume:%d\r\n",channel,url,repeat,volume);

    if(NULL != Mp3PlayUrl){
        free(Mp3PlayUrl);
    }

    Mp3PlayUrl = (char*)malloc(strlen(url)+1);
    if(NULL == Mp3PlayUrl){
        return -1;
    }

    memcpy(Mp3PlayUrl, url, strlen(url)+1);
    Mp3PlayStatus = MP3_OTHER;

    return 0;
}

OVD_int32 OVD_DEMO_CtrlLullby(IN OVD_int32 channel,IN const OVD_char *album_id,IN const OVD_char* music_id, IN const OVD_char* music_name, IN const OVD_char* url, IN int ctrl_type)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_DEMO_CtrlLullby\r\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_CtrlLullby channel:%d album_id:%s music_id:%s music_name:%s url:%s ctrl_type:%d\r\n",channel,album_id,music_id,music_name,url,ctrl_type);
    return 0;
}

int OVD_DEMO_Mp3PlayCtrl(int channel,OVDMp3PlayCtrl ctrl){
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_SetMp3Url channel %d, ctrl %d\r\n", channel, ctrl);

    Mp3PlayStatus = ctrl;
    if(MP3_CLOSE == ctrl){
        if(NULL != Mp3PlayUrl){
            free(Mp3PlayUrl);
			Mp3PlayUrl = NULL;
        }
    }

    return 0;
}

int OVD_DEMO_GetMp3PlayStatus(int channel,int *status, char *url){
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_GetMp3PlayStatus\r\n");

    //url = "\0";

	*status = 0;
    if(MP3_CLOSE != Mp3PlayStatus){
        if(NULL != Mp3PlayUrl){
            memcpy(url, Mp3PlayUrl, strlen(Mp3PlayUrl)+1);
			*status = 2;
        }
    }
    return 0;
}

int keepawake_time = 0;
int OVD_DEMO_KeepAwakenUtilExpired(int channel, int notAllowHibernate, int expired, OVDHibernateReason reason){
    time_t i;
	i=time(NULL);
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","%d\n",i);
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","channel %d will keep awake until %d 秒, notAllowHibernate %d, reason %d\n",channel,expired,notAllowHibernate,reason);
    keepawake_time = i+expired;

	return 0;
}

int OVD_DEMO_ResetConfig(){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","device will reset all configuration\n");
	return 0;
}

int OVD_DEMO_getsimpleovdinfo(SimpleOVDinfo *outinfo)
{
    int i=0;
    outinfo->state=OVD_DEMO_status;
    outinfo->channelcount=OVD_DEMO_ConfigureInfo.channelsInfoCount;
    Simplechannelinfo *channelarray = NULL;
    if(outinfo->channelcount <= 4){
        channelarray = outinfo->channelarray;
    }
    else {
        // 超过4个channel以上的设备，则使用ext_channelarray，由厂商自行分配内存
        outinfo->ext_channelarray = (Simplechannelinfo *)malloc(outinfo->channelcount*sizeof(Simplechannelinfo));
        channelarray = outinfo->ext_channelarray;
    }
    if (OVD_DEMO_CapInfoV2.device_type == DEVICE_TYPE_NVR)
    {
        for(i=0;i<outinfo->channelcount;i++)
        {
            channelarray[i].channelnum=OVD_DEMO_ConfigureInfo.channelsInfo[i].channel;
            channelarray[i].state=OVD_CHANNEL_UNUSE; //此state代表该channel是否物理在线
            sprintf(channelarray[i].name, "cam_%d", i);
        }
    }
    else if (OVD_DEMO_CapInfoV2.device_type == DEVICE_TYPE_MIPC)
    {
        outinfo->channelcount = OVD_DEMO_ConfigureInfo.channelsInfoCount;
        for (i = 0; i<outinfo->channelcount;i++)
        {
            channelarray[i].channelnum=OVD_DEMO_ConfigureInfo.channelsInfo[i].channel;
            channelarray[i].state=OVD_CHANNEL_ONLINE; //此state代表该channel是否物理在线
        }
    }
    else
    {
        outinfo->channelcount = 1;
        outinfo->channelarray[0].channelnum=OVD_DEMO_ConfigureInfo.channelsInfo[0].channel;
        outinfo->channelarray[0].state=OVD_CHANNEL_ONLINE; //此state代表该channel是否物理在线
    }

    return 0;
}

/*
 *  ####接口描述：
     获取sd卡状态


 *  ####参数说明：
     out_state: <必填，整数；sd卡状态，0：正常，1：未插卡，2：卡异常>,
     "out_total": <必填，整数, sd卡总容量，单位为MB>,
     "out_free":  <必填, 整数, sd卡剩余容量，单位为MB>
 *   ###返回值：
 *       成功：0
 *       失败：其他值
 */
 int OVD_DEMO_GetDiskInfo(int *out_state,int *out_total, int *out_free)
 {
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_GetDiskInfo\n");
    *out_state=g_sdcardstate;
    *out_total=1000;
    *out_free=g_freedisk;
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","==out_state:%d, out_total:%d M , out_free: %d M",g_sdcardstate,*out_total,*out_free);
    return 0;
 }


 typedef struct{
     char transid[64];
     char start[64];
     char end[64];
     char url[1024];
     unsigned long long size;
 }OVDDemoLogUploadAsyncContent;

 pthread_t OPENAPI_DEMO_LogUploadAsync_pid=0;

static size_t OVD_DEMO_ReadCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t ret =0;
	if (ptr != NULL && data != NULL)
		ret = fread(ptr, size, nmemb, data);
	return ret;
}

static const char* getFileName(const char* path){
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

unsigned long long OVD_DEMO_UpLoadlog(char *log_name, OVDDemoLogUploadAsyncContent *content)
{
    long file_size;
    if(strlen(log_name) < 1)
    {
        log_name = uploadlogFILEPATH;
    }
	FILE *fp = fopen(log_name, "rb");
    if (fp == NULL) {
		OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","fopen %s failed\n", log_name);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","%s file_size is: %d\n", log_name, file_size);
        fclose(fp);
        return -1;
    }
#ifdef CONFIG_SUPPORT_curl
	CURL *curl=NULL;
	CURLcode res;
	//初始化libcurl
	//得到 easy interface型指针
	curl = curl_easy_init();

	if (curl) {
        //设置不验证CA和host
        struct curl_slist *headers = NULL;
        char content_disp[256];
        sprintf(content_disp, "Content-Disposition: attachment; filename=\"%s\"", getFileName(log_name));
        headers = curl_slist_append(headers, content_disp);
        // headers = curl_slist_append(headers, "Content-Type: application/x-bzip-compressed-tar");
        // 设置 HTTP 请求头
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		//设置访问URL
		curl_easy_setopt(curl, CURLOPT_URL, content->url);
	    //读函数
	    curl_easy_setopt(curl, CURLOPT_READFUNCTION, OVD_DEMO_ReadCallback);
	    //上传使能
	    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	    //HTTP输入
	    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
	    //指定要上传的文件
	    curl_easy_setopt(curl, CURLOPT_READDATA, fp);
	    //提供上传文件的大小 必须确保使用正确的数据大小 */
	    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) file_size);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
        	OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","curl_easy_perform failed: %s\n", curl_easy_strerror(res));
    	}
	}

	curl_easy_cleanup(curl);

#endif
	fclose(fp);
	return file_size;
}

 void* pthread_OVD_DemoLogUploadAsync(void *args)
 {
    OVDDemoLogUploadAsyncContent *content = NULL;
    if (args == NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","Error # args is NULL !\n");
        return NULL;
    }
	prctl(PR_SET_NAME, "DemoLogup");
    content = (OVDDemoLogUploadAsyncContent *)args;
	//getScreenLog();//获取打屏日志接口
	//模拟日志上传完毕，同步给平台

   // OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","模拟器将模拟上传./%s/upload.log 到日志服务器\n","configure/resources");
	//上传日志 并获取文件大小
#ifdef CONFIG_SUPPORT_curl
	content->size = OVD_DEMO_UpLoadlog(uploadlogFILEPATH, content);
#endif

	OVD_LogDone(content->transid,0,content->start,content->end,content->url,content->size);

        free(content);
     OPENAPI_DEMO_LogUploadAsync_pid=0;

     return NULL;
 }



 /*
 *  ####接口描述：
    LogUploadAsync
     OVC可以通过该命令控制OVD上传相关本地日志，供调试分析使用。OVD收到该指令后，通过异步方式将指定时间范围日志上传到相应的URL。 若指定范围的日志已经滚动删除，则使用尽力原则上传剩余的日志。若指定范围没有任何日志记录，应上传一个长度为0的日志文件。 日志文件通过PUT方法上传。
     RPC ovc-->ovd
 *  ####参数说明：
    "trans_id": <必填，字符串；标记此日志上传的任务ID>
   "start": <必填，字符串；日志记录开始时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-05T02:15:32>,
   "end": <必填，字符串；日志记录结束时间，格式yyyy-MM-ddTHH:mm:ss，例子：2016-12-06T02:15:32>,
   "url": <必填，字符串；日志上传的URL，设备通过PUT方法上传相应的日志文件>

 *   ###返回值：
 *       成功：0
 *       失败：其他值
 */
 int OVD_DEMO_LogUploadAsync(char *trans_id, char *start, char* end,char* url)
 {
   OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ovc->ovd loguploadasync trans_id %s, start %s, end %s, url %s\n",trans_id,start,end,url);
   if(OPENAPI_DEMO_LogUploadAsync_pid != 0)
   {
        OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","has another upgrade thead run %lx\n",OPENAPI_DEMO_LogUploadAsync_pid);
        return -1;
   }
    OVDDemoLogUploadAsyncContent *content= (OVDDemoLogUploadAsyncContent*)malloc(sizeof(OVDDemoLogUploadAsyncContent));
    memset(content,0,sizeof(OVDDemoLogUploadAsyncContent));

    strncpy(content->transid,trans_id,sizeof(content->transid));
    strncpy(content->start,start,sizeof(content->start));
    strncpy(content->end,end,sizeof(content->end));
    strncpy(content->url,url,sizeof(content->url));

 #ifdef __ARCH_RTOS__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 128*1024);
    int result = pthread_create(&OPENAPI_DEMO_LogUploadAsync_pid,&attr,pthread_OVD_DemoLogUploadAsync,(void *)content);
#else
	int result = pthread_create(&OPENAPI_DEMO_LogUploadAsync_pid,NULL,pthread_OVD_DemoLogUploadAsync,(void *)content);
#endif
    return result;
}


int OVD_DEMO_StopAlarm(OVDAlarmType alarmtype)
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","begin to OVD_StopAlarm func:%s alarmtype:%d\n",__func__,alarmtype);
    return 0;
}



//设备运行信息
int OVD_DEMO_GetDevRunningInfo(ovd_probe_devrunning_info_e info_e, void* data)
{
    int ret = 0;
    switch(info_e)
    {
        case OVD_CMD_GET_PACKET_LOSSRATE:
        {
            *(int*)data = 1;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","packetLossRate is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_WIFI_QUALITY:
        {
            *(int*)data = -90;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","wifiQuality is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_CPU_LOAD:
        {
            *(int*)data = 78;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","cpuLoad is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_MEMORY_LOAD:
        {
            *(int*)data = 68;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","memoryLoad is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_RUNTIME:
        {
            *(int*)data = 120000;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","runtime is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_MEMORY_AVAILABLE:
        {
            *(int*)data = 60;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","memoryAvailable is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_MEMORY_TOTAL:
        {
            *(int*)data = 60;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","memory load is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_FLASH_TOTAL:
        {
            *(int*)data = 55;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","flash load is %d\n", *(int*)data);
            break;
        }
        case OVD_CMD_GET_CHIP_SUPPLIER:
        {
            ovd_common_info_t* buf = ( ovd_common_info_t*)data;
            strncpy(buf->buf, "FULLHAN222222",buf->size);
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","chip supplier is %s,%d\n",buf->buf,buf->size);
            break;
        }
        case OVD_CMD_GET_CHIP_MODEL:
        {
            ovd_common_info_t* buf = ( ovd_common_info_t*)data;
            strncpy(buf->buf, "FH8856",buf->size);
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","chip model is %s,%d\n",buf->buf,buf->size);
            break;
        }
        case OVD_CMD_GET_HJZH_SDKVER:
        {
            ovd_common_info_t* buf = ( ovd_common_info_t*)data;
            strncpy(buf->buf, "1.5.3",buf->size);
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","hjzh sdk version is %s,%d\n",buf->buf,buf->size);
            break;
        }
        case OVD_CMD_GET_ANDLINK_SDKVER:
        {
            ovd_common_info_t* buf = ( ovd_common_info_t*)data;
            strncpy(buf->buf, "1.1.1",buf->size);
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","andlink sdk version is %s,%d\n",buf->buf,buf->size);
            break;
        }
        case OVD_CMD_GET_PING:  //不支持ping或traceroute，回调返回101 ;服务器url不可达，回调返回102
        {
            OvdProbeData* pkgdata = (OvdProbeData*)data;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ping url is %s\n",pkgdata->url);
            pkgdata->delay_time =600;
            pkgdata->result_data =50;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","ping delay is %lld, result is %d\n",pkgdata->delay_time,pkgdata->result_data);
            //return 101;
            break;
        }
        case OVD_CMD_GET_TRACEROUTE: //不支持ping或traceroute，回调返回101 ;服务器url不可达，回调返回102
        {
            OvdProbeData* pkgdata = (OvdProbeData*)data;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","traceroute url is %s\n",pkgdata->url);
            sleep(600);//模拟过程需要10分钟
            pkgdata->delay_time =400;
            pkgdata->result_data =4;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","traceroute delay is %lld,hops is %d\n",pkgdata->delay_time,pkgdata->result_data);
            break;
        }
        case OVD_CMD_GET_RSSI_4G:
        {
            *(int*)data = 66;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","4G RSSI is %d\n",*(int*)data);
            break;
        }
        case OVD_CMD_GET_POWER_MODE:
        {
            *(int*)data = OVD_5V_POWER;
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","Power Mode is %d\n",*(int*)data);
            break;
        }
    }
    return 0;
}

//参考代码，请勿搬运
int OVD_Demo_AudioOpenDev(OVD_int32 channel,OVDAudioDataFormat* st)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AudioOpenDev channel:%d, st->codec:%d  samplesrate:%d channelcount:%d",channel,st->codec,st->samplesRate,st->channelNumber);
    return 0;
}

int OVD_Demo_AudioCloseDev(OVD_int32 channel)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AudioCloseDev: channel:%d\n",channel);
    return 0;
}

int  OVD_Demo_AudioPlayData(OVD_int32 channel,OVD_int64 timestamp, int flags, unsigned char* buff, int len)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","playdata channel:%d ,ts:%lld  flags:%d\n",channel,timestamp,flags);
    static int outputaudio_fd=-1;
    if(-1== outputaudio_fd)
    {
        outputaudio_fd = open("./outputaudio", O_RDWR|O_TRUNC|O_CREAT,0777);
        if (outputaudio_fd>= 0) {
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","open audio file success\n");
        }
    }

    if(outputaudio_fd>=0)
    {
        write(outputaudio_fd, buff, len);
    }
    return 0;
}


int OVD_DEMO_SetCloudStatus(OVD_bool cloudStatusType, OVD_SetCloudInfo_e info_e)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","set cloud status， cloudStatusType:%d, cloudInfo:%d ",cloudStatusType,info_e);



    return 0;
}

int OVD_DEMO_SetLiveStatus(int channel, OVD_SetLiveInfo_e info_e)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","set live channel:%d, status:%d ", channel, info_e);

    return 0;
}


int OVD_DEMO_set_aov_status(int channel, enum OvdAovStatus info_e)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","set aov status channel:%d, status:%d ", channel, info_e);

    return 0;
}


int ovd_demo_running_status(enum OvdProbeExceptionType status, OVD_int32 channel_id, void* data, void* user_data)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","ovd_probe_exception_type_e %d,data %s\n",status,(char*)data);

    //域名解析错误下，厂商可以通过调整系统配置进行规避
    switch(status)
    {
        case OVD_RUNNING_STATUS_HOST_RESOLVE_ERROR:
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","发现域名解析异常,回调厂商进行规避\n");
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","data: %s\n",(char*)data);
            break;

        default:
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "unknow type %d\n", status);
    }
    return 0;
}
OVD_int32 OVD_DEMO_LogNotifyCb(const IN char *log_path){
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","notify to move key logs file to log_path");
    return 0;
}

OVD_int32  OVD_DEMO_bind_status_cb(IN OVD_binding_status_e status_e)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "bind status cb, status %d\n", status_e);
}

OVD_int32 OVD_DEMO_WifiSensingStatusQuery_cb(IN char* wifi_status, IN char* linkage_status, IN char* desc)
{
    strcpy(wifi_status, "uneffective");
    strcpy(linkage_status, "effective");
    strcpy(desc, "unknown error");
    OVD_LOG(OVD_LOGLEVEL_INFO, "DEV/DEMO","wifi sensing status query cb, wifi status %s, linkage status %s, desc %s\n", wifi_status, linkage_status, desc);
    return 0;
}

int g_AI_speech_pushing_flag=0;

static void * ovd_demo_ai_speech_pushing(void* arg)
{
    prctl(PR_SET_NAME,"demoaispeechavpush");
    char *inputsession_id = (char *)(arg);
    char session_id[256]={0};
    strncpy(session_id,inputsession_id,strlen(inputsession_id));
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AI speech pushing thread start %s wjj\n",session_id);

    //因为本demo直接推送有效数据，故推送有效数据在多轮会话场景下通知下平台
    OVD_SpeechAudioSessionNotify(session_id);

    //音视频格式输入
    OVDAudioDataFormat audioInfo;
    audioInfo.codec = source_STREAM_CODEC_AAC_WITH_ADTS;
    audioInfo.samplesRate = 16000;
    audioInfo.channelNumber = 1;
    OVD_SpeechMediaSendStart(session_id,&audioInfo);

    //读取AAC文件
    unsigned char* aacframe =NULL;
    unsigned char* aacbuffer = NULL;
    unsigned char* input_data = NULL;
    int nfilesize=0;
    int aacbuffer_data_size=0;
    int size=0;
    int data_size=0;
    int cnt=0;
    FILE *AISpeechAudioFp = fopen("./aac_file.aac","rb");
    if(AISpeechAudioFp != NULL)
    {
        fseek(AISpeechAudioFp, 0, SEEK_END);
        nfilesize = ftell(AISpeechAudioFp);
        rewind(AISpeechAudioFp);
        printf("aac file size:%d\n",nfilesize);
        int malloc_size=(ceil(nfilesize/1024)+1)*1024;
        //aacbuffer = (unsigned char*)malloc(504832);
        aacbuffer = (unsigned char*)malloc(malloc_size);
        aacframe = (unsigned char*)malloc(1024*50);
        if(NULL== aacbuffer)
        {
            printf("malloc aacbuffer failed");
            return NULL;
        }

        if(NULL==aacframe)
        {
            printf("malloc aacframe failed");
            return NULL;
        }

        aacbuffer_data_size=fread(aacbuffer,1,malloc_size,AISpeechAudioFp);
        fclose(AISpeechAudioFp);

        input_data=aacbuffer;
        data_size=aacbuffer_data_size;

    }
    else
    {
        printf("open aac file failed\n");
        return NULL;
    }

    int inter_audio=(double)((1024*1000000)/16000);
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long long audio_time=tv.tv_sec*1000000+tv.tv_usec;
    while(g_AI_speech_pushing_flag)
    {
        int ret = getADTSFrame(input_data,data_size,aacframe,&size);
        if(-1== ret)
        {
            break;
        }
        else if(1 == ret)
        {
            //读到文件尾
            data_size=0;
            size=0;
            cnt=0;

            data_size=aacbuffer_data_size;
            input_data=aacbuffer;
            g_AI_speech_pushing_flag = 0;
            continue;
        }

        data_size -= size;
        input_data += size;
        cnt++;
        //喂流
        OVD_SpeechMediaSend(session_id, aacframe, size,(audio_time/1000));
        usleep(inter_audio);
        audio_time += inter_audio;
    }
    free(aacframe);
    free(aacbuffer);

}



 OVD_int32 OVD_DEMO_SpeechStatusNotify_cb(IN OVDSpeechStatusNotifyType_e type,  char* session_id ,const void* buf, unsigned int len)
 {
    static pthread_t AI_speech_push_thread_id=0;

    printf("=====speech status notify cb: session_id:%s type:%d=====\n",session_id,type);

    switch(type)
    {

        case SPEECHSTATUSNOTIFYTYPE_MEDIACONNECTSUCCESS:


            printf("====语音通道建立成功====\n");
            printf("===模拟一个线程进行推流===\n");
            g_AI_speech_pushing_flag=1;
            pthread_create(&AI_speech_push_thread_id,NULL,ovd_demo_ai_speech_pushing,(void *)session_id);

            break;
        case SPEECHSTATUSNOTIFYTYPE_MEDIADISCONNECT:
            printf("===语音通道断开=====\n");
            break;
        case SPEECHSTATUSNOTIFYTYPE_VADEND:
            printf("====平台vad检测到语音停止=====\n");
            break;
    /*
        case SPEECHSTATUSNOTIFYTYPE_BACKMEDIA_AUDIO:
            printf("====收到平台下行音频=====\n");

            OVD_speechBackMedia_audio_t *backdata= (OVD_speechBackMedia_audio_t*)buf;
            printf("===base64编码数据长度：%d====\n",strlen(backdata->base64data));
            if(backdata->size == 0 || backdata->size <0)
            {
                printf("===下行音频结束====\n");
            }
            else
            {

                FILE *file = fopen("./backmedia.pcm", "ab");
                if(file == NULL)
                {
                    printf("open backmedia.pcm failed\n");
                    return -1;
                }
                size_t bytesWriteen = fwrite(backdata->data, 1, backdata->size, file);
                printf("revive %d bytes write %d bytes\n", backdata->size, bytesWriteen);
            }
            break;
            */
        case SPEECHSTATUSNOTIFYTYPE_BACKMEDIA_TEXT:
            printf("=====收到平台下行文本====\n");
            break;
        /*
            OVD_speechBackMedia_text_t *backdata_text= (OVD_speechBackMedia_text_t *)buf;
            printf("==下行文本:%s===\n",backdata_text->text);
            if(  1 == backdata_text->last_flag)
            {
                printf("===本次下行文本结束===\n");
            }
            break;
        */
       case SPEECHSTATUSNOTIFYTYPE_MEDIAEND:
            printf("=====收到平台单次下行音频结束通知====\n");
            OVD_SpeechAudioSessionClose(session_id);
            break;

        case  SPEECHSTATUSNOTIFYTYPE_MEDIAASRTEXT:
            printf("=====收到平台ASR文本====\n");

            OVDSpeechBackMediaText_t *back_text_info = (OVDSpeechBackMediaText_t *)buf;
            printf("==asr_text:%s===\n",back_text_info->text);
            if ( 1== back_text_info->last_flag)
            {
                printf("===本次ASR识别文本结束===\n");
            }
            break;
        case SPEECHSTATUSNOTIFYTYPE_MEDIAAUDIOUNIDENTIFIED:
            printf("===平台媒体语音识别未识别到关键词==\n");
            break;

    }
    return 0;
 }

OVD_int32 OVD_DEMO_SpeechPTZCtrl_cb(IN OVDSpeechPTZControlType_e type,IN const char* session_id, IN const char* text, IN unsigned long long sync_ts_ms)
{
    printf("=====收到平台云台语音控制指令=====\n");
    printf("==type[%d] session_id[%s] text[%s] sync_ts_ms[%lld]===\n ",type,session_id,text,sync_ts_ms);
    return 0;
}

OVD_int32 OVD_DEMO_SpeechDEVCtrl_cb(IN OVDSpeechDEVControlType_e type,IN const char* session_id,IN const char* value, IN const char* text, IN unsigned long long sync_ts_ms)
{
    printf("=====收到平台设备语音控制指令=====\n");
    printf("==type[%d] session_id[%s] value[%s] text[%s] sync_ts_ms[%lld]===\n ",type,session_id,value,text,sync_ts_ms);
    return 0;
}
OVD_int32 OVD_DEMO_SpeechMediaDownloadAndPlay_cb(IN const char* session_id,IN unsigned long long sync_ts_ms, IN const char * url)
{
    printf("====收到平台语音控制播放第3方媒体资源====\n");
    printf("==session_id[%s] sync_ts_ms[%lld] url[%s]===\n ",session_id,sync_ts_ms,url);
    return 0;
}
OVD_int32 OVD_DEMO_SpeechMakeCall_cb(IN OVDSpeechCallType_e type,IN const char* session_id, IN const char* phonenum, IN const char* nickname, IN unsigned long long  sync_ts_ms)
{
    printf("====收到平台固话呼叫指令====\n");
    printf("===typed[%d] session_id[%s] phonenum[%s] nickname[%s] sync_ts_ms[%lld]===",
    type,session_id,phonenum,nickname,sync_ts_ms);
    return 0;
}
OVD_int32 OVD_DEMO_AI_format_partition_cb(void)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_AI_format_partition_cb");
    return 0;
}

OVD_int32 OVD_DEMO_AI_exposure_status_cb(IN int appId, IN OVD_bool on, IN OVD_bool exposure_on, IN int exposure_adjust)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "OVD_DEMO_AI_exposure_status_cb");
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "appId:%d, on:%d, exposure_on:%d, exposure_adjust:%d", appId, on, exposure_on, exposure_adjust);
    return 0;
}


OVD_int32 OVD_DEMO_LinkageTrackingManual_cb(IN OVD_int32 pan, IN OVD_int32 tilt){
    printf("=====收到平台[手动枪球联动]指令=====\n");
    printf("==pan[%d] tilt[%d] ===\n ",pan, tilt);
    return 0;
}

OVD_int32 OVD_DEMO_LinkageTrackingFix_cb(){
    printf("=====收到平台[枪球联动矫正]指令=====\n");
    g_linkage_tracking_fix_progress = 0;        // 可加锁保护
    return 0;
}

OVD_int32 OVD_DEMO_LinkageTrackingFixQuery_cb(OUT OVD_int32 *progress){
    printf("=====收到平台[枪球联动矫正进度查询]指令=====\n");
    g_linkage_tracking_fix_progress += 10;          //模拟矫正进度变化
    if(g_linkage_tracking_fix_progress < 0) {
        g_linkage_tracking_fix_progress = 0;
    }
    if(g_linkage_tracking_fix_progress >100){
        g_linkage_tracking_fix_progress = 100;
    }
    printf("==当前进度[%d]===\n ",g_linkage_tracking_fix_progress);
    *progress = g_linkage_tracking_fix_progress;
    return 0;
}

void OVD_DEMO_AIDIY_ConfigCallback(IN OVD_AIDIY_Config config)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","AIDIY Config Callback, min_report_interval:%d", config.min_report_interval);
}

OVD_int32 OVD_DEMO_GetInterfacesCallback(OUT char ifaces[][IFNAMSIZ], IN OVD_int32 max_count) {
    OVD_int32 count = 0;
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_GetInterfacesCallback");
    if(ifaces == NULL || max_count <= 0) {
        return -1; // 错误处理
    }

    while (OVD_DEMO_Interfaces[count] != NULL && count < max_count) {
        // 使用 strncpy 确保不超过IFNAMSIZ，包括 '\0'
        strncpy(ifaces[count], OVD_DEMO_Interfaces[count], IFNAMSIZ);
        ifaces[count][IFNAMSIZ - 1] = '\0';

        ++count;
    }

    return (int)count;
}

OVD_int32 OVD_DEMO_EnvCollection_Callback(IN OVD_int32 channel)
{
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO","OVD_DEMO_EnvCollection_Callback, channel:%d", channel);
    return 0;
}


void setCallBackFunList(OVD_CallBackFunList *pcallBackFunList){
    pcallBackFunList->OVD_GetOVDDeviceInfo = OVD_DEMO_GetOVDDeviceInfo;
    pcallBackFunList->OVD_GetOVDConfigureInfo = OVD_DEMO_GetOVDConfigureInfo;
    pcallBackFunList->OVC_SetOVDConfigureInfo = OVD_DEMO_SetOVDConfigureInfo;
    pcallBackFunList->OVD_GetOVDChannelConfigureInfo = OVD_DEMO_GetOVDChannelConfigureInfo;

	pcallBackFunList->OVD_OVCConnectStatus = OVD_DEMO_OVCConnectStatus;
    pcallBackFunList->OVD_ReBootChannel = OVD_DEMO_ReBootChannel;
    pcallBackFunList->OVD_ReBootDevice = OVD_DEMO_ReBootDevice;
    pcallBackFunList->OVD_GetGpsInfo = OVD_DEMO_GetGpsInfo;
    pcallBackFunList->OVD_FirmwareBinUpgrade = OVD_DEMO_FirmwareBinUpgrade;
    pcallBackFunList->OVD_FirmwareUpgradeConfirmCb = OVD_DEMO_FirmwareUpgradeStatusCb;
    //pcallBackFunList->OVD_SyncTime = OVD_DEMO_SyncTime;
    //pcallBackFunList->OVD_QueryTime = OVD_DEMO_QueryTime;
    pcallBackFunList->OVD_SetSDCardFormat = OVD_DEMO_SetSDCardFormat;
    pcallBackFunList->OVD_PTZCmd = OVD_DEMO_PTZCmd;
    pcallBackFunList->OVD_GetPresetList = OVD_DEMO_GetPresetList;
    pcallBackFunList->OVD_ForceIFrame = OVD_DEMO_ForceIFrame;
    pcallBackFunList->OVD_Snapshot = OVD_DEMO_Snapshot;
    pcallBackFunList->OVD_AlarmNotifyCallback_t=OVD_DEMO_AlarmNotifyCalback;
    pcallBackFunList->OVD_StreamNotifyCallback_t=OVD_DEMO_StreamNotifyCalback;
    pcallBackFunList->OVD_NotifyLoginFail=OVD_DEMO_NotifyLoginFail;
    pcallBackFunList->OVD_SetAudioOutPlay = OVD_DEMO_SetMp3Url;
    pcallBackFunList->OVD_CtrlLullby=OVD_DEMO_CtrlLullby;

    pcallBackFunList->OVD_KeepAwakenUtilExpired = OVD_DEMO_KeepAwakenUtilExpired;
	pcallBackFunList->OVD_ResetConfiguration = OVD_DEMO_ResetConfig;
    pcallBackFunList->OVD_settime=OVD_DEMO_settime;
    pcallBackFunList->OVD_gettime=OVD_DEMO_gettime;
    pcallBackFunList->OVD_getsimpleovdinfo=OVD_DEMO_getsimpleovdinfo;
    pcallBackFunList->OVD_GetDiskInfo=OVD_DEMO_GetDiskInfo;
    pcallBackFunList->OVD_LogUploadAsync=OVD_DEMO_LogUploadAsync;

#ifdef CONFIG_MODULE_CardPlayback
    //srt 第二期卡录像方案
    pcallBackFunList->OVD_DMEAPI_callback_RecordSearch=OVD_DEMO_DMEAPI_callback_RecordSearch2;
    pcallBackFunList->OVD_DMEAPI_callback_RecordOpen=OVD_DEMO_DMEAPI_callback_RecordOpen;
    pcallBackFunList->OVD_DMEAPI_callback_RecordSeek=OVD_DEMO_DMEAPI_callback_RecordSeek;
    pcallBackFunList->OVD_DMEAPI_callback_RecordReadFrame=OVD_DEMO_DMEAPI_callback_RecordReadFrame;
    pcallBackFunList->OVD_DMEAPI_callback_RecordSpeedReadFrame=OVD_DEMO_DMEAPI_callback_RecordSpeedReadFrame;
    pcallBackFunList->OVD_DMEAPI_callback_RecordClose=OVD_DEMO_DMEAPI_callback_RecordClose;

#ifdef CONFIG_MODULE_LOCAL_STORAGE
    //卡存储模块如果打开则替换原有demo功能
    pcallBackFunList->OVD_DMEAPI_callback_RecordSearch=local_storage_playback_serach;
    pcallBackFunList->OVD_DMEAPI_callback_RecordOpen=local_storage_playback_open;
    pcallBackFunList->OVD_DMEAPI_callback_RecordSeek=local_storage_playback_seek;
    pcallBackFunList->OVD_DMEAPI_callback_RecordReadFrame=local_storage_playback_read_frame;
    pcallBackFunList->OVD_DMEAPI_callback_RecordClose=local_storage_playback_close;
#endif

#ifdef CONFIG_MODULE_LOCAL_STORAGE2
    pcallBackFunList->OVD_DMEAPI_callback_RecordSearch=localstorage2_callback_RecordSearch;
    pcallBackFunList->OVD_DMEAPI_callback_RecordOpen=localstorage2_playback_open;
    pcallBackFunList->OVD_DMEAPI_callback_RecordSeek=localstorage2_playback_seek;
    pcallBackFunList->OVD_DMEAPI_callback_RecordReadFrame=localstorage2_playback_read_frame;
    pcallBackFunList->OVD_DMEAPI_callback_RecordClose=localstorage2_playback_close;
#endif

#endif


    //pcallBackFunList->OVD_extension_callback = OVD_DEMO_voip_Extension;  弃用
    pcallBackFunList->OVD_StopAlarm = OVD_DEMO_StopAlarm;
    pcallBackFunList->OVD_EBOCmd = OVD_DEMO_EBOCmd;
    pcallBackFunList->OVD_GetDevRunningInfo = OVD_DEMO_GetDevRunningInfo;
    pcallBackFunList->OVD_ROBOTCmd = OVD_DEMO_ROBOTCmd;

    //对讲
    pcallBackFunList->OVD_AudioOpenDev= OVD_Demo_AudioOpenDev;
    pcallBackFunList->OVD_AudioCloseDev=OVD_Demo_AudioCloseDev;
    pcallBackFunList->OVD_AudioPlayData=OVD_Demo_AudioPlayData;

    pcallBackFunList->OVD_SetCloudStatus = OVD_DEMO_SetCloudStatus;
    pcallBackFunList->OVD_set_live_status = OVD_DEMO_SetLiveStatus;
    pcallBackFunList->OVD_set_aov_switch = OVD_DEMO_set_aov_status;
    pcallBackFunList->ovd_running_status_cb = ovd_demo_running_status;
    pcallBackFunList->OVD_bind_status_cb = OVD_DEMO_bind_status_cb;
    pcallBackFunList->OVD_WifiSensingStatusQuery_cb = OVD_DEMO_WifiSensingStatusQuery_cb;
    pcallBackFunList->OVD_LogNotifyCb = OVD_DEMO_LogNotifyCb;

    //智能语音交互对讲
    pcallBackFunList->OVD_SpeechStatusNotify_cb=OVD_DEMO_SpeechStatusNotify_cb;
    pcallBackFunList->OVD_SpeechPTZCtrl_cb=OVD_DEMO_SpeechPTZCtrl_cb;
    pcallBackFunList->OVD_SpeechDevCtrl_cb=OVD_DEMO_SpeechDEVCtrl_cb;
    pcallBackFunList->OVD_SpeechMediaDownloadAndPlay_cb=OVD_DEMO_SpeechMediaDownloadAndPlay_cb;
    pcallBackFunList->OVD_SpeechMakeCall_cb=OVD_DEMO_SpeechMakeCall_cb;

    // 枪球联动
    pcallBackFunList->OVD_LinkageTrackingManual=OVD_DEMO_LinkageTrackingManual_cb;
    pcallBackFunList->OVD_LinkageTrackingFix=OVD_DEMO_LinkageTrackingFix_cb;
    pcallBackFunList->OVD_LinkageTrackingFixQuery=OVD_DEMO_LinkageTrackingFixQuery_cb;

    // aidiy
    pcallBackFunList->OVD_AIDIY_ConfigCallback=OVD_DEMO_AIDIY_ConfigCallback;

    // 网络接口获取
    pcallBackFunList->OVD_GetInterfacesCallback=OVD_DEMO_GetInterfacesCallback;

    // 3D光感知
    pcallBackFunList->OVD_EnvCollection = OVD_DEMO_EnvCollection_Callback;
}
