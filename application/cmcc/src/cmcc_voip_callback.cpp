#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include "third_party_device.h"
#include "cjson/cJSON.h"
#include "sys_logger.h"
#include "st_koks_uvc.h"
#include "st_koks_mediamanager.h"
#include "cmcc_voip_fun.h"
#include "cmcc_thread_pool.h"
#include "ovd.h"

using namespace st::sense_koks;

void (*voip_voice_status_changed)(int audio_index,int status) = nullptr;
int (*s_request_idr_func)() = nullptr;

void register_on_network_connect_func(void (*func)(int audio_index,int status)) {
    voip_voice_status_changed = func;
}

void register_voip_request_idr_callback(int (*request_idr_callback)())
{
    s_request_idr_func = request_idr_callback;
}

BlockingQueue<std::vector<uint8_t>> audio_queue(QUEUE_MAX);
BlockingQueue<std::vector<uint8_t>> video_queue(QUEUE_MAX);


static void *send_video_frame_thread(void *arg)
{
    std::vector<uint8_t> video_frame;

    while(VoipManager::getInstance().get_voip_flag_info().g_enable_video){
        if (video_queue.empty()) {
            usleep(1000 * 60);
            continue;
        }

        video_frame = video_queue.pop();
        //LOG_INFO("video_frame.size(): %d\n", video_frame.size());
        if(video_frame.size() > 0){
            cmcc_rtc_send_video(VoipManager::getInstance().get_voip_flag_info().g_call_session, (char*)video_frame.data(), video_frame.size());
        }
    }
}

static void create_send_video_thread() {
    LOG_INFO("================ create_send_video_thread ================\n");

    auto &voip_video_thread_info = VoipManager::getInstance().get_voip_thread_info(VOIP_VIDEO_THREAD);

    pthread_attr_init(&voip_video_thread_info.attr);
    pthread_attr_setstacksize(&voip_video_thread_info.attr, 1024*256);
    pthread_attr_setdetachstate(&voip_video_thread_info.attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&voip_video_thread_info.id, &voip_video_thread_info.attr, &send_video_frame_thread, NULL);

}

static void stop_send_video_thread() {
    LOG_INFO("================ stop_send_video_thread ================\n");
    auto &voip_video_thread_info = VoipManager::getInstance().get_voip_thread_info(VOIP_VIDEO_THREAD);
    pthread_join(voip_video_thread_info.id, NULL);
}

static int json_call_control_parse(const char *json_str, char *answer_type, int answer_type_size)
{
    if(json_str == NULL){
        LOG_ERROR("json str is null\n");
        return -1;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LOG_ERROR("Parse error at: %s\n", error_ptr);
        }
        return -1;
    }

    cJSON *value_obj = cJSON_GetObjectItem(root, "X-Call-Control");
    int ret = 0;
    if (cJSON_IsString(value_obj)) {
        LOG_INFO("value = %s\n", value_obj->valuestring);
        strncpy(answer_type, value_obj->valuestring, answer_type_size - 1);
    } else {
        ret = -1;
        LOG_ERROR("Value is not a string or key not found.\n");
    }

    cJSON_Delete(root);
    return ret;
}

void __on_login_success(const char *user) 
{
    auto &voip_flag = VoipManager::getInstance().get_voip_flag_info();

    LOG_INFO("================ __on_login_success ================\n");
    voip_flag.g_login_success = 1;
}

void __on_login_failed(const char *user, int err_code, const char *reason) 
{
    auto &voip_flag = VoipManager::getInstance().get_voip_flag_info();

    LOG_INFO("================ __on_login_failed ================\n");
    voip_flag.g_login_success = 0;
}

void __on_recv_call(int session, const char *from, const char *displayname, const char *to, cmcc_call_type_t call_type, const char *json_call_control) 
{

    LOG_INFO("__on_recv_call: session=%d, from=%s, displayname=%s, to=%s, call_type=%d, json_call_control=%s\n", session, from, displayname, to, call_type, json_call_control);
    char answer_type[64] = {0};

    if(json_call_control_parse(json_call_control, answer_type, 64) != 0){
        LOG_ERROR("json_call_control_parse return fail\n");
        return;
    }

    if(strcmp(answer_type, "answer_auto") == 0){
        voip_voice_status_changed(1,1);
        cmcc_rtc_setopt(CMCC_OPT_AUTO_ANSWER_FLAG,"1");
        if(cmcc_rtc_pickup(session) < 0)
        {
            LOG_ERROR("cmcc_rtc_pickup return fail\n");
            return;
        }

        VoipManager::getInstance().get_voip_flag_info().g_call_session = session;
        VoipManager::getInstance().get_voip_flag_info().g_calling = 1;
        VoipManager::getInstance().get_voip_flag_info().g_enable_audio = 1;
        if(VoipManager::getInstance().get_voip_flag_info().g_enable_video == 1 && call_type == CMCC_CALL_TYPE_IMS_1V1_VIDEO)
        {
            create_send_video_thread();
        }
    }
}

