/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-14 13:02:06
 * @LastEditors: 洪照
 * @LastEditTime: 2025-03-10 19:32:38
 * @FilePath: /sense-koks/application/common/include/sys_logger.h
 */

#ifndef sys_logger_h
#define sys_logger_h
#include <atomic>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#define gettid() syscall(SYS_gettid)

#define LEVEL_VERBOSE 0
#define ST_LEVEL_DEBUG 1
#define ST_LEVEL_INFO 2
#define ST_LEVEL_WARNING 3
#define ST_LEVEL_ERROR 4

#define LOG_BUF_SIZE 4096
#define MAX_FILE_NAME_LEN 256
#define MAX_FILE_HEAD_LEN 128

using namespace std;

#define LOG_DIR "/data/log/"

#define LOG_VERBOSE(format, args...) SysLogger::GetInstance()->WriteLog(LEVEL_VERBOSE, __FILE__, __LINE__, gettid(), format, ##args)

#define LOG_DEBUG(format, args...) SysLogger::GetInstance()->WriteLog(ST_LEVEL_DEBUG, __FILE__, __LINE__, gettid(), format, ##args)

#define LOG_INFO(format, args...) SysLogger::GetInstance()->WriteLog(ST_LEVEL_INFO, __FILE__, __LINE__, gettid(), format, ##args)

#define LOG_WARNING(format, args...) SysLogger::GetInstance()->WriteLog(ST_LEVEL_WARNING, __FILE__, __LINE__, gettid(), format, ##args)

#define LOG_ERROR(format, args...) SysLogger::GetInstance()->WriteLog(ST_LEVEL_ERROR, __FILE__, __LINE__, gettid(), format, ##args)
#define SET_LOG_LEVEL(level) SysLogger::GetInstance()->SetLoggerLevel(level)

#define LOGV(format, args...)   SysLogger::GetInstance()->WriteLog(LEVEL_VERBOSE,  __FILE__, __LINE__, gettid(), format "\n", ##args)
#define LOGD(format, args...)   SysLogger::GetInstance()->WriteLog(ST_LEVEL_DEBUG, __FILE__, __LINE__, gettid(), format "\n", ##args)
#define LOGI(format, args...)   SysLogger::GetInstance()->WriteLog(ST_LEVEL_INFO,  __FILE__, __LINE__, gettid(), format "\n", ##args)
#define LOGW(format, args...)   SysLogger::GetInstance()->WriteLog(ST_LEVEL_WARNING,  __FILE__, __LINE__, gettid(), format "\n", ##args)
#define LOGE(format, args...)   SysLogger::GetInstance()->WriteLog(ST_LEVEL_ERROR,  __FILE__, __LINE__, gettid(), format "\n", ##args)
/**
 *@ brief 获取对象的个数
 */
#define MISC_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/**
 *@ brief 计算数据差值
 */
