#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "qrcode_scan.h"
#include "sys_logger.h"
#include "wifi.h"
#include "su_base.h"
#include "LCDCameraPlayer.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <net/if_arp.h>
#ifdef KOKS_FEATURE_OVD
#include "cmcc_thread_pool.h"
#include "ovd.h"
#include "httpOTAdownloader.h"
#include "RTSP_main.h"
#endif
#ifdef CMCC_VOIP
#include "cmcc_voip_fun.h"
#endif
#include "factory.h"

#include <atomic>
#include <thread>
#include <unistd.h>
#include <future> 

// ========== 无锁消息队列 ==========
#define MSG_QUEUE_SIZE  32
#define MSG_MAX_LEN     2048

struct MsgNode {
    char data[MSG_MAX_LEN];
};

static MsgNode  sg_msg_buf[MSG_QUEUE_SIZE];
static std::atomic<int> sg_msg_wr{0};   // 生产者写位置（signal handler）
static std::atomic<int> sg_msg_rd{0};   // 消费者读位置（worker 线程）
static int      sg_notify_pipe[2];      // pipe: 用于唤醒 worker 线程
static bool     Is_vnet0_configured = false;



int g_fd = -1;

extern void OVD_cancel_schedule_hibernate();
static std::atomic<bool> g_is_scanning{false};
static std::future<void> g_current_scan_future;

