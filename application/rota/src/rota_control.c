/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_control.h"
#include "message_v2.h"
#include "rota_common.h"
#include "rota_image.h"
#include "rota_md5.h"
#include "rota_uart.h"
#include "rota_update.h"
#include "rota_utils.h"

#include <errno.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

#define MAX_RECEIVE_DATA_SIZE 4112
#define MAX_REPLY_DATA_SIZE 260
#define MAX_NOTE_DATA_SIZE 260

static uint8_t* g_rcvBuf[2] = { 0 };
static uint16_t g_rcvNum[2] = { 0 };
static volatile uint8_t g_lockState;

static uint8_t checksum(uint8_t* buff, int size)
{
    uint8_t result = 0;
    for (int i = 0; i < size; i++) {
        result ^= buff[i];
    }

    return result;
}

static uint8_t rota_lock_getState()
{
    return g_lockState;
}

static void rota_lock_setState(uint8_t state)
{
    g_lockState = state;
}

static void rota_lock_reply(uint8_t mid, uint8_t result, void* data, uint16_t dataSize, s_type  type)
{
    uint8_t buf[MAX_REPLY_DATA_SIZE] = { 0xEF, 0xAA, 0x00 };
    uint16_t totalLen = dataSize + 8;
    if (totalLen > sizeof(buf)) {
        LOG_ERROR("uart send buf not enough: %u\n", totalLen);
        return;
    }

    ROTA_PUTU16(&buf[3], dataSize + 2);
    buf[5] = mid;
    buf[6] = result;
    if (data && dataSize > 0) {
        memcpy(&buf[7], data, dataSize);
    }
    buf[totalLen - 1] = checksum(&buf[2], totalLen - 3);
    rota_uart_send(buf, totalLen, (type == TYPE_CDC));
}

static void rota_lock_sendMessage(uint8_t mid, void* data, uint16_t dataSize, s_type type)
{
    uint8_t buf[MAX_REPLY_DATA_SIZE] = { 0xEF, 0xAA, 0x01 };
    uint16_t totalLen = dataSize + 7;
    if (totalLen > sizeof(buf)) {
        LOG_ERROR("uart send buf not enough: %u\n", totalLen);
        return;
    }

    ROTA_PUTU16(&buf[3], dataSize + 1);
    buf[5] = mid; // nid
    if (data && dataSize > 0) {
        memcpy(&buf[6], data, dataSize);
    }
    buf[totalLen - 1] = checksum(&buf[2], totalLen - 3);
    rota_uart_send(buf, totalLen, (type == TYPE_CDC));
}

static void handleConfigBaudrate(s_msg* msg, s_type type)
{
    LOG_INFO("%s, type %d\n", __func__, type);

    s_note_config_baudrate* config_baudrate_t = (s_note_config_baudrate*)(msg->data);
    uint8_t last_state = MS_STANDBY;
    if (g_lockState == MS_OTA) {
        last_state = MS_OTA;
    }
    g_lockState = MS_BUSY;
    int baudrate = -1;
    const int BAUDRATE_MAP[] = { 0, 115200, 230400, 460800, 1500000 };

    if (config_baudrate_t->baudrate_index > sizeof(BAUDRATE_MAP) - 1) {
        LOG_ERROR("Not support this baudrate %u\n", config_baudrate_t->baudrate_index);
        rota_lock_reply(MID_CONFIG_BAUDRATE, MR_FAILED4_INVALIDPARAM, NULL, 0, type);
    } else {
        baudrate = BAUDRATE_MAP[config_baudrate_t->baudrate_index];
        LOG_INFO("Set baudrate to %d\n", baudrate);
        rota_lock_reply(MID_CONFIG_BAUDRATE, MR_SUCCESS, NULL, 0, type);
        usleep(20 * 1000);
        rota_uart_set_baudrate(baudrate);
    }

    if (last_state == MS_OTA) {
        g_lockState = MS_OTA;
    } else {
        g_lockState = MS_STANDBY;
    }
}

