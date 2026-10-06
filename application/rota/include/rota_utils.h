/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_UTILS_H__
#define __ROTA_UTILS_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_DEBUG(fmt, ...)
// #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) printf("[WARNING] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt, ##__VA_ARGS__)

#define ROTA_GETU32(p) (((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | ((uint32_t)(p)[2] << 8) | ((uint32_t)(p)[3]))
#define ROTA_GETU16(p) (((uint16_t)(p)[0] << 8) | ((uint16_t)(p)[1]))

#define ROTA_GETU32_LE(p) (((uint32_t)(p)[0]) | ((uint32_t)(p)[1] << 8) | ((uint32_t)(p)[2] << 16) | ((uint32_t)(p)[3] << 24))

#define ROTA_PUTU16(p, v)                                                                                                                                                          \
    do {                                                                                                                                                                           \
        (p)[0] = (uint8_t)(((v) >> 8) & 0xFF);                                                                                                                                     \
        (p)[1] = (uint8_t)((v) & 0xFF);                                                                                                                                            \
    } while (0)

#define ROTA_READ_U32_LE(p, v)                                                                                                                                                     \
    do {                                                                                                                                                                           \
        (v) = ROTA_GETU32_LE(p);                                                                                                                                                   \
        p += sizeof(uint32_t);                                                                                                                                                     \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_UTILS_H__ */