extern "C" int vnet_send_msg(int cmd, const char* fmt, ...) {
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

CallbackList callbackList = {
    .on_network_connected = NULL,
    .on_get_device_info = NULL,
    .get_image_data_func = NULL,
    .on_qrscan_success = NULL,
    .on_hiwd_check = NULL,
    .get_hiwd=NULL,
};

void register_callbacks(CallbackList *callbacks) {
    callbackList.on_network_connected = callbacks->on_network_connected;
    callbackList.on_get_device_info = callbacks->on_get_device_info;
    callbackList.get_image_data_func = callbacks->get_image_data_func;
    callbackList.on_qrscan_success = callbacks->on_qrscan_success;
    callbackList.on_hiwd_check = callbacks->on_hiwd_check;
    callbackList.get_hiwd = callbacks->get_hiwd;
    callbackList.OVD_updatechanelstateOnline = callbacks->OVD_updatechanelstateOnline;
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
    CMD_HOSTGOTOSLEEP=0x26,
    CMD_UPIFCONFIG = 0x27
};

int HOST_TST_MODE = 0; // 0: normal mode, 1: test mode

static char deviceid[64] = {0};
static char cmei[64] = {0};
static char password[73] = {0};
static char MAC[64] = {0};
static char hiwd[73] = {0};

#ifdef KOKS_FEATURE_OVD
std::atomic<bool> ManagedThread::s_blockStart{false};
std::atomic<bool> EventThread::s_blockStart{false};
bool is_production_test_mode = false;
static std::once_flag rtsp_once_flag; 

void entry_production_test()
{
    if(!is_production_test_mode)
    {
        is_production_test_mode = true;
        stop_qrcode_scan();
        ovd_stop();
#ifdef CMCC_VOIP
        voip_stop();
#endif
        std::call_once(rtsp_once_flag, [](){
            pcm_gateway_run();
            std::thread rtsp_thread(RTSPH264VideoServerMediaSubsession_thread_main);
            rtsp_thread.detach();
        });   
    }
}
#endif

void CUST_CMD_COMMAND_IND_hander(char* mem)
{

    int cmd;
    char body[1024];
    WifiConfig config;
    sscanf(mem, "MSG: %*d, cmd: %d", &cmd);
    switch (cmd) {
    case CMD_NET_DISTRIBUTE:
    {
        bool expected = false;
        if (!g_is_scanning.compare_exchange_strong(expected, true)) {
            LOG_WARNING(APP_NAME "QR scan already in progress, ignore this command\n");
            break;
        }

        auto cfg = config;
        auto get_img = callbackList.get_image_data_func;
        auto on_scan = callbackList.on_qrscan_success;

        g_current_scan_future = std::async(std::launch::async, [cfg, get_img, on_scan]() mutable {
            struct FlagResetter {
                ~FlagResetter() { g_is_scanning.store(false); }
            } resetter;

            LOG_INFO(APP_NAME "start scan QR code (async thread)\n");

            if (get_qrcode(cfg, get_img, on_scan) < 0) {
                LOG_ERROR(APP_NAME "scan QR code failed\n");
                return;
            }

            if (cfg.ssid.empty()) {
                LOG_ERROR(APP_NAME "SSID is empty\n");
                return;
            }

            char body[512] = {0};
            int len = snprintf(body, sizeof(body), "ssid=%s,extra=%s", cfg.ssid.c_str(), cfg.extra.c_str());
            if (len < 0 || (size_t)len >= sizeof(body)) {
                LOG_ERROR(APP_NAME "body is too long, len: %d\n", len);
                return;
            }

            if (!cfg.password.empty() && len >= 0 && (size_t)len < sizeof(body)) {
                snprintf(body + len, sizeof(body) - len, ",password=%s", cfg.password.c_str());
            }

            LOG_INFO(APP_NAME "%s\n", body);
            vnet_send_msg(CMD_NET_DISTRIBUTE, "%s", body);
        });
    } 
    break;
    case CMD_DRV_INIT_SUCCESS: {
        LOG_INFO(APP_NAME "CMD_DRV_INIT_SUCCESS: %s\n", mem);
        int result = sscanf(mem, "MSG: %*d, cmd: %*d,OVD_DEVICEID=%[^,],OVD_CMEI=%[^,],OVD_PASSWORD=%[^,],MAC=%s", deviceid, cmei, password, MAC);
        if (result == 4) {
            LOG_INFO(APP_NAME "recv cmd: %d, deviceid: %s, cmei: %s, password: %s, MAC: %s\n", cmd, deviceid, cmei, password, MAC);
        } else {
            if(cmd != 1) {
                LOG_ERROR(APP_NAME "解析失败，只匹配到 %d 个字段\n", result);
                return;
            }
        }
        if (result != 1) {
            if (callbackList.on_get_device_info) {
                callbackList.on_get_device_info(deviceid, cmei, password, MAC);
            }
        }
    } break;
    
    case CMD_OTAINFOSYNC: {
        OVD_cancel_schedule_hibernate();
        LOG_INFO(APP_NAME "CMD_OTAINFOSYNC: mem %s\n", mem);
        char firm_name[128] = {0};
        char firm_ver[64] = {0};
        int firm_size = 0;
        char firm_md5[32] = {0};
        char url[512] = {0};
        char save_path[256] = {0};
    
         //MSG: 3044, cmd: 22,FIRM_NAME=aic8800mc_v1.0.1_ota.bin,FIRM_VER=1.0.2,FIRM_SIZE=911144,FIRM_MD5=013c56ca362894327702275e460ac824,FIRM_URL=https://oss7.komect.com/fota/592282/aic8800mc_v1.0.1_ota.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20260521T113708Z&X-Amz-SignedHeaders=host&X-Amz-Expires=3600&X-Amz-Credential=D656MB8V0EUBQSG7VTXS%2F20260521%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Signature=ce7f074a0ebe39f874208e386f8565dced4c130e92726c0021d5c4fb6a95715a

        int result = sscanf(mem, "MSG: %*d, cmd: %*d,FIRM_NAME=%[^,],FIRM_VER=%[^,],FIRM_SIZE=%d,FIRM_MD5=%[^,],FIRM_URL=%s",firm_name, firm_ver, &firm_size, firm_md5, url);

        if (result == 5) {
            LOG_INFO(APP_NAME "OTA recv cmd: %d, firm_name: %s, firm_ver: %s, firm_size: %d, firm_md5: %s, url: %s\n", cmd, firm_name , firm_ver, firm_size, firm_md5, url);
        } else {
            LOG_ERROR(APP_NAME "OTA 解析失败，只匹配到 %d 个字段\n", result);
            return;
        }

#ifdef KOKS_FEATURE_OVD
        start_ota_download(url);
#endif
   
    } break;
    case CMD_HOSTTST:
        OVD_cancel_schedule_hibernate();
        LOG_INFO(APP_NAME "CMD_HOSTTST\n");
#ifdef KOKS_FEATURE_OVD
        ManagedThread::setBlockStart(true);
        EventThread::setBlockStart(true);
        entry_production_test();
#endif
        break;
    case CMD_HWIDSYNC: {
        LOG_INFO(APP_NAME "CMD_HWIDSYNC: %s\n", mem);
        int result = sscanf(mem, "MSG: %*d, cmd: %*d,HWID=%s", hiwd);
        if (result == 1) {
            LOG_INFO(APP_NAME "recv cmd: %d, HWID: %s\n", cmd, hiwd);
        } else {
            if (callbackList.get_hiwd) {
                int ret = callbackList.get_hiwd(deviceid, hiwd);
                if (ret == 0) {
                    LOG_INFO(APP_NAME "get hiwd success: %s\n", hiwd);
                     char buf[256] = {0};
                    sprintf(buf,"custom_msg vnet0 30 %d HWID=%s", CMD_HWIDWRITE, hiwd);
                    system(buf);
                } else {
                    LOG_ERROR(  APP_NAME "get hiwd failed\n");
                }
            }
        }
        if (callbackList.on_hiwd_check) {
            callbackList.on_hiwd_check(deviceid, hiwd);
        }
        break;
    }
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
    case CMD_HOSTREBOOT:
    {
        LOG_INFO(APP_NAME "CMD_HOSTREBOOT\n");
        system("sync");
        system("reboot");
    }
    break;
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
    case CMD_UPIFCONFIG:
    {
        LOG_INFO(APP_NAME "CMD_UPIFCONFIG, mem: %s\n", mem);
        // CMD_UPIFCONFIG, mem: MSG: 3044, cmd: 39,IP:192.168.100.100,MK:255.255.255.0,GW:192.168.100.1

        if(Is_vnet0_configured == false)
        {
            Is_vnet0_configured = true;
            system("ifconfig vnet0 up");
            char ip[32] = {0};
            char mk[32] = {0};
            char gw[32] = {0};
            int result = sscanf(mem, "MSG: %*d, cmd: %*d,IP:%[^,],MK:%[^,],GW:%s", ip, mk, gw);


            if (result == 3) {
                LOG_INFO(APP_NAME "recv cmd: %d, IP: %s, MK: %s, GW: %s\n", cmd, ip, mk, gw);
                char buff[128] = {0};
                sprintf(buff, "ifconfig vnet0 %s netmask %s", ip, mk);
                system(buff);
                sprintf(buff, "route add default gw %s", gw);
                system(buff);   

                system("ifconfig");
                system("route -n");
            }else {
                LOG_ERROR(APP_NAME "failed to parse CMD_UPIFCONFIG, only matched %d fields\n", result);
            }
        }   
    }   
    break;
    case CMD_HOSTGOTOSLEEP:
    {
        LOG_INFO(APP_NAME "CMD_HOSTGOTOSLEEP\n");
        if (callbackList.OVD_updatechanelstateOffline) {
            callbackList.OVD_updatechanelstateOffline();
        }  
        system("custom_msg vnet0 5");
    }
    break;
    default:
        LOG_ERROR(APP_NAME "未知的命令: %d\n", cmd);
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
            Is_vnet0_configured = true;
            system("ifconfig vnet0 up");
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

        

            char resolv_buf[512] = {0};
            //读取/tmp/resolv.conf 所有内容
            FILE *fp = fopen("/tmp/resolv.conf", "r");
            if (fp != NULL) {
                fread(resolv_buf, 1, sizeof(resolv_buf) - 1, fp);
                fclose(fp);
            }


            char dns_cmd[648] = {0};
            snprintf(dns_cmd, sizeof(dns_cmd),
                    "printf 'nameserver %d.%d.%d.%d\\n%s\\n' > /tmp/resolv.conf",
                     p[0], p[1], p[2], p[3], resolv_buf);


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
            if(callbackList.on_network_connected) {
                callbackList.on_network_connected(true);
            }
        }
        break;
    case CUST_CMD_COMMAND_IND: 
        CUST_CMD_COMMAND_IND_hander(mem);
        break;
    case CUSTMO_MSG_LOCK_EVENT_IND: 
        {
            //ep: mem MSG: 3200, lock event: 4
            enum CMD_EVENT_TYPE event_type;
            sscanf(mem, "MSG: %x, lock event: %d", &msg, &event_type);
            if(event_type == CMD_EVENT_BELL || event_type == CMD_EVENT_STAY || event_type == CMD_EVENT_TAMPER){
#ifdef KOKS_FEATURE_OVD
                trigger_alarm(event_type);
#endif
            }
#ifdef CMCC_VOIP
            if(event_type == CMD_EVENT_BELL && VoipManager::getInstance().get_voip_flag_info().isVoipTriggeredByButton == 0
                    && VoipManager::getInstance().get_voip_flag_info().g_calling == 0){
                VoipManager::getInstance().get_voip_flag_info().isVoipTriggeredByButton = 1;
            }
#endif
            if (event_type == CMD_EVENT_CLOSESCREEN) {
                LCDCameraPlayer::getInstance()->StopPlay();
            } else if(event_type == CMD_EVENT_OPENSCREEN){
                LCDCameraPlayer::getInstance()->StartPlay();
            }
            LOG_INFO(APP_NAME "recv lock event: %d\n", event_type);
        } 
        break;
    case 0x300e:
    {
        system("ifconfig vnet0 down");
        Is_vnet0_configured = false;
        // system("echo 0 > /sys/bus/usb/devices/1-1/authorized");
    }
    break;
    default:
        LOG_INFO("not handle: 0x%x\n", msg);
    }
}

