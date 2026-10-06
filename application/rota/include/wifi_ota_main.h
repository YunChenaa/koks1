#ifndef WIFI_OTA_MAIN_H
#define WIFI_OTA_MAIN_H

#include <stdint.h>

#define CDEV_BUF_MAX            2048
#define WLAN_SET_LEN            8
#define CUST_CMD_CONNECT_IND    0x3003  // rwnx_main.h
#define CUST_CMD_COMMAND_IND    0x3044
#define CUSTMO_MSG_LOCK_EVENT_IND 0x3200
#define CUSTMO_MSG_PIR_EVENT_IND 0x3201
#define APP_NAME                "[fasync] "


typedef struct {
    int mem_status;
    char mem[CDEV_BUF_MAX];
} rwnx_fasync_info_t;



#ifdef __cplusplus
extern "C" {
#endif
int main(int argc, char* argv[]);
int fasync_start();
int vnet_send_msg(int cmd, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // WIFI_OTA_MAIN_H