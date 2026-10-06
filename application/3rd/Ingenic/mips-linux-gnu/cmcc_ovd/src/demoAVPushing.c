#include "OVD_Demo.h"
#include "demoAVPushing_util.h"
#include <sys/prctl.h>

#include <string.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>


#define INFO_BUFF_SIZE	1024*600
#define STREAM_BUF_SIZE 0x100000

static OVD_bool debug_stop_video = OVD_FALSE;         //用于调试视频的异常事件上报（默认调试关闭）
static OVD_bool debug_stop_audio = OVD_FALSE;         //用于调试音频的异常事件上报（默认调试关闭）
ovd_get_fr_cb get_fr_cb;

uint8_t* find_next_nalu(uint8_t *data, int len, int *offset) {
    int i = *offset;
    while (i < len - 3) {
        if (data[i] == 0x00 && data[i+1] == 0x00) {
            if (data[i+2] == 0x01) {
                *offset = i + 3;
                return data + *offset;
            }
            if (i+3 < len && data[i+2] == 0x00 && data[i+3] == 0x01) {
                *offset = i + 4;
                return data + *offset;
            }
        }
        i++;
    }
    return NULL;
}

int find_IDR(unsigned   char *stream, int stream_len) {
    int offset = 0;
    uint8_t *nalu;
    
    while ((nalu = find_next_nalu(stream, stream_len, &offset)) != NULL) {
        int type = nalu[0] & 0x1F;
        switch(type) {
        case 5:
            return offset;
        case 7:  // SPS
        case 8:  // PPS
        case 1:  // not IDR;
        case 6:  // SEI
        default: // other
            break;
        }
    }
    return 0;
}


// 以下4个接口用于调试音视频异常事件上报，默认不调用
void lcd_debug_stop_video()
{
    debug_stop_video = OVD_TRUE;
    return;
}

void lcd_debug_start_video()
{
    debug_stop_video = OVD_FALSE;
    return;
}

void lcd_debug_stop_audio()
{
    debug_stop_audio = OVD_TRUE;
    return;
}

void lcd_debug_start_audio()
{
    debug_stop_audio = OVD_FALSE;
    return;
}

// 点云码流推送设置
static OVD_bool debug_start_push_pointcloud = OVD_FALSE;  

void start_push_pointcloud()
{
    debug_start_push_pointcloud = OVD_TRUE;
}

void stop_push_pointcloud()
{
    debug_start_push_pointcloud = OVD_TRUE;
}


void  free_p(void* p)
{
    if(p != NULL)
    {
        free(p);
    }
}

long long OVD_DEMO_getMonotonicMicroSec()
{
    struct timespec ts = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret != 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "clock_gettime fail:%d %d.\n", ret, errno);
        return 0;
    }

    long long usec = (long long)ts.tv_sec * 1000000 + ts.tv_nsec/1000;
    return usec;
}

long long  OVD_DEMO_getutcMicroSec()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long long utcms=tv.tv_sec*1000000+tv.tv_usec;
	return utcms;
}

