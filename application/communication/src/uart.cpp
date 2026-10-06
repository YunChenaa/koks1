/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-12 16:37:38
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-18 12:28:34
 * @FilePath: /sense-koks/application/communication/src/uart.cpp
 */
#include "uart.h"
#include "sys_logger.h"
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/reboot.h>
#include <unistd.h>
namespace st
{
namespace sense_koks
{

Uart& Uart::GetInstance(void)
{
    static Uart uart_dev{ "/dev/ttyS1", B115200};
    return uart_dev;
}

Uart::Uart(const std::string& path, const speed_t baudRate)
        : _path(path), _baudRate(baudRate)
{
    control = SenseControl::getInstance();
}

void Uart::SetConfig(const std::string& path, const speed_t baudRate)
{
    _path = path;
    _baudRate = baudRate;
}

void Uart::Init(void)
{
    // struct timeval tv;
	// gettimeofday(&tv, NULL);
    // auto start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    uart_fd = open(_path.c_str(), O_RDWR | O_NOCTTY);
    if (uart_fd < 0)
    {
        LOG_ERROR("Fail to Open %s device:%d\n", _path, uart_fd);
        return;
    }

    struct termios opt;
	tcgetattr(uart_fd, &opt);
    cfmakeraw(&opt);
    opt.c_cc[VMIN] = 0;
    opt.c_cc[VTIME] = 10;

    cfsetospeed(&opt, _baudRate);
    cfsetispeed(&opt, _baudRate);

    tcflush(uart_fd, TCIOFLUSH);
    opt.c_cflag &= ~CRTSCTS;
    opt.c_cflag &= ~(CLOCAL | CREAD);
    //设置数据位数
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    //校验位
    opt.c_cflag &= ~PARENB;
    opt.c_iflag &= ~INPCK;
    opt.c_iflag &= ~(IXON | IXOFF | IXANY); // 关闭软件流控
    //设置停止位
    opt.c_cflag &= ~CSTOPB;
    opt.c_cflag |= (CLOCAL | CREAD);

    opt.c_lflag = 0; // 关闭规范模式，不回显
    opt.c_oflag = 0; // 关闭输出处理

    tcsetattr(uart_fd, TCSANOW, &opt);

    // gettimeofday(&tv, NULL);
    // auto t1 = tv.tv_sec * 1000 + tv.tv_usec / 1000 - start;

    if (pipe(pipe_fd) == -1) {
        LOG_ERROR("Failed to create pipe, error %s\n", strerror(errno));
        throw std::runtime_error("Failed to create pipe");
    }
    run = true;
    uart_process = std::make_shared<std::thread>(&Uart::UartProcessData, this);

    // gettimeofday(&tv, NULL);
    // auto t4 = tv.tv_sec * 1000 + tv.tv_usec / 1000 - start;
    // printf("uart init elapsed time:  %ld , %ldms\n", t1, t4);
}

void Uart::cdc_Init(const std::string& path)
{
    if (cdc_is_init == false) {
        uart_cdc_fd = ::open(path.data(), O_RDWR | O_NONBLOCK);
        if (uart_cdc_fd < 0) {
            LOG_ERROR("Failed to open /dev/ttyGS0, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to open /dev/ttyGS0");
        }
        struct termios cdc_ios;
        if (tcgetattr(uart_cdc_fd, &cdc_ios) < 0) {
            LOG_ERROR("Failed to get cdc termios, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to get cdc termios");
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
        if (tcsetattr(uart_cdc_fd, TCSANOW, &cdc_ios) < 0) {
            LOG_ERROR("Failed to set cdc termios, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to set cdc termios");
        }
        if (tcflush(uart_cdc_fd, TCIOFLUSH) < 0) {
            LOG_ERROR("Failed to flush cdc termios, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to flush cdc termios");
        }
        int flags = fcntl(uart_cdc_fd, F_GETFL, 0);
        if (flags < 0) {
            LOG_ERROR("Failed to get cdc file descriptor flags, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to get cdc file descriptor flags");
        }
        if (fcntl(uart_cdc_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            LOG_ERROR("Failed to set cdc file descriptor to non-blocking mode, error %s\n", strerror(errno));
            throw std::runtime_error("Failed to set cdc file descriptor to non-blocking mode");
        }
        WritePipe('c');
        cdc_is_init = true;
    }
}

void Uart::cdc_DeInit()
{
    WritePipe('d');
    cdc_is_init = false;
}

void Uart::Deinit(void)
{
    run = false;
    char buf = 'x';
    write(pipe_fd[1], &buf, 1);
    uart_process->join();
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    uart_process.reset();
    if (uart_fd >= 0) {
        close(uart_fd);
        uart_fd = -1;
    }
    if (uart_cdc_fd >= 0) {
        close(uart_cdc_fd);
        uart_cdc_fd = -1;
    }
}

void Uart::SetBaudrate(const speed_t baudRate)
{
    struct termios opt;
    _baudRate = baudRate;
    tcgetattr(uart_fd, &opt);
    cfsetspeed(&opt, baudRate);
    tcsetattr(uart_fd, TCSANOW, &opt);
}

speed_t Uart::GetBaudrate(void)
{
    return _baudRate;
}

void* Uart::UartProcessData(void)
{
    LOG_DEBUG("UartProcessData serial file descriptor %d\n", uart_fd);
    Listen();
    return nullptr;
}

void Uart::WriteByte(unsigned char charbuffer)
{
    write(uart_fd, &charbuffer, 1);
}

void Uart::WriteByteSequence(uint8_t* buffer, uint32_t size)
{
    write(uart_fd, buffer, size);
}

void Uart::cdc_WriteByteSequence(uint8_t* buffer, uint32_t size)
{
    if (uart_cdc_fd >= 0) {
        ::write(uart_cdc_fd, buffer, size);
    }
}

void Uart::Listen(void)
{
    int ret;
    int errnum = 0;
    int read_length = 0;
    unsigned char uart_buf[256]; // 独立的缓冲区
    unsigned char cdc_buf[512];  // 独立的缓冲区
    constexpr int MAX_EVENT = 5;
    struct epoll_event ev, events[MAX_EVENT];

    // 初始化epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("%s %d: epoll_create1 failed, error %d\n", __FUNCTION__, __LINE__, errno);
        return;
    }

    auto epoll_fd_ptr = std::unique_ptr<int, std::function<void(int*)>>(new int(epoll_fd), [](int* p) {
        ::close(*p);
        delete p;
    });
    // 添加UART文件描述符到epoll
    ev.events = EPOLLIN;
    ev.data.fd = uart_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uart_fd, &ev) == -1) {
        LOG_ERROR("epoll_ctl add uart fd failed, error %d\n", errno);
        return;
    }

