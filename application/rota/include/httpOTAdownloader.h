#ifndef HTTPOTADOWNLOADER_H
#define HTTPOTADOWNLOADER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"


#define LOG_INFO(fmt, ...)  do { printf("[%s:%d][INFO] " fmt  ,__FUNCTION__ , __LINE__, ##__VA_ARGS__); } while(0)
#define LOG_ERROR(fmt, ...) do { printf("[%s:%d][ERROR] " fmt ,__FUNCTION__ , __LINE__, ##__VA_ARGS__); } while(0)

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// HTTPS 下载上下文（替代原来的 class）
// ============================================================
typedef struct {
    mbedtls_net_context      server_fd;
    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_x509_crt         cacert;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context  entropy;
    int                      is_https;   // 新增：1=HTTPS, 0=HTTP
} https_downloader_t;

// ============================================================
// API 函数
// ============================================================

/**
 * @brief 初始化 HTTPS 下载器上下文
 * @param ctx 上下文指针
 */
void https_downloader_init(https_downloader_t* ctx);

/**
 * @brief 清理并释放 HTTPS 下载器上下文
 * @param ctx 上下文指针
 */
void https_downloader_close(https_downloader_t* ctx);

/**
 * @brief 下载文件（HTTPS GET 请求）
 * @param ctx          上下文指针
 * @param url          下载 URL（支持 http:// 和 https://）
 * @param timeout_ms   超时时间（毫秒）
 * @param range_start  Range 起始字节（0 表示从头开始）
 * @param range_end    Range 结束字节（0 表示到文件末尾）
 * @return 0 成功，负数失败
 */
int https_download(https_downloader_t* ctx, const char* url,
                   int timeout_ms,
                   size_t range_start, size_t range_end);

/**
 * @brief 下载文件到内存缓冲区（支持 Range 分片下载，跳过 OTA 流解析）
 * @param ctx          上下文指针
 * @param url          下载 URL（支持 http:// 和 https://）
 * @param timeout_ms   超时时间（毫秒）
 * @param range_start  Range 起始字节（0 表示从头开始）
 * @param range_end    Range 结束字节（0 表示到文件末尾）
 * @param buf          输出缓冲区
 * @param buf_size     缓冲区大小
 * @param received     输出: 实际接收的字节数
 * @return 0 成功，负数失败
 */
int https_download_to_buf(https_downloader_t* ctx, const char* url,
                          int timeout_ms,
                          size_t range_start, size_t range_end,
                          uint8_t* buf, size_t buf_size, size_t* received);

// ============================================================
// OTA 文件结构
// ============================================================

/* 大端 */
typedef struct {
    uint32_t file_size;   /* 文件大小 */
    uint32_t offset;      /* 文件偏移 */
    char     version[32];   /* 版本字符串，最多 15 字节 + 1 字节结束符 */
    char     md5[32];     /* 16 字节 MD5 校验码（实际只用 16 字节） */
} OTAFileHeader;


typedef struct {
    OTAFileHeader AIC_header;   /* AIC 头部 */
    OTAFileHeader Lock_header;  /* Lock 头部 */
    OTAFileHeader T33_header;   /* T33 头部 */
} OTAFileInfo;

int parse_ota_file(OTAFileInfo* info, const uint8_t* buffer, size_t buffer_size);
void start_ota_download(const char* url);

#ifdef __cplusplus
}
#endif


#endif // HTTPOTADOWNLOADER_H
