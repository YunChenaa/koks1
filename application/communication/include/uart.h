/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-12 16:37:42
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-18 11:47:52
 * @FilePath: /sense-koks/application/communication/include/uart.h
 */
#pragma once
#include "sense_control.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <termios.h>
namespace st
{
namespace sense_koks
{
class Uart
{
    private:
    std::atomic_bool run{ false };
    int pipe_fd[2];
    int uart_fd{ -1 };
    int uart_cdc_fd{ -1 };
    bool cdc_is_init{ false };
    std::string _path;
    speed_t _baudRate;
    SenseControl* control;
    std::shared_ptr<std::thread> uart_process;
    void* UartProcessData(void);
    void cdc_TryAgain();

    /* data */
    public:
    static Uart& GetInstance(void);
    Uart(const std::string& path, const speed_t baudRate);
    ~Uart();
    int WritePipe(char data)
    {
        write(pipe_fd[1], reinterpret_cast<void*>(&data), 1);
    }
    void SetConfig(const std::string& path, const speed_t baudRate);
    void Init(void);
    void cdc_Init(const std::string& path);
    void cdc_DeInit();
    void Deinit(void);
    void SetBaudrate(const speed_t baudRate);
    speed_t GetBaudrate(void);
    void WriteByte(unsigned char charbuffer);
    void WriteByteSequence(uint8_t* buffer, uint32_t size);
    void cdc_WriteByteSequence(uint8_t* buffer, uint32_t size);
    void Listen(void);

    static uint32_t rate2int(const speed_t baudRate)
    {
        uint32_t rate = 115200;
        switch (baudRate) {
        case B9600:
            rate = 9600;
            break;
        case B19200:
            rate = 19200;
            break;
        case B38400:
            rate = 38400;
            break;
        case B57600:
            rate = 57600;
            break;
        case B115200:
            rate = 115200;
            break;
        case B230400:
            rate = 230400;
            break;
        case B460800:
            rate = 460800;
            break;
        case B500000:
            rate = 500000;
            break;
        case B576000:
            rate = 576000;
            break;
        case B921600:
            rate = 921600;
            break;
        case B1000000:
            rate = 1000000;
            break;
        case B1152000:
            rate = 1152000;
            break;
        case B1500000:
            rate = 1500000;
            break;

        default:
            break;
        }
        return rate;
    }
    static speed_t int2rate(uint32_t baudRate)
    {
        speed_t rate = B115200;
        switch (baudRate) {
        case 9600:
            rate = B9600;
            break;
        case 19200:
            rate = B19200;
            break;
        case 38400:
            rate = B38400;
            break;
        case 57600:
            rate = B57600;
            break;
        case 115200:
            rate = B115200;
            break;
        case 230400:
            rate = B230400;
            break;
        case 460800:
            rate = B460800;
            break;
        case 500000:
            rate = B500000;
            break;
        case 576000:
            rate = B576000;

            break;
        case 921600:
            rate = B921600;
            break;
        case 1000000:
            rate = B1000000;
            break;
        case 1152000:
            rate = B1152000;
            break;
        case 1500000:
            rate = B1500000;
            break;

        default:
            break;
        }
        return rate;
    }

    explicit Uart(Uart&&) = delete;
    explicit Uart(const Uart&) = delete;
    void operator=(const Uart&) = delete;
};

} // namespace sense_koks

} // namespace st