static void * AVPushing(void* arg)
{
#ifdef OVD_BUILDIN
    AVPushContext *cpush = (AVPushContext *)(arg);
    OVD_bool video = cpush->sendtype == OVD_Video;
    int len, sz = video ? 200 * 1024 : 20 * 1024;
    unsigned char* buf = (unsigned char*)malloc(sz);
    while (1) {
        len = get_fr_cb(video, buf, sz);
        if (len > 0) {
            OVD_bool IFr = video && find_IDR(buf, 128) != 0;
            int r = OVD_AVPushData(cpush->channel, cpush->sendtype, cpush->contentType, IFr, buf, len, OVD_DEMO_getutcMicroSec());
            if (r){
                OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/OVD","pushing data error %d, channel %d, sendtype %d, contenttype %d\n", r, cpush->channel, cpush->sendtype, cpush->contentType);
            }
        }
        usleep(1000);
    }
    free(buf);
#else
	OVD_bool send_audio = OVD_FALSE;
	OVD_bool send_video = OVD_FALSE;
	OVD_bool send_pointcloud = OVD_FALSE;

	AVPushContext *cpush = (AVPushContext *)(arg);
	prctl(PR_SET_NAME, "demoavpush");

	/*****************音频文件打开**********************/
	int data_size = 0;
    int size      = 0;
    int cnt       = 0;
    int offset    = 0;
	int tmp = 0;
	unsigned char* aacframe  = NULL;
    unsigned char* aacbuffer = NULL;
	unsigned char* input_data = NULL;
	if (cpush->audeo_filename == NULL || strlen(cpush->audeo_filename) == 0)
	{
		OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "没有音频文件\n");
	}
	else
	{
		send_audio = OVD_TRUE;
		/****获取音频文件大小***/
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "audio filename:%s\n", cpush->audeo_filename);
		FILE * audio_fp = fopen(cpush->audeo_filename, "rb");
		if (audio_fp != NULL)
		{

			fseek(audio_fp, 0L, SEEK_END);
			long int nfilesize = ftell(audio_fp);
			rewind(audio_fp);
			OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "audio filesize: %ld\n", nfilesize);
			int malloc_size = (ceil(nfilesize/1024)+1)*1024;
			aacbuffer = (unsigned char *)malloc(malloc_size);
			aacframe = (unsigned char *)malloc(1024*50);
			if (aacbuffer== NULL)
			{
				OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "申请音频内存失败");
				return NULL;
			}


			if (aacframe== NULL)
			{
				OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "申请音频帧失败");
				return NULL;
			}

			/**********************/

			char *new_fp = (char *)malloc(strlen(cpush->audeo_filename) + 1);
			strcpy(new_fp, cpush->audeo_filename);

			cpush->aacframe  = aacframe;
			cpush->aacbuffer = aacbuffer;

			FILE* ifile = fopen(new_fp, "rb");
			if (ifile != NULL)
			{
				data_size = fread(aacbuffer+offset, 1, malloc_size-offset, (FILE*)ifile);
				fclose(ifile);
			}

			free(new_fp);
			tmp = data_size;
			input_data = aacbuffer;
		}
	}


	/**************************************************/

	/*****************视频文件打开********************/
	int v_data_size = 0;
    int v_size      = 0;
    int v_cnt       = 0;
	int v_tmp = 0;
	unsigned char* vframe  = NULL;
	unsigned char* vbuffer = NULL;

	if (cpush->video_filename == NULL || strlen(cpush->video_filename) == 0)
	{
		OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "没有视频文件\n");
	}
	else
	{
		send_video = OVD_TRUE;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "video filename:%s\n", cpush->video_filename);
		FILE* video_fp = fopen(cpush->video_filename, "rb");
        if (video_fp == NULL)
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "open [%s] failed\n", cpush->video_filename);
            exit(1);
        }
		fseek(video_fp, 0L, SEEK_END);
		long int nfilesize = ftell(video_fp);
		rewind(video_fp);
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "video filesize: %ld\n", nfilesize);

		int malloc_size = (ceil(nfilesize/1024)+1)*1024;

		vbuffer = (unsigned char *)malloc(malloc_size);
		if (vbuffer== NULL)
		{
			OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "申请视频内存失败");
			return NULL;
		}

		vframe = (unsigned char *)malloc(1024*500);
		if (vframe== NULL)
		{
			OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "申请帧内存失败");
			return NULL;
		}


		char *new_fp = (char*)malloc(strlen(cpush->video_filename) + 1);
		strcpy(new_fp, cpush->video_filename);

		cpush->vframe  = vframe;
		cpush->vbuffer = vbuffer;

		FILE* ifile = fopen(new_fp, "rb");
		if (ifile != NULL)
		{
			v_data_size = fread(vbuffer, 1, malloc_size, (FILE*)ifile);
			fclose(ifile);
		}

		free(new_fp);
	}

	/*************************************************/

	/*****************点云文件打开********************/  // 新增点云文件处理
	int pc_data_size = 0;
    int pc_offset    = 0;
	unsigned char* pcbuffer = NULL;
	if (cpush->pointcloud_filename == NULL || strlen(cpush->pointcloud_filename) == 0)
	{
		OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "没有点云文件\n");
	}
	else
	{
		send_pointcloud = OVD_TRUE;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "pointcloud filename:%s\n", cpush->pointcloud_filename);
		FILE* pc_fp = fopen(cpush->pointcloud_filename, "rb");
        if (pc_fp == NULL)
        {
            OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "open [%s] failed\n", cpush->pointcloud_filename);
            exit(1);
        }
		fseek(pc_fp, 0L, SEEK_END);
		long int nfilesize = ftell(pc_fp);
		rewind(pc_fp);
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "pointcloud filesize: %ld\n", nfilesize);

		pcbuffer = (unsigned char *)malloc(nfilesize);
		if (pcbuffer == NULL)
		{
			OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "申请点云内存失败");
			fclose(pc_fp);
			return NULL;
		}

		pc_data_size = fread(pcbuffer, 1, nfilesize, pc_fp);
		fclose(pc_fp);

		cpush->pcbuffer = pcbuffer;
		cpush->pc_data_size = pc_data_size;
		cpush->pc_offset = 0;
	}
	/*************************************************/


	/*********************初始化发送时间********************/
	//struct timeval tv;
    //gettimeofday(&tv, NULL);
    long long msec = OVD_DEMO_getutcMicroSec();
    //if(tv.tv_sec < 31536000)
	//{   //not valid
    //    cpush->timestamp = -1;
    //    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "gettimeofday error, the timestamp is invalid\n");
    //    return NULL;
    //}
    cpush->timestamp  = msec; //ceil((double)tv.tv_sec*1000 + ((double)tv.tv_usec) / 1000);
	long long audio_time = msec; //ceil((double)tv.tv_sec*1000 + ((double)tv.tv_usec) / 1000);
	long long video_time = msec; //ceil((double)tv.tv_sec*1000 + ((double)tv.tv_usec) / 1000);
	long long pc_time = msec;	// 新增点云时间戳

	/*********************************************/
	long long inter_video = 1000000 / (cpush->fps);
	long long frequence = cpush->audioParams.samplesRate;
	long long inter_audio = (double)1024*1000000/frequence;
	long long inter_pointcloud = 100000;  // 10fps = 100ms间隔
	long long u_sleeptime = 0;
	cpush->conf_data = (CONFIG_DATA*)malloc(sizeof(CONFIG_DATA));
	cpush->conf_data->last_addr = NULL;
	cpush->conf_data->step = 0;

	while (1)
	{
        pthread_mutex_lock(&cpush->mutex);
        if (cpush->isBreak)
        {
            pthread_mutex_unlock(&cpush->mutex);
            break;
        }
        pthread_mutex_unlock(&cpush->mutex);
		cpush->timestamp  = OVD_DEMO_getutcMicroSec(); //ceil((double)tv.tv_sec*1000 + ((double)tv.tv_usec) / 1000);//当前发送时间 //更换为绝对时间戳
		if(video_time <= cpush->timestamp || audio_time <= cpush->timestamp || (send_pointcloud && pc_time <= cpush->timestamp))
		{
			if(video_time <= cpush->timestamp && send_video)
			{
				cpush->contentType = OVD_Video;
                if (cpush->videoParams.reserve)
                {
                    cpush->contentType = OVD_AOV;
                }
				v_size=500*1024;
				int rect;

				if (source_STREAM_CODEC_H265 == cpush->videoParams.codec)
				{
					rect = getH265Frame(vbuffer, v_data_size, vframe, &v_size, cpush->conf_data);
				}
				else
				{
					rect = getH264Frame(vbuffer, v_data_size, vframe, &v_size, cpush->conf_data);
				}

				if (rect)
				{
					OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "该视频没有有效帧");
					continue;
				}
				if(cpush->conf_data->cType == I_Frame)
				{
					cpush->isKey = OVD_TRUE;
				}
                else
                {
                    cpush->isKey = OVD_FALSE;
                }
                // OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "cpush->contentType:%d cpush->isKey:%d timestamp:%lld \n",cpush->contentType,cpush->isKey,(long long)video_time);
                if(debug_stop_video == OVD_FALSE)
                {
                    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "video timestamp:%lld \n",(long long)video_time/1000);
                    cpush->pushcallback(cpush, vframe, v_size, (long long)video_time/1000);
                }
				video_time += inter_video;


			}

			if(audio_time <= cpush->timestamp && send_audio)
			{
				int rect = getADTSFrame(input_data, data_size, aacframe, &size);
				if (rect == -1)
				{
					break;
				}
				else if (rect == 1)
				{
					data_size = 0;
					size	  = 0;
					cnt 	  = 0;
					offset	  = 0;

					data_size = tmp;
					input_data = aacbuffer;
					continue;
				}
				data_size -= size;
				input_data += size;
				cnt++;
				cpush->contentType = OVD_Audio;
				cpush->isKey = OVD_FALSE;
                if(debug_stop_audio == OVD_FALSE)
                {
                    //OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "audio timestamp:%lld \n",(long long)audio_time/1000);
				    cpush->pushcallback(cpush,aacframe,size,(long long)audio_time/1000);
                }
				audio_time+=inter_audio;
			}

			// 新增点云数据处理
			if (send_pointcloud && pc_time <= cpush->timestamp)
			{
				int frame_size = 19346;  // 固定点云帧大小
				if (cpush->pc_offset + frame_size > cpush->pc_data_size)
				{
					// 文件结束，循环播放
					cpush->pc_offset = 0;
				}
				cpush->contentType = OVD_3D_Point_Cloud;
                if (debug_start_push_pointcloud)
                {
                    cpush->pushcallback(cpush, cpush->pcbuffer + cpush->pc_offset, frame_size, (long long)pc_time/1000);
                }
				cpush->pc_offset += frame_size;
				pc_time += inter_pointcloud;  // 更新点云时间戳
			}
		}
        usleep(10);
    }
	free_p((void*)cpush->conf_data);
	free_p((void*)aacbuffer);
	free_p((void*)aacframe);
	free_p((void*)vframe);
	free_p((void*)vbuffer);
	free_p((void*)pcbuffer);  // 新增释放点云缓冲区
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "Pushing thread exit succuss.\n");
#endif
    return NULL;
}

