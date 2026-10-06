#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include "OVD_Demo.h"
#include "OVD_OpenAPI.h"
#include "inirw.h"

void osd_set()
{
	int i=0,osd_num=0,len=16;
	char tmp_on[len];
	char tmp_mode[len];
	char tmp_id[len];
	char tmp_font[len];
	char tmp_pos_x[len];
	char tmp_pos_y[len];
	char tmp_red[len];
	char tmp_green[len];
	char tmp_blue[len];
	char tmp_text[len];
	int osd_count=OVD_DEMO_ConfigureInfo.channelsInfo[i].osdcount;
	for (osd_num = 0; osd_num < osd_count; osd_num++) {
		sprintf(tmp_on, "%s_%d", "OSD_on", osd_num);
		iniSetInt("deviceOSD",tmp_on, OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].on,10);
		sprintf(tmp_mode, "%s_%d", "OSD_mode", osd_num);
		iniSetInt("deviceOSD",tmp_mode, OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].mode,10);
		sprintf(tmp_id, "%s_%d", "OSD_id", osd_num);
		iniSetInt("deviceOSD",tmp_id, OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].id,10);
		sprintf(tmp_text, "%s_%d", "OSD_text",osd_num);
		iniSetString("deviceOSD",tmp_text,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].text);
		sprintf(tmp_pos_x, "%s_%d", "OSD_pos_x", osd_num);
		iniSetInt("deviceOSD",tmp_pos_x,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.x, 10);
		sprintf(tmp_pos_y, "%s_%d", "OSD_pos_y", osd_num);
		iniSetInt("deviceOSD",tmp_pos_y,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdpos.y, 10);
		sprintf(tmp_red, "%s_%d", "red", osd_num);
		iniSetInt("deviceOSD",tmp_red,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.red, 10);
		sprintf(tmp_green, "%s_%d", "green", osd_num);
		iniSetInt("deviceOSD",tmp_green,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.green, 10);
		sprintf(tmp_blue, "%s_%d", "blue", osd_num);
		iniSetInt("deviceOSD",tmp_blue,OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].color.blue, 10);
		sprintf(tmp_font, "%s_%d", "OSD_font", osd_num);
		iniSetInt("deviceOSD",tmp_font, OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo[osd_num].osdfont,10);
	}
	iniSetInt("deviceOSD","OSD_logo_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].osd_logo, 10);
	return;
}

void feeder_set()
{
	int feeder_num=0;
	iniSetInt("DeviceFeeder","feeder_immed_quantity",OVD_DEMO_ConfigureInfo.feeder_immed_quantity, 10);
    char tmp_on[128];
    for (feeder_num=0; feeder_num<(OVD_DEMO_ConfigureInfo.feedercount); feeder_num++) {
        sprintf(tmp_on, "%s_%d", "feeder_plan_on", feeder_num);
		iniSetInt("DeviceFeeder",tmp_on, OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].on,10);
        sprintf(tmp_on, "%s_%d", "feeder_plan_name", feeder_num);
        iniSetString("DeviceFeeder",tmp_on,OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].name);
        sprintf(tmp_on, "%s_%d", "feeder_plan_time", feeder_num);
        iniSetString("DeviceFeeder",tmp_on,OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].time);
        sprintf(tmp_on, "%s_%d", "feeder_plan_quantity", feeder_num);
		iniSetInt("DeviceFeeder",tmp_on, OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].quantity,10);
        sprintf(tmp_on, "%s_%d", "feeder_plan_repeat", feeder_num);
		iniSetInt("DeviceFeeder",tmp_on, OVD_DEMO_ConfigureInfo.feeder_plan_config[feeder_num].repeat,10);

    }
	return;
}

void robot_set()
{
	int rebot_num=0;
	iniSetInt("DeviceRobot","robot_count",OVD_DEMO_ConfigureInfo.robotcount, 10);
    char tmp_on[128];
    for (rebot_num=0; rebot_num<(OVD_DEMO_ConfigureInfo.robotcount); rebot_num++) {
        sprintf(tmp_on, "%s_%d", "bizType", rebot_num);
		iniSetInt("DeviceRobot",tmp_on, OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].bizType,10);
        sprintf(tmp_on, "%s_%d", "startTime", rebot_num);
        iniSetString("DeviceRobot",tmp_on,OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].start_time);
        sprintf(tmp_on, "%s_%d", "endTime", rebot_num);
        iniSetString("DeviceRobot",tmp_on,OVD_DEMO_ConfigureInfo.robot_home_config[rebot_num].end_time);
    }
	return;
}

void mobocam_set()
{
	iniSetInt("mobocamDevice", "speedSwitch", OVD_DEMO_ConfigureInfo.mobocam_config.speedswith, 5);
	int robot_num=0;
	iniSetInt("mobocamDevice","robot_count",OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount, 3);
    
	char tmp_on[128];
    for (robot_num=0; robot_num<(OVD_DEMO_ConfigureInfo.mobocam_config.mobocamcount); robot_num++) {
        sprintf(tmp_on, "%s_%d", "cruiseId", robot_num);
		iniSetInt("mobocamDevice",tmp_on, OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].cruiseId,10);
        sprintf(tmp_on, "%s_%d", "start_time", robot_num);
        iniSetString("mobocamDevice",tmp_on,OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].start_time);
        sprintf(tmp_on, "%s_%d", "end_time", robot_num);
        iniSetString("mobocamDevice",tmp_on,OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].end_time);
		sprintf(tmp_on, "%s_%d", "time_long", robot_num);
		iniSetInt("mobocamDevice",tmp_on, OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].time_long,10);
		sprintf(tmp_on, "%s_%d", "repeat", robot_num);
		iniSetInt("mobocamDevice",tmp_on, OVD_DEMO_ConfigureInfo.mobocam_config.cruise_config[robot_num].repeat,10);
    }
	iniSetInt("mobocamDevice", "track_type", OVD_DEMO_ConfigureInfo.mobocam_config.track_type, 2);
	iniSetInt("mobocamDevice", "track_on", OVD_DEMO_ConfigureInfo.mobocam_config.track_on, 1);
	return;
}

