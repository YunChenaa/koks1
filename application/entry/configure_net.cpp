#include "senselock.h"
#include "st_koks_mediamanager.h"
#include "st_koks_uvc.h"
#include "qrcode_scan.h"
#include "wifi.h"
#ifdef KOKS_FEATURE_OVD
#include "ovd.h"
#endif
#ifdef HAVE_FDK_AAC
#include "aac_encoder.h"
#endif
#ifdef CMCC_VOIP
#include "cmcc_voip_fun.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <sstream>    

static bool get_image_data(uint8_t** imagedata, int &width, int &height)
{
    IMPFrameInfo info;
    Senselock* sLock = Senselock::getInstance();
    *imagedata = sLock->mImage.first_image;
    width = sLock->mImage.width;
    height = sLock->mImage.height;
    auto& media_manager = st::sense_koks::MediaManager::GetInstance();
    auto ret = media_manager.GetDeviceFrameSource(1)->SnapFrame(*imagedata, &info);
    LOG_INFO("SnapFrame ret: %d, width: %d, height: %d\n", ret, width, height);
    return ST_ERRNO_SUCCESS == ret;
}

static void on_qrscan_success()
{
    //Play success sound or do other actions after QR code is successfully scanned
    char file_path[] = "/cmcc_fs/di.wav";
    auto& media_manager = st::sense_koks::MediaManager::GetInstance();
    auto audio_output = media_manager.GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);
    audio_output->DeInit();
    audio_output->SetSampleRate(16000);
    audio_output->ReInit();
    const int pcmLen = 16000 * 2 * 1 / 1000 * 20;
    unsigned char *buffer = (unsigned char *)malloc(pcmLen);
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("Failed to open audio file: %s\n", file_path);
        return;
    }
    int offset = lseek(fd, 44, SEEK_SET);
    if(offset < 0) {
        LOG_ERROR("Failed to seek audio file: %s\n", file_path);
        close(fd);
        return;
    }

    audio_output->Start();
    while (true) {
        int size = read(fd, buffer, pcmLen);
        if(size > 0) {
            IMPAudioFrame frm;
            frm.virAddr = (uint32_t *)buffer;
            frm.len = size;
            int ret = audio_output->SendFrame(frm);
            if (ret != 0) {
                LOG_ERROR("Failed to send audio frame\n");
            }
        }
        else {
            audio_output->Stop();
            break;
        }
    }
    audio_output->DeInit();
    audio_output->SetSampleRate(8000);
    audio_output->ReInit();
    free(buffer);
    close(fd);
}


#ifdef KOKS_FEATURE_OVD
static int getVideoStream(OVDAVCodec stream_type, uint8_t* videoBuffer, bool &is_key_frame)
{
    auto& videomanager = MediaUvc::GetInstance();
    if(stream_type == OVDAVCodec::source_STREAM_CODEC_H264) {
        return videomanager.MediaStreamGetVideo(VIDEO_STREAM_H264, videoBuffer, is_key_frame);
    }
    else if(stream_type == OVDAVCodec::source_STREAM_CODEC_H265) {
        return videomanager.MediaStreamGetVideo(VIDEO_STREAM_H265, videoBuffer, is_key_frame);
    }
    else {
        LOG_ERROR("Not support stream type %d\n", stream_type);
        return -1;
    }
}

static int copy_audio_frame(IMPAudioFrame *dest, const IMPAudioFrame *src) {

    dest->bitwidth   = src->bitwidth;
    dest->soundmode  = src->soundmode;
    dest->timeStamp  = src->timeStamp;
    dest->seq        = src->seq;
    dest->len        = src->len;

    if (src->len > 0 && src->virAddr != NULL) {
        dest->virAddr = (uint32_t*)malloc(src->len);
        if (!dest->virAddr) {
            return -1;
        }
        memcpy(dest->virAddr, src->virAddr, src->len);
        dest->phyAddr = 0;
    } else {
        dest->virAddr = NULL;
        dest->phyAddr = 0;
        dest->len = 0;
    }

    return 0;
}

static int getAudioStream(OVDAVCodec stream_type, uint8_t* audioBuffer, int bufferSize, IMPAudioFrame* dstFrame)
{
    if(stream_type == OVDAVCodec::source_STREAM_CODEC_AAC_WITH_ADTS) {
#ifdef HAVE_FDK_AAC
        int length = 0;
        auto& media_manager = st::sense_koks::MediaManager::GetInstance();
        auto dev_audio_in = media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
        IMPAudioFrame frame;
        auto ret = dev_audio_in->GetFrame(frame);
        if (ret != ST_ERRNO_SUCCESS) {
            LOG_ERROR("GetFrame failed\n");
            return 0;
        }
        ret = aac_encode((char*)frame.virAddr, frame.len, audioBuffer, bufferSize, &length);
        if(ret != 0) {
            LOG_ERROR("aac_encode failed\n");
        }
#ifdef CMCC_VOIP
        if(copy_audio_frame(dstFrame, &frame) != 0){
            LOG_ERROR("copy audio frame failed\n");
        }
#endif
        dev_audio_in->ReleaseFrame(frame);
        return length;
#else
        LOG_ERROR("AAC encoder not supported\n");
        return -1;
#endif
    }
    else {
        LOG_ERROR("Not support stream type %d\n", stream_type);
        return -1;
    }
}