// ★ 信号处理函数：在中断上下文中调用，不能在此函数中执行耗时操作与阻塞
static void signal_handler(int signum) {
    int ret = 0;
    char data_buf[sizeof(rwnx_fasync_info_t)];
    rwnx_fasync_info_t *fsy_info = (rwnx_fasync_info_t *)data_buf;

    ret = read(g_fd, data_buf, sizeof(rwnx_fasync_info_t));
    if (ret < 0) {
        LOG_ERROR(APP_NAME "Read kernel-data fail: %s\n", strerror(errno));
    } else {
        int wr = sg_msg_wr.load(std::memory_order_relaxed);
        int next = (wr + 1) % MSG_QUEUE_SIZE;
        if (next != sg_msg_rd.load(std::memory_order_acquire)) {
            strncpy(sg_msg_buf[wr].data, fsy_info->mem, MSG_MAX_LEN - 1);
            sg_msg_buf[wr].data[MSG_MAX_LEN - 1] = '\0';
            sg_msg_wr.store(next, std::memory_order_release);
            char c = 1;
            write(sg_notify_pipe[1], &c, 1);
        }
        fsy_info->mem_status = 0;
        ret = write(g_fd, &fsy_info->mem_status, sizeof(fsy_info->mem_status));
    }
}

// ★ 消息处理线程：在安全的普通线程上下文中调用 vnet_recv_msg
static void msg_worker_thread() {
    while (true) {
        char c;
        // 阻塞等待信号通知
        if (read(sg_notify_pipe[0], &c, 1) < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // 非阻塞地消费队列中所有消息
        while (true) {
            int rd = sg_msg_rd.load(std::memory_order_relaxed);
            if (rd == sg_msg_wr.load(std::memory_order_acquire)) {
                break;  // 队列已空
            }

            vnet_recv_msg(sg_msg_buf[rd].data);  // ★ 安全调用！

            sg_msg_rd.store((rd + 1) % MSG_QUEUE_SIZE, std::memory_order_release);
        }
    }
}

bool fasync_start() {
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
        return false;
    }
    if (pipe(sg_notify_pipe) < 0) {
        LOG_ERROR(APP_NAME "pipe create fail\n");
        close(g_fd);
        return false;
    }

    signal(SIGIO, signal_handler);

    fcntl(g_fd, F_SETOWN, getpid());
    flag = fcntl(g_fd, F_GETFD);
    fcntl(g_fd, F_SETFL, flag | FASYNC);

    std::thread(msg_worker_thread).detach();

    char buf[256] = {0};
    sprintf(buf,"custom_msg vnet0 30 %d 'ready'", CMD_DRV_INIT_SUCCESS);
    system(buf);
    return true;
}
