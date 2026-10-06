/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_update.h"
#include "rota_common.h"
#include "rota_image.h"
#include "rota_md5.h"
#include "rota_utils.h"

#include <errno.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

typedef enum {
    OTA_STAGE_TYPE_DEACTIVE, // ota module is deactive
    OTA_STAGE_TYPE_IDLE,     // ota ready
    OTA_STAGE_TYPE_BURN,     // burn image to flash
    OTA_STAGE_TYPE_DONE,     // upgrade done
    OTA_STAGE_TYPE_CANCEL,   // upgrade cancel

    OTA_STAGE_TYPE_INVALID = 0xFF,
} ota_stage_type;

typedef enum { OTA_SUCCESS = 0, OTA_FAILED = 1 } ota_result;

static RotaLockCallbak* sLock = NULL;

static int ota_stage = OTA_STAGE_TYPE_DEACTIVE;
static int fw_size = 0;
static int total_packet_num = 0;
static int current_packet_id = 0;
static int current_packet_size = 0;
static int last_packet_id = -1;
static int current_packet_offset = 0;
static uint8_t* otabuf = NULL;
static s_msg_reply_getotastatus_data r_ota_data;
static s_note_ota_result s_ota_result;

#define MD5_LEN (32)
static char fw_md5[MD5_LEN + 1] = { 0 };

static void otabuf_release()
{
    if (otabuf != NULL) {
        free(otabuf);
        otabuf = NULL;
        LOG_DEBUG("delete otabuf\n");
    }
}

static void SendResult(ota_result result, s_type type)
{
    sync();
    usleep(2000 * 1000);
    s_ota_result.ota_result = result;
    sLock->sendMessage(NID_OTA_DONE, &s_ota_result, sizeof(s_ota_result), type);
    return;
}

void rota_update_init(RotaLockCallbak* callback)
{
    sLock = callback;
    r_ota_data.ota_status = MS_OTA;
    r_ota_data.next_pid_e[0] = 0;
    r_ota_data.next_pid_e[1] = 0;
}

void rota_update_startOTA(s_type type)
{
    fw_size = 0;
    total_packet_num = 0;
    current_packet_id = 0;
    current_packet_size = 0;
    last_packet_id = -1;
    current_packet_offset = 0;
    sLock->setState(MS_OTA);
    sLock->reply(MID_START_OTA, MR_SUCCESS, NULL, 0, type);
    ota_stage = OTA_STAGE_TYPE_IDLE;
}

void rota_update_getOTAStatus(s_type type)
{
    if (ota_stage != OTA_STAGE_TYPE_IDLE) {
        sLock->setState(MS_STANDBY);
        sLock->reply(MID_GET_OTA_STATUS, MR_REJECTED, NULL, 0, type);
        LOG_ERROR("getOTAStatus failed, ota_stage err %d\n", ota_stage);
        return;
    }
    sLock->setState(MS_OTA);
    sLock->reply(MID_GET_OTA_STATUS, MR_SUCCESS, &r_ota_data, sizeof(r_ota_data), type);
    LOG_INFO("getOTAStatus success\n");
    return;
}

void rota_update_handleMsgOTAHeader(s_msg* msg, s_type type)
{
    if (ota_stage != OTA_STAGE_TYPE_IDLE) {
        sLock->setState(MS_STANDBY);
        sLock->reply(MID_OTA_HEADER, MR_REJECTED, NULL, 0, type);
        LOG_ERROR("handleMsgOTAHeader failed, ota_stage err %d\n", ota_stage);
        return;
    }
    s_msg_otaheader_data* ota_header = (s_msg_otaheader_data*)(msg->data);
    fw_size = (ota_header->fsize_b[0] << 24) + (ota_header->fsize_b[1] << 16) + (ota_header->fsize_b[2] << 8) + ota_header->fsize_b[3];
    total_packet_num = (ota_header->num_pkt[0] << 24) + (ota_header->num_pkt[1] << 16) + (ota_header->num_pkt[2] << 8) + ota_header->num_pkt[3];
    strncpy(fw_md5, (char*)ota_header->md5_sum, 32);
    LOG_INFO("fw_size %d, total_packet_num %d, md5 %s\n", fw_size, total_packet_num, fw_md5);
    sLock->setState(MS_OTA);

    otabuf_release();
    otabuf = (uint8_t*)malloc(fw_size);
    if (NULL != otabuf) {
        sLock->reply(MID_OTA_HEADER, MR_SUCCESS, NULL, 0, type);
        memset(otabuf, 0, fw_size);
        LOG_INFO("handleMsgOTAHeader success\n");
    } else {
        LOG_ERROR("handleMsgOTAHeader failed, no memory\n");
        sLock->setState(MS_STANDBY);
        sLock->reply(MID_OTA_HEADER, MR_FAILED4_NOMEMORY, NULL, 0, type);
    }

    rota_image_init();
}

