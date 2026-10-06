#pragma once

#include "OVD_define.h"
#include "st_koks_audio.h"

struct VideoFrameData {
    std::vector<uint8_t> data;
    bool is_key_frame;
    size_t length;
    int64_t timestamp;
};
typedef struct
{
    int (*getVideoStream)(OVDAVCodec stream_type, uint8_t* videoBuffer, bool &is_key_frame);
    int (*getAudioStream)(OVDAVCodec stream_type, uint8_t* audioBuffer, int bufferSize, IMPAudioFrame* dstFrame);
    int (*requestIDR)();
    int (*updateVideoQuality)(int quality);
}OVDCallbackList;

bool ovd_sdk_init();
void OVD_SetDeviceLoginInfo(char *deviceID, char *cmei, char *password, char *mac);
int ovd_run(OVDCallbackList *callbackList);
int ovd_stop(void);
void startAllThreads();
int GetHServerInfo(ovd_hserverinfo_t * hserverInfo);

int start_alarm_monitor(void);
int trigger_alarm(int alarm_type);
int GetSnapshot(uint8_t** out_buffer, int out_width, int out_height, bool (*get_image_data_func)(uint8_t** imagedata, int &width, int &height));
extern volatile int g_alarm_pending;

