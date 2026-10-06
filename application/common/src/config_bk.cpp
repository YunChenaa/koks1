#include "config_bk.h"
#include "sys_logger.h"
#include "koks_project_def.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    uint32_t magic1; // 0x55AA55AA
    uint32_t crc32;
    uint64_t file_size;
    uint32_t commit;
    uint32_t magic2; // 0xAA55AA55
} file_crc_meta_t;

#define REMOUNT_CONFIG_RO() config_bk_remount(PARTITION_CONFIG, true)
#define REMOUNT_CONFIG_RW() config_bk_remount(PARTITION_CONFIG, false)
#define REMOUNT_BK_RO() config_bk_remount(PARTITION_BK, true)
#define REMOUNT_BK_RW() config_bk_remount(PARTITION_BK, false)

#define CRC_COMMIT_INIT 0x11111111
#define CRC_COMMIT_DONE 0x22222222
#define COPY_BUF_SIZE 4096
#define _PATH_MAX 128

static uint32_t crc32_table[256];
static int crc32_table_inited = 0;

static void crc32_init_table(void)
{
    if (crc32_table_inited)
        return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_inited = 1;
}

static int64_t crc32_file(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_WARNING("open file failed: %s\n", path);
        return -1;
    }

    uint32_t crc = 0xFFFFFFFF;
    uint8_t buf[COPY_BUF_SIZE];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            crc = (crc >> 8) ^ crc32_table[(crc ^ buf[i]) & 0xFF];
        }
    }

    if (n < 0) {
        LOG_WARNING("read file failed: %s\n", path);
        close(fd);
        return -2;
    }

    close(fd);
    return (int64_t)(crc ^ 0xFFFFFFFF);
}

static int fsync_dir(const char* path)
{
    char dir[_PATH_MAX];
    snprintf(dir, sizeof(dir), "%s", path);

    char* slash = strrchr(dir, '/');
    if (slash)
        *slash = '\0';
    if (dir[0] == '\0')
        strcpy(dir, "/");

    int dfd = open(dir, O_RDONLY | O_DIRECTORY);
    if (dfd < 0) {
        LOG_WARNING("open dir failed: %s\n", dir);
        return -1;
    }

    fsync(dfd);
    close(dfd);
    return 0;
}

static int mkdir_p(const char *path, mode_t mode)
{
    if (!path || path[0] == '\0')
        return -1;

    char tmp[_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);

    // 去除末尾多余的 '/'
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
        len--;
    }

    // 从根目录开始逐级创建
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) < 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }

    // 最后一级
    if (mkdir(tmp, mode) < 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

static int ensure_db_subdir(const char *filepath)
{
    if (!filepath)
        return -1;

    char dir[_PATH_MAX];
    snprintf(dir, sizeof(dir), "%s", filepath);

    // 提取父目录
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';               // 截断得到目录部分
        if (mkdir_p(dir, 0755) < 0) {
            LOG_WARNING("mkdir_p %s failed: %d\n", dir, errno);
            return -1;
        }
    }
    // 若无 '/'（即根目录），无需创建
    return 0;
}

static int copy_file(const char* src, const char* dst)
{
    int src_fd = -1;
    int dst_fd = -1;
    int ret = -1;

    uint8_t buf[COPY_BUF_SIZE];

    char tmp_path[_PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", dst);

    if (ensure_db_subdir(tmp_path) < 0)
        return -1;

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
        goto out;

    ret = -2;
    dst_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0)
        goto out;

    ret = -3;
    ssize_t n;
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(dst_fd, buf + written, n - written);
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                goto out;
            }
            written += w;
        }
    }

    ret = -4;
    if (n < 0)
        goto out;

    ret = -5;
    if (fsync(dst_fd) < 0)
        goto out;

    if (rename(tmp_path, dst) < 0)
        goto out;

    if (fsync_dir(dst) < 0)
        goto out;

    ret = 0;

