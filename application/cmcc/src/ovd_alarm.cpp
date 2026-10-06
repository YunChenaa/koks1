#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#include "koks_project_def.h"
#include "st_koks_uvc.h"
#include "soft_codec.h"
#include "sys_logger.h"
#include "OVD_define.h"
#include "OVD_OpenAPI.h"
#include "ovd.h"
#include "wifi.h"

#define ALARMS_INFO_LENGTH 128
#define ALARMS_INFO_SIZE 10

static volatile int g_alarm_type_mask = 0x0;      // 报警类型掩码
static volatile int g_thread_exit = 0;     // 线程退出标志


static int ParallelAlarmNum[ALARMS_INFO_LENGTH] = {0};
static OVDUpLoadAlarmInfo *ParallelAlarmInfo[ALARMS_INFO_LENGTH][ALARMS_INFO_SIZE];
long long global_timeStamp;

static void print_local_time(time_t ts) {
    struct tm tm_buf;
    localtime_r(&ts, &tm_buf);
    char out[64];
    strftime(out, sizeof(out), "%F %T", &tm_buf);
    LOG_INFO("DEV/OVD local time: %s\n", out);
}


int GetSnapshot(uint8_t** out_buffer, int out_width, int out_height, bool (*get_image_data_func)(uint8_t** imagedata, int &width, int &height))
{
    if(get_image_data_func == nullptr) {
        LOG_ERROR("get_image_data_func is not registered\n");
        return -1;
    }

    uint8_t* imagedata = nullptr;
    int width = 0, height = 0;
    if(get_image_data_func(&imagedata, width, height)){
        auto& videomanager = MediaUvc::GetInstance();

        size_t dst_nv12_size = (size_t)out_width * out_height * 3 / 2;
        uint8_t* dst_nv12 = (uint8_t*)malloc(dst_nv12_size);
        if (!dst_nv12) {
            LOG_ERROR("malloc for dst_nv12 failed\n");
            return -1;
        }

        int diff = width - out_width;
        if (diff < 0 || (diff & 1)) {
            LOG_ERROR("Width diff %d must be non-negative even\n", diff);
            return -1;
        }
        int crop_size = diff / 2;
        //LOG_INFO("%s(%d)\n", __func__, __LINE__);
        if (videomanager.MediaCorpNV12Width(imagedata, width, height, crop_size, crop_size, dst_nv12, out_width, out_height) != 0) {
            LOG_ERROR("nv12 crop failed\n");
            free(dst_nv12);
            return -1;
        }

        //LOG_INFO("%s(%d)\n", __func__, __LINE__);
        unsigned char* rgb_buffer = nullptr;
        if(st::sense_koks::SoftCodec::ConvertNV12ToRGB(dst_nv12, &rgb_buffer, out_width, out_height) != 0) {
            LOG_ERROR("Convert NV12 to RGB failed\n");
            free(dst_nv12);
            return -1;
        }
        free(dst_nv12);

        //LOG_INFO("%s(%d)\n", __func__, __LINE__);
        unsigned char* jpeg_buffer = nullptr;
        unsigned long jpeg_buf_size = 0;
        if(st::sense_koks::SoftCodec::EncodeRGBToJPEG(rgb_buffer, out_width, out_height, &jpeg_buffer, &jpeg_buf_size, 70) != 0) {
            LOG_ERROR("Encode RGB to JPEG failed\n");
            free(rgb_buffer);
            return -1;
        }
        free(rgb_buffer);

        //LOG_INFO("%s(%d)\n", __func__, __LINE__);
        if (jpeg_buf_size > 0) {
            *out_buffer = (uint8_t*)malloc(jpeg_buf_size);
            if (*out_buffer == nullptr) {
                LOG_ERROR("Failed to allocate memory for out_buffer\n");
                free(jpeg_buffer);
                return -1;
            }
            //LOG_INFO("%s(%d)\n", __func__, __LINE__);
            memcpy(*out_buffer, jpeg_buffer, jpeg_buf_size);
            free(jpeg_buffer);
            return jpeg_buf_size;
        } else {
            LOG_ERROR("Encode NV12 to MJPEG return fail\n");
            free(jpeg_buffer);
            return -1;
        }
    }else{
        LOG_ERROR("get_image_data_func return fail\n");
        return -1;
    }
}

