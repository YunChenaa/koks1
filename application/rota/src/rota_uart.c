/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_uart.h"
#include "rota_control.h"
#include "rota_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/reboot.h>
#include <termios.h>
#include <unistd.h>

#define MAX_EVENT 5

typedef struct {
    int baudrate;
    int fd;
    int cdc_fd;
    int cdc_is_init;
    int pipe_fd[2];
    pthread_t thread_id;
} RotaUart;

static RotaUart g_uart;
volatile uint8_t g_uart_run = 0;

static speed_t get_baud_rate(int baud)
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1500000:
        return B1500000;
    default:
        return -1;
    }
}

static void WritePipe(char data)
{
    write(g_uart.pipe_fd[1], &data, 1);
}

int rota_uart_init(const char* uart_path)
{
    memset(&g_uart, 0, sizeof(g_uart));
    g_uart.fd = -1;
    g_uart.cdc_fd = -1;
    g_uart.cdc_is_init = 0;

    // 打开串口设备
    g_uart.fd = open(uart_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (g_uart.fd < 0) {
        LOG_ERROR("Failed to open uart device\n");
        goto error_exit;
    }

    struct termios opt;
    tcgetattr(g_uart.fd, &opt);
    cfmakeraw(&opt);
    opt.c_cc[VMIN] = 0;
    opt.c_cc[VTIME] = 10;

    cfsetospeed(&opt, B115200);
    cfsetispeed(&opt, B115200);

    tcflush(g_uart.fd, TCIOFLUSH);
    // 设置数据位数
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    // 校验位
    opt.c_cflag &= ~PARENB;
    opt.c_iflag &= ~INPCK;
    // 设置停止位
    opt.c_cflag &= ~CSTOPB;
    tcsetattr(g_uart.fd, TCSANOW, &opt);

    if (pipe(g_uart.pipe_fd) == -1) {
        LOG_ERROR("Failed to create pipe, error %s\n", strerror(errno));
        goto error_exit;
    }

    return 0;

error_exit:
    if (g_uart.fd > 0) {
        close(g_uart.fd);
        g_uart.fd = -1;
    }
    return -1;
}

int rota_uart_cdc_init(const char* cdc_path)
{
    if (g_uart.cdc_is_init == 1) {
        return 0;
    }

    for (int i = 0; i < 20; ++i) {
        g_uart.cdc_fd = open(cdc_path, O_RDWR | O_NONBLOCK);
        if (g_uart.cdc_fd < 0) {
            LOG_INFO("Open cdc device error: %s, retrying %d\n", strerror(errno), i);
            usleep(10 * 1000);
        }
        else {
            LOG_INFO("Open cdc device OK\n");
            break;
        }
    }

    if (g_uart.cdc_fd < 0) {
        LOG_ERROR("Failed to open cdc device\n");
        goto error_exit;
    }

    struct termios cdc_ios;
    if (tcgetattr(g_uart.cdc_fd, &cdc_ios) < 0) {
        LOG_ERROR("Failed to get cdc termios, error %s\n", strerror(errno));
        goto error_exit;
    }
    cdc_ios.c_cflag &= ~CSIZE;
    cdc_ios.c_cflag |= CS8;
    cdc_ios.c_cflag &= ~PARENB;
    cdc_ios.c_cflag &= ~CSTOPB;
    cdc_ios.c_cflag |= CREAD | CLOCAL;
    cdc_ios.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    cdc_ios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    cdc_ios.c_oflag &= ~OPOST;
    cfsetispeed(&cdc_ios, B921600);
    cfsetospeed(&cdc_ios, B921600);
    if (tcsetattr(g_uart.cdc_fd, TCSANOW, &cdc_ios) < 0) {
        LOG_ERROR("Failed to set cdc termios, error %s\n", strerror(errno));
        goto error_exit;
    }
    if (tcflush(g_uart.cdc_fd, TCIOFLUSH) < 0) {
        LOG_ERROR("Failed to flush cdc termios, error %s\n", strerror(errno));
        goto error_exit;
    }
    int flags = fcntl(g_uart.cdc_fd, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERROR("Failed to get cdc file descriptor flags, error %s\n", strerror(errno));
        goto error_exit;
    }
    if (fcntl(g_uart.cdc_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_ERROR("Failed to set cdc file descriptor to non-blocking mode, error %s\n", strerror(errno));
        goto error_exit;
    }
    WritePipe('c');
    g_uart.cdc_is_init = 1;
    return 0;

error_exit:
    if (g_uart.cdc_fd > 0) {
        close(g_uart.cdc_fd);
        g_uart.cdc_fd = -1;
    }
    return -1;
}

int rota_uart_cdc_deinit()
{
    WritePipe('d');
    g_uart.cdc_is_init = 0;
}

int rota_uart_set_baudrate(int baudrate)
{
    speed_t speed = get_baud_rate(baudrate);
    if (speed == -1) {
        LOG_ERROR("Unsupported baudrate %d\n", baudrate);
        return -1;
    }

    struct termios options;
    if (tcgetattr(g_uart.fd, &options) != 0) {
        LOG_ERROR("tcgetattr failed\n");
        return -1;
    }

    cfsetspeed(&options, B0);
    if (cfsetspeed(&options, speed) != 0) {
        LOG_ERROR("cfsetspeed failed\n");
        return -1;
    }

    if (tcsetattr(g_uart.fd, TCSANOW, &options) != 0) {
        LOG_ERROR("tcsetattr failed\n");
        return -1;
    }

    tcflush(g_uart.fd, TCIOFLUSH);
    g_uart.baudrate = baudrate;
    return 0;
}

int rota_uart_deinit()
{
    LOG_DEBUG("%s\n", __func__);
    if (g_uart.fd > 0) {
        close(g_uart.fd);
        g_uart.fd = -1;

        if (g_uart.pipe_fd[0] > 0) {
            close(g_uart.pipe_fd[0]);
            close(g_uart.pipe_fd[1]);
        }
    }
}

static void* rota_uart_listen(void* argv)
{
    int i;
    int ret;
    int errnum = 0;
    int read_length = 0;
    unsigned char uart_buf[512]; // 独立的缓冲区
    unsigned char cdc_buf[512];  // 独立的缓冲区
    struct epoll_event ev, events[MAX_EVENT];

    // 初始化epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("%s %d: epoll_create1 failed, error %d\n", __FUNCTION__, __LINE__, errno);
        goto error_exit;
    }

    // 添加UART文件描述符到epoll
    ev.events = EPOLLIN;
    ev.data.fd = g_uart.fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, g_uart.fd, &ev) == -1) {
        LOG_ERROR("epoll_ctl add uart fd failed, error %d\n", errno);
        goto error_exit;
    }

    ev.events = EPOLLIN;
    ev.data.fd = g_uart.pipe_fd[0];
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, g_uart.pipe_fd[0], &ev) == -1) {
        LOG_ERROR("epoll_ctl add pipe fd failed, error %d\n", errno);
        goto error_exit;
    }

    while (g_uart_run) {
        ret = epoll_wait(epoll_fd, events, MAX_EVENT, 1000);
        if (ret == 0) {
            // LOG_VERBOSE("epoll timeout, try again\n");
            continue;
        } else if (ret < 0) {
            LOG_ERROR("epoll_wait ret: %d, error %d\n", ret, errno);
            if (errno == EAGAIN || errno == EINTR) {
                // usleep(10);
                continue;
            }
            break;
        }

        for (i = 0; i < ret; ++i) {
            if (events[i].data.fd == g_uart.fd) {
                read_length = read(g_uart.fd, uart_buf, sizeof(uart_buf));
                if (read_length <= 0) {
                    errnum = errno;
                    LOG_ERROR("%s %d: read error,errno: %d, errstr: %s\n", __FUNCTION__, __LINE__, errnum, strerror(errnum));
                    if (EAGAIN == errnum || EINTR == errnum) {
                        usleep(30000);
                        LOG_WARNING("retry\n");
                        continue;
                    }
                    if (read_length == 0 && EPIPE == errnum) {
                        LOG_ERROR("PIPE Brocker.\n");
                        continue;
                    }
                } else {
                    // LOG_INFO("received data size: %ld\n", read_length);
                    rota_control_process_data(uart_buf, read_length, 0);
                }
            } else if (events[i].data.fd == g_uart.pipe_fd[0]) {
                char buf;
                read(g_uart.pipe_fd[0], &buf, 1); // 读取管道数据，唤醒 epoll_wait
                if (buf == 'c') {
                    LOG_INFO("received cdc add signal\n");
                    if (g_uart.cdc_fd >= 0) {
                        // 添加CDC文件描述符到epoll
                        ev.events = EPOLLIN; //| EPOLLET;
                        ev.data.fd = g_uart.cdc_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, g_uart.cdc_fd, &ev) == -1) {
                            LOG_ERROR("epoll_ctl add cdc fd failed, error %d\n", errno);
                            goto error_exit;
                        }
                    }
                } else if (buf == 'd') {
                    LOG_INFO("received cdc del signal\n");
                    ev.events = EPOLLIN; //| EPOLLET;
                    ev.data.fd = g_uart.cdc_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, g_uart.cdc_fd, &ev) == -1) {
                        LOG_ERROR("epoll_ctl del cdc fd failed, error %d, system will reboot\n", errno);
                        reboot(RB_AUTOBOOT);
                    }
                    usleep(100 * 1000);
                    ret = close(g_uart.cdc_fd);
                    if (ret < 0) {
                        LOG_ERROR("Failed to close cdc fd, error %s\n", strerror(errno));
                        reboot(RB_AUTOBOOT);
                    }
                    g_uart.cdc_fd = -1; // 重置 g_uart.cdc_fd
                } else {
                    LOG_DEBUG("received pipe wakeup signal\n");
                    break;
                }
            } else if (events[i].data.fd == g_uart.cdc_fd) {
                memset(cdc_buf, 0, sizeof(cdc_buf));
                read_length = read(g_uart.cdc_fd, cdc_buf, sizeof(cdc_buf));
                // LOG_INFO("g_uart.cdc_fd %d,read_length: %d\n", g_uart.cdc_fd, read_length);
                if (read_length <= 0) {
                    errnum = errno;
                    LOG_ERROR("g_uart.cdc_fd %d, read cdc error,errno: %d, errstr: %s\n", g_uart.cdc_fd, errnum, strerror(errnum));
                    if (errnum == ENOENT || errnum == ESPIPE) {
                        if (g_uart.cdc_fd >= 0) {
                            // ev.events = EPOLLIN; //| EPOLLET;
                            // ev.data.fd = uart_cdc_fd;
                            // if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, uart_cdc_fd, &ev) == -1) {
                            //     LOG_ERROR("epoll_ctl add cdc fd failed, error %d, system will reboot\n", errno);
                            //     reboot(RB_AUTOBOOT);
                            // }
                            // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uart_cdc_fd, &ev) == -1) {
                            //     LOG_ERROR("epoll_ctl add cdc fd failed, error %d, system will reboot\n", errno);
                            //     reboot(RB_AUTOBOOT);
                            // }
                            LOG_ERROR("cdc device %s not found, system will reboot\n", "/dev/ttyGS0");
                            reboot(RB_AUTOBOOT);
                        }
                    }
                    if (EAGAIN == errnum || EINTR == errnum) {
                        usleep(30000);
                        LOG_WARNING("retry\n");
                        continue;
                    }
                    if (read_length == 0 && EPIPE == errnum) {
                        LOG_ERROR("PIPE Brocker.\n");
                        continue;
                    }
                } else {
                    // LOG_INFO("received cdc data size: %ld\n", read_length);
                    rota_control_process_data(cdc_buf, read_length, 1);
                }
            }
        }
    }

