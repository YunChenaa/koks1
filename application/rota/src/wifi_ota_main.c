#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <su_base.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include "wifi_ota_main.h"
#define CKEY_NO_OTA_CHECK_VERSION
#include "httpOTAdownloader.h"

int g_fd = -1;

int vnet_send_msg(int cmd, const char* fmt, ...) {
    char buf[256];
    int len;
    va_list args;

    len = sprintf(buf, "custom_msg vnet0 30 %d ", cmd);
    va_start(args, fmt);
    len += vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    va_end(args);
    if (len < 0 || len >= sizeof(buf)) {
        return -1;
    }
    LOG_INFO(APP_NAME "send msg: %s\n", buf);
    system(buf);
    return 0;
}


enum CUST_CMD_COMMAND_IND_CMD {
    CMD_NET_DISTRIBUTE = 1,
    CMD_DRV_INIT_SUCCESS = 0x15,
    CMD_OTAINFOSYNC = 0x16,
    CMD_HOSTTST = 0x18,
    CMD_HWIDSYNC = 0x19,
    CMD_HWIDWRITE = 0x1A,
    CMD_TIMESYNC = 0x1B,
    CMD_HOSTREBOOT = 0x1c,
    CMD_DNSSYNC = 0x1d,
};

void CUST_CMD_COMMAND_IND_hander(char* mem)
{
    int cmd;
    char body[1024];
    // WifiConfig config;
    sscanf(mem, "MSG: %*d, cmd: %d", &cmd);
    switch (cmd) {
    case CMD_TIMESYNC:
    {
        SUTime time;
        struct tm* ptm;
        time_t seconds = 0;
        struct timeval tv;
        LOG_INFO(APP_NAME "CMD_TIMESYNC: %s\n", mem);
        int result = sscanf(mem, "MSG: %*d, cmd: %*d,ts=%lld", &seconds);
        if(seconds < 1782389256) {
            LOG_ERROR(APP_NAME "CMD_TIMESYNC: ts=%d is invalid\n", seconds);
            return;
        }
        ptm = gmtime(&seconds);
        time.sec = ptm->tm_sec;
        time.min = ptm->tm_min;
        time.hour = ptm->tm_hour;
        time.mday = ptm->tm_mday;
        time.mon = ptm->tm_mon + 1;
        time.year = ptm->tm_year + 1900;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
    }
    break;
    case CMD_OTAINFOSYNC: {
        LOG_INFO(APP_NAME "CMD_OTAINFOSYNC: mem %s\n", mem);
        char firm_name[128] = {0};
        char firm_ver[64] = {0};
        int firm_size = 0;
        char firm_md5[32] = {0};
        char url[512] = {0};
        char save_path[256] = {0};
        int result = sscanf(mem, "MSG: %*d, cmd: %*d,FIRM_NAME=%[^,],FIRM_VER=%[^,],FIRM_SIZE=%d,FIRM_MD5=%[^,],FIRM_URL=%s",firm_name, firm_ver, &firm_size, firm_md5, url);
        if (result == 5) {
            LOG_INFO(APP_NAME "OTA recv cmd: %d, firm_name: %s, firm_ver: %s, firm_size: %d, firm_md5: %s, url: %s\n", cmd, firm_name , firm_ver, firm_size, firm_md5, url);
        } else {
            LOG_ERROR(APP_NAME "OTA parse failed, only matched %d fields\n", result);
            return;
        }
        start_ota_download(url);   
    } break;
     case CMD_DNSSYNC:
    {
        LOG_INFO(APP_NAME "CMD_DNSSYNC: %s\n", mem);
        char dns0[64] = {0};
        char dns1[64] = {0};
        int result = sscanf(mem, "MSG: %*d, cmd: %*d,DNS0=%[^,],DNS1=%s", dns0, dns1);
        if (result == 2) {
            LOG_INFO(APP_NAME "recv cmd: %d, DNS0: %s, DNS1: %s\n", cmd, dns0, dns1);
            char dns_cmd[512] = {0};

            char resolv_buf[512] = {0};
            //读取/tmp/resolv.conf 所有内容
            FILE *fp = fopen("/tmp/resolv.conf", "r");
            if (fp != NULL) {
                fread(resolv_buf, 1, sizeof(resolv_buf) - 1, fp);
                fclose(fp);
            }

            snprintf(dns_cmd, sizeof(dns_cmd),
                    "printf '%s\\nnameserver %s\\nnameserver %s\\n' > /tmp/resolv.conf",
                    resolv_buf, dns0, dns1);


            system(dns_cmd);


            //删除/tmp/resolv.conf 文件中的空行（连续的换行符）
            char buf_tmp[512] = {0};
            FILE *fp_tmp = fopen("/tmp/resolv.conf", "r");
            if (fp_tmp != NULL) {
                char line[256] = {0};
                while (fgets(line, sizeof(line), fp_tmp) != NULL) {
                    if (strlen(line) > 1) {
                        strcat(buf_tmp, line);
                    }
                }
                fclose(fp_tmp);
            }
            //将删除空行后的内容写回/tmp/resolv.conf
            FILE *fp_tmp_write = fopen("/tmp/resolv.conf", "w");
            if (fp_tmp_write != NULL) {
                fwrite(buf_tmp, 1, strlen(buf_tmp), fp_tmp_write);
                fclose(fp_tmp_write);
            }

        } else {
            LOG_ERROR(APP_NAME "解析失败，只匹配到 %d 个字段\n", result);
            return;
        }
    }
    break;
    default:
        LOG_ERROR(APP_NAME "unknown cmd: %d\n", cmd);
        return;
    }
}

