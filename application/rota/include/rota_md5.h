/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_MD5_H__
#define __ROTA_MD5_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int rota_md5_sum(char* data, int length, char* value);
int rota_md5_check_fw(uint8_t* data, uint32_t Len, const char* expected_md5);

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_MD5_H__ */
