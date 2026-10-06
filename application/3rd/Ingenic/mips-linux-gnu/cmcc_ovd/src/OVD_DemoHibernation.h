#ifndef __OVD_DEMO_Hibernation_h
#define __OVD_DEMO_Hibernation_h

typedef struct{
    char hdomain[64];
    int hport;
    int hb_interval;
    char hb_token[17];
}OVDDemoHibernationContent;




/*
设备获取休眠地址后回调

    [in] "hdomain": ovh 域名
    [in] "hport": ovh 端口
    [in] "hb_interval": <可选，整数：设备心跳间隔，单位秒，该字段不存在则默认为10秒>,
    "hb_token": <可选， 字符串：休眠服务的令牌，长度不超过16个字符。该字段不存在表示休眠服务不需要提供token>
*/
int OVD_DEMO_GetHServerInfo_handle(char *hdomain, int hport, int hb_interval,char *hb_token);

int OVD_DEMO_Hibernation_end();

void* pthread_OVD_DemoHibernation_test1(void *args);

void* pthread_OVD_DemoHibernation_test2(void *args);


int OVD_DEMO_USER(char* inputCmd);


#endif

