//厂商要实现的同步时间和获取时间机制
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "OVD_DemoTime.h"
#include "demoAVPushing.h"



#ifdef thirdparty_gettimeofday
//厂商重载gettimeofday settimeofday
int OVD_dut_getimeofday(struct timeval *tv, struct timezone *tz)
{
    //printf("[OVD_dut_gettimeofday] thirdparty\n");
    return gettimeofday(tv,tz);
}
#endif


#ifdef thirdparty_settimeofday
int OVD_dut_settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    //printf("[OVD_dut_settimeofday] thirdparty\n");
    return settimeofday(tv,tz);
}
#endif




//由厂商实现获取系统时间戳函数，精确度毫秒
OVD_int32 OVD_DEMO_gettime(OVD_uint64 *out_time)
{

    struct timeval tv;
    system("date");
    gettimeofday(&tv, NULL);

    printf("tv_sec: %d\n", tv.tv_sec);
    printf("tv_usec: %d\n",tv.tv_usec);

    *out_time=(unsigned long long )tv.tv_sec*1000+ (long long)tv.tv_usec/1000;
    return 0;

}


/*
由厂商实现设置系统时间戳函数，精确度为毫秒
in: input_time  设置的时间戳， 单位为毫秒
in: tolerance_value 设置的容忍值，单位为秒,即当前系统时间戳和预设置的时间相差小于容忍值，则不需要设置系统时间
*/
OVD_int32 OVD_DEMO_settime(OVD_uint64 input_time, OVD_int32 tolerance_value)
{
    long long cur_time=0;
    OVD_DEMO_gettime(&cur_time);

    struct timeval set_tv;
    set_tv.tv_sec=input_time/1000;
    set_tv.tv_usec=(input_time%1000)*1000;

    printf("OVD_DEMO_settime: input_time: %lld ms, cur_time: %lld ms, diff %d s \n",input_time,cur_time,tolerance_value);

    if(cur_time > input_time)
    {
        if((cur_time - input_time)>tolerance_value*1000)
        {
            settimeofday(&set_tv,NULL);
        }
    }
    else
    {
        if((input_time - cur_time)>tolerance_value*1000)
        {
            settimeofday(&set_tv,NULL);
        }
    }
    system("date");
    return 0;
}
