/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_IMAGE_H__
#define __ROTA_IMAGE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void rota_image_init();
int rota_image_is_head_done();
int rota_image_process_head(const uint8_t *buf, int bufSize);
int rota_image_start_burn(const uint8_t *buf, int bufSize);

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_IMAGE_H__ */
