#include "cmcc_voip_fun.h"
#include "cmcc_thread_pool.h"
#include "sys_logger.h"
#include "st_koks_mediamanager.h"
#include "st_koks_uvc.h"
#include "third_party_device.h"
#include "ovd_data.h"

#include <stdio.h>
#include <fcntl.h>

using namespace st::sense_koks;

VoipManager::VoipManager()
{
    voip_flag = {
        .g_call_session = 0,
        .g_calling = 0,
        .g_login_success = 0,
        .g_enable_video = 0,
        .g_enable_audio = 0,
        .g_call_type = CMCC_CALL_TYPE_IMS_1V1_AUDIO,
        .g_codec_type = 0,
        .g_video_width = 0,
        .g_video_height = 0,
        .g_video_resolution_changed = 0,
        .isVoipTriggeredByButton = 0,
    };

    device_info = {
        .os_version             = (char*) "3.10.14-Zeratul-Archon",
        .image_version          = (char*) "2.3.54",
        .hardware_version       = (char*) "2.1",
        .device_model           = (char*) "DoorLock|DESMAN|Linux",
        .sn                     = NULL,
        .cmei                   = NULL,
        .mac                    = NULL,
        .chip_model             = (char*) "",
        .chip_uuid              = (char*) "",
        .chip_brand             = (char*) "",
    };

    log_config = {
        .log_size           = 1024 * 1024,
        .console_log_enable = 1,
        .log_path           = (char*) "/tmp",
    };

    login_params = {
        .app_key        = (char*) "yenppvrgkaxtkymk",
        .app_secret     = (char*) "j21dztt1457y8xjg",
        .device_id      = NULL,
    };

    event_handler = {
        .on_login_success           = __on_login_success,
        .on_login_failed            = __on_login_failed,
        .on_recv_call               = __on_recv_call,
        .on_recv_ring               = __on_recv_ring,
        .on_recv_answer             = __on_recv_answer,
        .on_recv_hangup             = __on_recv_hangup,
        .on_recv_notify             = __on_recv_notify,
        .on_recv_keyframe_request   = __on_recv_keyframe_request,
        .on_recv_dtmf               = __on_recv_dtmf,
#ifdef SDK_INCLUDE_THIRD_PARTY_DEVICE
        .on_recv_audio_pcm_data     = __on_recv_audio_pcm_data,
        .on_send_audio_pcm_data     = __on_send_audio_pcm_data,
#else
        .on_recv_audio_pcm_data     = NULL,
        .on_send_audio_pcm_data     = NULL,
#endif
#ifndef ENABLE_AUDIO_CODEC
        .on_recv_audio_packet       = __on_recv_audio_packet,
#else
        .on_recv_audio_packet       = NULL,
#endif  
    };
}

VoipManager& VoipManager::getInstance() {
    static VoipManager instance;
    return instance;
}

cmcc_rtc_device_info_t& VoipManager::get_voip_device_info(){
    return device_info;
}

cmcc_rtc_log_config_t& VoipManager::get_voip_log_config(){
    return log_config;
}

cmcc_rtc_login_params_t& VoipManager::get_voip_login_params(){
    return login_params;
}

cmcc_rtc_event_handler_t& VoipManager::get_voip_event_handler(){
    return event_handler;
}

voip_flag_t & VoipManager::get_voip_flag_info()
{
    return voip_flag;
}

voip_thread_info_t &VoipManager::get_voip_thread_info(voip_thread_type_t thread_type)
{
    return voip_thread_info[thread_type];
}


static void voip_uac_status_callback(int audio_index,int status)
{
    LOG_INFO("voip_uac_status_callback, status=%d\n", status);

    auto& media_manager = MediaManager::GetInstance();

    if(audio_index == 0)
    {
        if(status == 1)
        {
            media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Start();
            LOG_INFO("Audio input device is ready\n");
        }
        else
        {
            LOG_INFO("Audio device is not ready\n");
            media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Stop();
        }
    }
    else if(audio_index == 1)
    {
        if(status == 1)
        {
            LOG_INFO("Audio output device is ready\n");
            media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Start();
        }
        else
        {
            LOG_INFO("Audio output device is not ready\n");
            media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0)->Stop();
        }
    }
}


