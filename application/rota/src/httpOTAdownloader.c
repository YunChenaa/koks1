#include "httpOTAdownloader.h"
#include <stdint.h>
#include <time.h>
#include "ovd_ota.h"
#include "sys_version.h"




/* ---- 内部辅助函数声明 ---- */

static int parse_url(const char* url, char* host, size_t host_size,
                     char* path, size_t path_size, int* port,
                     int* is_https);

static int build_http_get_request(const char* host, const char* path,
                                  size_t range_start, size_t range_end,
                                  char* request, size_t request_size);

static int receive_and_save_ssl(mbedtls_ssl_context* ssl);

static int receive_and_save_plain(mbedtls_net_context* fd);

/* ---- 前向声明 (供 https_download_to_buf 使用) ---- */
typedef int (*read_func_t)(void* ctx, unsigned char* buf, size_t len);
static int ssl_read_wrapper(void* ctx, unsigned char* buf, size_t len);
static int plain_read_wrapper(void* ctx, unsigned char* buf, size_t len);
static int receive_to_buf(void* io_ctx, read_func_t read_func,
                          uint8_t* buf, size_t buf_size, size_t* received);

/* ================================================================
 * 公开 API
 * ================================================================ */

void https_downloader_init(https_downloader_t* ctx)
{
    mbedtls_net_init(&ctx->server_fd);
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_x509_crt_init(&ctx->cacert);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);
    ctx->is_https = 0;   // 默认 HTTP
}

void https_downloader_close(https_downloader_t* ctx)
{
    if (ctx->is_https) {
        mbedtls_ssl_close_notify(&ctx->ssl);
    }
    mbedtls_net_free(&ctx->server_fd);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
}

int https_download(https_downloader_t* ctx, const char* url, int timeout_ms,
                       size_t range_start, size_t range_end)
{
    char host[256] = {0};
    char path[1024] = {0};
    char request[2048] = {0};
    char port_str[8] = {0};
    int port = 80;
    int ret;

    /* 1. 解析 URL（同时判断 HTTP / HTTPS） */
    if (parse_url(url, host, sizeof(host), path, sizeof(path),
                  &port, &ctx->is_https) != 0) {
        LOG_ERROR("parse URL failed: %s\n", url);
        return -1;
    }

    LOG_INFO("host: %s, port: %d, path: %s, %s\n",
             host, port, path, ctx->is_https ? "HTTPS" : "HTTP");

    /* 2. 连接服务器 */
    snprintf(port_str, sizeof(port_str), "%d", port);
    ret = mbedtls_net_connect(&ctx->server_fd, host, port_str,
                               MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        LOG_ERROR("mbedtls_net_connect failed: %d\n", ret);
        return -2;
    }

    /* 3. 仅 HTTPS 需要 SSL 初始化 */
    if (ctx->is_https) {
        /* 3a. 初始化随机数生成器 */
        {
            const char* pers = "https_download";
            ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                         &ctx->entropy,
                                         (const unsigned char*)pers,
                                         strlen(pers));
            if (ret != 0) {
                LOG_ERROR("mbedtls_ctr_drbg_seed failed: %d\n", ret);
                return -3;
            }
        }

        /* 3b. 配置 SSL */
        ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_config_defaults failed: %d\n", ret);
            return -4;
        }

        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
        mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random,
                              &ctx->ctr_drbg);
        mbedtls_ssl_conf_read_timeout(&ctx->conf, timeout_ms);

        ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_setup failed: %d\n", ret);
            return -5;
        }

        ret = mbedtls_ssl_set_hostname(&ctx->ssl, host);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_set_hostname failed: %d\n", ret);
            return -6;
        }

        mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd,
                             mbedtls_net_send, mbedtls_net_recv, NULL);

        /* 3c. SSL 握手 */
        while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
                ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                char err_buf[256];
                mbedtls_strerror(ret, err_buf, sizeof(err_buf));
                LOG_ERROR("mbedtls_ssl_handshake failed: -0x%x (%s)\n",
                          -ret, err_buf);
                return -7;
            }
        }
        LOG_INFO("SSL handshake OK\n");
    }

    /* 4. 构建 HTTP GET 请求 */
    build_http_get_request(host, path, range_start, range_end, request, sizeof(request));

    /* 5. 发送 HTTP 请求 */
    {
        size_t sent = 0;
        size_t req_len = strlen(request);

        while (sent < req_len) {
            if (ctx->is_https) {
                ret = mbedtls_ssl_write(&ctx->ssl,
                                         (const unsigned char*)request + sent,
                                         req_len - sent);
            } else {
                ret = mbedtls_net_send(&ctx->server_fd,
                                        (const unsigned char*)request + sent,
                                        req_len - sent);
            }

            if (ret < 0) {
                if (ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
                    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                    continue;
                }
                LOG_ERROR("send failed: %d\n", ret);
                return -8;
            }
            sent += ret;
        }
    }

    LOG_INFO("HTTP request sent\n");

    /* 6. 接收 HTTP 响应并保存到文件 */
    if (ctx->is_https) {
        ret = receive_and_save_ssl(&ctx->ssl);
    } else {
        ret = receive_and_save_plain(&ctx->server_fd);
    }

    if(ret == 0xff) {
        LOG_INFO("OTA skipped due to T33 version match\n");
        return 0xff;  /* 特殊情况，版本匹配，跳过 OTA，但不算错误 */
    }


    if (ret != 0) {
        LOG_ERROR("Failed to receive and save file: %d\n", ret);
        return -9;
    }

    LOG_INFO("Download completed\n");
    return 0;
}

