/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_UPDATE_H__
#define __ROTA_UPDATE_H__

#include "message_v2.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t (*getState)();
    void (*setState)(uint8_t state);
    void (*reply)(uint8_t cmd, uint8_t result, void* data, uint16_t size, s_type type);
    void (*sendMessage)(uint8_t cmd, void* data, uint16_t size, s_type type);
} RotaLockCallbak;

void rota_update_init(RotaLockCallbak* callback);
void rota_update_startOTA(s_type type);
void rota_update_getOTAStatus(s_type type);
void rota_update_handleMsgOTAHeader(s_msg* msg, s_type type);
void rota_update_OTAPacketTransmit(s_msg* msg, s_type type);
void rota_update_stopOTA(s_type type);

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_UPDATE_H__ */
