/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 20:32:37
 * @LastEditors: 洪照
 * @LastEditTime: 2025-03-05 17:58:23
 * @FilePath: /sense-koks/application/common/include/os_utils.h
 */
#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <fstream>

namespace st
{
namespace sense_koks
{

typedef enum {
    KOKS_RET_OK,         /**< successful */
    KOKS_RET_INVAL,      /**< invalid parameters */
    KOKS_RET_NOMEM,      /**< out of memory */
    KOKS_RET_NOSUPP,     /**< not supported */
    KOKS_RET_NOSEC_WL,   /**< not secure due to white list */
    KOKS_RET_NOT_EXIST,  /**< not exist */
    KOKS_RET_AGAIN,      /**< again */
    KOKS_RET_NOT_READY,  /**< dev not ready */
    KOKS_RET_EXIST,      /**< already exist */
    KOKS_RET_BUSY,       /**< busy */
    KOKS_RET_PENDING,    /**< pending */
    KOKS_RET_FAIL,       /**< failed */
    KOKS_RET_NOSEC_BL,   /**< not secure due to black list */
    KOKS_RET_CRC_LEN,    /**< calculated crc but len < 0 */
    KOKS_RET_DISCONNECT, /**< disconnect */
    KOKS_RET_TIMEOVER,   /**< timeout */
    KOKS_RET_CRC_FAIL,   /**< crc check failed */
} KOKS_RET;

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define GET_TIMEDIFF(t1, t2) ((t2) - (t1))
int64_t GetFileSize(std::string file_name);
int WriteFile(const std::string& file_name, void* data, uint32_t len);
int ReadFile(const std::string& file_name, void** data, int& len);
int releaseFileContent(char* content);
std::string GetCPUID(void);
uint64_t GetCurTimeMs(void);
uint64_t GetCurTimeUs(void);
uint64_t GetCurTimeS(void);
void os_delay(uint32_t millisec);
int utils_shell(const char* command, char* result, int length);

bool WriteFlagFile(const std::string& flag);
bool RemoveFlagFile(const std::string& flag);

class FileGuard
{
    public:
    FileGuard(const std::string& path)
    {
        fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (fd < 0) {
            printf("open %s failed!\n", path.c_str());
        } else
            is_valid = true;
    }
    ~FileGuard()
    {
        if (is_valid)
            close(fd);
    }
    bool IsOpened(void) const
    {
        return is_valid;
    }
    int GetFd(void) const
    {
        return fd;
    }

    private:
    int fd{ -1 };
    bool is_valid{ false };
};


template<typename T>
bool WriteSysFile(const std::string& filePath, const T& value)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        printf("Open file failed %s\n", filePath.c_str());
        return false;
    }

    file << value;
    if (file.fail()) {
        printf("Write file failed %s\n", filePath.c_str());
        return false;
    }

    file.close();
    return true;
}

} // namespace sense_koks

} // namespace st