int VOIP_GetDeviceInfo(void)
{
    OVDLoginInfo OVD_LoginInfo;
    int fd = open("/cmcc_fs/LogInfo.bin", O_RDONLY);
    if (fd < 0) {
        LOG_ERROR( "VOIP_GetDeviceInfo, open login info file failed\n");
        return -1;
    }

    if (read(fd, &OVD_LoginInfo, sizeof(OVD_LoginInfo)) != sizeof(OVD_LoginInfo)) {
        LOG_ERROR("read login info failed\n");
        close(fd);
        return -1;
    }

    LOG_INFO("Set device login info: device_id=%s, cmei=%s\n", OVD_LoginInfo.deviceid, OVD_LoginInfo.cmei);
    auto &voip_device_params = VoipManager::getInstance().get_voip_device_info();
    
    free(voip_device_params.sn);
    voip_device_params.sn = strdup(OVD_LoginInfo.deviceid);  
    if(voip_device_params.sn == NULL) {
        LOG_ERROR("Failed to allocate memory for sn\n");
        voip_device_params.sn = NULL;
        return -1;
    }

    free(voip_device_params.cmei);
    voip_device_params.cmei = strdup(OVD_LoginInfo.cmei);
    if(voip_device_params.cmei == NULL) {
        LOG_ERROR("Failed to allocate memory for cmei\n");
        voip_device_params.cmei = NULL;
        return -1;
    }

    free(voip_device_params.mac);
    voip_device_params.mac = strdup(OVD_LoginInfo.mac);
    if(voip_device_params.mac == NULL) {
        LOG_ERROR("Failed to allocate memory for mac\n");
        voip_device_params.mac = NULL;
        return -1;
    }

    auto &login_params = VoipManager::getInstance().get_voip_login_params();
    const char* prefix = "cmcc-592282-";
    size_t len = strlen(prefix) + strlen(voip_device_params.sn) + 1;
    login_params.device_id = (char*)malloc(len);
    if (login_params.device_id == NULL) {
        LOG_ERROR("Memory allocation failed for device_id");
        return -1;
    }
    snprintf(login_params.device_id, len, "%s%s", prefix, voip_device_params.sn);

    return 0;
}

static int voip_init(void)
{
    auto &voip_devinfo =  VoipManager::getInstance().get_voip_device_info();
    auto &voip_log_config = VoipManager::getInstance().get_voip_log_config();  
    auto &voip_event_handler = VoipManager::getInstance().get_voip_event_handler();

    if(VOIP_GetDeviceInfo() != 0)
    {
        LOG_ERROR("failed to get device info\n");
        return -1;
    }

    if(cmcc_rtc_init(&voip_devinfo, &voip_log_config, &voip_event_handler) != 0)
    {
        LOG_ERROR("voip info init fail\n");
        return -1;
    }

    register_on_network_connect_func(voip_uac_status_callback);
    return 0;
}


static int voip_config_setting(void)
{
    if(cmcc_rtc_setopt(CMCC_OPT_DISENABLE_CONTACTS, "1") != 0)
    {
        LOG_ERROR("disable contacts setting fail\n");
        return -1;
    }

    if(cmcc_rtc_setopt(CMCC_OPT_ENABLE_H265,"1") != 0)
    {
        LOG_ERROR("H265 setting fail\n");
        return -1;
    }

    if(cmcc_rtc_setopt(CMCC_OPT_VIDEO_RESOLUTION,"hd") != 0)
    {
        LOG_ERROR("H265 setting fail\n");
        return -1;
    }

    if(cmcc_rtc_setopt(CMCC_OPT_PORTRAIT,"1") != 0)
    {
        LOG_ERROR("H265 setting fail\n");
        return -1;
    }

    char log_file[256] = {0};
    snprintf(log_file, sizeof(log_file), "%s/voip.txt", VoipManager::getInstance().get_voip_log_config().log_path);
    if(cmcc_rtc_setopt(CMCC_OPT_LOG_PATH, log_file) != 0)
    {   
        LOG_ERROR("set log path fail\n");
        return -1;
    }

    return 0;
}

static int voip_login_device(void)
{
    int i=0;
    auto &voip_login_params = VoipManager::getInstance().get_voip_login_params();
    do {
        if(cmcc_rtc_login(&voip_login_params) == 0)
            break;

        LOG_ERROR("failed to invoke cmcc_rtc_login interface\n");
        usleep(1000 * 1000* 10);
        i++;
    }while(i<5);

    if(i == 5){
        LOG_ERROR("login webrtc service fail\n");
        return -1;
    }

    return 0;
}