static int requestIDR()
{
    return MediaUvc::GetInstance().SetFrameIDR();
}

extern int OVD_schedule_hibernate(IN OVD_int32 channel, IN OVD_int32 notAllowHibernate, IN OVD_int32 expired, IN OVDHibernateReason reason);
extern void set_host_keepalive();
static int updateVideoQuality(int quality)
{
    ovd_stop();
    auto& videomanager = MediaUvc::GetInstance();
    set_host_keepalive();
    OVD_schedule_hibernate(0, 0, 10, OVD_HIBERNATE_OVC_NOTIFY);
    if(quality == 1) {
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, false, 360, 640);
        usleep(10 * 1000);
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, true, 360, 640);
    }
    else if(quality == 2) {
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, false, 1080, 1920);
        usleep(10 * 1000);
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, true, 1080, 1920);
    }
    else {
        LOG_ERROR("Not support quality %d\n", quality);
        return -1;
    }

    startAllThreads();
    return 0;
}

void init_ovd_and_voip()
{
    if(!is_production_test_mode){
        auto& videomanager = MediaUvc::GetInstance();
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, false, 1080, 1920);
        if(!ovd_sdk_init()) {
            LOG_ERROR("ovd_sdk_init failed\n");
            return;
        }

        OVDCallbackList callbackList;
        videomanager.MediaStreamConfigVideo(VIDEO_STREAM_H265, true, 1080, 1920);
        auto& media_manager = st::sense_koks::MediaManager::GetInstance();
        media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0)->Start();
 #ifdef HAVE_FDK_AAC
        if(aac_encoder_init() != 0) {
            LOG_ERROR("aac_encoder_init failed\n");
            return;
        }
#endif
        LOG_INFO("Audio input device is ready\n");

        callbackList.getVideoStream = getVideoStream;
        callbackList.getAudioStream = getAudioStream;
        callbackList.requestIDR = requestIDR;
        callbackList.updateVideoQuality = updateVideoQuality;
        ovd_run(&callbackList);
#ifdef CMCC_VOIP
        register_voip_request_idr_callback(requestIDR);
        voip_run();
#endif
    }
}

/**
 * @brief 扫描 /cmcc_fs/ 目录，删除所有非当前 device_id 的 cfg_* 目录
 * @param device_id 当前设备 ID
 */
static void cleanup_other_device_cfg(const char* device_id)
{
    char current_prefix[64];
    snprintf(current_prefix, sizeof(current_prefix), "cfg_%s", device_id);

    DIR* dir = opendir("/cmcc_fs");
    if (!dir) {
        LOG_ERROR("Failed to open /cmcc_fs directory\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // 只处理 cfg_ 开头的条目
        if (strncmp(entry->d_name, "cfg_", 4) != 0) {
            continue;
        }
        // 跳过当前设备自己的目录
        if (strcmp(entry->d_name, current_prefix) == 0) {
            continue;
        }
        // 确认是目录后删除
        if (entry->d_type == DT_DIR) {
            char other_path[256];
            snprintf(other_path, sizeof(other_path), "/cmcc_fs/%s", entry->d_name);
            LOG_INFO("Removing stale device config folder: %s\n", other_path);

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "rm -rf %s", other_path);
            system(cmd);
        }
    }
    closedir(dir);
}