/* ================================================================
 * 公开 API: 下载到内存缓冲区 (支持 Range)
 * ================================================================ */

/**
 * @brief 下载文件到内存缓冲区（支持 Range 分片下载）
 * @param ctx          上下文指针
 * @param url          下载 URL
 * @param timeout_ms   超时时间（毫秒）
 * @param range_start  Range 起始字节
 * @param range_end    Range 结束字节
 * @param buf          输出缓冲区
 * @param buf_size     缓冲区大小
 * @param received     输出: 实际接收的字节数
 * @return 0 成功，负数失败
 */
int https_download_to_buf(https_downloader_t* ctx, const char* url,
                          int timeout_ms,
                          size_t range_start, size_t range_end,
                          uint8_t* buf, size_t buf_size, size_t* received)
{
    char host[256] = {0};
    char path[1024] = {0};
    char request[2048] = {0};
    char port_str[8] = {0};
    int port = 80;
    int ret;

    *received = 0;

    /* 1. 解析 URL */
    if (parse_url(url, host, sizeof(host), path, sizeof(path),
                  &port, &ctx->is_https) != 0) {
        LOG_ERROR("parse URL failed: %s\n", url);
        return -1;
    }

    LOG_INFO("host: %s, port: %d, path: %s, %s, range: %zu-%zu\n",
             host, port, path, ctx->is_https ? "HTTPS" : "HTTP",
             range_start, range_end);

    /* 2. 连接服务器 */
    snprintf(port_str, sizeof(port_str), "%d", port);
    ret = mbedtls_net_connect(&ctx->server_fd, host, port_str,
                               MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        LOG_ERROR("mbedtls_net_connect failed: %d\n", ret);
        return -2;
    }

    /* 3. HTTPS SSL 初始化 */
    if (ctx->is_https) {
        const char* pers = "https_download";
        ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                     &ctx->entropy,
                                     (const unsigned char*)pers, strlen(pers));
        if (ret != 0) {
            LOG_ERROR("mbedtls_ctr_drbg_seed failed: %d\n", ret);
            return -3;
        }

        ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_config_defaults failed: %d\n", ret);
            return -4;
        }

        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
        mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
        mbedtls_ssl_conf_read_timeout(&ctx->conf, timeout_ms);

        ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_setup failed: %d\n", ret);
            return -5;
        }

        ret = mbedtls_ssl_set_hostname(&ctx->ssl, host);
        if (ret != 0) {
            LOG_ERROR("mbedtls_ssl_set_hostname failed: %d\n", ret);
            return -6;
        }

        mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd,
                             mbedtls_net_send, mbedtls_net_recv, NULL);

        while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
                ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                char err_buf[256];
                mbedtls_strerror(ret, err_buf, sizeof(err_buf));
                LOG_ERROR("mbedtls_ssl_handshake failed: -0x%x (%s)\n",
                          -ret, err_buf);
                return -7;
            }
        }
        LOG_INFO("SSL handshake OK\n");
    }

    /* 4. 构建并发送 HTTP GET 请求 */
    build_http_get_request(host, path, range_start, range_end,
                           request, sizeof(request));

    {
        size_t sent = 0;
        size_t req_len = strlen(request);

        while (sent < req_len) {
            if (ctx->is_https) {
                ret = mbedtls_ssl_write(&ctx->ssl,
                                         (const unsigned char*)request + sent,
                                         req_len - sent);
            } else {
                ret = mbedtls_net_send(&ctx->server_fd,
                                        (const unsigned char*)request + sent,
                                        req_len - sent);
            }
            if (ret < 0) {
                if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                    continue;
                LOG_ERROR("send failed: %d\n", ret);
                return -8;
            }
            sent += ret;
        }
    }

    LOG_INFO("HTTP request sent\n");

    /* 5. 接收 HTTP 响应到缓冲区 */
    if (ctx->is_https) {
        ret = receive_to_buf(&ctx->ssl, ssl_read_wrapper, buf, buf_size, received);
    } else {
        ret = receive_to_buf(&ctx->server_fd, plain_read_wrapper, buf, buf_size, received);
    }

    if (ret != 0) {
        LOG_ERROR("Failed to receive to buffer: %d\n", ret);
        return -9;
    }

    LOG_INFO("Download to buffer completed: %zu bytes\n", *received);
    return 0;
}

