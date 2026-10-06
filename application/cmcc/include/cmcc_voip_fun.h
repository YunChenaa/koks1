#pragma once

#include "cmcc_rtc_api.h"
#include <pthread.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

#define LOCK_NUMBER "88886668888"
#define VIDEO_DATA_SIZE 640 * 360

#define true 1
#define false 0

typedef struct{
    int g_call_session;
    int g_calling;
    int g_login_success;
    int g_enable_video;
    int g_enable_audio;
    int g_call_type;
    int g_codec_type;
    int g_video_width;
    int g_video_height;
    bool g_video_resolution_changed;
    bool isVoipTriggeredByButton;
}voip_flag_t;

typedef struct{
    pthread_t id;
    pthread_attr_t attr;
}voip_thread_info_t;

typedef enum {
    VOIP_MAIN_THREAD,
    VOIP_AUDIO_THREAD,
    VOIP_VIDEO_THREAD,
    VOIP_MAX_THREAD
}voip_thread_type_t;

class VoipManager{
public:
    VoipManager();
    static VoipManager& getInstance();

    int voip_init(void);

    voip_flag_t &get_voip_flag_info();
    cmcc_rtc_device_info_t &get_voip_device_info();
    cmcc_rtc_log_config_t &get_voip_log_config();
    cmcc_rtc_login_params_t &get_voip_login_params();
    cmcc_rtc_event_handler_t &get_voip_event_handler();


    voip_thread_info_t &get_voip_thread_info(voip_thread_type_t thread_type);
private:
    voip_flag_t voip_flag;
    cmcc_rtc_device_info_t device_info;
    cmcc_rtc_log_config_t log_config;
    cmcc_rtc_login_params_t login_params;
    cmcc_rtc_event_handler_t event_handler;

    voip_thread_info_t voip_thread_info[VOIP_MAX_THREAD];
};

void __on_login_success(const char *user);
void __on_login_failed(const char *user,int err_code,const char *reason);
void __on_recv_call(int session, const char *from, const char *displayname, const char *to, cmcc_call_type_t call_type, const char *json_call_control);
void __on_recv_ring(int session, const char *from, const char *displayname, const char *to, int early_media);
void __on_recv_answer(int session, const char *from, const char *displayname, const char *to,cmcc_call_type_t call_type);
void __on_recv_hangup(int session,int err_code, const char *reason, cmcc_call_type_t call_type);
void __on_recv_notify(cmcc_notify_t notify_type, const char *content);
void __on_recv_keyframe_request();
void __on_send_audio_pcm_data(char *pcm_data, int size);
void __on_recv_dtmf(int value);
void __on_recv_audio_packet(char *packet ,int size);

void voip_run(void);
void voip_stop(void);
void register_voip_request_idr_callback(int (*request_idr_callback)());
