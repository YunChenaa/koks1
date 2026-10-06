/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_CONTROL_H__
#define __ROTA_CONTROL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void rota_control_init();
void rota_control_init_reply();
void rota_control_process_data(uint8_t* buf, uint16_t len, int type);

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_CONTROL_H__ */
