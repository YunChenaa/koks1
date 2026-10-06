/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 23:09:19
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-24 16:37:29
 * @FilePath: /sense-koks/application/communication/src/crypto.cpp
 */
#include "crypto.h"
#include "sys_logger.h"
#include "aes.h"
#include <fcntl.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "CRYPTO"
#define KEY_SIZE 16

#define IOT_AES_ENCRYPT 0
#define IOT_AES_DECRYPT 1
#define ALIGN_UP(v, n) (((unsigned long)(v) + (n)-1) & ~((n)-1))

int crypto::Init()
{
#ifndef USE_SOFT_AES_ENCRPT
    memset(&para, 0, sizeof(para));
    handle = open(aes_node.data(), O_RDWR);
    if (handle < 0) {
        LOG_ERROR("Create handle failed\n");
        return -1;
    }
#endif
    return 0;
}

int crypto::Deinit()
{
#ifndef USE_SOFT_AES_ENCRPT
    int ret = 0;
    if (handle < 0) {
        LOG_ERROR("Invalid handle\n");
        return -1;
    }
    ret = close(handle);
    if (ret != 0) {
        LOG_ERROR("Destroy handle failed\n");
        return -1;
    }
    LOG_DEBUG("Deinit success\n");
#endif
    return 0;
}

crypto& crypto::GetInstance()
{
    static crypto instance;
    return instance;
}

#ifndef USE_SOFT_AES_ENCRPT
static unsigned int reverse_int(unsigned int n)
{
    return ((n & 0x000000FF) << 24) | ((n & 0x0000FF00) << 8) | ((n & 0x00FF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

static void swap32_block(uint8_t *buf, int len)
{
    for (int i = 0; i < len; i += 4) {
        uint8_t t0 = buf[i];
        uint8_t t1 = buf[i + 1];
        buf[i]     = buf[i + 3];
        buf[i + 1] = buf[i + 2];
        buf[i + 2] = t1;
        buf[i + 3] = t0;
    }
}
#endif

int crypto::aes_crypto(unsigned char* in, int len, unsigned char* out, char* key, int mode)
{
#ifdef USE_SOFT_AES_ENCRPT
    struct AES_ctx ctx;
    int alin_len = ALIGN_UP(len, 16);
    int left = len;

    AES_init_ctx(&ctx, (uint8_t*)key);
    for(int i = 0; i < alin_len / 16; i++) {
        uint8_t buffer[16] = {0};
        if(left >= 16) {
            memcpy(buffer, in + i * 16, 16);
            left -= 16;
            if (mode == AES_ENCRYPT) {
                AES_ECB_encrypt(&ctx, buffer);
            }
            else {
                AES_ECB_decrypt(&ctx, buffer);
            }
            memcpy(out + i * 16, buffer, 16);
        }
        else {
            memcpy(buffer, in + i * 16, left);
            if (mode == AES_ENCRYPT) {
                AES_ECB_encrypt(&ctx, buffer);
            }
            else {
                AES_ECB_decrypt(&ctx, buffer);
            }
            memcpy(out + i * 16, buffer, 16);
        }
    }
#else
    std::unique_lock<std::mutex> _lock(aes_lock, std::defer_lock);
    if (!in || !out || len == 0 || !key) {
        LOG_ERROR("aes_encrypt:invalid params\n");
        return -1;
    }
    LOG_INFO("key = %s\n", key);
    int alin_len = ALIGN_UP(len, 16);
    unsigned char* data = new unsigned char[alin_len];
    std::unique_ptr<unsigned char[]> data_ptr(data);
    memset(data, 0, alin_len);
    memcpy(data, in, len);
    swap32_block(data, alin_len);
    _lock.lock();
    memset(&para, 0, sizeof(para));
    para.status = JZ_AES_STATUS_PREPARE;
    para.aeskey[0] = reverse_int(*(unsigned int*)key);
    para.aeskey[1] = reverse_int(*(unsigned int*)(key + 4));
    para.aeskey[2] = reverse_int(*(unsigned int*)(key + 8));
    para.aeskey[3] = reverse_int(*(unsigned int*)(key + 12));
    para.src = data;
    para.dst = out;
    para.datalen = alin_len;
    para.keybits = 128;

    if (mode == AES_ENCRYPT) {
        // LOG_INFO("AES_ENCRYPT\n");
        para.enworkmode = IN_UNF_CIPHER_WORK_MODE_ENC_ECB;
        if (ioctl(handle, IOCTL_AES_SG_PROCESSING, &para) < 0) {
            LOG_ERROR("AES_ENCRYPT failed\n");
            return -1;
        }
        swap32_block(out, alin_len);
    } else {
        // LOG_INFO("AES_DECRYPT\n");
        para.enworkmode = IN_UNF_CIPHER_WORK_MODE_DEC_ECB;
        if (ioctl(handle, IOCTL_AES_SG_PROCESSING, &para) < 0) {
            LOG_ERROR("AES_DECRYPT failed\n");
            return -1;
        }
        swap32_block(out, alin_len);
    }
    _lock.unlock();
#endif
    return 0;
}

int aes_crypto(unsigned char* in, int len, unsigned char* out, char* key, int mode)
{
    return crypto::GetInstance().aes_crypto(in, len, out, key, mode);
}