static void vnet_recv_msg(char *mem) {
    // MSG: 3003, ssid: sunvision-2.4G, rssi: -30, ip: 9564a8c0, gw: 0164a8c0, mk: 00ffffff
    int msg;

    sscanf(mem, "MSG: %x,", &msg);
    // auto wlan-settings
    switch (msg) {
    case CUST_CMD_CONNECT_IND:
        if ((mem = strstr(mem, "ip")) != NULL) {
            char buff[64];
            uint32_t ip, gw;
            sscanf(mem, "ip: %x, gw: %x,", &ip, &gw);

            // set vnet0 ip
            uint8_t* p = (uint8_t*)&ip;
            sprintf(buff, "ifconfig vnet0 %d.%d.%d.%d", p[0], p[1], p[2], p[3]);
            system(buff);
            LOG_INFO(APP_NAME "%x, %s\n", ip, buff);

            // set vnet0 gw
            p = (uint8_t*)&gw;
            sprintf(buff, "route add default gw %d.%d.%d.%d", p[0], p[1], p[2], p[3]);

            char dns_cmd[128] = {0};
            snprintf(dns_cmd, sizeof(dns_cmd),
                     "echo nameserver %d.%d.%d.%d > /tmp/resolv.conf",
                     p[0], p[1], p[2], p[3]);
            system(dns_cmd);
            system(buff);

             //删除/tmp/resolv.conf 文件中的空行（连续的换行符）
            char buf_tmp[512] = {0};
            FILE *fp_tmp = fopen("/tmp/resolv.conf", "r");
            if (fp_tmp != NULL) {
                char line[256] = {0};
                while (fgets(line, sizeof(line), fp_tmp) != NULL) {
                    if (strlen(line) > 1) {
                        strcat(buf_tmp, line);
                    }
                }
                fclose(fp_tmp);
            }
            //将删除空行后的内容写回/tmp/resolv.conf
            FILE *fp_tmp_write = fopen("/tmp/resolv.conf", "w");
            if (fp_tmp_write != NULL) {
                fwrite(buf_tmp, 1, strlen(buf_tmp), fp_tmp_write);
                fclose(fp_tmp_write);
            }

            LOG_INFO(APP_NAME "%x, %s\n", gw, buff);

            system("ifconfig");
            system("route -n");
        }
        break;
    case CUST_CMD_COMMAND_IND: 
        CUST_CMD_COMMAND_IND_hander(mem);
        break;
    default:
        LOG_INFO("not handle: 0x%x\n", msg);
    }
}

static void signal_handler(int signum) {
    int ret = 0;
    char data_buf[sizeof(rwnx_fasync_info_t)];
    rwnx_fasync_info_t *fsy_info = (rwnx_fasync_info_t *)data_buf;
    ret = read(g_fd, data_buf, sizeof(rwnx_fasync_info_t));
    if (ret < 0) {
        LOG_INFO(APP_NAME "Read kernel-data fail\n");
    } else {
        LOG_INFO(APP_NAME "%s\n", fsy_info->mem);
        fsy_info->mem_status = 0;
        ret = write(g_fd, &fsy_info->mem_status, sizeof(fsy_info->mem_status));
        if (ret < 0)
            LOG_INFO(APP_NAME "Write kernel-data fail\n");

        vnet_recv_msg(fsy_info->mem);
    }
}

int fasync_start() {
    system("vnet.sh");
    int i= 0, flag;
    const char* DEV_PATH = "/dev/aic_cdev";
    while (i < 5) {
        g_fd = open(DEV_PATH, O_RDWR);
        if (g_fd >= 0) {
            break;
        }
        system("mdev -s");
        usleep(200000);
        i++;
    }
    LOG_INFO(APP_NAME "open %s = %d\r\n", DEV_PATH, g_fd);
    if (g_fd < 0) {
        return 0;
    }

    signal(SIGIO, signal_handler);

    fcntl(g_fd, F_SETOWN, getpid());
    flag = fcntl(g_fd, F_GETFD);
    fcntl(g_fd, F_SETFL, flag|FASYNC);


    char buf[256] = {0};
    sprintf(buf,"custom_msg vnet0 30 %d 'ready'", CMD_DRV_INIT_SUCCESS);
    system(buf);
    return 1;
}


static void *configNet_thread(void *arg) {
    int try_count = 0;
    while (!fasync_start() && try_count++ < 5) {
        LOG_ERROR("fasync_start failed\n");
        sleep(1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t tid;
    pthread_create(&tid, NULL, configNet_thread, NULL);
    pthread_detach(tid);

    // 主循环或其他逻辑...
    while (1) {
        pause();  // 等待信号 (SIGIO)
    }
    return 0;
}