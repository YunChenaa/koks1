/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 13:02:10
 * @LastEditors: 洪照
 * @LastEditTime: 2025-08-22 10:44:11
 * @FilePath: /sense-koks/application/common/src/sys_logger.cpp
 */

#include "sys_logger.h"
#include <ctime>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ONE_DAY_SECOND 60 * 60 * 24
#define LOG_TO_STDOUT (0)
SysLogger* SysLogger::instance_ = NULL;

SysLogger* SysLogger::GetInstance()
{
    if (instance_ == NULL) {
        instance_ = new SysLogger();
    }

    return instance_;
}

SysLogger::SysLogger() : log_fp_(nullptr)
{
    min_level_ = 0;

    log_file_head_ = new char[MAX_FILE_HEAD_LEN];
    memset(log_file_head_, 0, MAX_FILE_HEAD_LEN);

    log_file_name_ = new char[MAX_FILE_NAME_LEN];
    memset(log_file_name_, 0, MAX_FILE_NAME_LEN);

    log_buf_ = new char[LOG_BUF_SIZE];
    memset(log_buf_, 0, LOG_BUF_SIZE);

    fixed_size_log_buf_ = new char[fixed_size];
    memset(fixed_size_log_buf_, 0, fixed_size);

    mutex_ = new pthread_mutex_t;
    pthread_mutex_init(mutex_, NULL);

    memset(&sys_tm_, 0, sizeof(sys_tm_));

    save_num_ = 7;
}

SysLogger::~SysLogger()
{
    if (log_file_head_ != NULL) {
        delete[] log_file_head_;
        log_file_head_ = NULL;
    }

    if (log_file_name_ != NULL) {
        delete[] log_file_name_;
        log_file_name_ = NULL;
    }

    if (log_buf_ != NULL) {
        delete[] log_buf_;
        log_buf_ = NULL;
    }

    if (log_fp_ != nullptr) {
        fclose(log_fp_);
        log_fp_ = nullptr;
    }

    pthread_mutex_destroy(mutex_);

    if (mutex_ != NULL) {
        delete mutex_;
    }
}

bool SysLogger::InitLogger(const char* file_name, int min_level, int save_day, unsigned int log_max_len)
{
    strncpy(log_file_head_, file_name, MAX_FILE_HEAD_LEN - 1);
    log_file_head_[MAX_FILE_HEAD_LEN - 1] = '\0';

    snprintf(log_file_name_, MAX_FILE_NAME_LEN - 1, "%s.log", log_file_head_);
    log_file_name_[MAX_FILE_NAME_LEN - 1] = '\0';

    time_t now = time(NULL);
    localtime_r(&now, &sys_tm_);

    if (!create_day_file(&sys_tm_)) {
        return false;
    }

    if (min_level >= 0 && min_level <= 4) {
        min_level_ = min_level;
    }

    save_num_ = save_day;

    log_max_len_ = log_max_len;
    cur_num_ = 0;
    if (!get_current_num())
        return false;

    printf("log file max len %d, cur_num_ %d\n", log_max_len_, cur_num_);

    return true;
}

void SysLogger::SetLoggerLevel(int min_level)
{
    if (min_level >= LEVEL_VERBOSE && min_level <= ST_LEVEL_ERROR) {
        min_level_ = min_level;
    }
}

/**
 * @name: FdToPath
 * @description: 将fd转成对应的文件名
 * @param {int fd} 文件句柄
 * @param {char *path_buf} 文件路径
 * @param {size_t buflen} 文件最大长度
 * @return:
 */
static int FdToPath(int fd, char* path_buf, size_t buflen)
{
    int r;
    char path[256];

    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    r = readlink(path, path_buf, buflen);
    if (r < 0) {
        return -1;
    }

    if (r >= buflen) {
        return -1;
    }

    path_buf[r] = '\0';

    return 0;
}

