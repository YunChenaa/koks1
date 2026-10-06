/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __ROTA_UART_H__
#define __ROTA_UART_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int rota_uart_init(const char* uart_path);
int rota_uart_cdc_init(const char* cdc_path);
int rota_uart_set_baudrate(int baudrate);

void rota_uart_start();
void rota_uart_wait();

void rota_uart_send(const uint8_t *buf, uint16_t bufSize, int type);

int rota_uart_deinit();
int rota_uart_cdc_deinit();

#ifdef __cplusplus
}
#endif

#endif /* __ROTA_UART_H__ */
