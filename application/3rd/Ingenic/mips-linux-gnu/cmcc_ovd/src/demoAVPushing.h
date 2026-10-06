#ifndef __DEMO_AVPUSHING_H_INCLUDED
#define __DEMO_AVPUSHING_H_INCLUDED


//#include "../../OVD_Demo.h"
#include "time.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "OVD_Demo.h"

typedef struct AVPushContext_s AVPushContext;

typedef struct
{
    unsigned int codec;
    unsigned int bitrate;
    unsigned short width;
    unsigned short height;
    unsigned char framerate;
    unsigned char colorDepth;
    unsigned char frameInterval;
    unsigned char reserve;

}sourceVideoDataFormat;

typedef struct
{
	unsigned int codec;
    unsigned int samplesRate;
    unsigned int bitrate;
    unsigned short waveFormat;
    unsigned short channelNumber;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    unsigned short frameInterval;
    unsigned short reserve;
}sourceAudioDataFormat;

typedef enum
{
	I_Frame=1,
	P_Frame,
	B_Frame,
	NONE
}FrameType;

typedef struct
{
	OVD_bool  bGetWH; //是否获取分辨率
    int   width, height; //分辨率
	FrameType  cType; //帧类型（'I' 'P' 'B'）
	unsigned char* last_addr;
	int step;

} CONFIG_DATA;

typedef int (*ovd_get_fr_cb)(OVD_bool video, unsigned char* buf, int max_len);
extern ovd_get_fr_cb get_fr_cb;

typedef void (*Demo_AVPushCallback)(AVPushContext *cpush, unsigned char* contentData, int dataLen, long long timestamp);

typedef struct AVPushContext_s {
    int channel;
    void *userdata;
    char *video_filename;
    char *audeo_filename;
    char *media_filename;
    char *pointcloud_filename;

    int sendtype;

    sourceVideoDataFormat videoParams;
    sourceAudioDataFormat audioParams;

    Demo_AVPushCallback pushcallback;

    unsigned int fps;

    OVDContentType contentType;

    OVD_bool isKey;
	OVD_bool isBreak;
	OVD_bool ForceIFrame;
    unsigned char* contentData;

    int datalen;

    long long timestamp;

    // if there is video stream
    int has_video;

    unsigned char* aacframe;
    unsigned char* aacbuffer;
	unsigned char* vframe;
    unsigned char* vbuffer;
    unsigned char* pcbuffer;
    int pc_data_size;
    int pc_offset;
    //writer pthread id
    pthread_t consumer_thread_id;
    pthread_t media_thread_id;
    pthread_t audio_thread_id;
    pthread_t video_thread_id;
    //writer pthread active flag
    volatile int consumer_active;

    //writer pthread exit code
    volatile int consumer_exit_code;

    //flag to indicate cahed_list cong
    volatile int cache_list_congested;


	CONFIG_DATA * conf_data;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
}AVPushContext;

extern int OVD_DEMOAV_GetMediaFormat(char* video_file, char* audio_file, char* pointcloud_file, OVDChannelsInfo *chn_info);
//成功返回0，失败返回-1
//设备单独起线程实现
extern void* OVD_DEMOAV_PushingStart(OVDChannelsInfo *chn_info, Demo_AVPushCallback Pushcallback,void* userdata);
//成功返回0，失败返回-1
extern int OVD_DEMOAVPushingStop(void* handler);

void lcd_debug_stop_video();


void lcd_debug_start_video();


void lcd_debug_stop_audio();


void lcd_debug_start_audio();

// 推送点云码流
void start_push_pointcloud();

void stop_push_pointcloud();





#ifdef __cplusplus
}
#endif


#endif // #ifndef __AUTO_TCHAR_H_INCLUDED

