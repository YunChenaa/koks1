#include "qrcode_scan.h"
#include "sys_logger.h"
#include <zbar.h>

static int parseCompactString(const std::string& input, WifiConfig &config) {
    size_t pos = 0;
    const size_t len = input.size();

    if(len <= pos){
        LOG_ERROR("Input string is empty\n");
        return -1;
    }

    config.ssid.clear();
    config.password.clear();
    config.extra.clear();

    while (pos < len) {
        if (!std::isdigit(static_cast<unsigned char>(input[pos]))) {
            //LOG_ERROR("Expected digit at position " + std::to_string(pos));
            return -1;
        }

        size_t num_start = pos;
        while (pos < len && std::isdigit(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }

        int segment_len = std::stoi(input.substr(num_start, pos - num_start));
        if (segment_len < 2) {
            LOG_ERROR("Segment length must be at least 2 (type + '=')");
            return -1;
        }

        if (pos + segment_len > len) {
            // LOG_ERROR("Not enough characters for segment of length " + std::to_string(segment_len));
            return -1;
        }

        std::string segment = input.substr(pos, segment_len);
        pos += segment_len;
        char type = segment[0];
        if (segment[1] != '=') {
            // LOG_ERROR("Expected '=' in segment, got '" + std::string(1, segment[1]) + "'");
            return -1;
        }

        std::string value = segment.substr(2);
        switch (type) {
            case 's': config.ssid = value; break;
            case 'p': config.password = value; break;
            case 'e': config.extra = value; break;
            default:
                LOG_ERROR("Unknown field type");
        }
    }

    if(!config.ssid.empty() && !config.extra.empty()){
        LOG_INFO("Parsed QR Code | SSID: %s password: %s extra: %s\n", config.ssid.c_str(), config.password.c_str(), config.extra.c_str());
        return 0;
    } else {
        LOG_ERROR("SSID and extra fields are required");
        return -1;
    }

}

static int scan_QRCode(uint8_t* image_in, int width, int height, WifiConfig &msg)
{
    if (image_in == nullptr || width <= 0 || height <= 0) {
        // LOG_ERROR("Invalid NV12 frame parameters\n");
        return -1;
    }

    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);

    zbar::Image zbar_image(width, height, "Y800", image_in, width * height);
    int n = scanner.scan(zbar_image);
    if (n == 0) {
        // LOG_ERROR("QR code scan fail\n");
        return -1;
    }

    for (auto symbol = zbar_image.symbol_begin();
         symbol != zbar_image.symbol_end();
         ++symbol) {
        std::string type = symbol->get_type_name();
        std::string data = symbol->get_data();

        if(type == "QR-Code")
        {
            LOG_INFO("QR Code found | Type: %s | Data: %s\n", type.c_str(), data.c_str());
            if(parseCompactString(data.c_str(), msg) == 0){
                return 0;
            }
        }
    }
    return -1;
}

int qr_code_runing_flag = 1;


int get_qrcode(WifiConfig &msg, bool (*get_image_data_func)(uint8_t** imagedata, int &width, int &height), void (play_success_sound)(void))
{
    uint8_t* imagedata = nullptr;
    int width = 0, height = 0;
    if(get_image_data_func == nullptr) {
        LOG_ERROR("get_image_data_func is not registered\n");
        return -1;
    }

    do {
        if(get_image_data_func(&imagedata, width, height)) {
            if(scan_QRCode(imagedata, width, height, msg) == 0) {
                if(play_success_sound != nullptr) {
                    LOG_INFO("QR Code scan successful, playing success sound\n");
                    play_success_sound();
                }
                break;
            }
        } else {
            LOG_ERROR("Failed to get image data\n");
        }
    } while (qr_code_runing_flag);
}


int stop_qrcode_scan()
{
    qr_code_runing_flag = 0;
    return 0;
}