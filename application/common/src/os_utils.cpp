/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 20:32:32
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-07 11:09:49
 * @FilePath: /sense-koks/application/common/src/os_utils.cpp
 */
#include "os_utils.h"
#include "partition_guard.h"
#include "su_base.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
namespace st
{
namespace sense_koks
{
int64_t GetFileSize(std::string file_name)
{
    struct stat stat_buf;
    int rc = lstat(file_name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int WriteFile(const std::string& file_name, void* data, uint32_t len)
{
    std::ofstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }
    file.write(reinterpret_cast<char*>(data), len);
    file.close();
    return 0;
}

//需要读取整个文件，返回文件数据指针和文件长度
int ReadFile(const std::string& file_name, void** data, int& len)
{
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }
    file.seekg(0, std::ios::end);
    len = file.tellg();
    file.seekg(0, std::ios::beg);
    *data = new char[len];
    file.read(reinterpret_cast<char*>(*data), len);
    file.close();
    return 0;
}

int releaseFileContent(char* content)
{
    if (content != nullptr) {
        delete[] content;
        content = nullptr;
    }
    return 0;
}

uint64_t GetCurTimeMs(void)
{
    auto dur = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
}

uint64_t GetCurTimeUs(void)
{
    auto dur = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
}

uint64_t GetCurTimeS(void)
{
    auto dur = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(dur).count();
}

void os_delay(uint32_t millisec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
}

std::string GetCPUID(void)
{
    SUDevID dev;
    std::memset(&dev, 0, sizeof(SUDevID));
    SU_Base_GetDevID(&dev);
    return std::string(dev.chr);
}

int utils_shell(const char* command, char* result, int length)
{
    FILE* handle = 0;
    char buffer[1024 * 4] = { 0 };
    int pos = 0;
    int rc = 0;

    handle = popen(command, "r");

    // printf("utils_shell,command = %s, handle = %p \n", command, handle);
    if (handle == NULL) {
        printf("fopenerror: %s\n", strerror(errno));
        rc = -1;
        return rc;
    }

    while (fgets(buffer, sizeof(buffer), handle) != NULL) {
        if (result != NULL && pos < length) {
            strncpy(result + pos, buffer, length - pos);
            pos += strlen(buffer);
        }
    }
    rc = pclose(handle);
    if (rc == -1) {
        printf("pclose error: %s\n", strerror(errno));
        return -1;
    }

    if (WIFEXITED(rc)) {
        int exit_status = WEXITSTATUS(rc);
        if (exit_status != 0) {
            return exit_status;
        }
    } else {
        printf("command did not exit properly\n");
        return -1;
    }
    return 0;
}

#if 0
bool WriteFlagFile(const std::string& flag)
{
    PartitionGuard configGuard(CONFIG_FILE_UNKNOWN);
    std::string path = "/config/" + flag;
    std::ofstream file(path);
    if (!file) {
        printf("Failed to create file %s\n", path.c_str());
        return false;
    }
    file.close();
    sync();
    return true;
}

bool RemoveFlagFile(const std::string &flag)
{
    PartitionGuard configGuard(CONFIG_FILE_UNKNOWN);
    std::string path = "/config/" + flag;

    if (std::remove(path.c_str()) != 0) {
        printf("Failed to remove file %s\n", path.c_str());
        return false;
    }

    sync();
    return true;
}
#endif

} // namespace sense_koks

} // namespace st
