/*
 * Copyright (c) 2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "ovd_ota.h"
#include "rota_image.h"
#include "rota_utils.h"
#include "rota_common.h"
#include "rota_md5.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

/* OTA 进度上报线程：每秒发送一次进度消息 */
static volatile int g_ota_progress_running = 0;

static void* ota_progress_thread_func(void* arg)
{
    (void)arg;
    while (g_ota_progress_running) {
        system("custom_msg vnet0 30 22 OTAT33_OFFSET 100");
        sleep(1);
    }
    return NULL;
}

int ovd_ota_run(const char* file_path, const char* md5)
{
    if (!file_path || !md5) {
        return -1;
    }

    LOG_INFO("%s %s START\n", __func__, file_path);

    int ret = -1;
    int fd = -1;
    size_t fw_size = 0;
    void* addr = MAP_FAILED;
    pthread_t progress_tid = 0;

    do {
        // 读取文件
        fd = open(file_path, O_RDONLY);
        if (fd < 0) {
            LOG_ERROR("open failed: %s\n", strerror(errno));
            break;
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            LOG_ERROR("fstat error: %s\n", strerror(errno));
            break;
        }
        fw_size = (size_t)st.st_size;

        LOG_INFO("%s file size %zu\n", __func__, fw_size);
        addr = mmap(NULL, fw_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            LOG_ERROR("mmap error: %s\n", strerror(errno));
            break;
        }

        // 校验MD5
        uint8_t* otabuf = (uint8_t*)addr;
        if (rota_md5_check_fw(otabuf, fw_size, md5) != 0) {
            LOG_ERROR("check md5 failed\n");
            break;
        }

        // 启动进度上报线程
        g_ota_progress_running = 1;
        pthread_create(&progress_tid, NULL, ota_progress_thread_func, NULL);

        // 开始OTA
        rota_enable_sfc1();
        rota_image_init();
        if (rota_image_process_head(otabuf, fw_size) != 0) {
            break;
        }

        if (!rota_image_is_head_done()) {
            LOG_ERROR("OTA head invalid\n");
            break;
        }

        if (rota_image_start_burn(otabuf, fw_size) != 0) {
            LOG_ERROR("burn image fail, ota fail\n");
            break;
        }

        LOG_INFO("startBurn success\n");
        ret = 0;
    } while (0);

    // 停止进度上报线程
    if (progress_tid) {
        g_ota_progress_running = 0;
        pthread_join(progress_tid, NULL);
    }

    // OTA结束
    if (addr != MAP_FAILED) {
        munmap(addr, fw_size);
    }

    if (fd != 0) {
        close(fd);
    }

    return ret;
}