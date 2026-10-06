#ifndef __OVD_DEMO_Time_h
#define __OVD_DEMO_Time_h
#include "OVD_Demo.h"

//若厂商需要重载gettimeofday 和 settimeofday, 则声明他们
//#define  thirdparty_gettimeofday
//#define  thirdparty_settimeofday

#ifdef thirdparty_gettimeofday
//厂商重载gettimeofday settimeofday
int OVD_dut_getimeofday(struct timeval *tv, struct timezone *tz);
#endif

#ifdef thirdparty_settimeofday
int OVD_dut_settimeofday(const struct timeval *tv, const struct timezone *tz);
#endif


//由厂商实现获取系统时间戳函数，精确度毫秒
OVD_int32 OVD_DEMO_gettime(OVD_uint64 *out_time);


/*
由厂商实现设置系统时间戳函数，精确度为毫秒
in: input_time  设置的时间戳， 单位为毫秒
in: tolerance_value 设置的容忍值，单位为秒,即当前系统时间戳和预设置的时间相差小于容忍值，则不需要设置系统时间
*/
OVD_int32 OVD_DEMO_settime(OVD_uint64 input_time, OVD_int32 tolerance_value);


#endif