error_exit:
    if (epoll_fd > 0) {
        close(epoll_fd);
    }
    return NULL;
}

void rota_uart_start()
{
    LOG_DEBUG("%s\n", __func__);
    g_uart_run = 1;
    g_uart.thread_id = 0;
    int ret = pthread_create(&g_uart.thread_id, NULL, rota_uart_listen, NULL);
    if (ret != 0) {
        LOG_ERROR("pthread_create failed: %s\n", strerror(ret));
    }
}

void rota_uart_wait()
{
    if (g_uart.thread_id > 0) {
        pthread_join(g_uart.thread_id, NULL);
    }
    LOG_DEBUG("%s end\n", __func__);
}

void rota_uart_send(const uint8_t* buf, uint16_t bufSize, int type)
{
    int total_written = 0;
    int attempts = 0;
    const int max_attempts = 100;
    int fd = (type == 1) ? g_uart.cdc_fd : g_uart.fd;

    while (fd > 0 && total_written < bufSize && attempts < max_attempts) {
        int written = write(fd, buf + total_written, bufSize - total_written);

        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                attempts++;
                usleep(10 * 1000); // 10ms
                continue;
            } else {
                LOG_ERROR("%s write error: %s\n", __func__, strerror(errno));
                return;
            }
        }

        total_written += written;
        attempts = 0;

        if (written == 0) {
            usleep(10 * 1000); // 10ms
        }
    }

    if (total_written != bufSize) {
        LOG_ERROR("%s write %u/%u bytes\n", __func__, total_written, bufSize);
    }
}
