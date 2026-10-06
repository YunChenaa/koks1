#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "inirw.h"
#include "OVD_DEMO_FirmwareUpgrade.h"
#include <OVD_define.h>

static char           fileName[64];
static unsigned char *firmware_buffer;
static size_t         recv_len;

extern char FILEPATH[256];
extern int OVD_DEMO_ReBootDevice(ovd_reboot_reason_e reason);

#define OTA_BLOCK_SIZE  8192
OVD_int32 OVD_DEMO_FirmwareBinUpgrade(OVD_int32 channel_id, OVD_Upgrade_cmd_e cmd, void* data, size_t size, void* user_data)
{
    int wr_len;
    FILE *fp_download =NULL;
    int total_write = 0;
    int block_size = 0;

    // OVD_LOG(OVD_LOGLEVEL_WARN,"DEV/DEMO",  "升级命令 %d\n", cmd);
    if (cmd == OVD_CMD_UPGRADE_PKG_INFO)
    {
        OVD_ota_upgrade_package_info_t *info = (OVD_ota_upgrade_package_info_t *)data;
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "即将启动升级\n");
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "firmware_model: %s", info->firmware_model);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "pkg_size: %lld Bytes", info->pkg_size);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "pgk_crc: 0x%04x", info->crc);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "pkg_url: %s", info->url);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "encrypt_type: %d", info->encrypt_type);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "aes_key: %s", info->aes_key);
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "key_size: %d", info->key_size);

        firmware_buffer = malloc(info->pkg_size - 128);
        sprintf(fileName, "%s.bin", info->firmware_model);

    }
    else if (cmd == OVD_CMD_UPGRADE_DOWNLOAD)
    {
        memcpy(firmware_buffer+recv_len, data, size);
        recv_len += size;

    }
    else if (cmd == OVD_CMD_UPGRADE_DOWNLOAD_FINISH)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "文件下载完成...\n");
    }
    else if (cmd == OVD_CMD_UPGRADE_DOWNLOAD_FAIL)
    {
        OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "文件下载失败...\n");
        if (firmware_buffer)
        {
            free(firmware_buffer);
            firmware_buffer = NULL;
        }
        OVD_DEMO_ReBootDevice(OVD_REBOOT_REASON_CLOUD_UPGRADE);
    }
    else if (cmd == OVD_CMD_UPGRADE_INSTALL)
    {
        OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "开始烧录固件...\n");
        // 将固件写入到某个文件，模拟升级过程
        fp_download = fopen(fileName, "w");

        if (fp_download != NULL)
        {
            while(total_write < recv_len)
            {
                if (recv_len - total_write > OTA_BLOCK_SIZE)
                {
                    block_size = OTA_BLOCK_SIZE;
                }
                else
                {
                    block_size = recv_len - total_write;
                }
                wr_len = fwrite(firmware_buffer+total_write, 1, block_size, fp_download);
                if (wr_len != block_size)
                {
                    OVD_LOG(OVD_LOGLEVEL_ERROR,"DEV/DEMO", "write %d bytes failed\n", block_size);
                }
                OVD_LOG(OVD_LOGLEVEL_INFO,"DEV/DEMO", "write %d bytes ok", block_size);
                total_write += wr_len;
                usleep(1000);
            }
            fclose(fp_download);

        }
        if (firmware_buffer)
        {
            free(firmware_buffer);
            firmware_buffer = NULL;
        }
        OVD_DEMO_ReBootDevice(OVD_REBOOT_REASON_CLOUD_UPGRADE);
    }

    return 0;
}

OVD_int32 OVD_DEMO_FirmwareUpgradeStatusCb(OVD_int32 channel_id, OVD_UpgradeConfirmInfo_t *info)
{
    time_t timetemp; //时间获取
    struct tm *p;
    time(&timetemp);
    p = localtime(&timetemp);

    sprintf(info->last_upgrade_time, "%d-%02d-%02dT%02d:%02d:%02d", 1900 + p->tm_year, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    info->confirm = OVD_UPGRADE_CONFIRM_SUCCUSS;

    iniFileLoad(FILEPATH);
    iniGetString("deviceConfiguration", "firmware_model", info->cur_version, sizeof(info->cur_version),"V1.0.0");
    iniFileFree();

    return 0;
}