/* ================================================================
 * 内部函数实现
 * ================================================================ */

static int parse_url(const char* url, char* host, size_t host_size,
                     char* path, size_t path_size, int* port,
                     int* is_https)
{
    const char* p = url;

    if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port = 443;
        *is_https = 1;
    } else if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        *port = 80;
        *is_https = 0;
    } else {
        return -1;
    }

    /* 查找 host 结束位置 */
    {
        const char* host_start = p;
        size_t host_len;

        while (*p && *p != '/' && *p != ':')
            p++;

        host_len = (size_t)(p - host_start);
        if (host_len >= host_size)
            host_len = host_size - 1;
        memcpy(host, host_start, host_len);
        host[host_len] = '\0';
    }

    /* 检查端口 */
    if (*p == ':') {
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
    }

    /* 获取 path */
    if (*p == '\0') {
        snprintf(path, path_size, "/");
    } else {
        size_t path_len = strlen(p);
        if (path_len >= path_size)
            path_len = path_size - 1;
        memcpy(path, p, path_len);
        path[path_len] = '\0';
    }

    return 0;
}

static int build_http_get_request(const char* host, const char* path,
                                  size_t range_start, size_t range_end,
                                  char* request, size_t request_size)
{
    int written;

    if (range_end > 0 && range_end >= range_start) {
        written = snprintf(request, request_size,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: aic8800mc-ota/1.0\r\n"
            "Accept: */*\r\n"
            "Range: bytes=%zu-%zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            path, host, range_start, range_end);
    } else if (range_start > 0) {
        written = snprintf(request, request_size,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: aic8800mc-ota/1.0\r\n"
            "Accept: */*\r\n"
            "Range: bytes=%zu-\r\n"
            "Connection: close\r\n"
            "\r\n",
            path, host, range_start);
    } else {
        written = snprintf(request, request_size,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: aic8800mc-ota/1.0\r\n"
            "Accept: */*\r\n"
            "Connection: close\r\n"
            "\r\n",
            path, host);
    }

    if (written < 0 || (size_t)written >= request_size) {
        LOG_ERROR("build_http_get_request: buffer too small\n");
        return -1;
    }

    LOG_INFO("Built HTTP request:%s\n", request);
    return 0;
}

/* ---------- 通用: 接收 HTTP 响应头 + 保存 body ---------- */

/*
 * 通用的 HTTP 响应处理逻辑：
 * - 从 fd 读取数据（SSL 或 plain 由调用方传入的 read_func 决定）
 * - 解析 HTTP 头，找到 Content-Length
 * - 将 body 写入文件
 */

/* 读回调：返回读到的字节数，<0 表示错误 */
typedef int (*read_func_t)(void* ctx, unsigned char* buf, size_t len);

static int ssl_read_wrapper(void* ctx, unsigned char* buf, size_t len)
{
    return mbedtls_ssl_read((mbedtls_ssl_context*)ctx, buf, len);
}

static int plain_read_wrapper(void* ctx, unsigned char* buf, size_t len)
{
    return mbedtls_net_recv((mbedtls_net_context*)ctx, buf, len);
}



/* ================================================================
 * OTA 环形缓冲区（跨 TCP 分包累积，支持流式读写）
 * ================================================================ */
#define OTA_HEAD_SIZE  sizeof(OTAFileInfo)  
#define OTA_BUF_SIZE   2048

static uint8_t  ota_buf[OTA_BUF_SIZE];  /* 环形缓冲区 */
static size_t   ota_w = 0;              /* 写指针 */
static size_t   ota_r = 0;              /* 读指针 */
static size_t   ota_cnt = 0;            /* 可读字节数 */
static int      ota_parsed = 0;         /* OTA 头是否已解析 */

/* 向环形缓冲区写入数据 */
static void ota_write(const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (ota_cnt >= OTA_BUF_SIZE) {
            /* 缓冲区满，丢弃最旧字节（覆盖前移读指针） */
            ota_r = (ota_r + 1) % OTA_BUF_SIZE;
            ota_cnt--;
        }
        ota_buf[ota_w] = data[i];
        ota_w = (ota_w + 1) % OTA_BUF_SIZE;
        ota_cnt++;
    }
}

/* 从环形缓冲区读取数据（不消费，仅查看） */
static size_t ota_peek(uint8_t* dst, size_t len)
{
    size_t actual = (len < ota_cnt) ? len : ota_cnt;
    size_t r = ota_r;
    for (size_t i = 0; i < actual; i++) {
        dst[i] = ota_buf[r];
        r = (r + 1) % OTA_BUF_SIZE;
    }
    return actual;
}

