#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/prctl.h>

#include <pthread.h>

#include "OVD_DemoHibernation.h"
#include "OVD_define.h"
#include "OVD_DemoCmd.h"
#include "OVD_Demo.h"

//模拟厂商进行休眠机制
pthread_t OPENAPI_DEMO_Hibernation_pid;
pthread_t OPENAPI_DEMO_Hibernation_listen_pid;

int DEMO_ovd_Hibernation_flag; //0:代表设备未处于休眠状态，1:代表设备处于休眠状态

extern OVDClientParam      OVD_DEMO_Client;

typedef struct
 {
     char magic[2];         // 同步字段，固定为0xcc, 0x00
     uint16_t length;       // 长度，固定为40，大端
     char macid[16];        // 设备id，16位十进制数字字符串，例如"2009550000011079"
     uint8_t cmd;           // 命令字, 固定为0x4A。
     uint8_t reserved1;     // 保留字节
     uint8_t reserved2;     // 保留字节
     uint8_t reserved3;     // 保留字节
     char token[16];        // 字符串令牌，最长16位字符，不足16位使用’\0'字符从后填充
 }HibernationHeartPacket;

typedef struct
 {
     char magic[2];         // 同步字段，固定为0xcd, 0x00
     uint16_t length;       // 长度，固定为56，大端
     char macid[32];        // 设备id，25位十进制数字字符串，例如"1713200129111112500010128"
     uint8_t cmd;           // 命令字, 固定为0x4A。
     uint8_t reserved1;     // 保留字节
     uint8_t reserved2;     // 保留字节
     uint8_t reserved3;     // 保留字节
     char token[16];        // 字符串令牌，最长16位字符，不足16位使用’\0'字符从后填充
 }HibernationHeartPacket2;
#if 0
//监听线程
void* pthread_OVD_DemoHibernation_listen(void *args)
{
    int *listensocket= (int *)args;
    char recv_buf[1024];
    memset(recv_buf,0,1024);
    int buflen=0;

    while(1)
    {
        buflen=recv(*listensocket,recv_buf,1024,0);
        if(buflen < 0)
        {
            if(buflen==EAGAIN)
                break;
            else
            {

                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "recvsocketfd meet errors,listen thread exit\n");
                OPENAPI_DEMO_Hibernation_listen_pid=0;
                return NULL;
            }
        }
        else if(buflen==0)
        {
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd has been closed, listen thread exit\n");
            OPENAPI_DEMO_Hibernation_listen_pid=0;
            return NULL;
        }
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "hibernation recv msg: %s\n",recv_buf);

        if((strncmp(recv_buf,"TOWAKEUP",8)==0))
        {
            if(DEMO_ovd_Hibernation_flag!=0)
            {
                DEMO_ovd_Hibernation_flag=0;
                OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd recv wake cmd, listen thread exit\n");
                OPENAPI_DEMO_Hibernation_listen_pid=0;
                OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备被唤起\n");
                //sleep(10);
                OVD_ServiceStart();
            }
            else
            {
               OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备已被唤起\n");
            }

            return NULL;

        }
    }

}
#endif