void osd_default()
{
	int i=0, len=16;
	int osd_count=OVD_DEMO_ConfigureInfo.channelsInfo[i].osdcount;
	int ini_osd_cnt=iniGetInt("deviceOSD","OSD_count",10);
	int osd_num=ini_osd_cnt-1;

	char tmp_on[len];
	char tmp_font[len];
	char tmp_pos_x[len];
	char tmp_pos_y[len];
	char tmp_red[len];
	char tmp_green[len];
	char tmp_blue[len];
	char tmp_text[len];

	while (ini_osd_cnt > osd_count) {
		sprintf(tmp_on, "%s_%d", "OSD_on", osd_num);
		iniSetInt("deviceOSD",tmp_on,0,10);
		sprintf(tmp_text, "%s_%d", "OSD_text",osd_num);
		iniSetString("deviceOSD",tmp_text,NULL);
		sprintf(tmp_pos_x, "%s_%d", "OSD_pos_x", osd_num);
		iniSetInt("deviceOSD",tmp_pos_x,-1, 10);
		sprintf(tmp_pos_y, "%s_%d", "OSD_pos_y", osd_num);
		iniSetInt("deviceOSD",tmp_pos_y,-1, 10);
		sprintf(tmp_red, "%s_%d", "red", osd_num);
		iniSetInt("deviceOSD",tmp_red,-1, 10);
		sprintf(tmp_green, "%s_%d", "green", osd_num);
		iniSetInt("deviceOSD",tmp_green,-1, 10);
		sprintf(tmp_blue, "%s_%d", "blue", osd_num);
		iniSetInt("deviceOSD",tmp_blue,-1, 10);
		sprintf(tmp_font, "%s_%d", "OSD_font", osd_num);
		iniSetInt("deviceOSD",tmp_font,0,10);
		osd_num--;
		ini_osd_cnt--;
	}

	return;
}


void OVD_DEMO_OSD()
{
	osd_set();
	osd_default();
	return;
}