out:
    if (src_fd >= 0)
        close(src_fd);
    if (dst_fd >= 0)
        close(dst_fd);

    if (ret != 0) {
        LOG_WARNING("copy file failed, %s -> %s, ret %d\n", src, dst, ret);
        unlink(tmp_path);
    }

    return ret;
}

static int write_crc_file(const char* crc_path, uint32_t crc, uint64_t size, uint32_t commit)
{
    file_crc_meta_t meta = { .magic1 = 0x55AA55AA, .crc32 = crc, .file_size = size, .commit = commit, .magic2 = 0xAA55AA55 };

    char tmp[_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.tmp", crc_path);

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOG_WARNING("open file failed: %s\n", tmp);
        return -1;
    }

    ssize_t w = write(fd, &meta, sizeof(meta));
    fsync(fd);
    close(fd);

    if (w != sizeof(meta)) {
        LOG_WARNING("write file failed: %s\n", tmp);
        unlink(tmp);
        return -1;
    }

    if (rename(tmp, crc_path) < 0) {
        LOG_WARNING("rename file failed: %s\n", tmp);
        unlink(tmp);
        return -1;
    }

    fsync_dir(crc_path);
    return 0;
}

static int verify_file(const char* data_path, const char* crc_path, file_crc_meta_t* meta_out)
{
    int fd = open(crc_path, O_RDONLY);
    if (fd < 0) {
        LOG_WARNING("open file failed: %s\n", crc_path);
        return -1;
    }

    file_crc_meta_t meta;
    memset(&meta, 0, sizeof(meta));

    if (read(fd, &meta, sizeof(meta)) != sizeof(meta)) {
        close(fd);
        return -1;
    }
    close(fd);

    if (meta.magic1 != 0x55AA55AA || meta.magic2 != 0xAA55AA55) {
        LOG_WARNING("meta magic incorrect: %s %x %x\n", crc_path, meta.magic1, meta.magic2);
        return -2;
    }

    if (meta_out)
        *meta_out = meta;

    struct stat st;
    if (stat(data_path, &st) < 0) {
        LOG_WARNING("stat failed: %s\n", data_path);
        return -2;
    }
    if ((uint64_t)st.st_size != meta.file_size) {
        LOG_INFO("file %s, size %llu, meta file size %llu\n", data_path, (uint64_t)st.st_size, meta.file_size);
        return -2;
    }

    int64_t crc = crc32_file(data_path);
    if (crc < 0)
        return -2;

    if ((uint32_t)crc != meta.crc32) {
        LOG_INFO("file %s, crc %x, meta crc %x\n", data_path, (uint32_t)crc, meta.crc32);
        return -3;
    }

    return 0;
}

static int create_initial_backup(const char* cfg, const char* cfg_crc, const char* bak, const char* bak_crc, uint64_t file_size, int64_t crc)
{
    LOG_INFO("creating initial backup %s\n", cfg);

    int ret = -1;

    // 写主 CRC（DONE）
    REMOUNT_CONFIG_RW();
    if (write_crc_file(cfg_crc, (uint32_t)crc, file_size, CRC_COMMIT_DONE) < 0) {
        REMOUNT_CONFIG_RO();
        return ret;
    }

    REMOUNT_CONFIG_RO();

    // 写备份
    REMOUNT_BK_RW();

    ret = -2;
    if (copy_file(cfg, bak) < 0) 
        goto out_bk;

    ret = -3;
    if (write_crc_file(bak_crc, (uint32_t)crc, file_size, CRC_COMMIT_DONE) < 0)
        goto out_bk;

    ret = 0;

out_bk:
    REMOUNT_BK_RO();
    if (ret < 0) {
        LOG_ERROR("creating initial backup failed, %s, ret %d\n", cfg, ret);
    }
    return ret;
}