static int isNetworkConnected(const std::string& target = "8.8.8.8", int count = 1, int timeout = 2) 
{

    static uint8_t isConnected = 0;
    if(isConnected == 1) {
        return 0;
    }

    std::string cmd = "ping -c " + std::to_string(count) + 
                      " -W " + std::to_string(timeout) + 
                      " " + target + " 2>&1";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    
    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    

    if (output.find("1 received") != std::string::npos ||
        output.find("0% packet loss") != std::string::npos ||
        output.find("64 bytes from") != std::string::npos) {
        pclose(pipe);
        isConnected = 1;
        return 0;
    }

    pclose(pipe);
    return -1;
}

static int voip_voice_call(void)
{
    LOG_INFO( "start voip voice\n");
    if(VoipManager::getInstance().get_voip_flag_info().g_calling == 0)
    {
        LOG_INFO( "start voip voice call\n");
        VoipManager::getInstance().get_voip_flag_info().g_calling = 1;
        if(cmcc_rtc_callout(LOCK_NUMBER, CMCC_CALL_TYPE_IMS_1V1_AUDIO) < 0)
        {
            LOG_ERROR("cmcc_rtc_callout fail\n");
            return -1;
        }
    }
    else 
    {
        LOG_ERROR("already in calling\n");
        return -1;
    }

    return 0;
}

static int voip_video_call(void)
{
    //LOG_INFO( "start voip video call\n");
    if(VoipManager::getInstance().get_voip_flag_info().g_calling == 0)
    {
        //LOG_INFO( "start voip video call\n");
        VoipManager::getInstance().get_voip_flag_info().g_calling = 1;
        if(cmcc_rtc_callout(LOCK_NUMBER, CMCC_CALL_TYPE_IMS_1V1_VIDEO) < 0)
        {
            LOG_ERROR("cmcc_rtc_callout fail\n");
            return -1;
        }
    }
    else 
    {
        LOG_ERROR("already in calling\n");
        return -1;
    }

    return 0;
}

static void voip_fini(void)
{
    cmcc_rtc_fini();
}

static ManagedThread voip_thread;
static void voip_task(std::atomic<bool>& stop)
{
    LOG_INFO("%s(%d) start create voip main thread\n", __FUNCTION__, __LINE__);

    if (stop) {
        LOG_INFO("VoIP thread stopped before network ready\n");
        return;
    }

    // ---------- 2. 执行初始化（仅一次） ----------
    if (voip_init() != 0) {
        LOG_ERROR("voip init fail\n");
        return;
    }

    if (voip_config_setting() != 0) {
        LOG_ERROR("voip config setting fail\n");
        voip_fini();
        return;
    }

    bool is_logged_in = false;
    LOG_INFO("VoIP initialized and logged in, entering service loop\n");
    while (!stop) {
        if (isNetworkConnected() == 0) {
            if (!is_logged_in) {
                auto& flag_info = VoipManager::getInstance().get_voip_flag_info();
                if (flag_info.g_login_success == 0 && voip_login_device() == -1) {
                    LOG_ERROR("voip_login_device fail\n");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    continue;
                }

                is_logged_in = true;
                LOG_INFO("VoIP logged in successfully\n");
            }

            auto& flag_info = VoipManager::getInstance().get_voip_flag_info();
            if (flag_info.isVoipTriggeredByButton == 1 && flag_info.g_calling == 0
                    && flag_info.g_login_success == 1) {
                LOG_INFO("VoipTriggeredByButton event\n");
                flag_info.isVoipTriggeredByButton = 0;
                if (voip_video_call() == -1) {
                    LOG_ERROR("voip_video_call fail\n");
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }else{
            //LOG_INFO("Network not connected, waiting...\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // ---------- 4. 清理 ----------
    voip_fini();
    LOG_INFO("VoIP main thread exited\n");
}



void voip_run(void)
{
    voip_thread = ManagedThread([](std::atomic<bool>& stop) {
        voip_task(stop);
    });

    voip_thread.reset();
    voip_thread.startDetached();
}


void voip_stop(void)
{
    voip_thread.stop();
    LOG_INFO("%s(%d)\n", __FUNCTION__, __LINE__);
}