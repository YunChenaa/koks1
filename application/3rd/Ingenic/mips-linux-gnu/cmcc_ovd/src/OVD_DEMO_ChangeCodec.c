#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/prctl.h>

#include "OVD_DEMO_ChangeCodec.h"
#include "OVD_Demo.h"
#include "OVD_DemoCmd.h"
#include "demoAVPushing.h"
#include "demoAVPushing_util.h"

extern OVDConfigrationInfo OVD_DEMO_ConfigureInfo;
extern char videoFILEPATH[256];
extern char h264filepath[256];
extern char h265filepath[256];
extern char AOVfilepath[256];


//模拟设备 视频格式在 h264 和 h265之间切换
unsigned char do_cmd_ChangeCodec(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===设备收到指令 h264与h265 之间切换\n");

    //停止推流；
    cmd_exec(PUSH_END);

    //变更新的推流格式参数

    printf("h264filepath: %s\n",h264filepath);
    printf("h265filepath: %s\n",h265filepath);
    //如果当前是source_STREAM_CODEC_H264
    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec == source_STREAM_CODEC_H264)
    {
        printf("==当前是264，变更为265===\n");
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H265;
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        strncpy(videoFILEPATH,h265filepath,sizeof(videoFILEPATH));
    }
    else
    {
        printf("===当前非h264,变更为h264===\n");
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H264;
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        strncpy(videoFILEPATH,h264filepath,sizeof(videoFILEPATH));

    }
    cmd_exec(AV_READY);

    //重新推流
    cmd_exec(PUSH_AV);
    return 0;
}


extern int parralelPushNum;
extern void* PushAVDataHandler[128][MAX_STREAM_COUNT];

int demo_callback_changecodec2(void)
{
    return cmd_exec(CHANGECODEC2);
}

//模拟设备 视频格式在 h264 和 h265之间切换
//方式二：厂商调用OVD_AVParamModify 接口，而不调用OVD_AVPushEnd 和OVD_AVPushStart
unsigned char do_cmd_ChangeCodec2(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====>另外一种方式切换视频码流格式h264，h265====\n\n");
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","===demo上层显示调用push_END和push_av,直接切换，而openapi来做这个动作");

    //变更新的推流格式参数
    OVD_Demo_stop_push(0);

    printf("h264filepath: %s\n",h264filepath);
    printf("h265filepath: %s\n",h265filepath);
    //如果当前是source_STREAM_CODEC_H264
    if(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec == source_STREAM_CODEC_H264)
    {
        printf("==当前是264，变更为265===\n");
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H265;
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        strncpy(videoFILEPATH,h265filepath,sizeof(videoFILEPATH));
    }
    else
    {
        printf("===当前非h264,变更为h264===\n");
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H264;
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        strncpy(videoFILEPATH,h264filepath,sizeof(videoFILEPATH));

    }
    //修改音视频格式
    OVD_AVParamModify(OVD_DEMO_ConfigureInfo.channelsInfo[0].channel, OVD_HIGH_STREAM, &(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo), &(OVD_DEMO_ConfigureInfo.channelsInfo[0].audioinfo));
    OVD_AVParamModify(OVD_DEMO_ConfigureInfo.channelsInfo[0].channel, OVD_LOW_STREAM, &(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo), &(OVD_DEMO_ConfigureInfo.channelsInfo[0].audioinfo));

    OVD_Demo_start_push(0);
    return 0;
}