/* 从环形缓冲区消费（跳过）指定字节数 */
static void ota_skip(size_t len)
{
    if (len > ota_cnt)
        len = ota_cnt;
    ota_r   = (ota_r + len) % OTA_BUF_SIZE;
    ota_cnt -= len;
}

/* 检查环形缓冲区中可用字节数 */
static size_t ota_available(void)
{
    return ota_cnt;
}

/* 尝试解析 OTA 头部：缓冲区有足够数据时，peek sizof(OTAFileInfo)字节解析并消费 */
static int ota_try_parse(OTAFileInfo* info)
{
    if (ota_parsed || ota_cnt < OTA_HEAD_SIZE)
        return 0;

    uint8_t head[OTA_HEAD_SIZE];
    ota_peek(head, OTA_HEAD_SIZE);

    if (parse_ota_file(info, head, OTA_HEAD_SIZE) == 0) {
        LOG_INFO("Parsed OTA file: AIC size=%u, Lock size=%u, T33 size=%u\n",
                 info->AIC_header.file_size,
                 info->Lock_header.file_size,
                 info->T33_header.file_size);
        LOG_INFO("AIC  MD5: %.32s, offset=%u\n",
                 info->AIC_header.md5, info->AIC_header.offset);
        LOG_INFO("Lock MD5: %.32s, offset=%u\n",
                 info->Lock_header.md5, info->Lock_header.offset);
        LOG_INFO("T33  MD5: %.32s, offset=%u\n",
                 info->T33_header.md5, info->T33_header.offset);

        LOG_INFO("AIC  version: %.16s\n", info->AIC_header.version);
        LOG_INFO("Lock version: %.16s\n", info->Lock_header.version);
        LOG_INFO("T33  version: %.16s\n", info->T33_header.version);
    }



    
    ota_skip(OTA_HEAD_SIZE);
    ota_parsed = 1;
    return 0;
}

/* 重置环形缓冲区 */
static void ota_reset(void)
{
    ota_w      = 0;
    ota_r      = 0;
    ota_cnt    = 0;
    ota_parsed = 0;
}

/* ================================================================
 * OTA 流消费回调机制（每种文件独立回调）
 * ================================================================ */

typedef enum {
    OTA_FILE_AIC  = 0,
    OTA_FILE_LOCK = 1,
    OTA_FILE_T33  = 2,
    OTA_FILE_NONE = -1,
    OTA_FILE_MAX  = 3
} ota_file_type_t;

/* 单个文件回调：返回 0 继续，<0 中止下载 */
typedef int (*ota_consume_cb_t)(size_t file_offset,
                                 const uint8_t* data,
                                 size_t len,
                                 void* user_data);

/* ---- 三个独立回调 + 上下文 ---- */
static ota_consume_cb_t ota_cb[OTA_FILE_MAX] = { NULL, NULL, NULL };
static void*            ota_cb_ctx[OTA_FILE_MAX] = { NULL, NULL, NULL };

static size_t           ota_total_body = 0;   /* body 总字节数 */
static int              ota_stream_valid = 0; /* 流是否已准备好 */

typedef struct {
    uint32_t file_size;
    uint32_t offset;      /* 绝对偏移（从 OTA 文件头 0 开始） */
} ota_file_desc_t;

static ota_file_desc_t ota_files[OTA_FILE_MAX];

/* 初始化流处理（解析头部后调用） */
static void ota_stream_init(const OTAFileInfo* info)
{
    ota_files[OTA_FILE_AIC].file_size  = info->AIC_header.file_size;
    ota_files[OTA_FILE_AIC].offset     = info->AIC_header.offset;
    ota_files[OTA_FILE_LOCK].file_size = info->Lock_header.file_size;
    ota_files[OTA_FILE_LOCK].offset    = info->Lock_header.offset;
    ota_files[OTA_FILE_T33].file_size  = info->T33_header.file_size;
    ota_files[OTA_FILE_T33].offset     = info->T33_header.offset;

    ota_total_body = OTA_HEAD_SIZE;
    ota_stream_valid = 1;

    LOG_INFO("OTA stream ready: AIC(%u@%u) Lock(%u@%u) T33(%u@%u)\n",
             ota_files[0].file_size, ota_files[0].offset,
             ota_files[1].file_size, ota_files[1].offset,
             ota_files[2].file_size, ota_files[2].offset);
}

/* 根据绝对流位置判断属于哪个文件 */
static ota_file_type_t ota_file_by_offset(size_t abs_offset, size_t* file_offset)
{
    for (int i = 0; i < OTA_FILE_MAX; i++) {
        if (abs_offset >= ota_files[i].offset &&
            abs_offset <  ota_files[i].offset + ota_files[i].file_size) {
            *file_offset = abs_offset - ota_files[i].offset;
            return (ota_file_type_t)i;
        }
    }
    return OTA_FILE_NONE;
}