static int restore_config(const char* cfg, const char* cfg_crc, const char* bak, const char* bak_crc)
{
    int ret = -1;
    LOG_INFO("restore config: %s\n", cfg);

    REMOUNT_CONFIG_RW();

    if (copy_file(bak, cfg) < 0)
        goto out;

    ret = -2;
    if (copy_file(bak_crc, cfg_crc) < 0)
        goto out;

    ret = 0;

out:
    REMOUNT_CONFIG_RO();
    if (ret < 0) {
        LOG_ERROR("restore_config failed, %s, ret %d\n", cfg, ret);
    }
    return ret;
}

static int init_complete_commit(const char* cfg, const char* cfg_crc, const char* bak, const char* bak_crc, const file_crc_meta_t* meta)
{
    int ret = -1;
    LOG_INFO("init complete: %s\n", cfg);

    int64_t crc = crc32_file(cfg);
    if (crc < 0 || (uint32_t)crc != meta->crc32) {
        LOG_WARNING("file corrupted: %s\n", cfg);

        if (verify_file(bak, bak_crc, NULL) == 0) {
            if (restore_config(cfg, cfg_crc, bak, bak_crc) == 0) {
                LOG_WARNING("restored config from backup: %s\n", cfg);
                return 0;
            }
        }
        LOG_WARNING("backup invalid: %s\n", bak);
        return -1;
    }

    // 备份分区 RW
    REMOUNT_BK_RW();

    if (copy_file(cfg, bak) < 0)
        goto out_bk;

    ret = -2;
    if (write_crc_file(bak_crc, meta->crc32, meta->file_size, CRC_COMMIT_DONE) < 0) {
        goto out_bk;
    }

    // 主分区 RW
    REMOUNT_CONFIG_RW();
    ret = -3;
    if (write_crc_file(cfg_crc, meta->crc32, meta->file_size, CRC_COMMIT_DONE) < 0) {
        goto out_cfg;
    }

    ret = 0;

out_cfg:
    REMOUNT_CONFIG_RO();
out_bk:
    REMOUNT_BK_RO();
    if (ret != 0) {
        LOG_WARNING("init complete failed, %s, ret %d\n", cfg, ret);
    }
    return ret;
}

static int repair_backup(const char* cfg, const char* cfg_crc, const char* bak, const char* bak_crc, const file_crc_meta_t* meta)
{
    int ret = -1;
    LOG_INFO("repair backup: %s\n", bak);

    REMOUNT_BK_RW();

    if (copy_file(cfg, bak) < 0)
        goto out;

    ret = -2;
    if (write_crc_file(bak_crc, meta->crc32, meta->file_size, CRC_COMMIT_DONE) < 0) {
        goto out;
    }

    ret = 0;

out:
    REMOUNT_BK_RO();
    if (ret != 0) {
        LOG_WARNING("repair backup failed: %s, ret %d\n", cfg, ret);
    }
    return ret;
}

void config_bk_init()
{
    crc32_init_table();
}

