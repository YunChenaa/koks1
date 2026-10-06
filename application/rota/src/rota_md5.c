/*
 * Copyright (c) 2025-2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#include "rota_md5.h"
#include "rota_utils.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define RL(x, y) (((x) << (y)) | ((x) >> (32 - (y))))
#define PP(x) (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24)
#define FF(a, b, c, d, x, s, ac) a = b + (RL((a + F(b, c, d) + x + ac), s))
#define GG(a, b, c, d, x, s, ac) a = b + (RL((a + G(b, c, d) + x + ac), s))
#define HH(a, b, c, d, x, s, ac) a = b + (RL((a + H(b, c, d) + x + ac), s))
#define II(a, b, c, d, x, s, ac) a = b + (RL((a + I(b, c, d) + x + ac), s))

typedef struct {
    unsigned int A;
    unsigned int B;
    unsigned int C;
    unsigned int D;
} md5_var;

static void utils_md5_transform(md5_var* var, unsigned int* array)
{
    unsigned int a, b, c, d;

    a = var->A;
    b = var->B;
    c = var->C;
    d = var->D;

    FF(a, b, c, d, array[0], 7, 0xd76aa478);
    FF(d, a, b, c, array[1], 12, 0xe8c7b756);
    FF(c, d, a, b, array[2], 17, 0x242070db);
    FF(b, c, d, a, array[3], 22, 0xc1bdceee);
    FF(a, b, c, d, array[4], 7, 0xf57c0faf);
    FF(d, a, b, c, array[5], 12, 0x4787c62a);
    FF(c, d, a, b, array[6], 17, 0xa8304613);
    FF(b, c, d, a, array[7], 22, 0xfd469501);
    FF(a, b, c, d, array[8], 7, 0x698098d8);
    FF(d, a, b, c, array[9], 12, 0x8b44f7af);
    FF(c, d, a, b, array[10], 17, 0xffff5bb1);
    FF(b, c, d, a, array[11], 22, 0x895cd7be);
    FF(a, b, c, d, array[12], 7, 0x6b901122);
    FF(d, a, b, c, array[13], 12, 0xfd987193);
    FF(c, d, a, b, array[14], 17, 0xa679438e);
    FF(b, c, d, a, array[15], 22, 0x49b40821);

    GG(a, b, c, d, array[1], 5, 0xf61e2562);
    GG(d, a, b, c, array[6], 9, 0xc040b340);
    GG(c, d, a, b, array[11], 14, 0x265e5a51);
    GG(b, c, d, a, array[0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, array[5], 5, 0xd62f105d);
    GG(d, a, b, c, array[10], 9, 0x02441453);
    GG(c, d, a, b, array[15], 14, 0xd8a1e681);
    GG(b, c, d, a, array[4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, array[9], 5, 0x21e1cde6);
    GG(d, a, b, c, array[14], 9, 0xc33707d6);
    GG(c, d, a, b, array[3], 14, 0xf4d50d87);
    GG(b, c, d, a, array[8], 20, 0x455a14ed);
    GG(a, b, c, d, array[13], 5, 0xa9e3e905);
    GG(d, a, b, c, array[2], 9, 0xfcefa3f8);
    GG(c, d, a, b, array[7], 14, 0x676f02d9);
    GG(b, c, d, a, array[12], 20, 0x8d2a4c8a);

    HH(a, b, c, d, array[5], 4, 0xfffa3942);
    HH(d, a, b, c, array[8], 11, 0x8771f681);
    HH(c, d, a, b, array[11], 16, 0x6d9d6122);
    HH(b, c, d, a, array[14], 23, 0xfde5380c);
    HH(a, b, c, d, array[1], 4, 0xa4beea44);
    HH(d, a, b, c, array[4], 11, 0x4bdecfa9);
    HH(c, d, a, b, array[7], 16, 0xf6bb4b60);
    HH(b, c, d, a, array[10], 23, 0xbebfbc70);
    HH(a, b, c, d, array[13], 4, 0x289b7ec6);
    HH(d, a, b, c, array[0], 11, 0xeaa127fa);
    HH(c, d, a, b, array[3], 16, 0xd4ef3085);
    HH(b, c, d, a, array[6], 23, 0x04881d05);
    HH(a, b, c, d, array[9], 4, 0xd9d4d039);
    HH(d, a, b, c, array[12], 11, 0xe6db99e5);
    HH(c, d, a, b, array[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, array[2], 23, 0xc4ac5665);

    II(a, b, c, d, array[0], 6, 0xf4292244);
    II(d, a, b, c, array[7], 10, 0x432aff97);
    II(c, d, a, b, array[14], 15, 0xab9423a7);
    II(b, c, d, a, array[5], 21, 0xfc93a039);
    II(a, b, c, d, array[12], 6, 0x655b59c3);
    II(d, a, b, c, array[3], 10, 0x8f0ccc92);
    II(c, d, a, b, array[10], 15, 0xffeff47d);
    II(b, c, d, a, array[1], 21, 0x85845dd1);
    II(a, b, c, d, array[8], 6, 0x6fa87e4f);
    II(d, a, b, c, array[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, array[6], 15, 0xa3014314);
    II(b, c, d, a, array[13], 21, 0x4e0811a1);
    II(a, b, c, d, array[4], 6, 0xf7537e82);
    II(d, a, b, c, array[11], 10, 0xbd3af235);
    II(c, d, a, b, array[2], 15, 0x2ad7d2bb);
    II(b, c, d, a, array[9], 21, 0xeb86d391);

    var->A += a;
    var->B += b;
    var->C += c;
    var->D += d;
}

/****************************************************************************
Function Name:           rota_md5_sum
Input Parameters:
Output Parameters:       void
Used Global Variables:   void
Return Code:             void
Description:
****************************************************************************/
int rota_md5_sum(char* data, int length, char* value)
{
    md5_var var;
    unsigned int count[2] = { 0 };
    unsigned int array[16] = { 0 };
    int index = 0;
    int pos = 0;

    if (data == NULL || length <= 0 || value == NULL) {
        return -1;
    }

    var.A = 0x67452301;
    var.B = 0xefcdab89;
    var.C = 0x98badcfe;
    var.D = 0x10325476;

    count[0] = (length % 0x20000000) * 8;
    count[1] = length / 0x20000000;

    memset(array, 0, 64);
    if (length > 64) {
        memcpy(array, data, 64);
    } else {
        memcpy(array, data, length);
    }

    for (index = 0; index < length / 64; index++) {
        utils_md5_transform(&var, array);
        memset(array, 0, 64);
        pos += 64;
        if (length - pos > 64) {
            memcpy(array, data + pos, 64);
        } else {
            memcpy(array, data + pos, length - pos);
        }
    }

    ((char*)array)[length % 64] = 0x80;
    if (length % 64 > 55) {
        utils_md5_transform(&var, array);
        memset(array, 0, 64);
    }
    memcpy(array + 14, count, 8);
    utils_md5_transform(&var, array);

    sprintf(value, "%08x%08x%08x%08x", PP(var.A), PP(var.B), PP(var.C), PP(var.D));

    return 0;
}

int rota_md5_check_fw(uint8_t* data, uint32_t Len, const char* expected_md5)
{
    char cacl_md5[32 + 1] = { 0 };
    int ret = rota_md5_sum((char*)data, Len, cacl_md5);
    if (ret != 0) {
        return ret;
    }

    ret = strncmp(expected_md5, cacl_md5, 32);
    LOG_INFO("expected_md5 %s, cacl_md5 %s, ret %d\n", expected_md5, cacl_md5, ret);
    return ret;
}