/* 处理环形队列中的 body 数据，根据文件类型调用对应的回调 */
static int ota_stream_dispatch(void)
{
    if (!ota_stream_valid)
        return 0;

    static const char* const ota_file_names[] = { "AIC", "Lock", "T33" };

    while (ota_available() > 0) {
        size_t foff = 0;
        ota_file_type_t ftype = ota_file_by_offset(ota_total_body, &foff);

        if (ftype == OTA_FILE_NONE) {
            /* 间隙/尾部，跳过 */
            ota_skip(1);
            ota_total_body++;
            continue;
        }

        size_t file_remaining = ota_files[ftype].file_size - foff;
        size_t buf_available  = ota_available();
        size_t chunk = (file_remaining < buf_available)
                       ? file_remaining : buf_available;

        if (ota_cb[ftype]) {
            /* 该文件有注册回调 → 调用 */
            uint8_t tmp[1024];  /* 临时缓冲区，分块调用回调，避免一次性 peek 太多数据 */
            size_t to_process = (chunk < sizeof(tmp)) ? chunk : sizeof(tmp);
            for (size_t pos = 0; pos < chunk; pos += to_process) {
                size_t n = (chunk - pos < sizeof(tmp))
                           ? (chunk - pos) : sizeof(tmp);
                ota_peek(tmp, n);
                int ret = ota_cb[ftype](foff + pos, tmp, n, ota_cb_ctx[ftype]);
                if (ret < 0) {
                    LOG_ERROR("Consumer aborted at %s offset %u\n",
                              ota_file_names[ftype], (unsigned)(foff + pos));
                    return -1;
                }
                ota_skip(n);
                ota_total_body += n;
            }
        } else {
            /* 该文件无回调 → 直接跳过 */
            ota_skip(chunk);
            ota_total_body += chunk;
        }
    }

    return 0;
}

/* 设置单个文件的消费回调（type 指定, cb=NULL 表示取消） */
static void ota_set_file_consumer(ota_file_type_t type,
                                   ota_consume_cb_t cb,
                                   void* user_data)
{
    if (type >= 0 && type < OTA_FILE_MAX) {
        ota_cb[type]     = cb;
        ota_cb_ctx[type] = user_data;
    }
}

/* 便捷接口：同时设置三个文件的回调 */
static void ota_set_all_consumers(ota_consume_cb_t aci_cb,  void* aci_ctx,
                                   ota_consume_cb_t lock_cb, void* lock_ctx,
                                   ota_consume_cb_t t33_cb,  void* t33_ctx)
{
    ota_set_file_consumer(OTA_FILE_AIC,  aci_cb,  aci_ctx);
    ota_set_file_consumer(OTA_FILE_LOCK, lock_cb, lock_ctx);
    ota_set_file_consumer(OTA_FILE_T33,  t33_cb,  t33_ctx);
}

/* 重置流状态 */
static void ota_stream_reset(void)
{
    ota_stream_valid = 0;
    ota_total_body   = 0;
    memset(ota_cb,     0, sizeof(ota_cb));
    memset(ota_cb_ctx, 0, sizeof(ota_cb_ctx));
    memset(ota_files,  0, sizeof(ota_files));
}
OTAFileInfo info;