void OVD_DEMOForceIFrame(void* handle)
{
	AVPushContext *cpush=(AVPushContext *)(handle);
	if(cpush->video_filename != NULL)
	{

		cpush->ForceIFrame = OVD_TRUE;
		return;
	}
	else
	{
		OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO", "该文件流没有视频文件\n");
	}
	return;

}

int OVD_DEMOAV_GetMediaFormat(char* video_file, char* audio_file, char* pointcloud_file, OVDChannelsInfo *chn_info)
{
    OVDVideoDataFormat *video_fmt;
    OVDAudioDataFormat *audio_fmt;

    if (chn_info == NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "#Error, param is NULL\n");
        return -1;
    }
    if (video_file != NULL)
    {
        chn_info->video_file = video_file;
    }
    else
    {
        chn_info->video_file = NULL;
        chn_info->videoinfo.codec = source_STREAM_NO_CODEC;
    }

    if (audio_file != NULL)
    {
        chn_info->audio_file = audio_file;
        FILE * audio_fp = fopen(audio_file, "rb");
		if (!audio_fp)
		{
	        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "Open audio file error, file name: %s\n", audio_file);
	        return -1;
	    }
		unsigned int data_size = 0;
	    int size      = 0;
		unsigned char* aacframe_tmp  = (unsigned char *)malloc(1024*5);
	    unsigned char* aacbuffer_tmp = (unsigned char *)malloc(1024*1024);
		data_size = fread(aacbuffer_tmp, 1, 1024*1024, audio_fp);


		int rect = getADTSFrame(aacbuffer_tmp, data_size, aacframe_tmp, &size);
	    unsigned char sampling_frequency_index = aacframe_tmp[2]&0x3C;
	    sampling_frequency_index = sampling_frequency_index>>2;
	    switch (sampling_frequency_index)
		{
	        case 0: chn_info->audioinfo.samplesRate = 96000;break;
	        case 1: chn_info->audioinfo.samplesRate = 88200;break;
	        case 2: chn_info->audioinfo.samplesRate = 64000;break;
	        case 3: chn_info->audioinfo.samplesRate = 48000;break;
	        case 4: chn_info->audioinfo.samplesRate = 44100;break;
	        case 5: chn_info->audioinfo.samplesRate = 32000;break;
	        case 6: chn_info->audioinfo.samplesRate = 24000;break;
	        case 7: chn_info->audioinfo.samplesRate = 22050;break;
	        case 8: chn_info->audioinfo.samplesRate = 16000;break;
	        case 9: chn_info->audioinfo.samplesRate = 12000;break;
	        case 10: chn_info->audioinfo.samplesRate = 11025;break;
	        case 11: chn_info->audioinfo.samplesRate = 8000;break;
	        default: chn_info->audioinfo.samplesRate = -1;break;
	    }

		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "samplesrate=%u\n",chn_info->audioinfo.samplesRate);
		free(aacbuffer_tmp);
		free(aacframe_tmp);
		fclose(audio_fp);
    }
    else
    {
        chn_info->audioinfo.codec = source_STREAM_NO_CODEC;
    }

    if (pointcloud_file != NULL)
    {
        chn_info->pointcloud_file = pointcloud_file;
    }
    else
    {
        chn_info->pointcloud_file = NULL;
        chn_info->videoinfo.codec = source_STREAM_NO_CODEC;
    }

    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","video format codec %u, bitrate %u, width %u, height %u, framerate %x, colorDepth %x, frameInterval %x, reserve %x\n",
        chn_info->videoinfo.codec, chn_info->videoinfo.bitrate, chn_info->videoinfo.width, chn_info->videoinfo.height, chn_info->videoinfo.framerate, chn_info->videoinfo.colorDepth, chn_info->videoinfo.frameInterval, chn_info->videoinfo.reserve);
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO","audio format codec %u, samplesRate %u, bitrate %u, waveFormat %u, channelNumber %u, blockAlign %u, bitsPerSample %u, reserve %u\n",
        chn_info->audioinfo.codec, chn_info->audioinfo.samplesRate, chn_info->audioinfo.bitrate, chn_info->audioinfo.waveFormat, chn_info->audioinfo.channelNumber, chn_info->audioinfo.blockAlign, chn_info->audioinfo.bitsPerSample, chn_info->audioinfo.reserve);

    return 0;

}

