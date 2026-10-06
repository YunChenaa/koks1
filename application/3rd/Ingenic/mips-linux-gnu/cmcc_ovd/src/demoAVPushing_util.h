#ifndef __DEMO_AVPUSHING_UTIL_H_INCLUDED
#define __DEMO_AVPUSHING_UTIL_H_INCLUDED


#include "demoAVPushing.h"
#include "pthread.h"
#include "OVD_define.h"
#define INFO_BUFF_SIZE	1024*600
#define STREAM_BUF_SIZE 0x100000


typedef struct t_frameinfo
{
	unsigned int offset;
	unsigned int size;
}frameInfo;

typedef struct t_CtxType
{
	frameInfo *info;
	int frameNum;
	int lastFrameNum;
	FILE *fin;
	char aInputBuf[STREAM_BUF_SIZE];
}CtxType;


void OVD_DEMOForceIFrame();
int getADTSFrame(unsigned char* buffer, unsigned int buf_size, unsigned char* data, int* data_size);

int getH264Frame(unsigned char* buffer_start_addr, int buffer_size, unsigned char* frame_start_addr, int* frame_size, CONFIG_DATA *conf_data);
int getH265Frame(unsigned char* buffer_start_addr, int buffer_size, unsigned char* frame_start_addr, int* frame_size, CONFIG_DATA *conf_data);
#endif //__DEMO_AVPUSHING_UTIL_H_INCLUDED