void* pthread_OVD_DemoHibernation(void *args)
{
    struct sockaddr_in server;
    int ret=0;
    int len=0;
    char send_buf[30]={0};
    int count=0;
    int OVD_DEMO_hibernation_socket=0;
    char recv_buf[1024]={0};
    memset(recv_buf,0,1024);
    int result=0;

    char *ptr = NULL, **pptr = NULL;
    struct hostent *hptr = NULL;
    char str[32]={0};

    if (args == NULL)
    {
        fprintf(stderr, "Error # %s:%d input param is NULL\n", __func__, __LINE__);
        return NULL;
    }
    prctl(PR_SET_NAME, "DemoHibernation");
    OVDDemoHibernationContent *content = (OVDDemoHibernationContent *)args;

    if(strlen(content->hdomain)==0)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "do not support hibiernation\n");
        return NULL;
    }

    if(content->hb_interval<=0)
    {
        content->hb_interval=60;
    }

    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "3. close other services forexample ws/wss server  \n");
    OVD_ServiceStop();
    DEMO_ovd_Hibernation_flag=1;
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "4. try to connect HServer\n");
    if((-1==(OVD_DEMO_hibernation_socket=socket(AF_INET,SOCK_STREAM,0))))
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "Hsocket create failed\n");
        return NULL;
    }

    if((hptr = gethostbyname(content->hdomain))==NULL)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "gethostbyname error host:%s\n",content->hdomain);
        return NULL;
    }
    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
            for(; *pptr!=NULL; pptr++)
               OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  " address:%s\n",inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  " first address: %s\n",inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str)));
            break;
        default:
            printf("unknown address type\n");
        break;
    }
    //使用第一个
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "address:%s \n",str);

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr(str);
    server.sin_port=htons(content->hport);

    ret=connect(OVD_DEMO_hibernation_socket,(struct sockaddr*)&server,sizeof(struct sockaddr));

    if(0>ret)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "connect HServer failed\n");
        return NULL;
    }



    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "5. send heartmsg to  HServer and listen msg from Hserver\n");

    
    HibernationHeartPacket heartpacket;

    memset(&heartpacket,0,sizeof(HibernationHeartPacket));
    //组装休眠心跳包
    heartpacket.magic[0]=0xcc;
    heartpacket.magic[1]=0x00;

    heartpacket.length=(uint16_t)htons(40);

    strncpy(heartpacket.macid,OVD_DEMO_Client.OVDDeviceID,16);

    heartpacket.cmd=0x4A;

    strncpy(heartpacket.token,content->hb_token,16);

    //count=content->hb_interval;
    struct timeval timeout;
    memset(&timeout,0,sizeof(struct timeval));
    timeout.tv_sec=content->hb_interval;
    timeout.tv_usec=0;
    ret=setsockopt(OVD_DEMO_hibernation_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(struct timeval));
    //OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "ret:%d\n",ret);
    while(DEMO_ovd_Hibernation_flag && result==0 )
    {

        if(1)
        {
            len=send(OVD_DEMO_hibernation_socket,&heartpacket,sizeof(heartpacket),0);
            if(0>len)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "send msg to HServer failed,  exit \n");
                result=-1;
                break;
            }
            else
            {
                OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "send %d msg to HServer\n",len);
            }

            len=recv(OVD_DEMO_hibernation_socket,recv_buf,1024,0);
            if(len < 0)
            {
                if(len==-1 && errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");
                }
                else
                {

                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "recvsocketfd meet errors,listen thread exit\n");
                    result=-1;
                    break;
                }
            }
            else if(len==0)
            {
                if(errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd has been closed, listen thread exit\n");
                    result= -1;
                    break;
                }

            }
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "hibernation recv msg: %s\n",recv_buf);

            if((strncmp(recv_buf,"TOWAKEUP",8)==0))
            {
                if(DEMO_ovd_Hibernation_flag!=0)
                {
                    DEMO_ovd_Hibernation_flag=0;
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd recv wake cmd, listen thread exit\n");

                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备被唤起\n");
                    result=1;
                    break;

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备已被唤起\n");
                }
            }

        }

   }


    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "Hibernation thread exit \n\n");
        free(content);
    close(OVD_DEMO_hibernation_socket);
    OPENAPI_DEMO_Hibernation_pid=0;
    //sleep(10);
    if(1==result)
    {
        sleep(10);
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备重新唤醒，开启推流,及开启服务\n");
        cmd_exec(AV_READY);
        cmd_exec(PUSH_AV);
        cmd_exec(SERVICE_OK);
    }

    return NULL;
}