void SysLogger::WriteLog(int level, const char* exec_file, int exec_line, int tid, const char* format, ...)
{
    pthread_mutex_lock(mutex_);
    va_list valst;
    va_start(valst, format);
    set_log(level, exec_file, exec_line, tid, format, valst);
    va_end(valst);

    struct stat fstat;
    if (stat(log_file_name_, &fstat) == -1) {
        /** 如果文件路径为空那么使用默认STDOUT输出 **/
        if (0 == strcmp(log_file_name_, "")) {
            FdToPath(STDOUT_FILENO, log_file_name_, MAX_FILE_NAME_LEN);
        }

        create_day_file(NULL);
    }

    if (level < min_level_ || log_fp_ == nullptr) {
        pthread_mutex_unlock(mutex_);
        return;
    }

    fputs(log_buf_, log_fp_);
    size_t write_size = strlen(log_buf_);
    if (write_size > fixed_size) {
        write_size = fixed_size; // 截断写入的数据
    }

    size_t remaining_space = fixed_size - fixed_size_log_buf_offset;
    if (write_size <= remaining_space) {
        strncpy(fixed_size_log_buf_ + fixed_size_log_buf_offset, log_buf_, write_size);
        fixed_size_log_buf_offset += write_size;
    } else {
        strncpy(fixed_size_log_buf_ + fixed_size_log_buf_offset, log_buf_, remaining_space);
        strncpy(fixed_size_log_buf_, log_buf_ + remaining_space, write_size - remaining_space);
        fixed_size_log_buf_offset = write_size - remaining_space;
    }

    if (fixed_size_log_buf_offset >= fixed_size) {
        fixed_size_log_buf_offset = 0;
        fixed_size_log_buf_cover_flag = true; // 被覆盖过
    }

#if LOG_TO_STDOUT
    fputs(log_buf_, stdout);
#endif
    fflush(log_fp_);
    pthread_mutex_unlock(mutex_);
}

void SysLogger::WriteBufLog(char* buf)
{
    pthread_mutex_lock(mutex_);

    struct stat fstat;
    if (stat(log_file_name_, &fstat) == -1) {

        create_day_file(NULL);
    }

    fputs(buf, log_fp_);
    fflush(log_fp_);

    pthread_mutex_unlock(mutex_);
}

// Used for mod exit
void SysLogger::WriteLogNoLock(int level, const char* exec_file, int exec_line, int tid, const char* format, ...)
{
    if (level < min_level_) {
        return;
    }

    va_list valst;
    va_start(valst, format);
    set_log(level, exec_file, exec_line, tid, format, valst);
    va_end(valst);
    fputs(log_buf_, log_fp_);
    fflush(log_fp_);

    // if (log_fp_ != NULL) {
    // fclose(log_fp_);
    // log_fp_ = NULL;
    // }
}

void SysLogger::set_log(int level, const char* exec_file, int exec_line, int tid, const char* format, va_list valst)
{
    char exec_filename[MAX_FILE_NAME_LEN];
    memset(exec_filename, 0, MAX_FILE_NAME_LEN);
    const char* pch = strrchr(exec_file, '/');

    if (pch == NULL) {
        strncpy(exec_filename, exec_file, MAX_FILE_NAME_LEN - 1);
    } else {
        strncpy(exec_filename, pch + 1, MAX_FILE_NAME_LEN - 1);
    }

    char levstr[16];
    memset(levstr, 0, 16);

    switch (level) {
    case LEVEL_VERBOSE:
        strncpy(levstr, "VERBOSE", sizeof(levstr) - 1);
        break;
    case ST_LEVEL_DEBUG:
        strncpy(levstr, "DEBUG", sizeof(levstr) - 1);
        break;
    case ST_LEVEL_INFO:
        strncpy(levstr, "INFO", sizeof(levstr) - 1);
        break;
    case ST_LEVEL_WARNING:
        strncpy(levstr, "WARN", sizeof(levstr) - 1);
        break;
    case ST_LEVEL_ERROR:
        strncpy(levstr, "ERROR", sizeof(levstr) - 1);
        break;
    default:
        strncpy(levstr, "INFO", sizeof(levstr) - 1);
        break;
    }

    memset(log_buf_, 0, LOG_BUF_SIZE);

    struct timeval now = { 0, 0 };
    gettimeofday(&now, NULL);
    struct tm sys_tm;
    localtime_r(&(now.tv_sec), &sys_tm);

    int n = snprintf(log_buf_, 128, "%04d-%02d-%02d %02d:%02d:%02d,%03d <%s> [%s:%d] [%d] ", sys_tm.tm_year + 1900, sys_tm.tm_mon + 1, sys_tm.tm_mday, sys_tm.tm_hour,
                     sys_tm.tm_min, sys_tm.tm_sec, now.tv_usec / 1000, levstr, exec_filename, exec_line, tid);
    // int n = snprintf(log_buf_, 128, "[%s:%d]", exec_filename, exec_line);
    vsnprintf(log_buf_ + n, LOG_BUF_SIZE - n, format, valst);
#ifdef DEBUG
    // printf("%s", log_buf_);
#endif
}