static void handlGetVersion(s_type type)
{
    LOG_INFO("Handle MID_GET_VERSION, type %d\n", type);
 
    const char* version = rota_get_version();
    s_msg_reply_version_data version_data = { 0 };
    strncpy((char*)&version_data.version_info, version, sizeof(version_data.version_info) - 1);

    rota_lock_reply(MID_GET_VERSION, MR_SUCCESS, &version_data, sizeof(s_msg_reply_version_data), type);
}

static RotaLockCallbak g_rota_lock_callback = { rota_lock_getState, rota_lock_setState, rota_lock_reply, rota_lock_sendMessage };
void rota_control_init()
{
    g_lockState = MS_STANDBY;
    for (int i = 0; i < 2; ++i) {
        if (!g_rcvBuf[i]) {
            g_rcvBuf[i] = malloc(MAX_RECEIVE_DATA_SIZE);
        }
        g_rcvNum[i] = 0;
    }
    rota_update_init(&g_rota_lock_callback);
}

void rota_control_init_reply()
{
    LOG_INFO("send NID_READY\n");
    rota_lock_sendMessage(NID_READY, NULL, 0, TYPE_UART);
}

void rota_control_process_data(uint8_t* buf, uint16_t len, int type)
{
    // LOG_DEBUG("received data, len %u\n", len);
    if (g_rcvNum[type] + len > MAX_RECEIVE_DATA_SIZE) {
        LOG_ERROR("received new data size: %u, total %u, type %d\n", len, g_rcvNum[type] + len, type);
        g_rcvNum[type] = 0;
        return;
    }

    memcpy(g_rcvBuf[type] + g_rcvNum[type], buf, len);
    g_rcvNum[type] += len;

    if (g_rcvNum[type] < 6) {
        return;
    }

    if (g_rcvBuf[type][0] != 0xEF || g_rcvBuf[type][1] != 0xAA) {
        LOG_ERROR("invalid header\n");
        g_rcvNum[type] = 0;
        return;
    }

    uint16_t dataSize = ROTA_GETU16(&g_rcvBuf[type][3]);
    uint16_t totalLen = dataSize + 6;
    if (g_rcvNum[type] < totalLen) {
        return;
    }

    if (g_rcvNum[type] > totalLen) {
        LOG_WARNING("received more data: %u > %u\n", g_rcvNum[type], totalLen);
    }

    if (checksum(&g_rcvBuf[type][2], totalLen - 3) != g_rcvBuf[type][totalLen - 1]) {
        LOG_ERROR("invalid checksum\n");
        g_rcvNum[type] = 0;
        return;
    }

    s_msg* msg = (s_msg*)malloc(sizeof(s_msg) + dataSize);
    memcpy(msg, &g_rcvBuf[type][2], sizeof(s_msg) + dataSize);
    uint8_t mid = g_rcvBuf[type][2];

    LOG_DEBUG("received mid %02X, data size %u\n", mid, dataSize);

    if (mid == MID_GET_VERSION) {
        handlGetVersion(type);
    } else if (mid == MID_START_OTA) {
        rota_update_startOTA(type);
    } else if (mid == MID_GET_OTA_STATUS) {
        rota_update_getOTAStatus(type);
    } else if (mid == MID_OTA_HEADER) {
        rota_update_handleMsgOTAHeader(msg, type);
    } else if (mid == MID_STOP_OTA) {
        rota_update_stopOTA(type);
    } else if (mid == MID_OTA_PACKET) {
        rota_update_OTAPacketTransmit(msg, type);
    } else if (mid == MID_CONFIG_BAUDRATE) {
        handleConfigBaudrate(msg, type);
    } else {
        LOG_WARNING("Unhandled mid 0x%x\n", mid);
    }

    free(msg);
    g_rcvNum[type] = 0;
}