//测试程序，根据之前低功耗休眠保活协议，模拟向测试环境维护心跳保活
void* pthread_OVD_DemoHibernation_test1(void *args)
{
    struct sockaddr_in server;
    int ret=0;
    int len=0;
    char send_buf[30]={0};
    int count=0;
    int OVD_DEMO_hibernation_socket=0;
    char recv_buf[1024]={0};
    memset(recv_buf,0,1024);
    int result=0;

    char *ptr = NULL, **pptr = NULL;
    struct hostent *hptr = NULL;
    char str[32]={0};

    prctl(PR_SET_NAME, "DemoHibernation_test1");
    OVDDemoHibernationContent content ;
    strcpy(content.hdomain,"36.138.107.149");
    content.hb_interval=60;
    content.hport=9106;
    strcpy(content.hb_token,"1234567890123456");
    
    DEMO_ovd_Hibernation_flag=1;
    
    if((-1==(OVD_DEMO_hibernation_socket=socket(AF_INET,SOCK_STREAM,0))))
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "Hsocket create failed\n");
        return NULL;
    }

    //使用第一个
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "address:%s \n",content.hdomain);

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr(content.hdomain);
    server.sin_port=htons(content.hport);

    ret=connect(OVD_DEMO_hibernation_socket,(struct sockaddr*)&server,sizeof(struct sockaddr));

    if(0>ret)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "connect HServer failed\n");
        return NULL;
    }



    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "5. send heartmsg to  HServer and listen msg from Hserver\n");

    
    HibernationHeartPacket heartpacket;

    memset(&heartpacket,0,sizeof(HibernationHeartPacket));
    //组装休眠心跳包
    heartpacket.magic[0]=0xcc;
    heartpacket.magic[1]=0x00;

    heartpacket.length=(uint16_t)htons(40);

    strncpy(heartpacket.macid,OVD_DEMO_Client.OVDDeviceID,16);

    heartpacket.cmd=0x4A;

    strncpy(heartpacket.token,content.hb_token,16);

    //count=content->hb_interval;
    struct timeval timeout;
    memset(&timeout,0,sizeof(struct timeval));
    timeout.tv_sec=content.hb_interval;
    timeout.tv_usec=0;
    ret=setsockopt(OVD_DEMO_hibernation_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(struct timeval));
    //OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "ret:%d\n",ret);
    while(DEMO_ovd_Hibernation_flag && result==0 )
    {

        if(1)
        {
            len=send(OVD_DEMO_hibernation_socket,&heartpacket,sizeof(heartpacket),0);
            if(0>len)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "send msg to HServer failed,  exit \n");
                result=-1;
                break;
            }
            else
            {
                OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "send %d msg to HServer\n",len);
            }

            len=recv(OVD_DEMO_hibernation_socket,recv_buf,1024,0);
            if(len < 0)
            {
                if(len==-1 && errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");
                }
                else
                {

                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "recvsocketfd meet errors,listen thread exit\n");
                    result=-1;
                    break;
                }
            }
            else if(len==0)
            {
                if(errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd has been closed, listen thread exit\n");
                    result= -1;
                    break;
                }

            }
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "hibernation recv msg: %s\n",recv_buf);

            if((strncmp(recv_buf,"TOWAKEUP",8)==0))
            {
                if(DEMO_ovd_Hibernation_flag!=0)
                {
                    DEMO_ovd_Hibernation_flag=0;
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd recv wake cmd, listen thread exit\n");

                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备被唤起\n");
                    result=1;
                    break;

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备已被唤起\n");
                }
            }

        }

   }


    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "Hibernation thread test1 exit \n\n");
    
    close(OVD_DEMO_hibernation_socket);
 
    

    return NULL;
}