bool SysLogger::create_day_file(const struct tm* sys_tm)
{
    if (log_fp_ != nullptr) {
        fclose(log_fp_);
        log_fp_ = nullptr;
    }

    log_fp_ = fopen(log_file_name_, "a+");

    if (log_fp_ == nullptr) {
        return false;
    }

    return true;
}
bool SysLogger::remove_day_file(const struct tm* sys_tm)
{
    char old_file_name[MAX_FILE_NAME_LEN] = { 0 };

    snprintf(old_file_name, MAX_FILE_NAME_LEN, "%s_%d-%d-%d.log", log_file_head_, sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday);
    ifstream fin(old_file_name);
    if (fin) {
        unlink(old_file_name);
    }

    return true;
}

bool SysLogger::get_current_num()
{
    DIR* dir = nullptr;
    struct dirent* item = nullptr;
    std::string base_name, dir_name;
    base_name = log_file_head_;
    base_name.erase(0, base_name.rfind("/") + 1);
    dir_name = log_file_name_;
    dir_name.erase(dir_name.rfind("/"), dir_name.length());
    dir = opendir(dir_name.c_str());
    if (nullptr == dir)
        return false;
    while ((item = readdir(dir)) != nullptr) {
        if (0 == strcmp(item->d_name, ".") || 0 == strcmp(item->d_name, ".."))
            continue;
        if ((strlen(item->d_name) > 4) && (strlen(item->d_name) > base_name.length())) {
            if ((0 == strcmp(reinterpret_cast<char*>(&item->d_name[strlen(item->d_name) - 4]), ".log")) && (0 == strncmp(item->d_name, base_name.c_str(), base_name.length()))) {
                cur_num_++;
            }
        }
    }
    closedir(dir);
    return true;
}

bool SysLogger::remove_oldest_file()
{
    DIR* dir = nullptr;
    struct dirent* item = nullptr;
    struct stat fstat;
    time_t ctime = 0;
    std::string base_name, dir_name, file_name, oldest_name;
    base_name = log_file_head_;
    base_name.erase(0, base_name.rfind("/") + 1);
    dir_name = log_file_name_;
    dir_name.erase(dir_name.rfind("/"), dir_name.length());
    dir = opendir(dir_name.c_str());
    if (nullptr == dir)
        return false;
    while ((item = readdir(dir)) != nullptr) {
        if (0 == strcmp(item->d_name, ".") || 0 == strcmp(item->d_name, ".."))
            continue;
        if ((strlen(item->d_name) > 4) && (strlen(item->d_name) > base_name.length())) {
            if ((0 == strcmp(reinterpret_cast<char*>(&item->d_name[strlen(item->d_name) - 4]), ".log")) && (0 == strncmp(item->d_name, base_name.c_str(), base_name.length()))) {
                file_name = dir_name;
                file_name += "/";
                file_name += item->d_name;
                if (ctime == 0) {
                    stat(file_name.c_str(), &fstat);
                    ctime = fstat.st_ctime;
                    oldest_name = file_name;
                } else {
                    stat(file_name.c_str(), &fstat);
                    if (ctime > fstat.st_ctime)
                        oldest_name = file_name;
                    ctime = fstat.st_ctime;
                }
            }
        }
    }
    closedir(dir);
    unlink(oldest_name.c_str());
    return true;
}
