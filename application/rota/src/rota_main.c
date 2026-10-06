/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_control.h"
#include "rota_uart.h"
#include "rota_utils.h"
#include "usbcamera.h"
#include "uvc_camera.h"

static int recovery_uvc_config_callback(struct uvc_video_params* video_param)
{
    LOG_INFO("video_onoff %d video_index %d fcc %x width %d height %d\n", video_param->video_onoff, video_param->video_index, video_param->fcc, video_param->width,
             video_param->height);
    return -1;
}

int main(int argc, char* argv[])
{
    LOG_INFO("rota START\n");
    rota_uart_init("/dev/ttyS0");
    register_UvcVideoParam_get_func(recovery_uvc_config_callback);
    uvc_camera_init_with_config_file("/config_bak/inlock.config", 4 * 1024);
    rota_uart_cdc_init("/dev/ttyGS0");
    rota_control_init();
    rota_uart_start();
    rota_control_init_reply();

    rota_uart_wait();
    rota_uart_cdc_deinit();
    rota_uart_deinit();
    LOG_INFO("rota EXIT\n");

    return 0;
}