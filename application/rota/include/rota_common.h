/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_COMMON_H__
#define __ROTA_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define ROTA_FLAG_FILE "/config/ota_flag"
#define ROTA_TMP_FLAG_FILE "/config/ota_flag.tmp"

typedef enum {
    ROTA_FLAG_NONE = 0,
    ROTA_FLAG_START_UART = 1,
    ROTA_FLAG_START_CDC = 2,
    ROTA_FLAG_BURN = 3
} RotaFlag;

int rota_flag_read();
int rota_flag_write(int flag);
int rota_flag_remove();
#endif

const char* rota_get_version();
int rota_set_done(const char* version);
int rota_enable_sfc1();
int rota_write_flag();
int rota_has_flag();

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_COMMON_H__ */