unsigned char do_cmd_ChangeAOV(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====>普通码流和AOV码流切换====\n\n");

    //变更新的推流格式参数
    OVD_Demo_stop_push(0);

    printf("videoFILEPATH: %s\n",videoFILEPATH);
    printf("AOVfilepath: %s\n",AOVfilepath);

    if(strcmp(videoFILEPATH, AOVfilepath) == 0)
    {
        printf("==当前是AOV，切换成H265普通码流===\n");
        memset(videoFILEPATH,0,sizeof(videoFILEPATH));
        strncpy(videoFILEPATH,h265filepath,sizeof(videoFILEPATH));
        memset(audioFILEPATH,0,sizeof(audioFILEPATH));
        strncpy(audioFILEPATH,"./configure/resources/audio.aac",sizeof(audioFILEPATH));
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate = 15;
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate_aov = 15;
        OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.reserve = 0;
    }
    else
    {
        //如果当前是source_STREAM_CODEC_H264
        if(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec == source_STREAM_CODEC_H264)
        {
            printf("==当前是264，变更为AOV===\n");
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H265;
            memset(videoFILEPATH,0,sizeof(videoFILEPATH));
            strncpy(videoFILEPATH,AOVfilepath,sizeof(videoFILEPATH));
            memset(audioFILEPATH,0,sizeof(audioFILEPATH));
            strncpy(audioFILEPATH,"./configure/resources/audio.aac",sizeof(audioFILEPATH));
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate = 15;
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate_aov = 15;
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.reserve = 1;
        }
        else
        {
            printf("===当前是普通h265,变更为AOV===\n");
            //OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.codec = source_STREAM_CODEC_H265;
            memset(videoFILEPATH,0,sizeof(videoFILEPATH));
            strncpy(videoFILEPATH,AOVfilepath,sizeof(videoFILEPATH));
            memset(audioFILEPATH,0,sizeof(audioFILEPATH));
            strncpy(audioFILEPATH,"./configure/resources/audio.aac",sizeof(audioFILEPATH));
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate = 15;
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.framerate_aov = 15;
            OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo.reserve = 1;

        }
    }

    //修改音视频格式
    OVD_AVParamModify(OVD_DEMO_ConfigureInfo.channelsInfo[0].channel, OVD_HIGH_STREAM, &(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo), &(OVD_DEMO_ConfigureInfo.channelsInfo[0].audioinfo));
    OVD_AVParamModify(OVD_DEMO_ConfigureInfo.channelsInfo[0].channel, OVD_LOW_STREAM, &(OVD_DEMO_ConfigureInfo.channelsInfo[0].videoinfo), &(OVD_DEMO_ConfigureInfo.channelsInfo[0].audioinfo));

    OVD_Demo_start_push(0);
    return 0;
}


static FILE *aov_hdl = NULL;
unsigned char *g_aov_buffer = NULL;
unsigned char *g_aov_frame = NULL;
CONFIG_DATA *g_aov_config_data = NULL;
OVD_AOV_Info g_aov_info = {0};
static OVD_int64 demo_aov_start_time = 0;
static OVD_int64 demo_aov_end_time = 0;

#define MAX_FRAME_BUFFER_LEN 1024*1024

OVD_void * AOV_open(IN OVD_int32 channel)
{
    if (channel != 0)
    {
        printf("暂不支持非0通道设备\n");
        return NULL;
    }

    /*需要打开某一个文件，然后读取数据，数据保存在全局变量中*/
    printf("aov filename:%s\n", AOVfilepath);
    FILE *aov_hdl = fopen(AOVfilepath, "rb");
    if (aov_hdl == NULL)
    {
        printf("open aov file failed\n");
        return NULL;
    }

    fseek(aov_hdl, 0, SEEK_END);
    int size = ftell(aov_hdl);
    rewind(aov_hdl);
    printf("aov size: %d\n", size);
    int malloc_size = size + 1024;
    g_aov_buffer = (unsigned char *)malloc(malloc_size);
    if (g_aov_buffer == NULL)
    {
        printf("malloc aov buffer failed\n");
        return NULL;
    }

    g_aov_frame = (unsigned char *)malloc(MAX_FRAME_BUFFER_LEN);
    if (g_aov_frame == NULL)
    {
        printf("malloc aov buffer failed\n");
        return NULL;
    }

    g_aov_config_data = (CONFIG_DATA*)malloc(sizeof(CONFIG_DATA));
    if (g_aov_config_data == NULL)
    {
        printf("malloc g_aov_config_data failed\n");
        return NULL;
    }
    g_aov_config_data->last_addr = 0;
    g_aov_config_data->step = 0;

    g_aov_info.size = fread(g_aov_buffer, 1, malloc_size, aov_hdl);
    printf("read buffer size: %d\n", g_aov_info.size);

    if (size != g_aov_info.size)
    {
        printf("read buffer size: %d, file size: %d \n", g_aov_info.size, size);
    }

    g_aov_info.durataion = (demo_aov_end_time - demo_aov_start_time)*1000;
    g_aov_info.size = g_aov_info.size;
    g_aov_info.frame_num = 755;
    g_aov_info.start_ts = demo_aov_start_time*1000;

    return (void *)aov_hdl;

exit:
    if (g_aov_buffer)
    {
        free(g_aov_buffer);
        g_aov_buffer = NULL;
    }
    if (g_aov_frame != NULL)
    {
        free(g_aov_frame);
        g_aov_frame = NULL;
    }
    if (g_aov_config_data != NULL)
    {
        free(g_aov_config_data);
        g_aov_config_data = NULL;
    }
    return NULL;
}