void* OVD_DEMOAV_PushingStart(OVDChannelsInfo *chn_info, Demo_AVPushCallback Pushcallback,void* userdata)
{

	int ret=0;
	int *sendtype = (int *)userdata;

	AVPushContext *cpush = (AVPushContext *)(malloc(sizeof(AVPushContext)));
    OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", " size:%zu\n",sizeof(AVPushContext));
    if(cpush == NULL)
	{
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "cpush init failed\n");
        return NULL;
    }
	memset(cpush, 0, sizeof(AVPushContext));
    pthread_mutex_init(&cpush->mutex, NULL);
    pthread_cond_init(&cpush->not_empty, NULL);

    cpush->channel = chn_info->channel;
	cpush->conf_data = NULL;
	cpush->fps = chn_info->videoinfo.framerate;
	cpush->pushcallback = Pushcallback;
	cpush->userdata = get_fr_cb;
    cpush->isBreak = OVD_FALSE;
	/****************************/
	if(sendtype != NULL)
	{
		cpush->sendtype = *sendtype;
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "cpush->sendtype : %d\n",cpush->sendtype);
	}

	cpush->video_filename = chn_info->video_file;
	cpush->audeo_filename = chn_info->audio_file;
    cpush->pointcloud_filename = chn_info->pointcloud_file;
    cpush->videoParams.codec = chn_info->videoinfo.codec;
    cpush->videoParams.bitrate = chn_info->videoinfo.bitrate;
    cpush->videoParams.width = chn_info->videoinfo.width;
    cpush->videoParams.height = chn_info->videoinfo.height;
    cpush->videoParams.framerate = chn_info->videoinfo.framerate;
    cpush->videoParams.colorDepth = chn_info->videoinfo.colorDepth;
    cpush->videoParams.frameInterval = chn_info->videoinfo.frameInterval;
    cpush->videoParams.reserve = chn_info->videoinfo.reserve;

    cpush->audioParams.codec = chn_info->audioinfo.codec;
    cpush->audioParams.samplesRate = chn_info->audioinfo.samplesRate;
    cpush->audioParams.bitrate = chn_info->audioinfo.bitrate;
    cpush->audioParams.waveFormat = chn_info->audioinfo.waveFormat;
    cpush->audioParams.channelNumber = chn_info->audioinfo.channelNumber;
    cpush->audioParams.blockAlign = chn_info->audioinfo.blockAlign;
    cpush->audioParams.bitsPerSample = chn_info->audioinfo.bitsPerSample;
    cpush->audioParams.frameInterval = chn_info->audioinfo.frameInterval;