//测试程序，根据之前低功耗休眠保活协议，模拟向测试环境维护心跳保活
void* pthread_OVD_DemoHibernation_test2(void *args)
{
    struct sockaddr_in server;
    int ret=0;
    int len=0;
    char send_buf[30]={0};
    int count=0;
    int OVD_DEMO_hibernation_socket=0;
    char recv_buf[1024]={0};
    memset(recv_buf,0,1024);
    int result=0;

    char *ptr = NULL, **pptr = NULL;
    struct hostent *hptr = NULL;
    char str[32]={0};

    prctl(PR_SET_NAME, "DemoHibernation_test2");
    OVDDemoHibernationContent content ;
    strcpy(content.hdomain,"36.138.107.149");
    content.hb_interval=60;
    content.hport=9106;
    strcpy(content.hb_token,"1234567890123456");
    
    DEMO_ovd_Hibernation_flag=1;
    
    if((-1==(OVD_DEMO_hibernation_socket=socket(AF_INET,SOCK_STREAM,0))))
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "Hsocket create failed\n");
        return NULL;
    }


    //使用第一个
    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "address:%s \n",content.hdomain);

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=inet_addr(content.hdomain);
    server.sin_port=htons(content.hport);

    ret=connect(OVD_DEMO_hibernation_socket,(struct sockaddr*)&server,sizeof(struct sockaddr));

    if(0>ret)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "connect HServer failed\n");
        return NULL;
    }



    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "5. send heartmsg to  HServer and listen msg from Hserver\n");

    
    HibernationHeartPacket2 heartpacket;

    memset(&heartpacket,0,sizeof(HibernationHeartPacket2));
    //组装休眠心跳包
    heartpacket.magic[0]=0xcd;
    heartpacket.magic[1]=0x00;

    heartpacket.length=(uint16_t)htons(56);

    strncpy(heartpacket.macid,"1713200129111112500010128",32);

    heartpacket.cmd=0x4A;

    strncpy(heartpacket.token,content.hb_token,16);

    //count=content->hb_interval;
    struct timeval timeout;
    memset(&timeout,0,sizeof(struct timeval));
    timeout.tv_sec=content.hb_interval;
    timeout.tv_usec=0;
    ret=setsockopt(OVD_DEMO_hibernation_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(struct timeval));
    //OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "ret:%d\n",ret);
    while(DEMO_ovd_Hibernation_flag && result==0 )
    {

        if(1)
        {
            len=send(OVD_DEMO_hibernation_socket,&heartpacket,sizeof(heartpacket),0);
            if(0>len)
            {
                OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "send msg to HServer failed,  exit \n");
                result=-1;
                break;
            }
            else
            {
                OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "send %d msg to HServer\n",len);
            }

            len=recv(OVD_DEMO_hibernation_socket,recv_buf,1024,0);
            if(len < 0)
            {
                if(len==-1 && errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");
                }
                else
                {

                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "recvsocketfd meet errors,listen thread exit\n");
                    result=-1;
                    break;
                }
            }
            else if(len==0)
            {
                if(errno==EAGAIN)
                {
                    OVD_LOG(OVD_LOGLEVEL_DEBUG,"DEV/DEMO",  "recv timeout\n");

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd has been closed, listen thread exit\n");
                    result= -1;
                    break;
                }

            }
            OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "hibernation recv msg: %s\n",recv_buf);

            if((strncmp(recv_buf,"TOWAKEUP",8)==0))
            {
                if(DEMO_ovd_Hibernation_flag!=0)
                {
                    DEMO_ovd_Hibernation_flag=0;
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "socketfd recv wake cmd, listen thread exit\n");

                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备被唤起\n");
                    result=1;
                    break;

                }
                else
                {
                    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "设备已被唤起\n");
                }
            }

        }

   }


    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "Hibernation thread test1 exit \n\n");
    
    close(OVD_DEMO_hibernation_socket);
    
    

    return NULL;
}


/*
设备获取休眠地址后回调

    [in] "hdomain": ovh 域名
    [in] "hport": ovh 端口
    [in] "hb_interval": <可选，整数：设备心跳间隔，单位秒，该字段不存在则默认为10秒>,
    "hb_token": <可选， 字符串：休眠服务的令牌，长度不超过16个字符。该字段不存在表示休眠服务不需要提供token>
*/
int OVD_DEMO_GetHServerInfo_handle(char *hdomain, int hport, int hb_interval,char *hb_token)
{
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "2. GetServerInfo from ws/wss server  \n");
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "hdomain:%s, hport:%d ,hb_interval: %d, hb_token: %s \n",hdomain,hport, hb_interval,hb_token);

    OVDDemoHibernationContent *content = (OVDDemoHibernationContent *)malloc(sizeof(OVDDemoHibernationContent));
    memset(content,0,sizeof(OVDDemoHibernationContent));
    strncpy(content->hdomain,hdomain,sizeof(content->hdomain));
    content->hport=hport;
    content->hb_interval=hb_interval;
    strncpy(content->hb_token,hb_token,sizeof(content->hb_token));

#ifdef __ARCH_RTOS__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 128*1024);
    int result = pthread_create(&OPENAPI_DEMO_Hibernation_pid,&attr,pthread_OVD_DemoHibernation,(void *)content);
#else
	int result = pthread_create(&OPENAPI_DEMO_Hibernation_pid,NULL,pthread_OVD_DemoHibernation,(void *)content);
#endif

    pthread_detach(OPENAPI_DEMO_Hibernation_pid);
    return 0;
}




int OVD_DEMO_Hibernation_end()
{
    if(DEMO_ovd_Hibernation_flag==0)
    {
        OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "<--- ovd 已经不处于休眠状态 --> \n");
        return 0;

    }
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "<-- ovd 退出 休眠,重启相关服务 --> \n");
    DEMO_ovd_Hibernation_flag=0;
        cmd_exec(AV_READY);
        cmd_exec(PUSH_AV);
        cmd_exec(SERVICE_OK);
        return 0;
}