#define MISC_DIST(val1, val2) ((val1) > (val2) ? ((val1) - (val2)) : ((val2) - (val1)))

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK(condition, ...)                                                                                                                                                   \
    if (!(condition)) {                                                                                                                                                            \
        return __VA_ARGS__;                                                                                                                                                        \
    }

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_CONDITION(condition, ...) MISC_CHK(condition, ##__VA_ARGS__)

/**
 *@ brief
 *检查ptr是否为空，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_UN_NULL(ptr, ...) MISC_CHK((NULL != (ptr)), ##__VA_ARGS__)

/**
 *@ brief
 *检查status是否为0，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_STATUS(val, ...) MISC_CHK((0 == (val)), ##__VA_ARGS__)

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_EXEC(condition, action, ...)                                                                                                                                      \
    if (!(condition)) {                                                                                                                                                            \
        action;                                                                                                                                                                    \
        return __VA_ARGS__;                                                                                                                                                        \
    }

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_CONDITION_EXEC(condition, action, ...) MISC_CHK_EXEC(condition, action, ##__VA_ARGS__)

/**
 *@ brief
 *检查ptr是否为空，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_UN_NULL_EXEC(ptr, action, ...) MISC_CHK_EXEC((NULL != (ptr)), action, ##__VA_ARGS__)

/**
 *@ brief
 *检查status是否为0，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_STATUS_EXEC(val, action, ...) MISC_CHK_EXEC((0 == (val)), action, ##__VA_ARGS__)

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_REPORT_EXEC(condition, ret, action, ...)                                                                                                                          \
    if (!(condition)) {                                                                                                                                                            \
        LOG_ERROR(__VA_ARGS__);                                                                                                                                                    \
        action;                                                                                                                                                                    \
        return ret;                                                                                                                                                                \
    }

#define MISC_CHK_WARN_REPORT_EXEC(condition, ret, action, ...)                                                                                                                     \
    if (!(condition)) {                                                                                                                                                            \
        LOG_WARN(__VA_ARGS__);                                                                                                                                                     \
        action;                                                                                                                                                                    \
        return ret;                                                                                                                                                                \
    }
/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_CONDITION_REPORT_EXEC(condition, ret, action, ...) MISC_CHK_REPORT_EXEC(condition, ret, action, ##__VA_ARGS__)

/**
 *@ brief
 *检查ptr是否为空，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_UN_NULL_REPORT_EXEC(ptr, ret, action, ...) MISC_CHK_REPORT_EXEC((NULL != (ptr)), ret, action, ##__VA_ARGS__)

/**
 *@ brief
 *检查status是否为0，如果不为真则执行需必要的代码及打印，执行完后返回ret，必要代码可在ret后加
 */
#define MISC_CHK_STATUS_REPORT_EXEC(val, ret, action, ...) MISC_CHK_REPORT_EXEC((0 == (val)), ret, action, ##__VA_ARGS__)

/**
 *@ brief 检查condition，如果不为真则先打印对应的日志信息再返回ret，日志信息可在ret后加
 */
#define MISC_CHK_CONDITION_REPORT(condition, ret, ...) MISC_CHK_REPORT_EXEC(condition, ret, , ##__VA_ARGS__)

#define MISC_CHK_WARN_CONDITION_REPORT(condition, ret, ...) MISC_CHK_WARN_REPORT_EXEC(condition, ret, , ##__VA_ARGS__)
/**
 *@ brief 检查ptr是否为空，如果为空则先打印对应的日志信息再返回ret，日志信息可在ret后加
 */
#define MISC_CHK_UN_NULL_REPORT(ptr, ret, ...) MISC_CHK_REPORT_EXEC((NULL != (ptr)), ret, , ##__VA_ARGS__)

/**
 *@ brief 检查status是否为0，如果不是0则先打印对应的日志信息再返回ret，日志信息可在ret后加
 */
#define MISC_CHK_STATUS_REPORT(val, ret, ...) MISC_CHK_REPORT_EXEC((0 == (val)), ret, , ##__VA_ARGS__)

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印
 */
#define MISC_CHK_REPORT_EXEC_NO_RET(condition, action, ...)                                                                                                                        \
    if (!(condition)) {                                                                                                                                                            \
        LOG_ERROR(__VA_ARGS__);                                                                                                                                                    \
        action;                                                                                                                                                                    \
    }

/**
 *@ brief 检查condition，如果不为真则执行需必要的代码及打印，必要代码可在判断条件后加
 */
#define MISC_CHK_CONDITION_REPORT_EXEC_NO_RET(condition, action, ...) MISC_CHK_REPORT_EXEC_NO_RET(condition, action, ##__VA_ARGS__)

/**
 *@ brief 检查ptr是否为空，如果不为真则执行需必要的代码及打印，必要代码可在判断条件后加
 */
#define MISC_CHK_UN_NULL_REPORT_EXEC_NO_RET(ptr, action, ...) MISC_CHK_REPORT_EXEC_NO_RET((NULL != (ptr)), action, ##__VA_ARGS__)

/**
 *@ brief 检查status是否为0，如果不为真则执行需必要的代码及打印，必要代码可在判断条件后加
 */
#define MISC_CHK_STATUS_REPORT_EXEC_NO_RET(val, action, ...) MISC_CHK_REPORT_EXEC_NO_RET((0 == (val)), action, ##__VA_ARGS__)

/**
 *@ brief 检查condition，如果不为真则先打印对应的日志信息再返回ret，日志信息可在判断条件后加
 */
#define MISC_CHK_CONDITION_REPORT_NO_RET(condition, ...) MISC_CHK_REPORT_EXEC_NO_RET(condition, , ##__VA_ARGS__)

/**
 *@ brief 检查ptr是否为空，如果为空则先打印对应的日志信息再返回ret，日志信息可在判断条件后加
 */
#define MISC_CHK_UN_NULL_REPORT_NO_RET(ptr, ...) MISC_CHK_REPORT_EXEC_NO_RET((NULL != (ptr)), , ##__VA_ARGS__)

/**
 *@ brief 检查status是否为0，如果不是0则先打印对应的日志信息再返回ret，日志信息可在判断条件后加
 */
#define MISC_CHK_STATUS_REPORT_NO_RET(val, ...) MISC_CHK_REPORT_EXEC_NO_RET((0 == (val)), , ##__VA_ARGS__)

/**
 *@ brief Traverse the two containers sequentially, execute _Function, the input parameter is the traversal value of the two containers
 */
template <typename Container1, typename Container2, typename _Function>
int32_t for_each2(Container1&& contain1, Container2&& contain2, _Function __f)
{
    if (contain1.size() != contain2.size()) {
        throw -1;
    }

    auto&& iter1 = contain1.begin();
    auto&& iter2 = contain2.begin();
    for (; iter1 != contain1.end(); ++iter1, ++iter2) {
        __f(*iter1, *iter2);
    }

    return 0;
}

/**
 *@ brief Traverse the container and pointer sequentially, execute _Function, the input parameter is the traversal value of the two containers
 */
template <typename Container1, typename Container2, typename _Function>
int32_t for_each2(Container1&& contain1, Container2* contain2, _Function __f)
{
    uint32_t idx = 0;
    for (auto& iter1 : contain1) {
        __f(iter1, contain2[idx]);
        idx++;
    }

    return 0;
}

// Pthread trylock, lock, unlock
#define MUTEX_LOCK(mutex)                                                                                                                                                          \
    {                                                                                                                                                                              \
        int oldstate;                                                                                                                                                              \
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);                                                                                                                 \
        pthread_mutex_lock(mutex);                                                                                                                                                 \
        pthread_setcancelstate(oldstate, NULL);                                                                                                                                    \
    }

