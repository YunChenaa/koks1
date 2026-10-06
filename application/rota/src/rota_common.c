/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_common.h"
#include "rota_utils.h"
#include "sys_version.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#define ROTA_VERSION_FILE "/config/version"
#define ROTA_VERSION_FILE_TMP "/config/version.tmp"
#define ROTA_OTA_FLAG_FILE "/config/ota_flag"
#define MAX_VERSION_LEN 32

static int rota_remount_config(int readonly)
{
    unsigned long flags = MS_REMOUNT;
    if (readonly) {
        flags |= MS_RDONLY;
    }

    int ret = mount("/dev/mtdblock6", "/config", "jffs2", flags, NULL);
    if (ret != 0) {
        LOG_WARNING("remount config failed, error %d\n", errno);
    }
    return ret;
}

static int rota_sync_dir(const char* dirpath)
{
    int fd = open(dirpath, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    if (fsync(fd) != 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

#if 0
int rota_flag_read(void)
{
    int fd = open(ROTA_FLAG_FILE, O_RDONLY);
    if (fd == -1) {
        LOG_INFO("No ota flag file or cannot open\n");
        return -1;
    }

    unsigned char byte;
    ssize_t nread = read(fd, &byte, 1);
    close(fd);

    if (nread != 1) {
        LOG_ERROR("Read ota flag file failed\n");
        return -1;
    }

    LOG_INFO("Read ota flag: %u\n", byte);
    return (int)byte;
}

int rota_flag_write(int flag)
{
    int fd = open(ROTA_TMP_FLAG_FILE, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        LOG_ERROR("Open ota tmp flag file failed.\n");
        return -1;
    }

    unsigned char byte = (unsigned char)(flag & 0xFF);
    LOG_INFO("Write ota flag: %u\n", byte);
    ssize_t written = write(fd, &byte, sizeof(byte));

    fsync(fd);
    close(fd);

    if (written != sizeof(byte)) {
        unlink(ROTA_TMP_FLAG_FILE);
        LOG_ERROR("Write ota tmp flag file failed.\n");
        return -1;
    }

    if (rename(ROTA_TMP_FLAG_FILE, ROTA_FLAG_FILE) != 0) {
        LOG_ERROR("Rename ota tmp flag file failed.\n");
        unlink(ROTA_TMP_FLAG_FILE);
        return -1;
    }
    sync();

    return 0;
}

int rota_flag_remove()
{
    unlink(ROTA_FLAG_FILE);
    unlink(ROTA_TMP_FLAG_FILE);
    sync();
    return 0;
}
#endif

const char* rota_get_version()
{
    static char version[32] = { 0 };

    if (version[0] != '\0') {
        return version;
    }

    int success = 0;
    FILE* fp = fopen(ROTA_VERSION_FILE, "r");
    if (fp) {
        if (fgets(version, sizeof(version), fp) != NULL) {
            success = 1;
        }
        fclose(fp);
    }

    if (!success) {
        strncpy(version, SENSELOCK_V3X_VERSION, sizeof(version) - 1);
        LOG_INFO("Version %s (koks)\n", version);
    } else {
        LOG_INFO("Version %s (ota)\n", version);
    }

    version[sizeof(version) - 1] = '\0';
    return version;
}

// 记录版本号，并且删除标记文件
int rota_set_done(const char* version)
{
    int ret = -1;
    rota_remount_config(0); // read write
    FILE* fp = NULL;
    do {
        // 记录版本号
        fp = fopen(ROTA_VERSION_FILE_TMP, "w");
        LOG_INFO("Version %s (ota)\n", version);

        if (!fp) {
            LOG_ERROR("open file failed: %s\n", ROTA_VERSION_FILE_TMP);
            break;
        }

        if (fprintf(fp, "%s", version) < 0) {
            break;
        }

        if (fflush(fp) != 0 || fsync(fileno(fp)) != 0) {
            break;
        }

        if (fclose(fp) != 0) {
            break;
        }
        fp = NULL;

        if (rename(ROTA_VERSION_FILE_TMP, ROTA_VERSION_FILE) != 0) {
            break;
        }

        // 删除标记文件
        if (unlink(ROTA_OTA_FLAG_FILE) != 0) {
            break;
        }

        rota_sync_dir("/config");

        ret = 0;
    } while (0);

    if (fp) {
        fclose(fp);
    }
    if (ret != 0) {
        unlink(ROTA_VERSION_FILE_TMP);
    }
    sync();
    rota_remount_config(1); // read only
    return ret;
}

int rota_enable_sfc1()
{
    if (access("/lib/modules/jz_sfc1.ko", F_OK) == 0) {
        system("insmod /lib/modules/jz_sfc1.ko");
    }
    return 0;
}

int rota_write_flag()
{
    rota_remount_config(0); // read write

    int ret = -1;
    do {
        int fd = open(ROTA_OTA_FLAG_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd < 0) {
            break;
        }
        close(fd);

        rota_sync_dir("/config");
        ret = 0;
    } while (0);

    rota_remount_config(1); // read only
    return ret;
}

int rota_has_flag()
{
    return (access(ROTA_OTA_FLAG_FILE, F_OK) == 0);
}
