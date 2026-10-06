/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-05-13 11:55:45
 * @LastEditors: 洪照
 * @LastEditTime: 2025-05-13 14:48:17
 * @FilePath: /sense-koks/application/entry/serial_sample.cpp
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE 512 // 每次读取的字节数

int main()
{
    int fd;
    unsigned char buffer[BUFFER_SIZE];
    struct termios options;
    ssize_t bytes_read;
    speed_t baud;

    // 打开串口
    fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open_port: Unable to open /dev/ttyS0 - ");
        return errno;
    }

    // 配置串口选项
    tcgetattr(fd, &options); // 获取当前串口配置

    // 设置波特率
    baud = B1500000;
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // 设置为原始模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;   // 8 数据位
    options.c_cc[VMIN] = 1;   // 最小接收字节数
    options.c_cc[VTIME] = 10; // 等待时间

    tcsetattr(fd, TCSANOW, &options); // 应用配置

    // 循环读取数据
    printf("Receiving data... Press Ctrl+C to stop\n");
    while (1) {
        bytes_read = read(fd, buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            perror("read");
            break;
        }
        if (bytes_read == 0) {
            printf("No more data\n");
            break;
        }

        printf("Received %zd bytes\n", bytes_read);

        // 处理接收到的数据
        // TODO: 在这里添加你的数据处理代码

        // 清空缓冲区以便下一次读取
        memset(buffer, 0, BUFFER_SIZE);
    }

    // 清理
    close(fd);

    return 0;
}