#include "OVD_define.h"
#include "OVD_OpenAPI.h"
#include "sys_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#define CMD_HOSTSLEEP 0x17
#define CMD_KEEP_HOSTALIVE 0x24

static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static timer_t timer_id = 0;
static int timer_initialized = 0;
static int timer_cancelled = 0;
static struct timespec target_time;
static int pending = 0;
static int last_channel;
static int last_notAllowHibernate;
static OVDHibernateReason last_reason;

int GetHServerInfo(ovd_hserverinfo_t * hserverInfo)
{
    int ret = 0;
    ret = OVD_GetHServerInfo(hserverInfo);
    LOG_INFO("DEV/OVD GetHServerInfo, ret: %d\n", ret);
    if(ret != 0)
    {
        LOG_ERROR("DEV/OVD Get Sleep server info failed, ret: %d\n", ret);
        return ret;
    }
    LOG_INFO("DEV/OVD Sleep server domain: %s, hport: %d, hb_interval: %d, hb_token: %s\n",
            hserverInfo->hdomain, hserverInfo->hport, hserverInfo->hb_interval, hserverInfo->hb_token);
    return ret;
}

static ovd_channel_state last_channel_state = OVD_CHANNEL_UNUSE;


static void execute_command(int channel, int notAllowHibernate, OVDHibernateReason reason) {
    LOG_INFO("DEV/OVD Executing command after delay, channel: %d, notAllowHibernate: %d, reason: %d\n",
            channel, notAllowHibernate, reason);
  

    char cmdBuffer[512] = { 0 };
    snprintf(cmdBuffer, sizeof(cmdBuffer), "custom_msg vnet0 30 %d", CMD_HOSTSLEEP);
    system(cmdBuffer);
    //
}

int OVD_updatechanelstateOffline()
{
    LOG_INFO("DEV/OVD Updating channel state to sleep, channel: %d\n", last_channel);
    if(last_channel < 0) {
        LOG_ERROR("DEV/OVD Invalid channel: %d\n", last_channel);
        return -1;
    }

    OVD_UpdateChannelState(last_channel_state, OVD_CHANNEL_DISABLE);
    OVD_updateOVDstate(OVD_LOWPOWER_OFFLINESLEEP);

    last_channel_state = OVD_CHANNEL_DISABLE;
}   

int OVD_updatechanelstateOnline() {

    if(last_channel_state!= OVD_CHANNEL_DISABLE) {
        LOG_INFO("DEV/OVD Channel state is not disable, skipping update to online, last_channel_state: %d\n", last_channel_state);
        return 0;
    }

    LOG_INFO("DEV/OVD Updating channel state to online, channel: %d\n", last_channel);
    if(last_channel < 0) {
        LOG_ERROR("DEV/OVD Invalid channel: %d\n", last_channel);
        return -1;
    }
    int ret = 0; 
    ret = OVD_UpdateChannelState(last_channel, OVD_CHANNEL_ONLINE);
    if(ret != 0) {
        LOG_ERROR("DEV/OVD Failed to update channel state to online, ret: %d\n", ret);
        return ret;
    }
    ret = OVD_updateOVDstate(OVD_ONLINE);
    if(ret != 0) {
        LOG_ERROR("DEV/OVD Failed to update OVD state to online, ret: %d\n", ret);
        return ret;
    }
    last_channel_state = OVD_CHANNEL_ONLINE;
    return 0;
}


static void timer_callback(union sigval sv) {
    pthread_mutex_lock(&timer_mutex);
    if (!pending) {
        pthread_mutex_unlock(&timer_mutex);
        return;
    }
    pending = 0;
    int channel = last_channel;
    int notAllowHibernate = last_notAllowHibernate;
    OVDHibernateReason reason = last_reason;
    pthread_mutex_unlock(&timer_mutex);
    execute_command(channel, notAllowHibernate, reason);
}

static int setup_timer(void) {
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_callback;
    sev.sigev_notify_attributes = NULL;
    return timer_create(CLOCK_REALTIME, &sev, &timer_id);
}

static int set_timer_absolute(const struct timespec *abstime) {
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    its.it_value = *abstime;
    return timer_settime(timer_id, TIMER_ABSTIME, &its, NULL);
}

void set_host_keepalive() {
    char cmdBuffer[128] = { 0 };
    snprintf(cmdBuffer, sizeof(cmdBuffer), "custom_msg vnet0 30 %d", CMD_KEEP_HOSTALIVE);
    system(cmdBuffer);
}

void OVD_cancel_schedule_hibernate()
{
    pthread_mutex_lock(&timer_mutex);
    timer_cancelled = 1;
    if(timer_id > 0) {
        timer_delete(timer_id);
    }
    LOG_INFO("DEV/OVD OVD_cancel_schedule_hibernate: timer has been cancelled\n");
    pthread_mutex_unlock(&timer_mutex);
}

int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason)
{
    pthread_mutex_lock(&timer_mutex);
    if(timer_cancelled) {
        pthread_mutex_unlock(&timer_mutex);
        LOG_INFO("DEV/OVD OVD_schedule_hibernate: timer has been cancelled, skipping scheduling\n");
        return -1;
    }

    if (!timer_initialized) {
        if (setup_timer() != 0) {
            pthread_mutex_unlock(&timer_mutex);
            perror("timer_create");
            return -1;
        }
        timer_initialized = 1;
    }

    // 2. 计算新的绝对触发时间
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    if(target_time.tv_sec < now.tv_sec + expired) {
        target_time = now;
        target_time.tv_sec += expired;
        if (!pending) {
            pending = 1;
        }

        // 3. 保存参数，供回调函数使用
        last_channel = channel;
        last_notAllowHibernate = notAllowHibernate;
        last_reason = reason;

        // 4. 设置（或重置）定时器到新的绝对时间
        // 注：timer_settime 会原子地取消之前的任何未决定时器
        int ret = set_timer_absolute(&target_time);
        // set_host_keepalive();
        LOG_INFO("DEV/OVD OVD_schedule_hibernate, expired: %d\n", expired);
        pthread_mutex_unlock(&timer_mutex);
        return ret;
    }
    pthread_mutex_unlock(&timer_mutex);
    return 0;
}