OVD_int32 AOV_get_info(IN OVD_void *ctx, OVD_AOV_Info *info)
{
    info->durataion = g_aov_info.durataion;
    info->size =  g_aov_info.size;
    info->frame_num = g_aov_info.frame_num;
    info->start_ts =  g_aov_info.start_ts;
    return 0;
}


OVD_int32 AOV_readframe(IN OVD_void *ctx, IN OVD_FrameInfo *pframe_info)
{
    long long inter_video = 40;
    int rect = 0;
    int v_size = MAX_FRAME_BUFFER_LEN;

    if (g_aov_info.start_ts >= demo_aov_end_time*1000)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "达到AOV码流结束时间");
        return OVD_RET_READ_FRAME_EOF;
    }


    rect = getH265Frame(g_aov_buffer, g_aov_info.size, g_aov_frame, &v_size, g_aov_config_data);
    if (rect)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "该视频没有有效帧");
        return -1;
    }
    pframe_info->frame_type = OVD_AOV;
    pframe_info->codec_type = source_STREAM_CODEC_H265;
    pframe_info->timestamp = g_aov_info.start_ts;
    g_aov_info.start_ts += inter_video;
    pframe_info->relative_timestamp = g_aov_info.start_ts;
    if (g_aov_config_data->cType == I_Frame)
    {
        pframe_info->video_info.iskey_frame = 1;
    }
    else
    {
        pframe_info->video_info.iskey_frame = 0;
    }


    pframe_info->frame_buf = g_aov_frame;
    pframe_info->framebuf_len = v_size;


    return 0;
}


OVD_int32 AOV_set_status(IN OVD_void *ctx, OVD_aov_read_status_e status)
{
    printf("AOV_set_status: %d\n", status);

    return 0;
}


OVD_int32 AOV_close(IN OVD_void *ctx)
{
    if (ctx != NULL)
    {
        fclose(ctx);
        aov_hdl = NULL;
    }
    if (g_aov_buffer != NULL)
    {
        free(g_aov_buffer);
        g_aov_buffer = NULL;
    }
    if (g_aov_frame != NULL)
    {
        free(g_aov_frame);
        g_aov_frame = NULL;
    }
    if (g_aov_config_data != NULL)
    {
        free(g_aov_config_data);
        g_aov_config_data = NULL;
    }
    demo_aov_start_time = 0;
    demo_aov_end_time = 0;
    return 0;
}


unsigned char do_cmd_start_lowpower_aov(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====>切换低功耗AOV码流====\n\n");
    if (argc != 3)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO","参数个数不对，请检查，当前参数个数：%d\n", argc);
        return 0;
    }

    demo_aov_start_time = atoi(argv[1]);
    demo_aov_end_time = atoi(argv[2]);

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","切换AOV码流开始时间：%d，结束时间：%d\n",demo_aov_start_time,demo_aov_end_time);

    OVD_AOV_CallBackList aov_cb = {0};
    aov_cb.OVD_callback_AOV_open = AOV_open;
    aov_cb.OVD_callback_AOV_close = AOV_close;
    aov_cb.OVD_callback_AOV_get_info = AOV_get_info;
    aov_cb.OVD_callback_AOV_readframe = AOV_readframe;
    aov_cb.OVD_callback_AOV_set_status = AOV_set_status;

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","OVD_callback_AOV_close: %p\n", aov_cb.OVD_callback_AOV_close);
    OVD_start_push_AOV(0, &aov_cb);
    return 0;
}


unsigned char do_cmd_stop_lowpower_aov(int argc, char *argv[])
{
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","=====>停止低功耗AOV====\n\n");
    OVD_stop_push_AOV(0);
    return 0;
}