static int receive_and_save_impl(void* io_ctx, read_func_t read_func)
{
    ota_reset();
    memset(&info, 0, sizeof(info));
#define BUF_SIZE 2048
    char *buf = NULL;
    char *header = NULL;
    int now_offset = 0;
    
    size_t header_len = 0;
    int header_done = 0;
    int content_length = -1;
    int received_body = 0;
    int ret;
    static int first_time = 1;

    buf = ( char *)malloc(BUF_SIZE);
    if (!buf) {
        LOG_ERROR("Failed to allocate buffer\n");
        ret = -1;
        goto cleanup;
    }

    header = ( char *)malloc(BUF_SIZE);
    if (!header) {
        LOG_ERROR("Failed to allocate header buffer\n");
        ret = -1;
        goto cleanup;
    }


    while (1) {
        ret = read_func(io_ctx, (unsigned char*)buf, BUF_SIZE);
        if (ret < 0) {
            if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
                ret == MBEDTLS_ERR_SSL_TIMEOUT) {
                LOG_INFO("Read timeout, retrying...\n");
                continue;
            }
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                LOG_INFO("Peer closed connection\n");
                break;  /* 正常关闭 */
            }
            LOG_ERROR("read failed: %d\n", ret);
            first_time = 1;  /* 连接异常，下一次重试时重新解析 header */
            ret = -1;
            goto cleanup;
            
        }
        if (ret == 0) {
            LOG_INFO("Connection closed by peer\n");
            first_time = 1;  /* 连接关闭，下一次重试时重新解析 header */
            ret = -1;
            goto cleanup;
        }
        

        if (!header_done) {
            size_t to_copy = (size_t)ret;
            if (header_len + to_copy >= BUF_SIZE)
                to_copy = BUF_SIZE - header_len - 1;
            memcpy(header + header_len, buf, to_copy);
            header_len += to_copy;
            header[header_len] = '\0';

            LOG_INFO("Received %d bytes, header so far:\n%s", ret, header);

             {
                const char* header_end = strstr(header, "\r\n\r\n");
                if (header_end != NULL) {
                    header_done = 1;

                    /* 解析 Content-Length */
                    {
                        const char* cl_pos = strstr(header, "Content-Length:");
                        if (cl_pos == NULL)
                            cl_pos = strstr(header, "content-length:");
                        if (cl_pos != NULL) {
                            content_length = atoi(cl_pos + 15);
                            LOG_INFO("Content-Length: %d\n", content_length);
                        }
                    }

                    /* 检查 HTTP 状态码 (200 OK 或 206 Partial Content) */
                    if (strstr(header, "200 OK") == NULL &&
                        strstr(header, "200 ") == NULL &&
                        strstr(header, "206 ") == NULL) {
                        LOG_ERROR("HTTP error: %.80s\n", header);
                    }

                    /* 处理 header 之后的 body 残余数据 */
                    {
                        /*
                         * 当前 chunk 在 header 缓冲区中的起始位置:
                         *   chunk_start = header_len - to_copy
                         * body 在 header 缓冲区中的起始位置:
                         *   body_start  = (header_end - header) + 4
                         *
                         * 如果 body_start > chunk_start，说明 body 从当前 chunk 中间开始
                         *   → buf 中的 body 偏移 = body_start - chunk_start
                         *   → body_len = ret - body_offset
                         */
                        size_t chunk_start = header_len - to_copy;
                        size_t body_start  = (size_t)(header_end - header) + 4;

                        LOG_INFO("Header length: %u, Body starts at: %u\n",
                                 (unsigned)header_len, (unsigned)body_start);

                        if (body_start > chunk_start) {
                            size_t body_offset = body_start - chunk_start;
                            size_t body_len    = (size_t)ret - body_offset;

                            if (body_len > 0) {
                                ota_write((const uint8_t *)(buf + body_offset), body_len);
                                received_body += body_len;
                                LOG_INFO("Received body  %d bytes (in header chunk)\n",
                                         (int)body_len);
                            }
                        }
                    }
                }
            }
        } else {
            /* 纯 body 包：累积到 OTA 缓冲区 */
            ota_write((uint8_t*)buf, (size_t)ret);
            received_body += ret;
        }
        
        ota_try_parse(&info);

        /* 头部解析成功后初始化流，后续每轮自动分发 */
        if (ota_parsed && !ota_stream_valid) {
            if(strcmp(info.T33_header.version, SENSELOCK_V3X_VERSION) == 0) {
                LOG_INFO("Received T33 version matches SenseLock V3X, skipping OTA\n");
                ret = 0xff;  /* 特殊返回值，表示版本不匹配，跳过 OTA */
                goto cleanup;
            }



            ota_stream_init(&info);
        }

        /* 驱动流分发：将环形缓冲区中的数据分发给回调 */
        if (ota_stream_dispatch() < 0) {
            ret = -1;
            goto cleanup;
        }
 

        if (content_length > 0 && received_body >= content_length) {
            break;
        }
    }

    LOG_INFO("Received body: %d bytes\n", received_body);
    first_time = 1;  /* 成功完成一次下载，下一次重试时重新解析 header */
  

    return 0;
cleanup:
    if(buf) free(buf);
    if(header) free(header);

    
    LOG_ERROR("Failed to receive and save file: %d\n", ret);
    return ret;

#undef BUF_SIZE
}

static int receive_and_save_ssl(mbedtls_ssl_context* ssl)
{
    return receive_and_save_impl(ssl, ssl_read_wrapper);
}

static int receive_and_save_plain(mbedtls_net_context* fd)
{
    return receive_and_save_impl(fd, plain_read_wrapper);
}

/* ================================================================
 * 内部: 接收 HTTP 响应到内存缓冲区 (无 OTA 解析)
 * ================================================================ */