void config_bk_check_file(const char *filename)
{
    if (!filename) return;

    LOG_DEBUG("checking %s\n", filename);

    char cfg[_PATH_MAX];
    char cfg_crc[_PATH_MAX];
    char bak[_PATH_MAX];
    char bak_crc[_PATH_MAX];

    snprintf(cfg, sizeof(cfg), "/config/%s", filename);
    snprintf(cfg_crc, sizeof(cfg_crc), "%s.crc", cfg);
    snprintf(bak, sizeof(bak), "/config_b/%s", filename);
    snprintf(bak_crc, sizeof(bak_crc), "%s.crc", bak);

    file_crc_meta_t cfg_meta;
    file_crc_meta_t bak_meta;
    memset(&cfg_meta, 0, sizeof(cfg_meta));
    memset(&bak_meta, 0, sizeof(bak_meta));

    int cfg_st = verify_file(cfg, cfg_crc, &cfg_meta);
    int bk_st = verify_file(bak, bak_crc, &bak_meta);
    if (cfg_st != 0 || bk_st != 0) {
        LOG_WARNING("check file %s, cfg_st %d, bk_st %d\n", filename, cfg_st, bk_st);
    }

    // 首次开机创建备份
    if (bk_st == -1) {
        struct stat st;
        if (stat(cfg, &st) == 0 && st.st_size > 0) {
            int64_t crc = crc32_file(cfg);
            if (crc >= 0) {
                create_initial_backup(cfg, cfg_crc, bak, bak_crc, (uint64_t)st.st_size, crc);
                return;
            }
        }
    }

    // INIT 补提交
    if ((cfg_st == 0 || cfg_st == -2 || cfg_st == -3) &&
        cfg_meta.magic1 == 0x55AA55AA &&
        cfg_meta.magic2 == 0xAA55AA55 &&
        cfg_meta.commit == CRC_COMMIT_INIT) {
        init_complete_commit(cfg, cfg_crc, bak, bak_crc, &cfg_meta);
        return;
    }

    // 修复备份（主分区完好，备份有损坏）
    if (cfg_st == 0 && bk_st != 0) {
        repair_backup(cfg, cfg_crc, bak, bak_crc, &cfg_meta);
        return;
    }

    // 恢复主分区（主分区损坏，备份完好）
    if (cfg_st != 0 && bk_st == 0) {
        restore_config(cfg, cfg_crc, bak, bak_crc);
        return;
    }

    // 双分区都损坏，恢复出厂
    if (cfg_st != 0 && bk_st != 0) {
        LOG_WARNING("file may be corrupted: %s\n", cfg);
        return;
    }
}

int config_bk_update(const char* filename, bool force)
{
    if (!filename) return -1;

    LOG_DEBUG("updating %s\n", filename);

    char cfg[_PATH_MAX];
    char cfg_crc[_PATH_MAX];
    char bak[_PATH_MAX];
    char bak_crc[_PATH_MAX];

    snprintf(cfg, sizeof(cfg), "/config/%s", filename);
    snprintf(cfg_crc, sizeof(cfg_crc), "%s.crc", cfg);
    snprintf(bak, sizeof(bak), "/config_b/%s", filename);
    snprintf(bak_crc, sizeof(bak_crc), "%s.crc", bak);

    file_crc_meta_t meta;
    if (!force && verify_file(cfg, cfg_crc, &meta) == 0 && meta.commit == CRC_COMMIT_DONE) {
        LOG_DEBUG("verify_file ok, skip update\n");
        return 0;
    }

    struct stat st;
    if (stat(cfg, &st) < 0) {
        LOG_WARNING("stat file failed, %s\n", filename);
        return -1;
    }

    int64_t crc = crc32_file(cfg);
    if (crc < 0)
        return -1;

    int ret = -1;

    REMOUNT_BK_RW();

    if (write_crc_file(cfg_crc, (uint32_t)crc, st.st_size, CRC_COMMIT_INIT))
        goto out;

    ret = -2;
    if (copy_file(cfg, bak))
        goto out;

    ret = -3;
    if (write_crc_file(bak_crc, (uint32_t)crc, st.st_size, CRC_COMMIT_DONE))
        goto out;

    ret = -4;
    if (write_crc_file(cfg_crc, (uint32_t)crc, st.st_size, CRC_COMMIT_DONE))
        goto out;

    ret = 0;

out:
    REMOUNT_BK_RO();
    if (ret != 0) {
        LOG_WARNING("update failed, %s, ret %d\n", filename, ret);
    }
    return ret;
}

int config_bk_remount(int partition, bool readonly)
{
    const char* src = (partition == PARTITION_CONFIG) ? "/dev/mtdblock6" : "/dev/mtdblock7";
    const char* dst = (partition == PARTITION_CONFIG) ? "/config" : "/config_b";

    unsigned long flags = MS_REMOUNT | MS_NOATIME;
    if (readonly) {
        flags |= MS_RDONLY;
        sync();
    }

    if (mount(src, dst, "jffs2", flags, NULL) < 0) {
        LOG_WARNING("remount %s %s ro=%d failed, errno=%d\n", src, dst, readonly, errno);
        return -1;
    }
    return 0;
}
