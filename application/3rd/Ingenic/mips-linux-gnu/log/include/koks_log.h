/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-11 18:40:42
 * @LastEditors: 洪照
 * @LastEditTime: 2024-12-11 21:40:31
 * @FilePath: /log_lib/log/koks_log.h
 */
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif


#define ST_LEVEL_VERBOSE 0
#define ST_LEVEL_DEBUG 1
#define ST_LEVEL_INFO 2
#define ST_LEVEL_WARNING 3
#define ST_LEVEL_ERROR 4

/**
 * @name: koks_log_init
 * @brief: 初始化日志（如果不调用改接口则打印到控制台）
 * @param {char} *file_name 文件路径
 * @param {int} min_level   最低打印等级
 * @param {int} file_num    保存日志最大数量    
 * @param {unsigned int} log_max_len 日志文件大小
 * @return {*}
 */
extern bool koks_log_init(const char *file_name, int min_level, int file_num, unsigned int log_max_len);
extern void koks_loge(const char *format, ...);
extern void koks_logw(const char *format, ...);
extern void koks_logi(const char *format, ...);
extern void koks_logd(const char *format, ...);
extern void koks_logv(const char *format, ...);



#ifdef  __cplusplus
}
#endif