static int receive_to_buf(void* io_ctx, read_func_t read_func,
                          uint8_t* buf, size_t buf_size, size_t* received)
{
#define RBUF_SIZE 2048
    char* rbuf = NULL;
    char* header = NULL;
    size_t header_len = 0;
    int header_done = 0;
    int content_length = -1;
    size_t total_body = 0;
    int ret;

    *received = 0;

    rbuf = (char*)malloc(RBUF_SIZE);
    if (!rbuf) {
        LOG_ERROR("Failed to allocate read buffer\n");
        return -1;
    }

    header = (char*)malloc(RBUF_SIZE);
    if (!header) {
        LOG_ERROR("Failed to allocate header buffer\n");
        free(rbuf);
        return -1;
    }

    while (1) {
        ret = read_func(io_ctx, (unsigned char*)rbuf, RBUF_SIZE);
        if (ret < 0) {
            if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
                ret == MBEDTLS_ERR_SSL_TIMEOUT) {
                continue;
            }
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                break;
            }
            LOG_ERROR("read failed: %d\n", ret);
            ret = -1;
            goto cleanup;
        }
        if (ret == 0) {
            break;
        }

        if (!header_done) {
            size_t to_copy = (size_t)ret;
            if (header_len + to_copy >= RBUF_SIZE)
                to_copy = RBUF_SIZE - header_len - 1;
            memcpy(header + header_len, rbuf, to_copy);
            header_len += to_copy;
            header[header_len] = '\0';

            const char* header_end = strstr(header, "\r\n\r\n");
            if (header_end != NULL) {
                header_done = 1;

                /* 解析 Content-Length */
                const char* cl_pos = strstr(header, "Content-Length:");
                if (cl_pos == NULL)
                    cl_pos = strstr(header, "content-length:");
                if (cl_pos != NULL) {
                    content_length = atoi(cl_pos + 15);
                    LOG_INFO("Content-Length: %d\n", content_length);
                }

                /* 检查 HTTP 状态码 */
                if (strstr(header, "200 OK") == NULL &&
                    strstr(header, "200 ") == NULL &&
                    strstr(header, "206 ") == NULL) {
                    LOG_ERROR("HTTP error: %.80s\n", header);
                    ret = -2;
                    goto cleanup;
                }

                /* 处理 header 之后的残余 body 数据 */
                size_t chunk_start = header_len - to_copy;
                size_t body_start  = (size_t)(header_end - header) + 4;

                if (body_start > chunk_start) {
                    size_t body_offset = body_start - chunk_start;
                    size_t body_len    = (size_t)ret - body_offset;

                    if (body_len > 0) {
                        size_t copy = (total_body + body_len <= buf_size)
                                      ? body_len : (buf_size - total_body);
                        memcpy(buf + total_body, rbuf + body_offset, copy);
                        total_body += copy;
                    }
                }
            }
        } else {
            /* 纯 body 数据 */
            size_t copy = (total_body + (size_t)ret <= buf_size)
                          ? (size_t)ret : (buf_size - total_body);
            memcpy(buf + total_body, rbuf, copy);
            total_body += copy;
        }

        if (content_length > 0 && (int)total_body >= content_length) {
            break;
        }
        if (total_body >= buf_size) {
            break;
        }
    }

    *received = total_body;
    LOG_INFO("Received body: %zu bytes\n", total_body);
    ret = 0;

cleanup:
    free(rbuf);
    free(header);
    return ret;
#undef RBUF_SIZE
}


int parse_ota_file(OTAFileInfo* info, const uint8_t* buffer, size_t buffer_size)
{
    if(!info || !buffer) {
        LOG_ERROR("Invalid arguments to parse_ota_file\n");
        return -1;
    }

    if (buffer_size < sizeof(OTAFileInfo)) {
        LOG_ERROR("Buffer too small for OTAFileInfo\n");
        return -1;
    }
    
    /* 解析 AIC 头部 */
    info->AIC_header.file_size = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    info->AIC_header.offset = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    memcpy(info->AIC_header.version, buffer + 8, 32);
    memcpy(info->AIC_header.md5, buffer + 40, 32);

    /* 解析 Lock 头部 */
    info->Lock_header.file_size = (buffer[72] << 24) | (buffer[73] << 16) | (buffer[74] << 8) | buffer[75];
    info->Lock_header.offset = (buffer[76] << 24) | (buffer[77] << 16) | (buffer[78] << 8) | buffer[79];
    memcpy(info->Lock_header.version, buffer + 80, 32);
    memcpy(info->Lock_header.md5, buffer + 112, 32);

    /* 解析 T33 头部 */
    info->T33_header.file_size = (buffer[144] << 24) | (buffer[145] << 16) | (buffer[146] << 8) | buffer[147];
    info->T33_header.offset = (buffer[148] << 24) | (buffer[149] << 16) | (buffer[150] << 8) | buffer[151];
    memcpy(info->T33_header.version, buffer + 152, 32);
    memcpy(info->T33_header.md5, buffer + 184, 32);
  
    return 0;
}




static time_t last_msg_time = 0;



