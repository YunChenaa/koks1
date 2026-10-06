/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_image.h"
#include "rota_utils.h"
#include "rota_common.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <mtd/mtd-user.h>

#define IMAGE_HEAD_SIZE 4096
#define IMAGE_VERSION_SIZE 32
#define IMAGE_MAGIC 0x53454657
#define IMAGE_MAGIC_MAX (IMAGE_MAGIC + 1)

#define MAX_IMAGE_COUNT 20

typedef struct {
    uint32_t tag; // 分区index
    uint32_t sub_offset; // 分区偏移
    uint32_t offset; // OTA文件偏移
    uint32_t size;
} RotaImageInfo;

static int g_image_head_done = 0;
static uint32_t g_image_count = 0;
static uint32_t g_ota_format = 0;
static RotaImageInfo g_image_infos[MAX_IMAGE_COUNT];
static char g_image_version[IMAGE_VERSION_SIZE];

static int full_write(int fd, const void* buf, size_t len)
{
    ssize_t n;
    size_t total_written = 0;
    const char* ptr = (const char*)buf;

    while (total_written < len) {
        n = write(fd, ptr + total_written, len - total_written);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (n == 0) {
            break;
        }

        total_written += n;
    }

    return (int)total_written;
}

static int full_read(int fd, void* buf, size_t len)
{
    ssize_t n;
    size_t total_read = 0;
    char* ptr = (char*)buf;

    while (total_read < len) {
        n = read(fd, ptr + total_read, len - total_read);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (n == 0) {
            break;
        }

        total_read += n;
    }

    return (int)total_read;
}

static int burn_image(uint32_t tag, uint32_t sub_offset, const uint8_t* buf, uint32_t size)
{
    char mdt_device[64];
    int fd = -1;
    int ret = -1;
    struct mtd_info_user mtd_info;
    struct erase_info_user erase_info;
    uint32_t offset = 0;
    uint8_t* buf2 = NULL;

    LOG_INFO("%s tag %u sub_offset %u size %u\n", __func__, tag, sub_offset, size);

    snprintf(mdt_device, sizeof(mdt_device), "/dev/mtd%u", tag);

    struct stat st;
    if (stat(mdt_device, &st) != 0) {
        LOG_ERROR("Device not found\n");
        return -1;
    }

    fd = open(mdt_device, O_RDWR | O_SYNC);
    if (fd < 0) {
        LOG_ERROR("Failed to open MTD device\n");
        return -1;
    }

    if (ioctl(fd, MEMGETINFO, &mtd_info) != 0) {
        LOG_ERROR("MEMGETINFO failed\n");
        goto cleanup;
    }

    uint32_t block_size = mtd_info.erasesize;
    uint32_t aligned_size = ((size + block_size - 1) / block_size) * block_size;
    offset = sub_offset;
    while (offset < sub_offset + aligned_size) {
        erase_info.start = offset;
        erase_info.length = block_size;

        if (ioctl(fd, MEMERASE, &erase_info) != 0) {
            LOG_ERROR("MEMERASE failed, start %u, length %u\n", erase_info.start, erase_info.length);
            goto cleanup;
        }
        offset += block_size;
    }

    buf2 = (uint8_t *)malloc(block_size);
    if (buf2 == NULL) {
        LOG_ERROR("malloc failed, size %u\n", block_size);
        goto cleanup;
    }

    for (int i = 0; i <= 1; i++) {
        if (lseek(fd, sub_offset, SEEK_SET) < 0) {
            LOG_ERROR("lseek failed %u\n", sub_offset);
            goto cleanup;
        }

        offset = 0;
        while (offset < size) {
            uint32_t count = (size - offset > block_size) ? block_size : (size - offset);
            if (i == 0) { // write
                if (full_write(fd, buf + offset, count) != (int)count) {
                    LOG_ERROR("Write failed\n");
                    goto cleanup;
                }
            } else { // read
                if (full_read(fd, buf2, count) != (int)count) {
                    LOG_ERROR("read failed\n");
                    goto cleanup;
                }
                if (memcmp(buf + offset, buf2, count) != 0) {
                    LOG_ERROR("verification mismatch at %u\n", sub_offset + offset);
                    goto cleanup;
                }
            }
            offset += count;
        }
    }

    ret = 0;

cleanup:
    if (fd >= 0) {
        close(fd);
    }
    if (buf2) {
        free(buf2);
    }
    return ret;
}

void rota_image_init()
{
    g_image_head_done = 0;
    g_image_count = 0;
    memset(g_image_infos, 0, sizeof(g_image_version));
    memset(g_image_version, 0, sizeof(g_image_version));
}

int rota_image_is_head_done()
{
    return g_image_head_done;
}

int rota_image_process_head(const uint8_t *buf, int bufSize)
{
    if (bufSize < IMAGE_HEAD_SIZE) {
        return 0;
    }

    const uint8_t *p = buf;

    // Magic number
    uint32_t magic_number;
    ROTA_READ_U32_LE(p, magic_number);
    if (!(magic_number >= IMAGE_MAGIC && magic_number <= IMAGE_MAGIC_MAX)) {
        LOG_ERROR("Invalid magic number 0x%x\n", magic_number);
        return -1;
    }

    g_ota_format = magic_number - IMAGE_MAGIC;
    LOG_INFO("OTA format %d\n", g_ota_format);

    // Image count
    ROTA_READ_U32_LE(p, g_image_count);
    if (g_image_count == 0 || g_image_count > MAX_IMAGE_COUNT) {
        LOG_ERROR("Invalid image count %u\n", g_image_count);
        return -1;
    }

    // Image infos
    int i;
    for (i = 0; i < g_image_count; ++i) {
        ROTA_READ_U32_LE(p, g_image_infos[i].tag);
        if (g_ota_format == 1) {
            ROTA_READ_U32_LE(p, g_image_infos[i].sub_offset);
        }
        ROTA_READ_U32_LE(p, g_image_infos[i].offset);
        ROTA_READ_U32_LE(p, g_image_infos[i].size);
        LOG_INFO("image[%u] tag %u sub_offset %u offset %u size %u\n", i, g_image_infos[i].tag, g_image_infos[i].sub_offset, g_image_infos[i].offset, g_image_infos[i].size);
    }

    // Version
    memcpy(g_image_version, p, sizeof(g_image_version) - 1);
    LOG_INFO("version: %s\n", g_image_version);
    p += sizeof(g_image_version);

    const char* cur_version = rota_get_version();
    if (strncmp(g_image_version, cur_version, 5) != 0) {
        LOG_ERROR("version not match: %s\n", cur_version);
        return -1;
    }

    g_image_head_done = 1;
    return 0;
}

int rota_image_start_burn(const uint8_t *buf, int bufSize)
{
    if (!rota_image_is_head_done()) {
        LOG_ERROR("No valid image head\n");
        return -1;
    }

    rota_write_flag();

    uint32_t i;
    for (i = 0; i < g_image_count; ++i) {
        if (burn_image(g_image_infos[i].tag, g_image_infos[i].sub_offset, buf + g_image_infos[i].offset, g_image_infos[i].size) != 0) {
            return -1;
        }
    }

    rota_set_done(g_image_version);
    LOG_INFO("Burn image done\n");
    return 0;
}
