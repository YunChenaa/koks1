#pragma once

#include <stdint.h>
#include "koks_project_def.h"

enum CMD_ID {
    CMD_ID_FW_VERSION = 0x01,
    CMD_ID_SERIAL_NO = 0x02,
    CMD_ID_STATION = 0x03,
    CMD_ID_UVC_SWITCH = 0x04,
    CMD_ID_CPUID = 0x05,
    CMD_ID_LICENSE = 0x06,
    CMD_ID_IRLED_SWITCH = 0x07,
    CMD_ID_CAMERA_CTRL = 0x08,
    CMD_ID_CAMERA_EXP_CTRL = 0x09,
    CMD_ID_FACTORY_MODE_SWITCH = 0x0A,
    CMD_ID_CAMERA_ISP_MODE = 0x0B,
    CMD_ID_CAMERA_CALIB_PARAM = 0x0C,
    CMD_ID_AUDIO_TEST = 0x0D,
    CMD_ID_UVC_STREAM = 0x0E,
    CMD_ID_UNKNOWN = 0xFF,
};

enum MSG_TYPE {
    MSG_TYPE_UNKNOWN = 0,
    MSG_TYPE_SET = 1,
    MSG_TYPE_GET = 2,
    MSG_TYPE_ACK = 3,
};

enum SWITCH_STATE {
    SWITCH_STATE_OFF = 0,
    SWITCH_STATE_ON = 1,
};

#pragma pack(push)
#pragma pack(1)
typedef struct {
    char serialNo[20];
    char serialNoLen;
} sn_params;

typedef struct {
    char total_num;
    char cur_station;
    bool is_total;
    char station[20];
    char result[5];
} station_params;

typedef struct {
    char sensor_type;
    bool switch_state;
} camera_params;

typedef struct {
    bool switch_state;
} irled_params;

typedef struct {
    char license[192];
    short licenseLen;
} license_params;

typedef struct {
    unsigned int exp_h;
    unsigned int exp_l;
    unsigned int gain;
} cameara_exp_params;

typedef struct {
    int mode;
} factory_mode_params;

typedef struct {
    int mode;
} camera_isp_mode_params;

typedef struct {
    float threshold;
} audio_test_params;

typedef enum {
    CAL_KL = 0x01,
    CAL_KR = 0x02,
    CAL_R = 0x04,
    CAL_T = 0x08,
    CAL_DL = 0x10,
    CAL_DR = 0x20,
    CAL_ALL = 0x3F,
}cal_type;

typedef struct{
    cal_type type;
    float param[10];
} camera_cal_params;

typedef struct {
    char mid;
    char type; // MSG_TYPE_SET, MSG_TYPE_GET
    union {
        sn_params sn;
        station_params station;
        camera_params camera;
        irled_params irled;
        license_params license;
        cameara_exp_params camera_exp;
        factory_mode_params factory_mode;
        camera_isp_mode_params camera_isp_mode;
        audio_test_params audio_test;
        camera_cal_params cal_params;
    } params;
} factory_req_t;

typedef struct {
    char mid;
    char type;     // MSG_TYPE_ACK
    char err_code; // 0: ok, 1: fail, 2: not implement
    union {
        char version[32];
        char sn[20];
        char chipid[32];
        char station[128];
        char license[192];
        int  isp_mode;
        float calib_val[9 + 9 + 9 + 3 + 5 + 5];
        unsigned short resolutions[2 * 3 * 2]; // 两个sensor 3组分辨率
    } data;
} factory_ack_t;

#pragma pack(pop)

extern volatile bool g_FactoryTest;

int factory_process(factory_req_t* req, factory_ack_t* ack);
int get_uvc_switch_mode(void);
void set_uvc_switch_mode(int mode);

int get_camera_isp_mode(void);
void set_camera_isp_mode(int mode);

#ifdef KOKS_PROJECT_M53D
int pcm_gateway_run(void);
#endif
