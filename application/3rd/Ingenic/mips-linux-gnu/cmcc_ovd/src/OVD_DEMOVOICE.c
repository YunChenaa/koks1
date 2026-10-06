#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OVD_Demo.h"
#include <unistd.h>
//#define _printd(msg,...) printf ("\033[22m\033[34m[%s][%d]\033[22m\033[37m"msg"\n", (char *)(__FILE__), __LINE__ , ##__VA_ARGS__)

#define READPCMLEN  1920

/*#######################编译选项#################################
gcc -o demowave demowave.c  libwave.a -lpthread -lm  -lrt
##################################################################*/


int recgnizrsucess = 0;
static void *recognizer = NULL;

// struct SsidWiFiInfo
// {
//     char ssid[33];
//     int ssidLen;
//     char pwd[80];
//     int pwdLen;
//     char userid[17];
// }wifi;
// struct WiFiInfo
//{
//	char ssid[33];
//	int ssidLen;
//	char pwd[80];
//	int pwdLen;
//};
////static RecognizStart RgnStart = NULL;
////static RecognizEnd   RgnEnd   = NULL;
//
//void recognizerStart()
//{
//	_printd("..............recognize start\n");
//
////	if(RgnStart != NULL)	RgnStart();
//}
//
//void recognizerEnd(int type, void *info, int infoLen)
//{
//	struct WiFiInfo		minfo;
////	struct SSIDWiFiInfo wifi;
//	struct XXXSSIDWiFiInfo xwifi;
//
//
//	_printd("type:%d\n",type);
//	memset(&minfo,0,sizeof(minfo));
//
//	if(info == NULL || infoLen <= 0)
//	{
//		_printd("recognize Error\n");
//		goto Exit ;
//	}
//	if(type == IT_SSID_WIFI)
//	{
//		memcpy(&minfo,info,infoLen);
//		_printd("SUCCESSFUL ssid:%s, pwd:%s\n", minfo.ssid, minfo.pwd);
//
////		if(RgnEnd != NULL)	RgnEnd(minfo);
//	}
//	else if(type == IT_XX_SSID_WIFI)
//	{
//		memcpy(&xwifi,info,infoLen);
//		_printd("SUCCESSFUL ssid:%s, pwd:%s,phoneID:%s\n", xwifi.ssid, xwifi.pwd,xwifi.phone);
//
//		memcpy(&minfo,&xwifi,sizeof(minfo));
//		//RealTimeSetUserId(xwifi.phone);
//
//		//if(RgnEnd != NULL)	RgnEnd(minfo);
//		recgnizrsucess = 1;
//	}
//	else
//	{
//		_printd("unknow type\n");
//	}
//
//Exit:
//	_printd("..............recognize end\n\n");
//}

void SoundWave_PreInit(int sampleRate,int bitWidth)
{
    printf("SoundWave_PreInit ====[%d]====[%d]=========>>>>\n",sampleRate,bitWidth);
//	VoiceRecognizer_Prepare(sampleRate,bitWidth);
}
void* SoundWave_Init(int sampleRate,int bitWidth)
{
    printf("SoundWave_Init ====[%d]====[%d]=========>>>>\n",sampleRate,bitWidth);
	return OVD_SoundWaveInit(sampleRate,bitWidth);
}

int SoundWave_Start(void *recognizer,void* start_cbfunc,void* end_cbfunc)
{
    printf("SoundWave_Start =================>>>>\n");
//	RgnStart = start_cbfunc;
//	RgnEnd	 = end_cbfunc;

	return OVD_SoundWaveStart(recognizer,OVD_DEMOE_RecognizerStart,OVD_DEMOE_RecognizerFinish);
}

int SoundWave_WriteData(void *recognizer, const void *data, unsigned long long len)
{
//	usleep(1*1000);
	return OVD_SoundWaveWriteData(recognizer,data,len);
}

int SoundWave_Stop(void *recognizer)
{
//	RgnStart = NULL;
//	RgnEnd	 = NULL;

	return OVD_SoundWaveStop(recognizer);
}
//static void recognizerStart1()
//{
//    printf("------------wave recognize start\n");
//}
//
////识别结束回调函数
//static void recognizerEnd1(struct WiFiInfo info)
//{
//	//char s[100]={0};
//	memcpy(&wifi,&info,sizeof(info));
//    printf("ssid:%s, pwd:%s\n", wifi.ssid, wifi.pwd);
//}
int WaveDecodeInit(int FreqBand,int samfre)
{
	int s32Ret = 0;

	if(recognizer == NULL)
	{
		recognizer = SoundWave_Init(samfre,FreqBand);
		if(recognizer == NULL)
			return -1;

		s32Ret = SoundWave_Start(recognizer,NULL,NULL);
        printf("SoundWave_Start return %d\n\n",s32Ret);
        printf("WaveDecodeInit Success\n");
	}
	return s32Ret;
}
int ReadPCMdate(const char *fileName)
{
	FILE *fp = NULL;
	unsigned int read_bytes = 0;
	char buf[READPCMLEN] ={0};

//	char filePath[100];
//	memset(filePath, 0, 100);
//    sprintf(filePath, "./resources/%s", fileName);

	printf("file name %s\n", fileName);

//	fp = fopen("./resources/data1.pcm", "r");
    fp = fopen(fileName, "r");
    if(fp == NULL)
    {
        printf("open %s fail\n", fileName);
		return -1;
	}
	do{
		memset(buf,0,READPCMLEN);
		read_bytes = fread(buf, 1, READPCMLEN, fp);
		SoundWave_WriteData(recognizer, buf, read_bytes);
		usleep(1*1000);
		//_printd("read_bytes[%d]\n",read_bytes);
	}while(read_bytes == READPCMLEN && 0 == recgnizrsucess);
	fclose(fp);
    return 0;
}
int OVD_DEMOVoice(const char *fileName)
{
	int s32Ret = 0;

    printf("OVD_DEMOVoice begin");
    WaveDecodeInit(16,16000);
    recgnizrsucess = 0;
    s32Ret = ReadPCMdate(fileName);
    if(s32Ret != 0 || recognizer == NULL)
    {
        printf("WaveDecodeInit error!!!!\n");
        return -1;
    }
    s32Ret = SoundWave_Stop(recognizer);
    recognizer = NULL;

	return s32Ret;
}