    ev.events = EPOLLIN;
    ev.data.fd = pipe_fd[0];
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd[0], &ev) == -1) {
        LOG_ERROR("epoll_ctl add pipe fd failed, error %d\n", errno);
        return;
    }
    while (run) {
        ret = epoll_wait(epoll_fd, events, MAX_EVENT, -1);
        if (ret == 0) {
            LOG_VERBOSE("epoll timeout, try again\n");
            continue;
        } else if (ret < 0) {
            LOG_ERROR("epoll_wait ret: %d, error %d\n", ret, errno);
            if (errno == EAGAIN || errno == EINTR) {
                // usleep(10);
                continue;
            }
            break;
        }

        for (int i = 0; i < ret; ++i) {
            if (events[i].data.fd == uart_fd) {
                read_length = read(uart_fd, uart_buf, sizeof(uart_buf));
                if (read_length <= 0) {
                    errnum = errno;
                    if (read_length == 0) {
                        LOG_ERROR("uart device disconnected (EOF)\n");
                        continue;
                    }
                    LOG_ERROR("%s %d: read error,errno: %d, errstr: %s\n", __FUNCTION__, __LINE__, errnum, strerror(errnum));
                    if (EAGAIN == errnum || EINTR == errnum) {
                        usleep(30000);
                        LOG_WARNING("retry\n");
                        continue;
                    }
                } else {
                    // LOG_INFO("received data size: %ld\n", read_length);
                    control->process_data(uart_buf, read_length);
                }
            } else if (events[i].data.fd == pipe_fd[0]) {
                char buf;
                read(pipe_fd[0], &buf, 1); // 读取管道数据，唤醒 epoll_wait
                if (buf == 'c') {
                    LOG_INFO("received pipe wakeup signal\n");
                    if (uart_cdc_fd >= 0) {
                        // 添加CDC文件描述符到epoll
                        ev.events = EPOLLIN; //| EPOLLET;
                        ev.data.fd = uart_cdc_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uart_cdc_fd, &ev) == -1) {
                            LOG_ERROR("epoll_ctl add cdc fd failed, error %d\n", errno);
                            return;
                        }
                    }
                } else if (buf == 'd') {
                    LOG_INFO("received pipe close signal\n");
                    ev.events = EPOLLIN; //| EPOLLET;
                    ev.data.fd = uart_cdc_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, uart_cdc_fd, &ev) == -1) {
                        LOG_WARNING("epoll_ctl del cdc fd failed, error %d\n", errno);
                        // reboot(RB_AUTOBOOT);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ret = ::close(uart_cdc_fd);
                    if (ret < 0) {
                        LOG_ERROR("Failed to close uart_cdc_fd, error %s\n", strerror(errno));
                        // reboot(RB_AUTOBOOT);
                    }
                    uart_cdc_fd = -1; // 重置 uart_cdc_fd
                } else if (buf == 'a') {
                    cdc_TryAgain();
                } else if (buf == 'r') {
                    control->sendReadyMessage();
                } else {
                    LOG_INFO("received pipe wakeup signal\n");
                    break;
                }
            } else if (events[i].data.fd == uart_cdc_fd) {
                memset(cdc_buf, 0, sizeof(cdc_buf));
                read_length = read(uart_cdc_fd, cdc_buf, sizeof(cdc_buf));
                // LOG_INFO("uart_cdc_fd %d,read_length: %d\n", uart_cdc_fd, read_length);
                if (read_length <= 0) {
                    errnum = errno;

                    if (read_length == 0) {
                        LOG_WARNING("uart_cdc_fd %d, cdc device disconnected (EOF), will retry\n", uart_cdc_fd);

                        struct epoll_event dummy;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, uart_cdc_fd, &dummy);
                        ::close(uart_cdc_fd);
                        uart_cdc_fd = -1;
                        cdc_is_init = false;
                        WritePipe('a');
                        continue;
                    }

                    LOG_ERROR("uart_cdc_fd %d, read cdc error,errno: %d, errstr: %s\n", uart_cdc_fd, errnum, strerror(errnum));
                    if (errnum == ENOENT || errnum == ESPIPE) {
                        if (uart_cdc_fd >= 0 && cdc_is_init) {
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
                            LOG_ERROR("cdc device %s not found, will retry\n", "/dev/ttyGS0");
                            cdc_DeInit();
                            // reboot(RB_AUTOBOOT);
                            WritePipe('a');
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
                    control->cdc_process_data(cdc_buf, read_length);
                }
            }
        }
    }
}

void Uart::cdc_TryAgain()
{
    if (!cdc_is_init) {
        for (int i = 0; i < 3; ++i) {
            LOG_WARNING("cdc retry %d\n", i + 1);
            try {
                cdc_Init("/dev/ttyGS0");
                if (cdc_is_init) {
                    LOG_INFO("cdc retry ok\n");
                    return;
                }
            }
            catch (...) {
            }
            usleep(10 * 1000);
        }
        LOG_ERROR("cdc retry failed\n");
    }
}

Uart::~Uart()
{
    // run = false;
    // uart_process->join();
    // uart_process.reset();
    // serial_device.Close();
}
} // namespace sense_koks

} // namespace st