static void check_hiwd(char* device_id , char* HWID)
{
    if (!device_id || !HWID) {
        LOG_ERROR("check_hiwd: invalid parameters\n");
        return;
    }

    if(strlen(device_id) == 0 || strlen(HWID) == 0) {
        LOG_ERROR("check_hiwd: device_id or HWID is empty\n");
        return;
    }

    // 构建路径: /cmcc_fs/cfg_{device_id}/product.jsonc
    char dir_path[256]={0};
    char file_path[256]={0};
    snprintf(dir_path, sizeof(dir_path), "/cmcc_fs/cfg_%s", device_id);
    snprintf(file_path, sizeof(file_path), "%s/product.jsonc", dir_path);

    // 1. 读取 product.jsonc，若不存在则创建
    if (access(file_path, F_OK) != 0) {
        LOG_ERROR("product.jsonc not found: %s, creating...\n", file_path);
        mkdir(dir_path, 0755);

        json11::Json::object obj;
        obj["hwid"] = std::string(HWID);
        std::string content = json11::Json(obj).dump();

        FILE* fp = fopen(file_path, "w");
        if (fp) {
            fwrite(content.c_str(), 1, content.size(), fp);
            fclose(fp);
            LOG_INFO("Created product.jsonc with HWID: %s\n", HWID);
        } else {
            LOG_ERROR("Failed to create product.jsonc: %s\n", file_path);
        }
        return;
    }
    

    // 2. 解析 product.jsonc 文件，获取 hwid 字段的值
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        LOG_ERROR("Failed to open product.jsonc: %s\n", file_path);
        return;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    ifs.close();

    std::string err;
    json11::Json json = json11::Json::parse(buffer.str(), err);
    if (!err.empty()) {
        LOG_ERROR("Failed to parse product.jsonc: %s, err: %s\n", file_path, err.c_str());
        // 解析失败，走重写逻辑
    }

    // 3. 对比 hwid 字段的值和 HWID 是否一致
    std::string stored_hwid = json["hwid"].string_value();

    // 4. 如果不一致，打印错误日志，清空文件夹，写入正确的 hwid 值
    if (stored_hwid != std::string(HWID)) {
        LOG_ERROR("HWID mismatch: stored=[%s], expected=[%s]\n",
                  stored_hwid.c_str(), HWID);

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", dir_path);
        system(cmd);
        mkdir(dir_path, 0755);

        json11::Json::object obj;
        obj["hwid"] = std::string(HWID);
        std::string content = json11::Json(obj).dump();

        FILE* fp = fopen(file_path, "w");
        if (fp) {
            fwrite(content.c_str(), 1, content.size(), fp);
            fclose(fp);
            LOG_INFO("Updated product.jsonc with correct HWID: %s\n", HWID);
        } else {
            LOG_ERROR("Failed to write product.jsonc: %s\n", file_path);
        }
    } else {
        // 5. 如果一致，打印日志，表示 hwid 校验通过
        LOG_INFO("HWID verification passed: %s\n", HWID);
    }
}


static int get_hiwd(char* device_id , char* HWID)
{
    // 1. 读取 /cmcc_fs/cfg_{device_id}/product.jsonc 若不存在，打印错误日志, 退出
    // 2. 解析 product.jsonc 文件，{例子：{"hwid":"807e6654-5a54-306c-37ea-1b5639e71021"} }获取 hwid 字段的值
    // 3. 赋值给 HWID 参数，返回
    if (!device_id || !HWID) {
        LOG_ERROR("send_hiwd: invalid parameters\n");
        return -1;
    }

    if(strlen(device_id) == 0) {
        LOG_ERROR("send_hiwd: device_id is empty\n");
        return -1;
    }

    // 构建路径: /cmcc_fs/cfg_{device_id}/product.jsonc
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/cmcc_fs/cfg_%s/product.jsonc", device_id);

    if (access(file_path, F_OK) != 0) {
        LOG_ERROR("product.jsonc not found: %s\n", file_path);
        return -1;
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        LOG_ERROR("Failed to open product.jsonc: %s\n", file_path);
        return -1;
    }else
    {
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ifs.close();

        std::string err;
        json11::Json json = json11::Json::parse(buffer.str(), err);
        if (!err.empty()) {
            LOG_ERROR("Failed to parse product.jsonc: %s, err: %s\n", file_path, err.c_str());
            return -1;
        }

        std::string stored_hwid = json["hwid"].string_value();

        int len = stored_hwid.length();
        if (len >= 72) {
            LOG_ERROR("send_hiwd: stored_hwid is too long: %s\n", stored_hwid.c_str());
            return -1;
        }else
        {
            LOG_INFO("send_hiwd: stored_hwid: %s\n", stored_hwid.c_str());
            strncpy(HWID, stored_hwid.c_str(), len);
            HWID[len] = '\0'; // 确保字符串以 null 结尾 
        }
    }

    return 0;
}

static void OnConnectStatusChanged(bool connected)
{
    if (!connected) {
        LOG_INFO("Network disconnected\n");
        return;
    }
    LOG_INFO("Network connected\n");
}

static void get_device_info(char* device_id, char* cmei, char* secret_key, char*mac)
{
    //检查有没有其他device_id的文件夹，若有先删除其他devid 的文件夹
    cleanup_other_device_cfg(device_id);
    OVD_SetDeviceLoginInfo(device_id, cmei, secret_key, mac);
}
#endif

void configure_net_ovd()
{
    CallbackList callbackList;

    memset(&callbackList, 0, sizeof(CallbackList));
#ifdef KOKS_FEATURE_OVD
    callbackList.on_network_connected = OnConnectStatusChanged;
    callbackList.on_get_device_info = get_device_info;
    callbackList.on_hiwd_check = check_hiwd;
    callbackList.get_hiwd = get_hiwd;
#endif
    callbackList.get_image_data_func = get_image_data;
    callbackList.on_qrscan_success = on_qrscan_success;
    extern int OVD_updatechanelstateOnline();
    callbackList.OVD_updatechanelstateOnline = OVD_updatechanelstateOnline;
    extern int OVD_updatechanelstateOffline();
    callbackList.OVD_updatechanelstateOffline = OVD_updatechanelstateOffline;

    
    register_callbacks(&callbackList);

    std::thread configNet([]() {
        int try_count = 0;
        while (!fasync_start() && try_count++ < 5) {
            LOG_ERROR("fasync_start failed\n");
            sleep(1);
        }
    });
    configNet.detach();
}