void rota_update_OTAPacketTransmit(s_msg* msg, s_type type)
{
    if (ota_stage != OTA_STAGE_TYPE_IDLE) {
        sLock->setState(MS_STANDBY);
        otabuf_release();
        sLock->reply(MID_OTA_PACKET, MR_REJECTED, NULL, 0, type);
        LOG_ERROR("OTAPacketTransmit failed, ota_stage err %d\n", ota_stage);
        return;
    }
    s_msg_otapacket_data* otapacketdata = (s_msg_otapacket_data*)(msg->data);
    current_packet_id = (otapacketdata->pid[0] << 8) + otapacketdata->pid[1];
    current_packet_size = (otapacketdata->psize[0] << 8) + otapacketdata->psize[1];

    // LOG_INFO("last_packet_id:%d, current_packet_id:%d, current_packet_size:%d, current_packet_offset:%d\n", last_packet_id, current_packet_id, current_packet_size,
    //          current_packet_offset);
    sLock->setState(MS_OTA);
    if (current_packet_id != last_packet_id + 1) {
        LOG_WARNING("ota packet id jump %d -> %d\n", last_packet_id, current_packet_id);
    }
    if (current_packet_id % 100 == 0) {
        LOG_INFO("ota packet id %d\n", current_packet_id);
    }
    if (current_packet_id > last_packet_id) {
        memcpy(otabuf + current_packet_offset, otapacketdata->data, current_packet_size);
        last_packet_id = current_packet_id;
        current_packet_offset += current_packet_size;

        if (!rota_image_is_head_done()) {
            if (rota_image_process_head(otabuf, current_packet_offset) != 0) {
                sLock->setState(MS_STANDBY);
                sLock->reply(MID_OTA_PACKET, MR_REJECTED, NULL, 0, type);
                otabuf_release();
                return;
            }
        }
    } else {
        LOG_INFO("received dup MID_OTA_PACKET: %d\n", current_packet_id);
    }

    if (current_packet_id >= (total_packet_num - 1)) {
        LOG_INFO("received done\n");
        sLock->reply(MID_OTA_PACKET, MR_SUCCESS, NULL, 0, type);

        /* check firware md5 */
        if (!rota_md5_check_fw(otabuf, fw_size, fw_md5)) {
            LOG_INFO("md5 verfiy pass, startBurn\n");
            int ret = rota_image_start_burn(otabuf, fw_size);
            if (ret != 0) {
                LOG_ERROR("burn image fail, ota fail\n");
                SendResult(OTA_FAILED, type);
                otabuf_release();
                return;
            } else {
                SendResult(OTA_SUCCESS, type);
                LOG_INFO("startBurn success, waitting reboot for 1000 milliseconds\n");
                usleep(1000 * 1000);
                reboot(RB_AUTOBOOT);
            }
        } else {
            LOG_ERROR("md5 verfiy fail, ota cancel\n");
            SendResult(OTA_FAILED, type);
            otabuf_release();
            return;
        }
    } else {
        r_ota_data.next_pid_e[0] = (current_packet_id + 1) >> 8;   // set next pack_id
        r_ota_data.next_pid_e[1] = (current_packet_id + 1) & 0xFF; // set next pack_id
        LOG_DEBUG("r_ota_data.next_pid_e[0]: %d, r_ota_data.next_pid_e[1]: %d\n", r_ota_data.next_pid_e[0], r_ota_data.next_pid_e[1]);
    }
    sLock->reply(MID_OTA_PACKET, MR_SUCCESS, NULL, 0, type);
    ota_stage = OTA_STAGE_TYPE_IDLE;
}

void rota_update_stopOTA(s_type type)
{
    sLock->setState(MS_OTA);
    if (ota_stage == OTA_STAGE_TYPE_IDLE) {
        LOG_INFO("ota_stage is idle, stop ota\n");
        ota_stage = OTA_STAGE_TYPE_CANCEL;
        sLock->reply(MID_STOP_OTA, MR_SUCCESS, NULL, 0, type);
        otabuf_release();
    } else {
        LOG_ERROR("ota_stage is not idle, stop ota failed\n");
        sLock->reply(MID_STOP_OTA, MR_REJECTED, NULL, 0, type);
        sLock->setState(MS_STANDBY);
    }
}
