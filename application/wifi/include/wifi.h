#pragma once
#include <stdint.h>

#define CDEV_BUF_MAX            2048
#define WLAN_SET_LEN            8
#define CUST_CMD_CONNECT_IND    0x3003  // rwnx_main.h
#define CUST_CMD_COMMAND_IND    0x3044
#define CUSTMO_MSG_LOCK_EVENT_IND 0x3200
#define CUSTMO_MSG_PIR_EVENT_IND 0x3201
#define APP_NAME                "[fasync] "

enum CMD_EVENT_TYPE {
    CMD_EVENT_STAY = 0, //00 - 逗留通知
    CMD_EVENT_BELL = 1, //01 - 门铃通知
    CMD_EVENT_TAMPER = 2, // 02 - 防撬告警
    CMD_EVENT_PROHIBIT = 3, //03 - 禁试告警
    CMD_EVENT_CLOSESCREEN = 4,//04 - 关闭后屏
    CMD_EVENT_OPENSCREEN = 5, //05 - 开屏通知
};

typedef struct {
    int mem_status;
    char mem[CDEV_BUF_MAX];
} rwnx_fasync_info_t;

typedef struct _callback {
    void (*on_network_connected)(bool connected);
    void (*on_get_device_info)(char* device_id, char* cmei, char* secret_key, char *mac);
    bool (*get_image_data_func)(uint8_t** imagedata, int &width, int &height);
    void (*on_qrscan_success)(void);
    void (*on_hiwd_check)(char* device_id, char *hiwd);
    int (*get_hiwd)(char* device_id, char *hiwd);
    int (*OVD_updatechanelstateOnline)();
    int (*OVD_updatechanelstateOffline)();
}CallbackList;

extern CallbackList callbackList;

bool fasync_start();
void register_callbacks(CallbackList *callbacks);


#ifdef __cplusplus
extern "C" {
#endif

int vnet_send_msg(int cmd, const char* fmt, ...);
extern bool is_production_test_mode;

#ifdef __cplusplus
}
#endif