#ifdef __ARCH_RTOS__
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 128*1024);
		ret = pthread_create(&(cpush->audio_thread_id), &attr,AVPushing, (void *)cpush);
#else
		ret = pthread_create(&(cpush->audio_thread_id), NULL,AVPushing, (void *)cpush);
#endif

    if(ret)
    {
        ret = -1;
        cpush->audio_thread_id  = 0;
        cpush->video_thread_id  = 0;
        pthread_mutex_destroy(&cpush->mutex);
        free(cpush);
        return NULL;
    }


    return (void*)(cpush);

}








int OVD_DEMOAVPushingStop(void* handler)
{

	AVPushContext *cpush = (AVPushContext *)(handler);
	if(cpush == NULL)
	{
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "the cpush handle is NULL\n");
        return -1;
    }
    pthread_mutex_lock(&cpush->mutex);
    cpush->isBreak = OVD_TRUE;
    pthread_mutex_unlock(&cpush->mutex);

    if(cpush->audio_thread_id != 0)
	{
        void * res;
        int ret;
        pthread_mutex_lock(&cpush->mutex);
        cpush->consumer_active = 0;
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "停止音频\n");
        pthread_cond_signal(&cpush->not_empty); //wakeup comsumer
        //cpush->is_break = OVD_TRUE;
        pthread_mutex_unlock(&cpush->mutex);

        ret = pthread_join(cpush->audio_thread_id, &res);
        if (ret != 0)
		{
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "stop AVPushing thread failed, pthread_join failed, return [%d]\n", ret);
        }
        cpush->audio_thread_id = 0;

    }

    if(cpush->video_thread_id != 0)
	{
        void * res;
        int ret;
        pthread_mutex_lock(&cpush->mutex);
        cpush->consumer_active = 0;
		OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "停止视频\n");
        pthread_cond_signal(&cpush->not_empty); //wakeup comsumer
        pthread_mutex_unlock(&cpush->mutex);

        ret = pthread_join(cpush->video_thread_id, &res);
        if (ret != 0)
		{
            OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMlO", "stop AVPushing thread failed\n");
        }
        cpush->video_thread_id = 0;

    }


    pthread_cond_destroy(&cpush->not_empty);
    pthread_mutex_destroy(&cpush->mutex);

    cpush->consumer_thread_id  = 0;
	cpush->video_thread_id  = 0;
	cpush->audio_thread_id = 0;

    //cpush_freep(&cpush->audeo_filename);
    //cpush_freep(&cpush->video_filename);
    //pthread_cond_destroy(&cpush->not_empty);
    //pthread_mutex_destroy(&cpush->mutex);

	//pthread_exit(cpush->audio_thread_id);
	free(cpush);
	OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "stop success\n");

	return 0;
}