static int OVD_do_alarm_report(int alarm_type)
{
    LOG_INFO("DEV/OVD OVD_do_alarm_report alarm_type = %d\n", alarm_type);
    OVDUpLoadAlarmInfo *uploadAlarmInfo = (OVDUpLoadAlarmInfo *)malloc(sizeof(OVDUpLoadAlarmInfo));
    if(NULL == uploadAlarmInfo){
        LOG_ERROR("DEV/OVD malloc uploadAlarmInfo fail\n");
        return -1;
    }
    memset(uploadAlarmInfo, 0, sizeof(OVDUpLoadAlarmInfo));
    uploadAlarmInfo->channel = 0;

    time_t localTimeStamp;
    localTimeStamp = time(NULL);
	uploadAlarmInfo->startTimeStamp = localTimeStamp;
    //print_local_time(uploadAlarmInfo->startTimeStamp);

    if(alarm_type == CMD_EVENT_BELL)
        uploadAlarmInfo->AlarmType = OVD_BELL;
    else if(alarm_type == CMD_EVENT_STAY)
        uploadAlarmInfo->AlarmType = OVD_PIR;
    else if(alarm_type == CMD_EVENT_TAMPER)
        uploadAlarmInfo->AlarmType = OVD_LOSS_LOCK;

    uploadAlarmInfo->desc = (char*)malloc(ALARMS_INFO_LENGTH);
    if(NULL != uploadAlarmInfo->desc){
        sprintf(uploadAlarmInfo->desc, "An alarm has been triggered, with the type being %d", (int)uploadAlarmInfo->AlarmType);
    }

    int data_size = 0;
    uint8_t* buffer = nullptr;

    data_size = GetSnapshot(&buffer, 360, 640, callbackList.get_image_data_func);
    if(data_size > 0){
        uploadAlarmInfo->ImageInfo.buf = (OVD_char *)malloc(data_size);
        if (uploadAlarmInfo->ImageInfo.buf) {
            uploadAlarmInfo->ImageInfo.size = data_size;
            memcpy(uploadAlarmInfo->ImageInfo.buf, buffer, data_size);
        }
        free(buffer);
    }else{
        LOG_ERROR("DEV/OVD %s %d: [demo] GetSnapshot return %d\n",__func__, __LINE__, data_size);
        free(uploadAlarmInfo->desc);
        return -1;
    }

    uploadAlarmInfo->extension = (char*)malloc(sizeof(char)*128);
    if(uploadAlarmInfo->extension == NULL)
    {
        LOG_ERROR("DEV/OVD uploadAlarmInfo->AlarmType...%d\n", uploadAlarmInfo->AlarmType);
        return -1;
    }
    sprintf(uploadAlarmInfo->extension, "{\"tags\":[{\"tag\":44, \"rate\":1}]}");

    int result = OVD_AlarmInfoStart(uploadAlarmInfo);

    LOG_INFO("DEV/OVD Trigger alarm result: %d\n",result);
    if(result == 0)
    {
        LOG_INFO("DEV/OVD OVD_do_alarm_report\n");
        ParallelAlarmInfo[0][ParallelAlarmNum[0]] = uploadAlarmInfo;
        ParallelAlarmNum[0]++;
    }

    return 0;
}

static int OVD_do_alarm_end(int alarm_type)
{
    OVDUpLoadAlarmInfo *uploadAlarmInfo = NULL;
    if(ParallelAlarmNum[0] <= 0){
        LOG_ERROR("DEV/OVD all alarm is over\n");
        return -1;
    }
    ParallelAlarmNum[0]--;
    uploadAlarmInfo = ParallelAlarmInfo[0][ParallelAlarmNum[0]];
    if(NULL == uploadAlarmInfo){
        LOG_ERROR("DEV/OVD alarm info no exist\n");
        return -1;
    }

    time_t localTimeStamp;
    localTimeStamp = time(NULL);
	global_timeStamp = localTimeStamp;

    if(alarm_type == CMD_EVENT_BELL)
        uploadAlarmInfo->AlarmType = OVD_BELL;
    else if(alarm_type == CMD_EVENT_STAY)
        uploadAlarmInfo->AlarmType = OVD_PIR;
    else if(alarm_type == CMD_EVENT_TAMPER)
        uploadAlarmInfo->AlarmType = OVD_LOSS_LOCK;

    int result = OVD_AlarmInfoEnd(0, uploadAlarmInfo->AlarmType, global_timeStamp);

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
    ParallelAlarmInfo[0][ParallelAlarmNum[0]] = NULL;

    return 0;
}

extern int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);
extern void set_host_keepalive(void);
// 线程函数：轮询检查全局标志
static void* alarm_worker(void* arg) {
    // 分离自身，线程结束时自动回收资源
    pthread_detach(pthread_self());

    while (!g_thread_exit) {
        if (g_alarm_type_mask > 0) {
            if(g_alarm_type_mask & (1 << CMD_EVENT_BELL)) {
                g_alarm_type_mask &= ~(1 << CMD_EVENT_BELL);
                OVD_do_alarm_report(CMD_EVENT_BELL);
                set_host_keepalive();

                OVD_schedule_hibernate(0, 0, 30, OVD_HIBERNATE_OTHER);
            }
            if(g_alarm_type_mask & (1 << CMD_EVENT_TAMPER)) {
                g_alarm_type_mask &= ~(1 << CMD_EVENT_TAMPER);
                OVD_do_alarm_report(CMD_EVENT_TAMPER);
                set_host_keepalive();

                OVD_schedule_hibernate(0, 0, 30, OVD_HIBERNATE_OTHER);
            }
            if(g_alarm_type_mask & (1 << CMD_EVENT_STAY)) {
                g_alarm_type_mask &= ~(1 << CMD_EVENT_STAY);
                OVD_do_alarm_report(CMD_EVENT_STAY);
                set_host_keepalive();
                OVD_schedule_hibernate(0, 0, 30, OVD_HIBERNATE_OTHER);
                sleep(20);
                OVD_do_alarm_end(CMD_EVENT_STAY);
                set_host_keepalive();
                OVD_schedule_hibernate(0, 0, 30, OVD_HIBERNATE_OTHER);
            }
        }
        usleep(100000);
    }
    return NULL;
}

int start_alarm_monitor(void) {
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, alarm_worker, NULL);
    if (ret != 0) {
        perror("pthread_create");
        return -1;
    }
    return 0;
}

void stop_alarm_monitor(void) {
    g_thread_exit = 1;
    usleep(200000);
}

int trigger_alarm(int alarm_type) {
    g_alarm_type_mask |= (1 << alarm_type);
    return 0;
}