int OVD_DEMO_SetConfig()
{
	iniFileLoad(CFG_FILEPATH);
	int i=0;

   	char *sect="setableKeys";

    iniSetInt(sect, "channel_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].on, 10);
	iniSetInt(sect, "smart_mode_switch_2", OVD_DEMO_ConfigureInfo.channelsInfo[i].smart_mode_switch_2, 10);
    iniSetInt(sect, "static_scence_ABR_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].static_scence_ABR_on,10);
	iniSetInt(sect, "audioOutValume", OVD_DEMO_ConfigureInfo.channelsInfo[i].audioOutValume, 10);
	iniSetInt(sect, "doorbellOutValume", OVD_DEMO_ConfigureInfo.channelsInfo[i].doorbell_volume, 10);
	iniSetInt(sect, "traceAbility", OVD_DEMO_ConfigureInfo.channelsInfo[i].traceAbility,10);
	iniSetInt(sect, "tracetarget", OVD_DEMO_ConfigureInfo.channelsInfo[i].traces.tracetarget,10);

	//帧率码率设置
	iniSetInt(sect, "bitrate_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_mode,10);
	iniSetInt(sect, "bitrate_average",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_average,10);
	iniSetInt(sect, "bitrate_peak",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_peak,10);
	iniSetInt(sect, "bitrate_statically",OVD_DEMO_ConfigureInfo.channelsInfo[i].bitrate_statically,10);
	iniSetInt(sect, "framerate",OVD_DEMO_ConfigureInfo.channelsInfo[i].framerate,10);
	iniSetInt(sect, "gop", OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.frameInterval, 15);
    iniSetInt(sect, "width", OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.width, 10);
    iniSetInt(sect, "height", OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.height, 10);

	//led
	iniSetInt(sect,"led", OVD_DEMO_ConfigureInfo.led, 10);

    iniSetInt(sect,"light_supplement_lamp_on", OVD_DEMO_ConfigureInfo.light_supplement_lamp_on, 10);
    //screen
    iniSetInt(sect,"screen",OVD_DEMO_ConfigureInfo.screen,10);
	//智能夜视
	iniSetInt(sect,"normal_nightvision_mode", OVD_DEMO_ConfigureInfo.normal_nightvision_mode, 10);
	iniSetInt(sect,"color_nightvision_mode", OVD_DEMO_ConfigureInfo.color_nightvision_mode, 10);
    iniSetInt(sect,"nightvision_detect_mode", OVD_DEMO_ConfigureInfo.nightvision_detect_mode, 10);
	//补光灯
	iniSetInt(sect,"fill_light_on", OVD_DEMO_ConfigureInfo.fill_light.on, 10);
	iniSetInt(sect,"fill_light_brightness", OVD_DEMO_ConfigureInfo.fill_light.brightness, 10);
	iniSetInt(sect,"fill_light_schedule_on", OVD_DEMO_ConfigureInfo.fill_light.schedule.on, 10);
	iniSetString(sect,"fill_light_start_time", OVD_DEMO_ConfigureInfo.fill_light.schedule.starttime);
	iniSetString(sect,"fill_light_end_time", OVD_DEMO_ConfigureInfo.fill_light.schedule.endtime);
	iniSetInt(sect,"fill_light_repeat", OVD_DEMO_ConfigureInfo.fill_light.schedule.repeat, 10);
	//智能叠加显示
	iniSetInt(sect,"AI_rules_mode_on", OVD_DEMO_ConfigureInfo.AI_rules_mode_on, 10);
	//图像翻转
	iniSetInt(sect,"horflip", OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.horflip, 10);
	iniSetInt(sect,"verflip", OVD_DEMO_ConfigureInfo.channelsInfo[i].flipInfo.verflip, 10);
	//音频 视频编码 质量 加密
	iniSetInt(sect,"video_codec", OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.codec, 10);
	iniSetInt(sect,"video_quality", OVD_DEMO_ConfigureInfo.channelsInfo[i].videoinfo.quality, 10);
	iniSetInt(sect,"audio_codec", OVD_DEMO_ConfigureInfo.channelsInfo[i].audioinfo.codec, 10);

    // 是否外接音柱
    iniSetInt(sect,"has_speek", OVD_DEMO_ConfigureInfo.channelsInfo[i].has_speek, 10);

	//sd card
	iniSetInt(sect,"sd_state", OVD_DEMO_SdInfo.SDExist, 10);
	iniSetInt(sect,"sd_total_size", OVD_DEMO_SdInfo.SDTotalSize, 10);
	iniSetInt(sect,"sd_free_size", OVD_DEMO_SdInfo.SDFreeSize, 10);
	//autoreboot
	iniSetInt(sect, "autoreboot_on", OVD_DEMO_ConfigureInfo.autorebootinfo.on,10);
	iniSetInt(sect, "autoreboot_cycle", OVD_DEMO_ConfigureInfo.autorebootinfo.cycle,10);
	iniSetString(sect,"autoreboot_start",OVD_DEMO_ConfigureInfo.autorebootinfo.start);
	iniSetString(sect,"autoreboot_end",OVD_DEMO_ConfigureInfo.autorebootinfo.end);

	//庭院灯
	iniSetInt(sect,"light_brightness",OVD_DEMO_ConfigureInfo.yardlightconfig.light_brightness,50);
	iniSetInt(sect,"control_mode", OVD_DEMO_ConfigureInfo.yardlightconfig.control_mode,0);
	iniSetInt(sect,"light_on",OVD_DEMO_ConfigureInfo.yardlightconfig.light_on, 0);
	iniSetString(sect,"start_time_0",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].start_time);
	iniSetString(sect,"end_time_0",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[0].end_time);
	iniSetString(sect,"start_time_1",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[1].start_time);
	iniSetString(sect,"end_time_1",OVD_DEMO_ConfigureInfo.yardlightconfig.timed_schedule[1].end_time);


	//switch_schedule
	iniSetInt(sect,"switch_schedule_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.on,10);
	iniSetInt(sect,"switch_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.repeat,10);
	iniSetString(sect,"switch_start_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.start_time);
	iniSetString(sect,"switch_shutdown_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].switchschedule.shutdown_time);

	//行车记录仪
	iniSetInt(sect,"collision_alarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.collision_alarm_sensitivity,10);
	iniSetInt(sect,"parking_notice", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_notice,10);
	iniSetInt(sect,"parking_collision", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.parking_collision,10);
	iniSetInt(sect,"driving_collision_before", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.before,10);
	iniSetInt(sect,"driving_collision_after", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.driving_collision.after,10);
	iniSetInt(sect,"voice_capture_before", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.before,10);
	iniSetInt(sect,"voice_capture_after", OVD_DEMO_ConfigureInfo.channelsInfo[i].dashcam_info.alarm_duration.voice_capture.after,10);

	iniSetInt(sect, "dashcam_findcar_on", OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_findcar_on,10);
	iniSetInt(sect, "dashcam_rescue_on", OVD_DEMO_ConfigureInfo.dashcam_config.dashcam_rescue_on,10);

	
	iniSetInt(sect, "EBO_nofall_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_nofall_on,10);
	iniSetInt(sect, "EBO_laser_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_laser_on,10);
	iniSetInt(sect, "EBO_track_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_on,10);
	iniSetInt(sect, "EBO_track_type", OVD_DEMO_ConfigureInfo.eboconfig.EBO_track_type,10);
	iniSetInt(sect, "EBO_collision_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_collision_on,10);
	iniSetInt(sect, "EBO_reminder_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_reminder_on,10);
	iniSetInt(sect, "EBO_speed_adjust", OVD_DEMO_ConfigureInfo.eboconfig.EBO_speed_adjust,10);
	iniSetInt(sect, "EBO_cruise_on", OVD_DEMO_ConfigureInfo.eboconfig.EBO_cruise_on,10);
	iniSetString(sect, "EBO_cruise_start_time", OVD_DEMO_ConfigureInfo.eboconfig.start_time);
	iniSetInt(sect, "EBO_cruise_time_long", OVD_DEMO_ConfigureInfo.eboconfig.time_long,10);
	iniSetInt(sect, "EBO_cruise_repeat", OVD_DEMO_ConfigureInfo.eboconfig.repeat,10);

	iniSetInt(sect, "OWL_food_on", OVD_DEMO_ConfigureInfo.robot_config.OWL_food_on,10);
	iniSetInt(sect, "OWL_track_mode", OVD_DEMO_ConfigureInfo.robot_config.OWL_track_mode,10);
	iniSetInt(sect, "OWL_touch_mode", OVD_DEMO_ConfigureInfo.robot_config.OWL_touch_mode,10);
	iniSetInt(sect, "OWL_sound_on", OVD_DEMO_ConfigureInfo.robot_config.OWL_sound_on,10);
	iniSetInt(sect, "OWL_speed_switch", OVD_DEMO_ConfigureInfo.robot_config.OWL_speed_switch,10);
	iniSetString(sect, "OWL_cruise_start_time", OVD_DEMO_ConfigureInfo.robot_config.start_time);
	iniSetInt(sect, "OWL_cruise_repeat", OVD_DEMO_ConfigureInfo.robot_config.repeat,10);

	//喂食器
	feeder_set();

	//机器人管家
	robot_set();

	//可移动摄像头
	mobocam_set();


	//osd
	OVD_DEMO_OSD();
/*
	iniSetInt("deviceOSD", "OSD_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.on, 10);
	iniSetString("deviceOSD","OSD_text",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.text);
	iniSetInt("deviceOSD","OSD_font", OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.osdfont,10);
	iniSetInt("deviceOSD","OSD_pos_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.osdpos.x, 10);
	iniSetInt("deviceOSD","OSD_pos_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.osdpos.y, 10);
	iniSetInt("deviceOSD","red",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.color.red, 10);
	iniSetInt("deviceOSD","green",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.color.green, 10);
	iniSetInt("deviceOSD","blue",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.color.blue, 10);
	iniSetInt("deviceOSD","OSD_logo_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].osdinfo.osd_logo, 10);
*/

	//AI face
	iniSetInt("deviceAI","AI_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.on, 10);
	iniSetInt("deviceAI","AI_exposure_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_on, 10);
	iniSetInt("deviceAI","AI_exposure_adjust", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.exposure_adjust, 10);
	iniSetInt("deviceAI","AI_quality_value", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.quality_value, 10);
	iniSetString("deviceAI","AI_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.mode);
	iniSetInt("deviceAI", "AI_alarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.sensitivity,10);
	iniSetInt("deviceAI", "faceAlarm_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_x,10);
	iniSetInt("deviceAI", "faceAlarm_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.left_up_y,10);
	iniSetInt("deviceAI", "faceAlarm_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_x,10);
	iniSetInt("deviceAI", "faceAlarm_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.zoneInfo.right_bottom_y,10);

    // mask detection
    iniSetInt("deviceAI", "mask_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.on,10);

    iniSetInt("deviceAI", "maskDete_alerttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.on,10);
	iniSetString("deviceAI", "maskDete_alerttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.starttime);
	iniSetString("deviceAI", "maskDete_alerttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.endtime);
	iniSetInt("deviceAI", "maskDete_alerttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.maskSchedule.repeat,10);

	iniSetInt("deviceAI", "maskDete_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_on,10);
	iniSetInt("deviceAI", "maskDete_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_vol,10);
	iniSetInt("deviceAI", "maskDete_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat,10);
	iniSetInt("deviceAI", "maskDete_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_repeat_constant,10);
	iniSetInt("deviceAI", "maskDete_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.speech_interval,10);
	iniSetInt("deviceAI", "maskDete_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur_constant,10);
	iniSetInt("deviceAI", "maskDete_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_on,10);
	iniSetInt("deviceAI", "maskDete_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_mode,10);
	iniSetInt("deviceAI", "maskDete_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.light_dur,10);
	iniSetInt("deviceAI", "maskDete_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_on,10);
	iniSetInt("deviceAI", "maskDete_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIface.maskDeteInfo.masklinkage.buz_dur,10);


	//AI 厨房
	iniSetInt("deviceAI","kitchen_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.on, 1);
	iniSetInt("deviceAI","kitchen_mask_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.mask_on ,1);
	iniSetInt("deviceAI","kitchen_cap_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.cap_on,1);
	iniSetInt("deviceAI","kitchen_clothes_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_on,1);
	iniSetString("deviceAI", "kitchen_clothes_color", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.clothes_color);
	iniSetInt("deviceAI","kitchen_alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.sensitivity,100);
	iniSetInt("deviceAI","kitchenAlarm_left_up_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_x,1);
   	iniSetInt("deviceAI","kitchenAlarm_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.left_up_y,1);
    iniSetInt("deviceAI","kitchenAlarm_right_bottom_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_x,100);
	iniSetInt("deviceAI","kitchenAlarm_right_bottom_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIkitchen.zoneInfo.right_bottom_y,100);

	//AI 车辆
	iniSetInt("deviceAI","vehicle_detection_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.on,1);
	iniSetInt("deviceAI","vehicle_alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.sensitivity,100);
	iniSetInt("deviceAI","vehicle_quality",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.quality,80);
	iniSetInt("deviceAI","vehicle_capture_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.capture_mode,0);
	iniSetInt("deviceAI","vehicle_scheduled_capture_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.scheduled_capture_time,60);
	iniSetInt("deviceAI", "veh_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_on,10);
	iniSetInt("deviceAI", "veh_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_vol,10);
	iniSetInt("deviceAI", "veh_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat,10);
	iniSetInt("deviceAI", "veh_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_repeat_constant,10);
	iniSetInt("deviceAI", "veh_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.speech_interval,10);
	iniSetInt("deviceAI", "veh_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur_constant,10);
	iniSetInt("deviceAI", "veh_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_on,10);
	iniSetInt("deviceAI", "veh_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_mode,10);
	iniSetInt("deviceAI", "veh_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.light_dur,10);
	iniSetInt("deviceAI", "veh_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_on,10);
	iniSetInt("deviceAI", "veh_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.vehlinkage.buz_dur,10);
	iniSetInt("deviceAI", "veh_detect_site", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.detect_site,10);
	iniSetInt("deviceAI", "veh_exit_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.exit_mode,10);
	iniSetInt("deviceAI","vehicleAlarm_left_up_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_x,1);
	iniSetInt("deviceAI","vehicleAlarm_left_up_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.left_up_y,1);
	iniSetInt("deviceAI","vehicleAlarm_right_bottom_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_x,100);
	iniSetInt("deviceAI","vehicleAlarm_right_bottom_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIvehicle.zoneInfo.right_bottom_y,100);

	//AI 非机动车（梯控）
	iniSetInt("deviceAI","nonmotorvehicle_detection_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.on,1);
	iniSetString("deviceAI", "nonmotorvehicle_detect_mode_list", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.detect_mode_list);
	iniSetInt("deviceAI","nonmotorvehicle_alarm_sensitivity",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.sensitivity,100);
	iniSetInt("deviceAI","nonmotorvehicle_quality",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.quality,80);
	iniSetInt("deviceAI","nonmotorvehicle_capture_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.capture_mode,0);
	iniSetInt("deviceAI","nonmotorvehicle_scheduled_capture_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.scheduled_capture_time,60);
	iniSetInt("deviceAI", "nonveh_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_on,10);
	iniSetInt("deviceAI", "nonveh_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_vol,10);
	iniSetInt("deviceAI", "nonveh_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat,10);
	iniSetInt("deviceAI", "nonveh_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_repeat_constant,10);
	iniSetInt("deviceAI", "nonveh_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.speech_interval,10);
	iniSetInt("deviceAI", "nonveh_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur_constant,10);
	iniSetInt("deviceAI", "nonveh_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_on,10);
	iniSetInt("deviceAI", "nonveh_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_mode,10);
	iniSetInt("deviceAI", "nonveh_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.light_dur,10);
	iniSetInt("deviceAI", "nonveh_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_on,10);
	iniSetInt("deviceAI", "nonveh_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.nonvehlinkage.buz_dur,10);
	iniSetInt("deviceAI","nonmotorvehicleAlarm_left_up_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_x,1);
	iniSetInt("deviceAI","nonmotorvehicleAlarm_left_up_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.left_up_y,1);
	iniSetInt("deviceAI","nonmotorvehicleAlarm_right_bottom_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_x,100);
	iniSetInt("deviceAI","nonmotorvehicleAlarm_right_bottom_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AInonmotorvehicle.zoneInfo.right_bottom_y,100);


    //AI passenger
    iniSetInt("deviceAI","AI_passenger_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.on, 10);
    iniSetInt("deviceAI","AI_passenger_exposure_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_on, 10);
    iniSetInt("deviceAI","AI_passenger_osd_status",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.osd_status,10);
    iniSetInt("deviceAI","AI_passenger_exposure_adjust", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.exposure_adjust, 10);
    iniSetInt("deviceAI","AI_passenger_quality_value", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.quality_value, 10);
    iniSetString("deviceAI","AI_passenger_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.mode);
    iniSetInt("deviceAI", "AI_passenger_alarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.sensitivity,10);
    iniSetInt("deviceAI", "passenger_staticstic_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.statistics_mode,0);
    iniSetInt("deviceAI", "passenger_staticstic_line_a_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_x,100);
    iniSetInt("deviceAI", "passenger_staticstic_line_a_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_a_y,100);
    iniSetInt("deviceAI", "passenger_staticstic_line_b_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_x,200);
    iniSetInt("deviceAI", "passenger_staticstic_line_b_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.lineInfo.point_b_y,100);
    iniSetInt("deviceAI", "passenger_Alarm_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_x,10);
    iniSetInt("deviceAI", "passenger_Alarm_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.left_up_y,10);
    iniSetInt("deviceAI", "passenger_Alarm_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_x,10);
    iniSetInt("deviceAI", "passenger_Alarm_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.zoneInfo.right_bottom_y,10);
    iniSetInt("deviceAI", "passenger_welcome_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_on,10);
	iniSetInt("deviceAI", "passenger_welcome_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_vol,10);
	iniSetInt("deviceAI", "passenger_welcome_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_repeat,10);
	iniSetString("deviceAI","passenger_welcome_speech_url_enter", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_enter);
	iniSetString("deviceAI","passenger_welcome_speech_url_leave", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.speech.speech_url_leave);
	iniSetString("deviceAI","passenger_welcome_alerttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.start_time);
	iniSetString("deviceAI","passenger_welcome_alerttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.shutdown_time);
	iniSetInt("deviceAI","passenger_welcome_alerttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPassenger.switchschedule.repeat,10);

	//AI regional people
    iniSetInt("deviceAI","regional_people_stat_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.on, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.on, 10);
   	iniSetInt("deviceAI","regional_people_stat_plan0_id", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_id, 10);
    iniSetString("deviceAI", "regional_people_stat_plan0_name", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_name);
    iniSetInt("deviceAI","regional_people_stat_plan0_count", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.regional_people_count, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_alarm_report_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.alarm_report_duration, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_detect_result_report_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.detect_result_report_duration, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_osd_status", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.osd_status, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_alerttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.on, 10);
    iniSetString("deviceAI","regional_people_stat_plan0_alerttime_start",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.starttime);
    iniSetString("deviceAI","regional_people_stat_plan0_alerttime_end",OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.endtime);
    iniSetInt("deviceAI","regional_people_stat_plan0_alerttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.schedule.repeat, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_on, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_vol, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_repeat, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_repeat_constant, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.speech_interval, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_dur_constant, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_on, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_mode, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.light_dur, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.buz_on, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.linkage.buz_dur, 10);
	iniSetInt("deviceAI","regional_people_stat_plan0_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_up_x, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_up_y, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_bottom_x, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_bottom_y, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_left_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_bottom_x, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_left_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.left_bottom_y, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_right_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_up_x, 10);
    iniSetInt("deviceAI","regional_people_stat_plan0_right_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIRegionalPeopleStat.detect_plan.zoneInfo.right_up_y, 10);

	//AI helmet
	iniSetInt("deviceAI","helmet_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.on, 10);
    iniSetInt("deviceAI","helmet_detecttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.on, 10);
    iniSetString("deviceAI","helmet_detecttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.starttime);
    iniSetString("deviceAI","helmet_detecttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.endtime);
    iniSetInt("deviceAI","helmet_detecttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.schedule.repeat, 10);
    iniSetInt("deviceAI","helmet_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_x, 10);
    iniSetInt("deviceAI","helmet_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_up_y, 10);
    iniSetInt("deviceAI","helmet_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_x, 10);
    iniSetInt("deviceAI","helmet_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_bottom_y, 10);
    iniSetInt("deviceAI","helmet_left_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_x, 10);
    iniSetInt("deviceAI","helmet_left_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.left_bottom_y, 10);
    iniSetInt("deviceAI","helmet_right_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_x, 10);
    iniSetInt("deviceAI","helmet_right_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.zoneInfo.right_up_y, 10);
    iniSetInt("deviceAI","helmet_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_on, 10);
    iniSetInt("deviceAI","helmet_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_vol, 10);
    iniSetInt("deviceAI","helmet_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat, 10);
	iniSetInt("deviceAI","helmet_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_repeat_constant,10);
	iniSetInt("deviceAI","helmet_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.speech_interval,10);
	iniSetInt("deviceAI","helmet_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur_constant,10);
    iniSetInt("deviceAI","helmet_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_on, 10);
    iniSetInt("deviceAI","helmet_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_mode, 10);
    iniSetInt("deviceAI","helmet_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.light_dur, 10);
    iniSetInt("deviceAI","helmet_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_on, 10);
    iniSetInt("deviceAI","helmet_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIHelmet.linkage.buz_dur, 10);

	// 烟火检测
	iniSetInt("deviceAI","smokefire_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.on, 10);
    iniSetInt("deviceAI","smokefire_detecttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.on, 10);
    iniSetString("deviceAI","smokefire_detecttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.starttime);
    iniSetString("deviceAI","smokefire_detecttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.endtime);
    iniSetInt("deviceAI","smokefire_detecttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.schedule.repeat, 10);

    iniSetInt("deviceAI","smokefire_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_on, 10);
    iniSetInt("deviceAI","smokefire_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_vol, 10);
    iniSetInt("deviceAI","smokefire_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_repeat, 10);
    iniSetInt("deviceAI","smokefire_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_interval, 10);
    iniSetString("deviceAI","smokefire_speech_url", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.speech_url);
    iniSetInt("deviceAI","smokefire_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_on, 10);
    iniSetInt("deviceAI","smokefire_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_mode, 10);
    iniSetInt("deviceAI","smokefire_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.light_dur, 10);
    iniSetInt("deviceAI","smokefire_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.buz_on, 10);
    iniSetInt("deviceAI","smokefire_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AISmokefire.linkage.buz_dur, 10);

	//AI gesture
	iniSetInt("deviceAI","gesture_recognition_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.on, 10);
	iniSetInt("deviceAI","gesture_call_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_on, 10);
	iniSetInt("deviceAI","gesture_call_duration", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_duration, 10);
	iniSetInt("deviceAI","gesture_call_schedule_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.on, 10);
	iniSetString("deviceAI","gesture_call_schedule_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.starttime);
	iniSetString("deviceAI","gesture_call_schedule_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.endtime);
	iniSetInt("deviceAI","gesture_call_schedule_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.call_schedule.repeat, 10);
	iniSetInt("deviceAI","gesture_help_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIGesture.help_on, 10);

	// AI family
	iniSetInt("deviceAI","family_recognition_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.on, 10);

	iniSetInt("deviceAI","family_face_num", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.facesNum, 10);
    iniSetString("deviceAI","family_face_name_0", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[0].name);
    iniSetString("deviceAI","family_face_url_0", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[0].url);
	iniSetString("deviceAI","family_face_name_1", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[1].name);
    iniSetString("deviceAI","family_face_url_1", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.face_management.face_info[1].url);

	iniSetInt("deviceAI","family_detecttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.on, 10);
    iniSetString("deviceAI","family_detecttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.starttime);
    iniSetString("deviceAI","family_detecttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.endtime);
    iniSetInt("deviceAI","family_detecttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.schedule.repeat, 10);

    iniSetInt("deviceAI","family_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_on, 10);
    iniSetInt("deviceAI","family_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_vol, 10);
    iniSetInt("deviceAI","family_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_repeat, 10);
    iniSetInt("deviceAI","family_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_interval, 10);
    iniSetString("deviceAI","family_speech_url", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.speech_url);
    iniSetInt("deviceAI","family_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_on, 10);
    iniSetInt("deviceAI","family_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_mode, 10);
    iniSetInt("deviceAI","family_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.light_dur, 10);
    iniSetInt("deviceAI","family_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.buz_on, 10);
    iniSetInt("deviceAI","family_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFamilyRecognition.linkage.buz_dur, 10);

	//AI privacy masking
	iniSetInt("deviceAI","privacy_masking_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.on, 10);
	iniSetInt("deviceAI","privacy_masking_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_up_x, 10);
    iniSetInt("deviceAI","privacy_masking_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_up_y, 10);
    iniSetInt("deviceAI","privacy_masking_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_bottom_x, 10);
    iniSetInt("deviceAI","privacy_masking_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_bottom_y, 10);
    iniSetInt("deviceAI","privacy_masking_left_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_bottom_x, 10);
    iniSetInt("deviceAI","privacy_masking_left_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.left_bottom_y, 10);
    iniSetInt("deviceAI","privacy_masking_right_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_up_x, 10);
    iniSetInt("deviceAI","privacy_masking_right_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIPrivacyMasking.zoneInfo.right_up_y, 10);

	//AI fall detection
	iniSetInt("deviceAI","fall_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.on, 10);
	iniSetInt("deviceAI","fall_detection_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.sensitivity, 10);
	iniSetInt("deviceAI","fall_detection_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_x, 10);
	iniSetInt("deviceAI","fall_detection_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_up_y, 10);
	iniSetInt("deviceAI","fall_detection_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_x, 10);
	iniSetInt("deviceAI","fall_detection_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_bottom_y, 10);
	iniSetInt("deviceAI","fall_detection_left_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_x, 10);
	iniSetInt("deviceAI","fall_detection_left_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.left_bottom_y, 10);
	iniSetInt("deviceAI","fall_detection_right_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_x, 10);
	iniSetInt("deviceAI","fall_detection_right_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.zoneInfo.right_up_y, 10);
	iniSetInt("deviceAI","fall_detectiontime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.on, 10);
	iniSetString("deviceAI","fall_detectiontime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.starttime);
    iniSetString("deviceAI","fall_detectiontime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.endtime);
	iniSetInt("deviceAI","fall_detectiontime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.schedule.repeat, 10);
	iniSetInt("deviceAI","fall_detection_speech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_on, 10);
	iniSetInt("deviceAI","fall_detection_speech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_vol, 10);
	iniSetInt("deviceAI","fall_detection_speech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_repeat, 10);
	iniSetInt("deviceAI","fall_detection_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_repeat_constant, 10);
	iniSetInt("deviceAI","fall_detection_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.speech_interval, 10);
	iniSetInt("deviceAI","fall_detection_light_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_on, 10);
	iniSetInt("deviceAI","fall_detection_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_dur_constant, 10);
	iniSetInt("deviceAI","fall_detection_light_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_dur, 10);
	iniSetInt("deviceAI","fall_detection_light_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.light_mode, 10);
	iniSetInt("deviceAI","fall_detection_buz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_on, 10);
	iniSetInt("deviceAI","fall_detection_buz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.AIFallDetection.linkage.buz_dur, 10);

	iniSetInt("deviceAI", "sleep_monitor_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.on,0);
	iniSetInt("deviceAI", "sleep_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.on,0);
	iniSetInt("deviceAI", "sleep_detection_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.sleep_detection.sensitivity,2);
	iniSetInt("deviceAI", "face_cover_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.on,0);
	iniSetInt("deviceAI", "face_cover_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.sensitivity,2);
	iniSetInt("deviceAI", "face_cover_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.face_cover.mode,0);
	iniSetInt("deviceAI", "wakeup_detection_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.on,0);
	iniSetInt("deviceAI", "wakeup_detection_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].aiset.sleepAlarm.wakeup_detection.sensitivity,2);

	/** 枪球联动 */
	iniSetInt(sect, "linkage_tracking_smart_on", OVD_DEMO_ConfigureInfo.linkage_tracking_smart_on, 0);
	iniSetInt(sect, "linkage_tracking_manual_on", OVD_DEMO_ConfigureInfo.linkage_tracking_manual_on, 0);

	char *alarm_sect = "deviceAlarms";
	//外部报警
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

	iniSetInt(alarm_sect, "ioAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.isEffect,10);
	iniSetInt(alarm_sect, "ioAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.on,10);
	iniSetInt(alarm_sect, "ioAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.ioAlarm.sensitivity,10);

	iniSetInt(alarm_sect, "voiceAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.isEffect,10);
	iniSetInt(alarm_sect, "voiceAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.on,10);
	iniSetInt(alarm_sect, "voiceAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.voiceAlarm.sensitivity,10);

	iniSetInt(alarm_sect, "pirAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.isEffect,10);
	iniSetInt(alarm_sect, "pirAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.on,10);
	iniSetInt(alarm_sect, "pirAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.pirAlarm.sensitivity,10);

	iniSetInt(alarm_sect, "lossLockAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.lossLockAlarm.isEffect,10);
	iniSetInt(alarm_sect, "lossLockAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.lossLockAlarm.on,10);

	iniSetInt(alarm_sect, "bodyAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.isEffect,10);
	iniSetInt(alarm_sect, "bodyAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.on,10);
	iniSetInt(alarm_sect, "bodyAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.sensitivity,10);
	iniSetInt(alarm_sect, "bodyAlarm_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_x,10);
	iniSetInt(alarm_sect, "bodyAlarm_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.left_up_y,10);
	iniSetInt(alarm_sect, "bodyAlarm_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_x,10);
	iniSetInt(alarm_sect, "bodyAlarm_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.bodyAlarm.zoneInfo.right_bottom_y,10);

	iniSetInt(alarm_sect, "motionAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.isEffect,10);
	iniSetInt(alarm_sect, "motionAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.on,10);
	iniSetInt(alarm_sect, "motionAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.motionAlarm.sensitivity,10);

	iniSetInt(alarm_sect, "cryAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.isEffect,10);
	iniSetInt(alarm_sect, "cryAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.on,10);
	iniSetInt(alarm_sect, "cryAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.sensitivity,10);
    iniSetInt(alarm_sect, "crying_pacify_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_on,1);
    iniSetInt(alarm_sect, "crying_pacify_audio_playing_count", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_count,10);
    iniSetInt(alarm_sect, "crying_pacify_audio_playing_volumn", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_volumn,60);
    iniSetString(alarm_sect, "crying_pacify_audio_playing_url", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.cryAlarm.crying_pacify_audio_playing_url);

	iniSetInt(alarm_sect, "crossAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.isEffect,10);
	iniSetInt(alarm_sect, "crossAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.on,10);
	iniSetInt(alarm_sect, "crossAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.crossAlarm.sensitivity,10);

	iniSetInt(alarm_sect, "alertareaAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.isEffect,10);
	iniSetInt(alarm_sect, "alertareaAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.on,10);
	iniSetInt(alarm_sect, "alertareaAlarm_expel", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.expel,10);
	iniSetInt(alarm_sect, "alertareaAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.sensitivity,10);
	int zone_size = OVD_DEMO_CapInfoV2.alarm.alertarea_params.alertarea_num;
    //有多少个区域要写入配置文件
    int k_zone = 0;
    for(k_zone = 0;k_zone < zone_size ;k_zone++)
    {
		sprintf(tmp_alertareaAlarm_zone_on, "%s_%d", "alertareaAlarm_zone_on", k_zone);
		iniSetString(alarm_sect, tmp_alertareaAlarm_zone_on, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].on);  // 1.41
		sprintf(tmp_alertareaAlarm_left_up_x, "%s_%d", "alertareaAlarm_left_up_x", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_left_up_x, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_up_x,10);
		sprintf(tmp_alertareaAlarm_left_up_y, "%s_%d", "alertareaAlarm_left_up_y", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_left_up_y, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_up_y,10);
		sprintf(tmp_alertareaAlarm_right_bottom_x, "%s_%d", "alertareaAlarm_right_bottom_x", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_right_bottom_x, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_bottom_x,10);
		sprintf(tmp_alertareaAlarm_right_bottom_y, "%s_%d", "alertareaAlarm_right_bottom_y", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_right_bottom_y, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_bottom_y,10);
		sprintf(tmp_alertareaAlarm_left_bottom_x, "%s_%d", "alertareaAlarm_left_bottom_x", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_left_bottom_x, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_bottom_x,10);
		sprintf(tmp_alertareaAlarm_left_bottom_y, "%s_%d", "alertareaAlarm_left_bottom_y", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_left_bottom_y, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].left_bottom_y,10);
		sprintf(tmp_alertareaAlarm_right_up_x, "%s_%d", "alertareaAlarm_right_up_x", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_right_up_x, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_up_x,10);
		sprintf(tmp_alertareaAlarm_right_up_y, "%s_%d", "alertareaAlarm_right_up_y", k_zone);
		iniSetInt(alarm_sect, tmp_alertareaAlarm_right_up_y, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.zoneInfo[k_zone].right_up_y,10);
	}
	iniSetInt(alarm_sect, "alerttime_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.on,10);
	iniSetString(alarm_sect, "alerttime_start", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.starttime);
	iniSetString(alarm_sect, "alerttime_end", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.endtime);
	iniSetInt(alarm_sect, "alerttime_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.areaschedule.repeat,10);
	iniSetInt(alarm_sect, "alertspeech_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_on,10);
	iniSetInt(alarm_sect, "alertspeech_vol", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_vol,10);
	iniSetInt(alarm_sect, "alertspeech_repeat", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat,10);
	iniSetInt(alarm_sect,"alertspeech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_repeat_constant,10);
	iniSetInt(alarm_sect,"alertspeech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.speech_interval,10);
	iniSetInt(alarm_sect,"alertlight_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur_constant,10);
	iniSetInt(alarm_sect, "alertlight_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_on,10);
	iniSetInt(alarm_sect, "alertlight_mode", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_mode,10);
	iniSetInt(alarm_sect, "alertlight_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.light_dur,10);
	iniSetInt(alarm_sect, "alertbuz_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_on,10);
	iniSetInt(alarm_sect, "alertbuz_dur", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.arealinkage.buz_dur,10);
	iniSetInt(alarm_sect, "alert_targetType",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.targetType,1);
    iniSetInt(alarm_sect, "alert_staymode_time",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.alertareaAlarm.staymode_time,1);
    //越界侦测
    iniSetInt(alarm_sect, "transgression_isEffect",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.isEffect,1);
    iniSetInt(alarm_sect, "transgression_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.on,1);
    iniSetInt(alarm_sect, "transgression_targetType",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.targetType,1);
    iniSetInt(alarm_sect, "transgressiontime_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.on,1);
    iniSetString(alarm_sect,"transgressiontime_start",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.starttime);
    iniSetString(alarm_sect,"transgressiontime_end",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.endtime);
    iniSetInt(alarm_sect, "transgressiontime_repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.areaschedule.repeat,127);
	// 第一条规则线
	iniSetInt(alarm_sect, "statistics_line_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.on,1);
	iniSetInt(alarm_sect, "statistics_line_A_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.on,1);
    iniSetInt(alarm_sect, "statistics_line_A_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_A_y,1);
    iniSetInt(alarm_sect, "statistics_line_B_x",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_x,100);
    iniSetInt(alarm_sect, "statistics_line_B_y",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_line_B_y,100);
    iniSetInt(alarm_sect, "transgression_statistics_mode_style",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_style,1);
    iniSetInt(alarm_sect, "transgression_statistics_mode_direction",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo.statistics_mode_direction,1);
	// 额外的规则线
	int line_another_size = OVD_DEMO_CapInfoV2.alarm.transgression_params.statistics_line_another_num;
    int k_line = 0;
    for(k_line = 0;k_line < line_another_size ;k_line++)
    {
		sprintf(tmp_statistics_line_on, "%s_%d", "statistics_line_on", k_line);
		iniSetInt(alarm_sect, tmp_statistics_line_on,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].on,1);
		sprintf(tmp_statistics_line_A_x, "%s_%d", "statistics_line_A_x", k_line);
		iniSetInt(alarm_sect, tmp_statistics_line_A_x,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_A_x,1);
		sprintf(tmp_statistics_line_A_y, "%s_%d", "statistics_line_A_y", k_line);
    	iniSetInt(alarm_sect, tmp_statistics_line_A_y,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_A_y,1);
		sprintf(tmp_statistics_line_B_x, "%s_%d", "statistics_line_B_x", k_line);
    	iniSetInt(alarm_sect, tmp_statistics_line_B_x,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_B_x,100);
		sprintf(tmp_statistics_line_B_y, "%s_%d", "statistics_line_B_y", k_line);
    	iniSetInt(alarm_sect, tmp_statistics_line_B_y,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_line_B_y,100);
		sprintf(tmp_transgression_statistics_mode_style, "%s_%d", "transgression_statistics_mode_style", k_line);
    	iniSetInt(alarm_sect, tmp_transgression_statistics_mode_style,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_mode_style,1);
		sprintf(tmp_transgression_statistics_mode_direction, "%s_%d", "transgression_statistics_mode_direction", k_line);
    	iniSetInt(alarm_sect, tmp_transgression_statistics_mode_direction,OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.LineInfo_another[k_line].statistics_mode_direction,1);
	}
    iniSetInt(alarm_sect, "transgression_speech_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_on,1);
    iniSetInt(alarm_sect, "transgression_speech_vol",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_vol,1);
    iniSetInt(alarm_sect, "transgression_speech_repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat,1);
	iniSetInt(alarm_sect, "transgression_speech_repeat_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_repeat_constant,1);
	iniSetInt(alarm_sect, "transgression_speech_interval", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.speech_interval,1);
	iniSetInt(alarm_sect, "transgression_light_dur_constant", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur_constant,1);
    iniSetInt(alarm_sect, "transgression_light_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_on,1);
    iniSetInt(alarm_sect, "transgression_light_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_mode,1);
    iniSetInt(alarm_sect, "transgression_light_dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.light_dur,1);
    iniSetInt(alarm_sect, "transgression_buz_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_on,1);
    iniSetInt(alarm_sect, "transgression_buz_dur",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.transgressionAlarm.arealinkage.buz_dur,1);

	iniSetInt(alarm_sect, "wifisensing_isEffect",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.isEffect,1);
    iniSetInt(alarm_sect, "wifisensing_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.on,1);
	iniSetInt(alarm_sect, "wifisensing_linkage_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.linkage_on,1);
	iniSetInt(alarm_sect, "wifisensing_sensing_mode",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.sensing_mode,1);
    iniSetInt(alarm_sect, "wifisensingtime_on",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.on,1);
    iniSetString(alarm_sect,"wifisensingtime_start",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.starttime);
    iniSetString(alarm_sect,"wifisensingtime_end",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.endtime);
    iniSetInt(alarm_sect, "wifisensingtime_repeat",OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.areaschedule.repeat,127);
	int entire_dev_num = OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_dev_num;
	iniSetInt(alarm_sect, "wifisensing_entire_dev_num", entire_dev_num, 127);
	int k_dev = 0;
	char entire_dev_name[64] = {0};
	char entire_dev_on[64] = {0};
	for(k_dev = 0; k_dev < entire_dev_num; k_dev++)
	{
		sprintf(entire_dev_name, "wifisensing_entire_dev_name_%d", k_dev);
		iniSetString(alarm_sect, entire_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[k_dev].name);
		sprintf(entire_dev_name, "wifisensing_entire_dev_option60_%d", k_dev);
		iniSetString(alarm_sect, entire_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[k_dev].option60);
		sprintf(entire_dev_name, "wifisensing_entire_dev_ip_%d", k_dev);
		iniSetString(alarm_sect, entire_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[k_dev].ip);
		sprintf(entire_dev_name, "wifisensing_entire_dev_mac_%d", k_dev);
		iniSetString(alarm_sect, entire_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[k_dev].mac);
		sprintf(entire_dev_on, "wifisensing_entire_dev_on_%d", k_dev);
		iniSetInt(alarm_sect, entire_dev_on, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.entire_devmanagement[k_dev].on, 127);
	}
	int added_dev_num = OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_dev_num;
	iniSetInt(alarm_sect, "wifisensing_added_dev_num", added_dev_num, 127);
	char added_dev_name[64] = {0};
	for(k_dev = 0; k_dev < added_dev_num; k_dev++)
	{
		sprintf(added_dev_name, "wifisensing_added_dev_name_%d", k_dev);
		iniSetString(alarm_sect, added_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[k_dev].name);
		sprintf(added_dev_name, "wifisensing_added_dev_option60_%d", k_dev);
		iniSetString(alarm_sect, added_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[k_dev].option60);
		sprintf(added_dev_name, "wifisensing_added_dev_ip_%d", k_dev);
		iniSetString(alarm_sect, added_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[k_dev].ip);
		sprintf(added_dev_name, "wifisensing_added_dev_mac_%d", k_dev);
		iniSetString(alarm_sect, added_dev_name, OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.wifisensingAlarm.added_devmanagement[k_dev].mac);
	}

	iniSetInt(alarm_sect, "3d_light_sensor_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.on,10);
	iniSetInt(alarm_sect, "3d_light_sensor_falldown_detect_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.on,10);
	iniSetInt(alarm_sect, "3d_light_sensor_falldown_detect_time", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.detect.time, 10);
	iniSetInt(alarm_sect, "3d_light_sensor_wc_trapped_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.on,10);
	iniSetInt(alarm_sect, "3d_light_sensor_wc_trapped_time", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.falldown_3dAlarm.trapped.time, 10);

	iniSetInt(alarm_sect, "petAlarm_isEffect", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.isEffect,10);
	iniSetInt(alarm_sect, "petAlarm_on", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.on,10);
	iniSetInt(alarm_sect, "petAlarm_sensitivity", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.sensitivity,10);
	iniSetInt(alarm_sect, "petAlarm_left_up_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_x,10);
	iniSetInt(alarm_sect, "petAlarm_left_up_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.left_up_y,10);
	iniSetInt(alarm_sect, "petAlarm_right_bottom_x", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_x,10);
	iniSetInt(alarm_sect, "petAlarm_right_bottom_y", OVD_DEMO_ConfigureInfo.channelsInfo[i].alarms.petAlarm.zoneInfo.right_bottom_y,10);

	iniFileFree();
    return 0;
}