#define MUTEX_UNLOCK(mutex)                                                                                                                                                        \
    {                                                                                                                                                                              \
        int oldstate;                                                                                                                                                              \
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);                                                                                                                 \
        pthread_mutex_unlock(mutex);                                                                                                                                               \
        pthread_setcancelstate(oldstate, NULL);                                                                                                                                    \
    }

inline int MutexTryLock(pthread_mutex_t* mutex)
{
    int ret = 0;
    int oldstate;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    ret = pthread_mutex_trylock(mutex);
    pthread_setcancelstate(oldstate, NULL);
    return ret;
}

inline void MutexLock(pthread_mutex_t* mutex)
{
    int oldstate;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    pthread_mutex_lock(mutex);
    pthread_setcancelstate(oldstate, NULL);
}

inline void MutexUnlock(pthread_mutex_t* mutex)
{
    int oldstate;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    pthread_mutex_unlock(mutex);
    pthread_setcancelstate(oldstate, NULL);
}

class SysLogger
{
    public:
    static SysLogger* GetInstance();

    bool InitLogger(const char* file_name, int min_level, int save_day = 7, unsigned int log_max_len = 10 * 1024 * 1024);
    void SetLoggerLevel(int min_level);

    void WriteLog(int level, const char* exec_file, int exec_line, int tid, const char* format, ...);
    void WriteLogNoLock(int level, const char* exec_file, int exec_line, int tid, const char* format, ...);
    void WriteBufLog(char* buf);
    const char* LogFileHead()
    {
        return log_file_head_;
    }
    bool isNeed(int level)
    {
        return ((level < min_level_) ? false : true);
    }
    void GetFixedLogBufInfo(uint32_t& buf_addr, int& len)
    {
        buf_addr = (uint32_t)fixed_size_log_buf_;
        if (fixed_size_log_buf_cover_flag) {
            len = fixed_size;
        } else {
            len = fixed_size_log_buf_offset;
        }
    }

    private:
    SysLogger();
    ~SysLogger();
    void set_log(int level, const char* exec_file, int exec_line, int tid, const char* format, va_list valst);

    bool create_day_file(const struct tm* sys_tm);

    bool remove_day_file(const struct tm* sys_tm);

    bool get_current_num();

    bool remove_oldest_file();

    public:
    static SysLogger* instance_;

    private:
    std::atomic<uint8_t> min_level_{ 1 };
    char* log_file_head_;
    char* log_buf_;
    char* log_file_name_;
    char* fixed_size_log_buf_;
    uint32_t fixed_size_log_buf_offset{ 0 };
    bool fixed_size_log_buf_cover_flag{ false };
    constexpr static uint32_t fixed_size = 65536;
    FILE* log_fp_;
    pthread_mutex_t* mutex_;

    struct tm sys_tm_;

    int save_num_;
    int cur_num_;
    uint32_t log_max_len_;
};

#endif /* sys_logger_h */
