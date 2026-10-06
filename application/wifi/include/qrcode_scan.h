#pragma once

#include <string>
#include <cstdint>

typedef struct _WifiConfig {
    std::string ssid;
    std::string password;
    std::string extra;
}WifiConfig;

int get_qrcode(WifiConfig &msg, bool (*get_image_data_func)(uint8_t** imagedata, int &width, int &height), void (play_success_sound)(void));

int stop_qrcode_scan();