void start_ota_download(const char* url)
{
    https_downloader_t downloader;
    OTAFileInfo ota_info;
    uint8_t header_buf[sizeof(OTAFileInfo)];
    size_t received = 0;
    int ret;
    char cmd[128] = {0};

    LOG_INFO("=== OTA Range download start ===\n");

    /* 1. 下载 OTA 头部 (前 %u 字节) */
    https_downloader_init(&downloader);
    ret = https_download_to_buf(&downloader, url, 5000,
                                0, sizeof(OTAFileInfo) - 1,
                                header_buf, sizeof(header_buf), &received);
    https_downloader_close(&downloader);

    if (ret != 0 || received < sizeof(OTAFileInfo)) {
        LOG_ERROR("Failed to download OTA header: ret=%d, received=%zu\n",
                  ret, received);
        sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
        system(cmd);
        return;
    }

    /* 2. 解析 OTA 头部 */
    memset(&ota_info, 0, sizeof(ota_info));
    if (parse_ota_file(&ota_info, header_buf, received) != 0) {
        LOG_ERROR("Failed to parse OTA header\n");
        sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
        system(cmd);
        return;
    }

    LOG_INFO("T33 offset=%u, size=%u, version=%.32s\n",
             ota_info.T33_header.offset, ota_info.T33_header.file_size,
             ota_info.T33_header.version);

#ifndef CKEY_NO_OTA_CHECK_VERSION
    /* 3. 检查 T33 版本，匹配则跳过 */
    if (strcmp(ota_info.T33_header.version, SENSELOCK_V3X_VERSION) == 0) {
        LOG_INFO("T33 version matches SenseLock V3X, skipping OTA\n");
        sprintf(cmd, "custom_msg vnet0 30 22 OTA_VERSIONSAME");
        system(cmd);
        return;
    }
#endif
    /* 4. 按 256K 分片下载 T33 数据（跳过 AIC 和 Lock） */
#define CHUNK_SIZE (256 * 1024)

    uint8_t* chunk = (uint8_t*)malloc(CHUNK_SIZE);
    if (!chunk) {
        LOG_ERROR("Failed to allocate chunk buffer\n");
        sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
        system(cmd);
        return;
    }

    FILE* t33_file = fopen("/tmp/t33.bin", "wb");
    if (!t33_file) {
        LOG_ERROR("Failed to open /tmp/t33.bin for writing\n");
        free(chunk);
        sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
        system(cmd);
        return;
    }

    size_t offset  = ota_info.T33_header.offset;
    size_t remaining = ota_info.T33_header.file_size;
    int chunk_index = 0;
    last_msg_time = 0;

    while (remaining > 0) {
        size_t chunk_size = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        size_t range_end  = offset + chunk_size - 1;

        int retry;
        for (retry = 0; retry < 5; retry++) {
            https_downloader_init(&downloader);
            ret = https_download_to_buf(&downloader, url, 10000,
                                         offset, range_end,
                                         chunk, chunk_size, &received);
            https_downloader_close(&downloader);

            if (ret == 0 && received > 0)
                break;

            LOG_ERROR("Chunk %d download failed (attempt %d/5): ret=%d, offset=%zu, size=%zu\n",
                      chunk_index, retry + 1, ret, offset, chunk_size);
            sprintf(cmd, "custom_msg vnet0 30 22 OTAT33_OFFSET %u",
                        (unsigned)(offset - ota_info.T33_header.offset));
            system(cmd);
        }

        if (retry == 5) {
            LOG_ERROR("Chunk %d download failed after 5 retries\n", chunk_index);
            fclose(t33_file);
            free(chunk);
            sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
            system(cmd);
            return;
        }

        size_t written = fwrite(chunk, 1, received, t33_file);
        if (written != received) {
            LOG_ERROR("Chunk %d: fwrite failed, wrote %zu of %zu\n",
                      chunk_index, written, received);
            fclose(t33_file);
            free(chunk);
            sprintf(cmd, "custom_msg vnet0 30 22 OTA_FAILED");
            system(cmd);
            return;
        }

        offset    += received;
        remaining -= received;
        chunk_index++;

        /* 每秒通知一次主控进度 */
        {
            time_t now = time(NULL);
            if (now - last_msg_time >= 1) {
                sprintf(cmd, "custom_msg vnet0 30 22 OTAT33_OFFSET %u",
                        (unsigned)(offset - ota_info.T33_header.offset));
                system(cmd);
                last_msg_time = now;
            }
        }

        LOG_INFO("Chunk %d: %zu bytes, offset=%zu, remaining=%zu\n",
                 chunk_index - 1, received, offset, remaining);
    }

    fclose(t33_file);
    free(chunk);

    /* 5. 校验 MD5 并执行 OTA */
    {
        char md5_str[33] = {0};
        memcpy(md5_str, ota_info.T33_header.md5, 32);
        md5_str[32] = '\0';
        LOG_INFO("Expected T33 MD5: %.32s\n", md5_str);

        int res = ovd_ota_run("/tmp/t33.bin", md5_str);
        sprintf(cmd, "custom_msg vnet0 30 22 %s",
                res == 0 ? "OTA_SUCCESS" : "OTA_FAILED");
        system(cmd);
    }

    LOG_INFO("=== OTA Range download end ===\n");
#undef CHUNK_SIZE
}
    