int test_ipaddr4_reuse(char* inputCmd)
{
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "test_ipaddr4_reuse ==> %s \n", inputCmd);

    int ret = 0;
    char str[20] = {0};
    strcpy(str, inputCmd+4);
    int port = atoi(str);
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  " ==> port:%s-%d \n", str, port);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> socket fail \n");
        return -1;
    }

    int yes = 1;
    //ret = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&yes, sizeof yes);
    //ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    // if (ret != 0) {
    //     OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> setsockopt SO_REUSEPORT or SO_REUSEADDR fail \n");
    //     return;
    // }

    //ipv4 bind
    struct sockaddr_in sa4Buf = {0};
    sa4Buf.sin_family = AF_INET;
    sa4Buf.sin_port = htons(port);
    sa4Buf.sin_addr.s_addr = htonl(INADDR_ANY);
    struct sockaddr* p_addr = (struct sockaddr*)&sa4Buf;
    int addrLen = sizeof(sa4Buf);
    ret = bind(sock, p_addr, addrLen);
    if (ret != 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> bind v4 fail:%d(%s).\n", errno, strerror(errno));
        return ret;
    }

    struct in_addr addr = {0};
    if (inet_aton("192.168.1.11", &addr) == 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> Invalid address.\n");
        return ret;
    }
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "==> dst_addr: %s\n", inet_ntoa(addr));

    struct sockaddr dest_addr = {0};
    memset(&dest_addr, 0, sizeof(dest_addr));
    struct sockaddr_in* p_addr4_in = (struct sockaddr_in*)&dest_addr;
    p_addr4_in->sin_family = AF_INET;
    p_addr4_in->sin_port = htons(28000);
    memcpy(&p_addr4_in->sin_addr, &addr, sizeof(addr));
    ssize_t s = sendto(sock, str, 18, 0, &dest_addr, sizeof(dest_addr));

    //not close for test
    //close(sock);
    return 0;
}

int test_ipaddr6_reuse(char* inputCmd)
{
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "test_ipaddr6_reuse ==> %s \n", inputCmd);

    int ret = 0;
    char str[20] = {0};
    strcpy(str, inputCmd+4);
    int port = atoi(str);
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  " ==> port:%s-%d \n", str, port);

    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> socket fail \n");
        return -1;
    }

    int yes = 1;
    //ret = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&yes, sizeof yes);
    //ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    // if (ret != 0) {
    //     OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> setsockopt SO_REUSEPORT or SO_REUSEADDR fail \n");
    //     return;
    // }

    //ipv6 bind
    struct sockaddr_in6 sa6Buf = {0};
    sa6Buf.sin6_family = AF_INET6;
    sa6Buf.sin6_port = htons(port);
    sa6Buf.sin6_addr = in6addr_any;
    struct sockaddr* psa = (struct sockaddr* )&sa6Buf;
    int saLen = sizeof(sa6Buf);
    ret = bind(sock, psa, saLen);
    if (ret != 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> bind v6 fail:%d(%s).\n", errno, strerror(errno));
        return -1;
    }

    #if 0
    struct in6_addr addr = {0};
    if (inet_aton("2409:8728:ef2:11:313::30", &addr) == 0) {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", " ==> Invalid address.\n");
        return;
    }
    OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "==> dst_addr: %s\n", inet_ntoa(addr));

    struct sockaddr dest_addr = {0};
    memset(&dest_addr, 0, sizeof(dest_addr));
    struct sockaddr_in6* p_addr6_in = (struct sockaddr_in6*)&dest_addr;
    p_addr6_in->sin_family = AF_INET6;
    p_addr6_in->sin_port = htons(28000);
    memcpy(&p_addr6_in->sin_addr, &addr, sizeof(addr));
    ssize_t s = sendto(sock, str, 18, 0, &dest_addr, sizeof(dest_addr));
    #endif

    //not close for test
    //close(sock);
    return ret;
}

int OVD_DEMO_USER(char* inputCmd)
{
#if 1
    test_ipaddr4_reuse(inputCmd);
    test_ipaddr6_reuse(inputCmd);
#endif

#if 0
    //case1: 将设备设置为升级状态， 休眠30s后，重新设置为在线状态
    OVD_updateOVDstate(2);

    sleep(30);

    OVD_updateOVDstate(1);
#endif
    return 0;
}