void __on_recv_ring(int session, const char *from, const char *displayname, const char *to, int early_media) {
    LOG_INFO("__on_recv_ring: early_media=%d\n", early_media);
    VoipManager::getInstance().get_voip_flag_info().g_call_session = session;
    voip_voice_status_changed(1,1); // turn on speaker
    VoipManager::getInstance().get_voip_flag_info().g_enable_audio = 1;
}


void __on_recv_answer(int session, const char *from, const char *displayname, const char *to, cmcc_call_type_t call_type) {

    LOG_INFO("session=%d, from=%s, displayname=%s, to=%s, call_type=%d\n", session, from, displayname, to, call_type);
    VoipManager::getInstance().get_voip_flag_info().g_calling = 1;
    
    if(VoipManager::getInstance().get_voip_flag_info().g_enable_video == 1&& call_type == CMCC_CALL_TYPE_IMS_1V1_VIDEO)
    {
        create_send_video_thread();
    }
}

void __on_recv_hangup(int session, int err_code, const char *reason, cmcc_call_type_t call_type){

    LOG_INFO("session=%d, err_code=%d, reason=%s, call_type=%d\n", session, err_code, reason, call_type);

    if(VoipManager::getInstance().get_voip_flag_info().g_enable_audio == 1) {
        voip_voice_status_changed(1,0); // turn off speaker
        VoipManager::getInstance().get_voip_flag_info().g_enable_audio = 0;
    }

    if(VoipManager::getInstance().get_voip_flag_info().g_enable_video  == 1)
    {
        LOG_INFO("stop send video thread\n");
        VoipManager::getInstance().get_voip_flag_info().g_enable_video = 0;
        stop_send_video_thread();
    }

    VoipManager::getInstance().get_voip_flag_info().isVoipTriggeredByButton = 0;
    VoipManager::getInstance().get_voip_flag_info().g_call_session = 0;
    VoipManager::getInstance().get_voip_flag_info().g_codec_type = 0;
    VoipManager::getInstance().get_voip_flag_info().g_calling = 0;
}

void __on_recv_notify(cmcc_notify_t notify_type, const char *content){

    switch(notify_type)
    {
        case CMCC_NOTIFY_UNBIND:
            LOG_INFO("recv unbind notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_BIND:
            LOG_INFO("recv bind notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_ENABLE:
            LOG_INFO("recv enable notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_DISABLE:
            LOG_INFO("recv disable notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_OPEN:
            LOG_INFO("recv open notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_CLOSE:
            LOG_INFO("recv close notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_CONTACT:
            LOG_INFO("recv contact notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_LOGOUT:
            LOG_INFO("recv logout notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_REMOTE_CONTROL:
            LOG_INFO("recv remote control notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_REMOTE_QUERY:
            LOG_INFO("recv remote query notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_VIDEO_CODEC:
            LOG_INFO("recv video codec notify, content=%s\n", content);
            if(s_request_idr_func != NULL) {
                s_request_idr_func();
            }
            if(strstr(content, "H264") != NULL){
                VoipManager::getInstance().get_voip_flag_info().g_codec_type = 1;
            }   
            else if(strstr(content, "H265") != NULL){
                VoipManager::getInstance().get_voip_flag_info().g_codec_type = 2;
            }
            else{
                LOG_ERROR("unknown codec type in notify content: %s\n", content);
            }
            LOG_INFO("currenr codec type: %d\n", VoipManager::getInstance().get_voip_flag_info().g_codec_type);
            g_monitorThread->notify();
            
            VoipManager::getInstance().get_voip_flag_info().g_enable_video = 1;
            break;
        case CMCC_NOTIFY_VIDEO_RESOLUTION:
            LOG_INFO("recv video resolution notify, content=%s\n", content);
            sscanf(content, "width=%d,height=%d", &VoipManager::getInstance().get_voip_flag_info().g_video_width, &VoipManager::getInstance().get_voip_flag_info().g_video_height);
            break;
        case CMCC_NOTIFY_VIDEO_CODEC_AND_RESOLUTION:
            LOG_INFO("recv video codec and resolution notify, content=%s\n", content);
            break;
        case CMCC_NOTIFY_SWITCH_TO_AUDIO:
            LOG_INFO("recv switch to audio notify, content=%s\n", content);
            break;
        default:
            LOG_INFO("recv unknown type %d notify, content=%s\n", notify_type, content);
            break;
    }
}

void __on_recv_keyframe_request(){
    LOG_INFO("%s(%d)\n", __FUNCTION__, __LINE__);
}

void __on_send_audio_pcm_data(char *pcm_data, int size){
    LOG_INFO("%s(%d)\n", __FUNCTION__, __LINE__);
}

void __on_recv_dtmf(int value)
{
    LOG_INFO("%s(%d)\n", __FUNCTION__, __LINE__);
}

void __on_recv_audio_packet(char *packet ,int size)
{
    LOG_INFO("%s(%d)\n", __FUNCTION__, __LINE__);
}

