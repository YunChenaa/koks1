/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 19:47:19
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:34:29
 * @FilePath: /sense-koks/application/media_core/src/st_koks_video.cpp
 */
#include "st_koks_video.h"
#include "imp_encoder.h"
#include "imp_system.h"
#include "os_utils.h"
#include "st_koks_isp.h"
#include "sys_logger.h"
#include "koks_project_def.h"
#include "fast_frame.h"
#include "factory.h"
#include "koks1_config_types.h"

#include <cstdint>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <mxu2.h>
#include <termios.h>

#ifdef KOKS_PROJECT_M55
#include "bitmapinfo52.h"
#else
#include "bitmapinfo.h"
#endif

namespace st
{
namespace sense_koks
{

// static void rotate90(const unsigned char* src, int src_width, int src_height, unsigned char* dst)
// {
//     const int dst_width = src_height;
//     const int dst_height = src_width;

//     for (int y = 0; y < src_height; ++y) {
//         for (int x = 0; x < src_width; ++x) {
//             unsigned char val = src[y * src_width + x];
//             int dst_x = src_height - 1 - y; // 原始的高度减去当前行号再减一，以实现逆时针旋转
//             int dst_y = x;                  // 当前列号变为新行号
//             dst[dst_y * dst_width + dst_x] = val;
//         }
//     }
// }


// a : 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30
// b : 1 3 5 7 9 11 13 15 17 19 21 23 25 27 29 31
static inline v16i8 mxu_unpacklo_epi8(v16i8 a, v16i8 b) {
    // [a0 b0 a1 b1 ... a7 b7]
    static const v16i8 m = { 0,1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpackhi_epi8(v16i8 a, v16i8 b) {
    // [a8 b8 ... a15 b15]
    static const v16i8 m = { 16,17,18,19,20,21,22,23, 24,25,26,27,28,29,30,31 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpacklo_epi16(v16i8 a, v16i8 b) {
    // [a0 a1 b0 b1 a2 a3 b2 b3 ...]
    static const v16i8 m = {0,2,1,3,4,6,5,7,8,10,9,11,12,14,13,15};
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpackhi_epi16(v16i8 a, v16i8 b) {
    static const v16i8 m = {16,18,17,19,20,22,21,23,24,26,25,27,28,30,29,31};
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpacklo_epi32(v16i8 a, v16i8 b) {
    static const v16i8 m = { 0,2,4,6, 1,3,5,7, 8,10,12,14, 9,11,13,15 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpackhi_epi32(v16i8 a, v16i8 b) {
    static const v16i8 m = { 16,18,20,22, 17,19,21,23, 24,26,28,30, 25,27,29,31 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpacklo_epi64(v16i8 a, v16i8 b) {
    static const v16i8 m = { 0,2,4,6,8,10,12,14,1,3,5,7,9,11,13,15 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_unpackhi_epi64(v16i8 a, v16i8 b) {
    static const v16i8 m = { 16,18,20,22,24,26,28,30, 17,19,21,23,25,27,29,31 };
    return _mx128_shufv(b, a, m);
}
static inline v16i8 mxu_reverse_16B(v16i8 a) {
    // 16B 逆序
    static const v16i8 m = {30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0};
    return _mx128_shufv(a, a, m);
}

// 16x16 字节矩阵转置
static inline void transpose_16x16_bytes(
    v16i8 &r0, v16i8 &r1, v16i8 &r2, v16i8 &r3,
    v16i8 &r4, v16i8 &r5, v16i8 &r6, v16i8 &r7,
    v16i8 &r8, v16i8 &r9, v16i8 &r10, v16i8 &r11,
    v16i8 &r12, v16i8 &r13, v16i8 &r14, v16i8 &r15)
{
    // Stage 1: 8-bit -> 16-bit pairs
    v16i8 t0  = mxu_unpacklo_epi8(r0,  r1);
    v16i8 t1  = mxu_unpackhi_epi8(r0,  r1);
    v16i8 t2  = mxu_unpacklo_epi8(r2,  r3);
    v16i8 t3  = mxu_unpackhi_epi8(r2,  r3);
    v16i8 t4  = mxu_unpacklo_epi8(r4,  r5);
    v16i8 t5  = mxu_unpackhi_epi8(r4,  r5);
    v16i8 t6  = mxu_unpacklo_epi8(r6,  r7);
    v16i8 t7  = mxu_unpackhi_epi8(r6,  r7);
    v16i8 t8  = mxu_unpacklo_epi8(r8,  r9);
    v16i8 t9  = mxu_unpackhi_epi8(r8,  r9);
    v16i8 t10 = mxu_unpacklo_epi8(r10, r11);
    v16i8 t11 = mxu_unpackhi_epi8(r10, r11);
    v16i8 t12 = mxu_unpacklo_epi8(r12, r13);
    v16i8 t13 = mxu_unpackhi_epi8(r12, r13);
    v16i8 t14 = mxu_unpacklo_epi8(r14, r15);
    v16i8 t15 = mxu_unpackhi_epi8(r14, r15);

    // Stage 2: 16-bit -> 32-bit pairs
    r0  = mxu_unpacklo_epi16(t0,  t2);
    r1  = mxu_unpackhi_epi16(t0,  t2);
    r2  = mxu_unpacklo_epi16(t1,  t3);
    r3  = mxu_unpackhi_epi16(t1,  t3);
    r4  = mxu_unpacklo_epi16(t4,  t6);
    r5  = mxu_unpackhi_epi16(t4,  t6);
    r6  = mxu_unpacklo_epi16(t5,  t7);
    r7  = mxu_unpackhi_epi16(t5,  t7);
    r8  = mxu_unpacklo_epi16(t8,  t10);
    r9  = mxu_unpackhi_epi16(t8,  t10);
    r10 = mxu_unpacklo_epi16(t9,  t11);
    r11 = mxu_unpackhi_epi16(t9,  t11);
    r12 = mxu_unpacklo_epi16(t12, t14);
    r13 = mxu_unpackhi_epi16(t12, t14);
    r14 = mxu_unpacklo_epi16(t13, t15);
    r15 = mxu_unpackhi_epi16(t13, t15);

    // Stage 3: 32-bit -> 64-bit pairs
    t0  = mxu_unpacklo_epi32(r0,  r4);
    t1  = mxu_unpackhi_epi32(r0,  r4);
    t2  = mxu_unpacklo_epi32(r1,  r5);
    t3  = mxu_unpackhi_epi32(r1,  r5);
    t4  = mxu_unpacklo_epi32(r2,  r6);
    t5  = mxu_unpackhi_epi32(r2,  r6);
    t6  = mxu_unpacklo_epi32(r3,  r7);
    t7  = mxu_unpackhi_epi32(r3,  r7);
    t8  = mxu_unpacklo_epi32(r8,  r12);
    t9  = mxu_unpackhi_epi32(r8,  r12);
    t10 = mxu_unpacklo_epi32(r9,  r13);
    t11 = mxu_unpackhi_epi32(r9,  r13);
    t12 = mxu_unpacklo_epi32(r10, r14);
    t13 = mxu_unpackhi_epi32(r10, r14);
    t14 = mxu_unpacklo_epi32(r11, r15);
    t15 = mxu_unpackhi_epi32(r11, r15);

    // Stage 4: 64-bit -> 128-bit 
    r0  = mxu_unpacklo_epi64(t0,  t8);
    r1  = mxu_unpackhi_epi64(t0,  t8);
    r2  = mxu_unpacklo_epi64(t1,  t9);
    r3  = mxu_unpackhi_epi64(t1,  t9);
    r4  = mxu_unpacklo_epi64(t2,  t10);
    r5  = mxu_unpackhi_epi64(t2,  t10);
    r6  = mxu_unpacklo_epi64(t3,  t11);
    r7  = mxu_unpackhi_epi64(t3,  t11);
    r8  = mxu_unpacklo_epi64(t4,  t12);
    r9  = mxu_unpackhi_epi64(t4,  t12);
    r10 = mxu_unpacklo_epi64(t5,  t13);
    r11 = mxu_unpackhi_epi64(t5,  t13);
    r12 = mxu_unpacklo_epi64(t6,  t14);
    r13 = mxu_unpackhi_epi64(t6,  t14);
    r14 = mxu_unpacklo_epi64(t7,  t15);
    r15 = mxu_unpackhi_epi64(t7,  t15);
}

static void mxu_copy_bulk_v16i8_optimized(void *dst, const void *src, int byte_count)
{
    uint8_t *d = (uint8_t *)dst;
    uint8_t *s = (uint8_t *)src;

    int full_vectors = byte_count / 16;
    int remain_bytes = byte_count % 16;

    // 批量处理：每次4个向量（64字节）
    for (int i = 0; i < full_vectors; i += 4) {
        v16i8 vecs[4];

        // 批量加载
        vecs[0] = _mx128_lu1q(s + i * 16, 0);
        vecs[1] = _mx128_lu1q(s + (i + 1) * 16, 0);
        vecs[2] = _mx128_lu1q(s + (i + 2) * 16, 0);
        vecs[3] = _mx128_lu1q(s + (i + 3) * 16, 0);

        // 批量存储
        _mx128_su1q(vecs[0],  (d + (i + 0) * 16), 0);
        _mx128_su1q(vecs[1],  (d + (i + 1) * 16), 0);
        _mx128_su1q(vecs[2],  (d + (i + 2) * 16), 0);
        _mx128_su1q(vecs[3],  (d + (i + 3) * 16), 0);
    }

    // 处理尾部剩余字节
    if (remain_bytes > 0) {
        for(int i = full_vectors * 16; i < full_vectors * 16 + remain_bytes; i++) {
            d[i] = s[i];
        }
    }
}

/// @brief 90 度顺时针旋转（MXU2 128 位版，按 16x16 tile）
/// 要求：width、height 为 16 的倍数（否则补标量收尾）
static void rotate90(uint8_t* src, int src_width, int src_height, uint8_t* dst)
{
    const int dst_w = src_height;

    for (int y = 0; y < src_height; y += 16) {
        for (int x = 0; x < src_width; x += 16) {
            // 1) 载入 16 行
            v16i8 r0  = _mx128_lu1q(src + (y +  0) * src_width + x, 0);
            v16i8 r1  = _mx128_lu1q(src + (y +  1) * src_width + x, 0);
            v16i8 r2  = _mx128_lu1q(src + (y +  2) * src_width + x, 0);
            v16i8 r3  = _mx128_lu1q(src + (y +  3) * src_width + x, 0);
            v16i8 r4  = _mx128_lu1q(src + (y +  4) * src_width + x, 0);
            v16i8 r5  = _mx128_lu1q(src + (y +  5) * src_width + x, 0);
            v16i8 r6  = _mx128_lu1q(src + (y +  6) * src_width + x, 0);
            v16i8 r7  = _mx128_lu1q(src + (y +  7) * src_width + x, 0);
            v16i8 r8  = _mx128_lu1q(src + (y +  8) * src_width + x, 0);
            v16i8 r9  = _mx128_lu1q(src + (y +  9) * src_width + x, 0);
            v16i8 r10 = _mx128_lu1q(src + (y + 10) * src_width + x, 0);
            v16i8 r11 = _mx128_lu1q(src + (y + 11) * src_width + x, 0);
            v16i8 r12 = _mx128_lu1q(src + (y + 12) * src_width + x, 0);
            v16i8 r13 = _mx128_lu1q(src + (y + 13) * src_width + x, 0);
            v16i8 r14 = _mx128_lu1q(src + (y + 14) * src_width + x, 0);
            v16i8 r15 = _mx128_lu1q(src + (y + 15) * src_width + x, 0);

            // 2) 16x16 转置（块列 -> 寄存器行）
            transpose_16x16_bytes(
                r0, r1, r2, r3, r4, r5, r6, r7,
                r8, r9, r10, r11, r12, r13, r14, r15);

            // 3) 行内倒序，映射到顺时针90°的目标行
            r0  = mxu_reverse_16B(r0);
            r1  = mxu_reverse_16B(r1);
            r2  = mxu_reverse_16B(r2);
            r3  = mxu_reverse_16B(r3);
            r4  = mxu_reverse_16B(r4);
            r5  = mxu_reverse_16B(r5);
            r6  = mxu_reverse_16B(r6);
            r7  = mxu_reverse_16B(r7);
            r8  = mxu_reverse_16B(r8);
            r9  = mxu_reverse_16B(r9);
            r10 = mxu_reverse_16B(r10);
            r11 = mxu_reverse_16B(r11);
            r12 = mxu_reverse_16B(r12);
            r13 = mxu_reverse_16B(r13);
            r14 = mxu_reverse_16B(r14);
            r15 = mxu_reverse_16B(r15);

            // 4) 写回（每个 rK 成为目标图的连续一行 16B 片段）
            const int dst_base_col = src_height - 16 - y; // 水平方向起始（已倒序）
            const int dst_base_row = x;                   // 目标的行号基（由原列 x 推导）

            _mx128_su1q(r0,  dst + (dst_base_row +  0) * dst_w + dst_base_col, 0);
            _mx128_su1q(r1,  dst + (dst_base_row +  1) * dst_w + dst_base_col, 0);
            _mx128_su1q(r2,  dst + (dst_base_row +  2) * dst_w + dst_base_col, 0);
            _mx128_su1q(r3,  dst + (dst_base_row +  3) * dst_w + dst_base_col, 0);
            _mx128_su1q(r4,  dst + (dst_base_row +  4) * dst_w + dst_base_col, 0);
            _mx128_su1q(r5,  dst + (dst_base_row +  5) * dst_w + dst_base_col, 0);
            _mx128_su1q(r6,  dst + (dst_base_row +  6) * dst_w + dst_base_col, 0);
            _mx128_su1q(r7,  dst + (dst_base_row +  7) * dst_w + dst_base_col, 0);
            _mx128_su1q(r8,  dst + (dst_base_row +  8) * dst_w + dst_base_col, 0);
            _mx128_su1q(r9,  dst + (dst_base_row +  9) * dst_w + dst_base_col, 0);
            _mx128_su1q(r10, dst + (dst_base_row + 10) * dst_w + dst_base_col, 0);
            _mx128_su1q(r11, dst + (dst_base_row + 11) * dst_w + dst_base_col, 0);
            _mx128_su1q(r12, dst + (dst_base_row + 12) * dst_w + dst_base_col, 0);
            _mx128_su1q(r13, dst + (dst_base_row + 13) * dst_w + dst_base_col, 0);
            _mx128_su1q(r14, dst + (dst_base_row + 14) * dst_w + dst_base_col, 0);
            _mx128_su1q(r15, dst + (dst_base_row + 15) * dst_w + dst_base_col, 0);
        }
    }
}

/// @brief 180 度旋转（MXU2 128 位版，按 16x16 tile）
/// 要求：width、height 为 16 的倍数
static void rotate180(uint8_t* src, int src_width, int src_height, uint8_t* dst)
{
    const int dst_w = src_width;  // 180° 宽高不变

    for (int y = 0; y < src_height; y += 16) {
        for (int x = 0; x < src_width; x += 16) {
            // 1) 载入 16 行
            v16i8 r0  = _mx128_lu1q(src + (y +  0) * src_width + x, 0);
            v16i8 r1  = _mx128_lu1q(src + (y +  1) * src_width + x, 0);
            v16i8 r2  = _mx128_lu1q(src + (y +  2) * src_width + x, 0);
            v16i8 r3  = _mx128_lu1q(src + (y +  3) * src_width + x, 0);
            v16i8 r4  = _mx128_lu1q(src + (y +  4) * src_width + x, 0);
            v16i8 r5  = _mx128_lu1q(src + (y +  5) * src_width + x, 0);
            v16i8 r6  = _mx128_lu1q(src + (y +  6) * src_width + x, 0);
            v16i8 r7  = _mx128_lu1q(src + (y +  7) * src_width + x, 0);
            v16i8 r8  = _mx128_lu1q(src + (y +  8) * src_width + x, 0);
            v16i8 r9  = _mx128_lu1q(src + (y +  9) * src_width + x, 0);
            v16i8 r10 = _mx128_lu1q(src + (y + 10) * src_width + x, 0);
            v16i8 r11 = _mx128_lu1q(src + (y + 11) * src_width + x, 0);
            v16i8 r12 = _mx128_lu1q(src + (y + 12) * src_width + x, 0);
            v16i8 r13 = _mx128_lu1q(src + (y + 13) * src_width + x, 0);
            v16i8 r14 = _mx128_lu1q(src + (y + 14) * src_width + x, 0);
            v16i8 r15 = _mx128_lu1q(src + (y + 15) * src_width + x, 0);

            // 2) 行内水平翻转
            r0  = mxu_reverse_16B(r0);
            r1  = mxu_reverse_16B(r1);
            r2  = mxu_reverse_16B(r2);
            r3  = mxu_reverse_16B(r3);
            r4  = mxu_reverse_16B(r4);
            r5  = mxu_reverse_16B(r5);
            r6  = mxu_reverse_16B(r6);
            r7  = mxu_reverse_16B(r7);
            r8  = mxu_reverse_16B(r8);
            r9  = mxu_reverse_16B(r9);
            r10 = mxu_reverse_16B(r10);
            r11 = mxu_reverse_16B(r11);
            r12 = mxu_reverse_16B(r12);
            r13 = mxu_reverse_16B(r13);
            r14 = mxu_reverse_16B(r14);
            r15 = mxu_reverse_16B(r15);

            // 3) 写回：行倒序 (r15→r0) + 列倒序
            //    dst[H-16-y + j][W-16-x + k] = src[y+15-j][x+15-k]
            const int dst_base_row = src_height - 16 - y;
            const int dst_base_col = src_width  - 16 - x;

            _mx128_su1q(r15, dst + (dst_base_row +  0) * dst_w + dst_base_col, 0);
            _mx128_su1q(r14, dst + (dst_base_row +  1) * dst_w + dst_base_col, 0);
            _mx128_su1q(r13, dst + (dst_base_row +  2) * dst_w + dst_base_col, 0);
            _mx128_su1q(r12, dst + (dst_base_row +  3) * dst_w + dst_base_col, 0);
            _mx128_su1q(r11, dst + (dst_base_row +  4) * dst_w + dst_base_col, 0);
            _mx128_su1q(r10, dst + (dst_base_row +  5) * dst_w + dst_base_col, 0);
            _mx128_su1q(r9,  dst + (dst_base_row +  6) * dst_w + dst_base_col, 0);
            _mx128_su1q(r8,  dst + (dst_base_row +  7) * dst_w + dst_base_col, 0);
            _mx128_su1q(r7,  dst + (dst_base_row +  8) * dst_w + dst_base_col, 0);
            _mx128_su1q(r6,  dst + (dst_base_row +  9) * dst_w + dst_base_col, 0);
            _mx128_su1q(r5,  dst + (dst_base_row + 10) * dst_w + dst_base_col, 0);
            _mx128_su1q(r4,  dst + (dst_base_row + 11) * dst_w + dst_base_col, 0);
            _mx128_su1q(r3,  dst + (dst_base_row + 12) * dst_w + dst_base_col, 0);
            _mx128_su1q(r2,  dst + (dst_base_row + 13) * dst_w + dst_base_col, 0);
            _mx128_su1q(r1,  dst + (dst_base_row + 14) * dst_w + dst_base_col, 0);
            _mx128_su1q(r0,  dst + (dst_base_row + 15) * dst_w + dst_base_col, 0);
        }
    }
}


/// @brief 270 度顺时针旋转（MXU2 128 位版，按 16x16 tile）
///        等价于 90 度逆时针旋转
///        算法：reverse_rows(transpose(src))
///        即：先矩阵转置，再将行倒序写回
/// 要求：width、height 为 16 的倍数
static void rotate270(uint8_t* src, int src_width, int src_height, uint8_t* dst)
{
    const int dst_w = src_height;

    for (int y = 0; y < src_height; y += 16) {
        for (int x = 0; x < src_width; x += 16) {
            // 1) 载入 16 行
            v16i8 r0  = _mx128_lu1q(src + (y +  0) * src_width + x, 0);
            v16i8 r1  = _mx128_lu1q(src + (y +  1) * src_width + x, 0);
            v16i8 r2  = _mx128_lu1q(src + (y +  2) * src_width + x, 0);
            v16i8 r3  = _mx128_lu1q(src + (y +  3) * src_width + x, 0);
            v16i8 r4  = _mx128_lu1q(src + (y +  4) * src_width + x, 0);
            v16i8 r5  = _mx128_lu1q(src + (y +  5) * src_width + x, 0);
            v16i8 r6  = _mx128_lu1q(src + (y +  6) * src_width + x, 0);
            v16i8 r7  = _mx128_lu1q(src + (y +  7) * src_width + x, 0);
            v16i8 r8  = _mx128_lu1q(src + (y +  8) * src_width + x, 0);
            v16i8 r9  = _mx128_lu1q(src + (y +  9) * src_width + x, 0);
            v16i8 r10 = _mx128_lu1q(src + (y + 10) * src_width + x, 0);
            v16i8 r11 = _mx128_lu1q(src + (y + 11) * src_width + x, 0);
            v16i8 r12 = _mx128_lu1q(src + (y + 12) * src_width + x, 0);
            v16i8 r13 = _mx128_lu1q(src + (y + 13) * src_width + x, 0);
            v16i8 r14 = _mx128_lu1q(src + (y + 14) * src_width + x, 0);
            v16i8 r15 = _mx128_lu1q(src + (y + 15) * src_width + x, 0);

            // 2) 16x16 转置（无 reverse！）
            //    转置后 r_k[j] = src[y+j][x+k]
            transpose_16x16_bytes(
                r0, r1, r2, r3, r4, r5, r6, r7,
                r8, r9, r10, r11, r12, r13, r14, r15);

            // 3) 写回：行倒序 (r15→r0)，垂直翻转
            //    dst[W-16-x + (15-k)][y + j] = src[y+j][x+k]
            const int dst_base_row = src_width - 16 - x;
            const int dst_base_col = y;

            _mx128_su1q(r15, dst + (dst_base_row +  0) * dst_w + dst_base_col, 0);
            _mx128_su1q(r14, dst + (dst_base_row +  1) * dst_w + dst_base_col, 0);
            _mx128_su1q(r13, dst + (dst_base_row +  2) * dst_w + dst_base_col, 0);
            _mx128_su1q(r12, dst + (dst_base_row +  3) * dst_w + dst_base_col, 0);
            _mx128_su1q(r11, dst + (dst_base_row +  4) * dst_w + dst_base_col, 0);
            _mx128_su1q(r10, dst + (dst_base_row +  5) * dst_w + dst_base_col, 0);
            _mx128_su1q(r9,  dst + (dst_base_row +  6) * dst_w + dst_base_col, 0);
            _mx128_su1q(r8,  dst + (dst_base_row +  7) * dst_w + dst_base_col, 0);
            _mx128_su1q(r7,  dst + (dst_base_row +  8) * dst_w + dst_base_col, 0);
            _mx128_su1q(r6,  dst + (dst_base_row +  9) * dst_w + dst_base_col, 0);
            _mx128_su1q(r5,  dst + (dst_base_row + 10) * dst_w + dst_base_col, 0);
            _mx128_su1q(r4,  dst + (dst_base_row + 11) * dst_w + dst_base_col, 0);
            _mx128_su1q(r3,  dst + (dst_base_row + 12) * dst_w + dst_base_col, 0);
            _mx128_su1q(r2,  dst + (dst_base_row + 13) * dst_w + dst_base_col, 0);
            _mx128_su1q(r1,  dst + (dst_base_row + 14) * dst_w + dst_base_col, 0);
            _mx128_su1q(r0,  dst + (dst_base_row + 15) * dst_w + dst_base_col, 0);
        }
    }
}

// ============================================================
// NV12 旋转主函数
// ============================================================

/// @brief NV12 270 度顺时针旋转（等价于 90 度逆时针）
/// @param src      源 NV12 图像（Y 平面起始地址）
/// @param width    图像宽度（须为 16 的倍数）
/// @param height   图像高度（须为 16 的倍数）
/// @param dst      目标 NV12 图像（Y 平面起始地址），需预分配 width*height*3/2 字节
static void rotate270_nv12(uint8_t* src, int width, int height, uint8_t* dst)
{
    // --- 1) Y 平面旋转 ---
    //     复用已有的字节级 16x16 tile 旋转
    rotate270(src, width, height, dst);

    // --- 2) UV 平面旋转 ---
    //     UV 平面 = 图像宽 W 字节 × 高 H/2 行
    //     但 UV 对不可拆分，所以先解交织 → 分别旋转 U、V → 再交织
    int src_width = width;
    int src_height = height;
    int dst_width = src_height;
    int dst_height = src_width;
    
    const uint8_t* src_uv = src + src_width * src_height;
    uint8_t* dst_uv = dst + dst_width * dst_height;

    // 注意：UV平面的宽高是Y平面的一半
    int src_uv_width = src_width / 2;
    int src_uv_height = src_height / 2;
    int dst_uv_width = dst_width / 2;
    int dst_uv_height = dst_height / 2;

    for (int y = 0; y < src_uv_height; ++y) {
        for (int x = 0; x < src_uv_width; ++x) {
            // 对每个2x2块，其UV坐标映射与Y平面一致
            int src_idx = (y * src_uv_width + x) * 2; // 每块2字节(U和V)
            uint8_t u = src_uv[src_idx];
            uint8_t v = src_uv[src_idx + 1];

            // 计算目标2x2块的位置 (应用相同的旋转公式)
            int dst_x = y;
            int dst_y_dst = src_uv_width - 1 - x;

            int dst_idx = (dst_y_dst * dst_uv_width + dst_x) * 2;
            dst_uv[dst_idx] = u;
            dst_uv[dst_idx + 1] = v;
        }
    }
}


enum {
    PRO_ZRT_IOCTL_DMA_GET_FRAME = 1,
    PRO_ZRT_IOCTL_FREE_DMA_FRAME,
    PRO_ZRT_IOCTL_FRAME_DONE,
    PRO_ZRT_IOCTL_INVALUE,
};

#define MEM_DEV "/dev/mem"
#define SNAP_DEV "/proc/jz/zeratul/dma_frame"
#define ZRT_IOCTL_TYPE 'Z'
#define ZRT_IOCTL_GET_DMA_FRAME _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_DMA_GET_FRAME, struct fast_frame_info)
#define ZRT_IOCTL_FREE_DMA_FRAME _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_FREE_DMA_FRAME, struct fast_frame_info)
#define ZRT_IOCTL_FRAME_DONE    _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_FRAME_DONE, struct fast_frame_ev_info)


ST_ERR GetFastFrame(void* firstImage, void* secondImage, int16_t width, int16_t height, struct fast_frame_ev_info *info)
{
    int mem_fd = 0;
    void *vaddr = NULL, *firstImagePtr = NULL, *secondImagePtr = NULL;
    int ret = 0, retries = 0;
    const int PAGE_SIZE = sysconf(_SC_PAGESIZE);
    unsigned int image_size = width * height * 3 / 2 * 3;

    off_t offset = FRAME_PYADDR & ~(PAGE_SIZE - 1);
    size_t extra = FRAME_PYADDR - offset;
    size_t alloc_size = image_size + extra;

    int fd = open(SNAP_DEV, O_RDWR);
    if (fd == -1) {
        LOG_ERROR("Failed to open device\n");
        return -1;
    }

    while(retries++ < 10) {
        ret = ioctl(fd, ZRT_IOCTL_FRAME_DONE, info);
        if (ret == -1) {
            LOG_ERROR("ioctl error: %s\n", strerror(errno));
            break;
        }

        if(info->isReady != 0x3) {
            LOG_INFO("no frame done\n");
            usleep(10 * 1000);
            continue;
        }

        st::sense_koks::Isp::ev_info_ = *info;
        LOG_INFO("isDone: %d, irAeIntegrationTime0:%d, irAeIntegrationTime1:%d, irAeAGain1:%d, irAeIspDGain1:%d, rgbAeAGain:%d\n",
                    info->isReady, info->irAeIntegrationTime0, info->irAeIntegrationTime1, info->irAeAGain1, info->irAeIspDGain1, info->rgbAeAGain);
        mem_fd = open(MEM_DEV, O_RDONLY);
        if (mem_fd <= 0) {
            LOG_ERROR("open %s failed\n", MEM_DEV);
            ret = -1;
            break;
        }

        vaddr = mmap(NULL, alloc_size, PROT_READ, MAP_SHARED, mem_fd, offset);
        if (vaddr == MAP_FAILED) {
            LOG_ERROR("mmap failed, error: %s\n", strerror(errno));
            ret = -1;
            break;
        }

        firstImagePtr = (unsigned char*)vaddr + extra;
#ifdef KOKS_FEATURE_FAST_LDC
        secondImagePtr = (unsigned char*)vaddr + extra + width * height * 3;
        mxu_copy_bulk_v16i8_optimized(firstImage, firstImagePtr, width * height * 3 / 2);
        mxu_copy_bulk_v16i8_optimized(secondImage, secondImagePtr, width * height * 3 / 2);
#else
        secondImagePtr = (unsigned char*)vaddr + extra + width * height * 3 / 2;
#ifdef KOKS_PROJECT_M53D
        rotate270((uint8_t *)firstImagePtr, width, height, (uint8_t *)firstImage);
        rotate270((uint8_t *)secondImagePtr, width, height, (uint8_t *)secondImage);
#else
        rotate90((uint8_t *)firstImagePtr, width, height, (uint8_t *)firstImage);
        rotate90((uint8_t *)secondImagePtr, width, height, (uint8_t *)secondImage);
#endif // KOKS_PROJECT_M53D

   
#endif
        break;
    }

    if (vaddr) {
        munmap(vaddr, alloc_size);
    }

    if (mem_fd > 0) {
        close(mem_fd);
    }

    if (fd > 0) {
        close(fd);
    }

    return ret;
}

/**
 * @name: DeviceFrameSource
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*}
 */
DeviceFrameSource::DeviceFrameSource(int32_t dev_id) : Device(dev_id)
{
    // source_id_ = dev_id;
}

/**
 * @name: DeviceFrameSource
 * @brief: destructor
 * @return {*}
 */
DeviceFrameSource::~DeviceFrameSource()
{
}

/**
 * @name: Init
 * @brief: 视频源初始化
 * @param {Json&} config 配置文件
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::Init(const struct FrameSourceConfig& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto dev_id = config.group;
    source_id_ = dev_id;
    memset(&ChnConfig, 0, sizeof(ChnConfig));
    ChnConfig.fs_chn_attr.pixFmt = PIX_FMT_NV12;
    ChnConfig.fs_chn_attr.outFrmRateNum = Isp::GetInstance().GetSensorFrameRate();
    ChnConfig.fs_chn_attr.outFrmRateDen = SENSOR_FRAME_RATE_DEN;
    if (source_id_ == 0 || source_id_ == 3) {
        ChnConfig.fs_chn_attr.nrVBs = 1;
    } else {
#ifdef KOKS_FEATURE_T33_ZN
        ChnConfig.fs_chn_attr.nrVBs = 2;
#else
        ChnConfig.fs_chn_attr.nrVBs = 1;
#endif
    }

    ChnConfig.fs_chn_attr.type = FS_PHY_CHANNEL;
    LOG_DEBUG("source %d pixFmt %d nrVBs %d\n", source_id_, ChnConfig.fs_chn_attr.pixFmt, ChnConfig.fs_chn_attr.nrVBs);
    // memset(&ChnConfig.fs_chn_attr, 0, sizeof(ChnConfig.fs_chn_attr));
    is_enable = config.enable;
    auto input_width = config.input.width;
    auto input_height = config.input.height;
    if (is_enable == true) {
        ChnConfig.fs_chn_attr.picWidth = input_width;
        ChnConfig.fs_chn_attr.picHeight = input_height;
        ChnConfig.fs_chn_attr.scaler.enable = config.scaler.enable;
        ChnConfig.fs_chn_attr.scaler.outwidth = config.scaler.width;
        ChnConfig.fs_chn_attr.scaler.outheight = config.scaler.height;

        ChnConfig.fs_chn_attr.crop.enable = config.crop.enable;
        ChnConfig.fs_chn_attr.crop.left = config.crop.left;
        ChnConfig.fs_chn_attr.crop.top = config.crop.top;
        ChnConfig.fs_chn_attr.crop.width = config.crop.width;
        ChnConfig.fs_chn_attr.crop.height = config.crop.height;

        auto& i2d_config = config.input._2d;
        ChnConfig.fs_chn_attr.i2dattr.i2d_enable = i2d_config.enable;
        ChnConfig.fs_chn_attr.i2dattr.flip_enable = i2d_config.flip;
        ChnConfig.fs_chn_attr.i2dattr.mirr_enable = i2d_config.mirror;
        ChnConfig.fs_chn_attr.i2dattr.rotate_enable = i2d_config.rotate.enable;
        ChnConfig.fs_chn_attr.i2dattr.rotate_angle = i2d_config.rotate.angle;

        rotate_angle_ = i2d_config.enable ? ChnConfig.fs_chn_attr.i2dattr.rotate_angle : 0;

        if (source_id_ == 0 || source_id_ == 3) {
            ret = IMP_FrameSource_CreateChn(source_id_, &ChnConfig.fs_chn_attr);
            MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_CreateChn failed with %#x\n", source_id_, ret);
            LOG_DEBUG("source %d IMP_FrameSource_CreateChn success!\n", source_id_);
            ret = IMP_FrameSource_SetChnAttr(source_id_, &ChnConfig.fs_chn_attr);
            LOG_INFO("i2d enable %d, flip %d, mirror %d, rotate %d->%d°\n", ChnConfig.fs_chn_attr.i2dattr.i2d_enable,
                     ChnConfig.fs_chn_attr.i2dattr.flip_enable, ChnConfig.fs_chn_attr.i2dattr.mirr_enable, ChnConfig.fs_chn_attr.i2dattr.rotate_enable,
                     ChnConfig.fs_chn_attr.i2dattr.rotate_angle);
            MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SetChnAttr failed with %#x\n", source_id_, ret);
            LOG_DEBUG("source %d IMP_FrameSource_SetChnAttr success!\n", source_id_);
        }
        // if (ChnConfig.fs_chn_attr.i2dattr.i2d_enable == true) {
        //     if (source_id_ != 0 && source_id_ != 3) {
        //     } else {
        //         ret = IMP_FrameSource_SetI2dAttr(source_id_, &ChnConfig.fs_chn_attr.i2dattr);
        //         MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SetI2dAttr failed with %#x\n", source_id_, ret);
        //         LOG_DEBUG("source %d 2d attr enable %d,flip %d, mirror %d, rotate %d->%d°\n", source_id_, ChnConfig.fs_chn_attr.i2dattr.i2d_enable,
        //                   ChnConfig.fs_chn_attr.i2dattr.flip_enable, ChnConfig.fs_chn_attr.i2dattr.mirr_enable, ChnConfig.fs_chn_attr.i2dattr.rotate_enable,
        //                   ChnConfig.fs_chn_attr.i2dattr.rotate_angle);
        //     }
        // }
        SetIMPCell();
        LOG_DEBUG("frame source dev %d Init success!\n", dev_id);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 视频源启动
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::Start(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable == true) {
        ret = IMP_FrameSource_EnableChn(source_id_);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_EnableChn failed with %#x\n", source_id_, ret);
        LOG_DEBUG("source %d IMP_FrameSource_EnableChn success!\n", source_id_);
        if (source_id_ == 1 || source_id_ == 5 || source_id_ == 7) { // mjpeg
            ret = IMP_FrameSource_SetFrameDepth(source_id_, 1);
            MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SetFrameDepth failed with %#x\n", source_id_, ret);
        }
        LOG_DEBUG("source %d IMP_FrameSource_SetFrameDepth success!\n", source_id_);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReInit
 * @brief: 视频源重启
 * @param {uint16_t} width 宽度
 * @param {uint16_t} height 高度
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::ReInit(const RestartAttr& param)
{
    ChnConfig.fs_chn_attr.picWidth = param.crop_width;
    ChnConfig.fs_chn_attr.picHeight = param.crop_height;
    ChnConfig.fs_chn_attr.scaler.enable = param.enable;
    ChnConfig.fs_chn_attr.scaler.outwidth = param.width;
    ChnConfig.fs_chn_attr.scaler.outheight = param.height;
    ChnConfig.fs_chn_attr.crop.enable = param.crop_enable;
    ChnConfig.fs_chn_attr.crop.left = param.crop_left;
    ChnConfig.fs_chn_attr.crop.top = param.crop_top;
    ChnConfig.fs_chn_attr.crop.width = param.crop_width;
    ChnConfig.fs_chn_attr.crop.height = param.crop_height;

    if (is_enable) {
        auto ret = IMP_FrameSource_CreateChn(source_id_, &ChnConfig.fs_chn_attr);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_CreateChn failed with %#x\n", source_id_, ret);
        ret = IMP_FrameSource_SetChnAttr(source_id_, &ChnConfig.fs_chn_attr);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SetChnAttr failed with %#x\n", source_id_, ret);
        // if (ChnConfig.fs_chn_attr.i2dattr.i2d_enable == true) {
        //     ret = IMP_FrameSource_SetI2dAttr(source_id_, &ChnConfig.fs_chn_attr.i2dattr);
        //     MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SetI2dAttr failed with %#x\n", source_id_, ret);
        // }
    }
    return ST_ERRNO_SUCCESS;
}

void DeviceFrameSource::EnableI2d(int rotate_angle)
{
    ChnConfig.fs_chn_attr.i2dattr.i2d_enable = (rotate_angle != 0);
    ChnConfig.fs_chn_attr.i2dattr.flip_enable = false;
    ChnConfig.fs_chn_attr.i2dattr.mirr_enable = false;
    ChnConfig.fs_chn_attr.i2dattr.rotate_enable = true;
    ChnConfig.fs_chn_attr.i2dattr.rotate_angle = rotate_angle;
}

ST_ERR DeviceFrameSource::Join(uint32_t chn, uint16_t join_type)
{
    memset(&joint_attr_, 0xff, sizeof(IMPFSJointAttr));
    if (join_type == 0) {
        joint_attr_.position[0][0] = source_id_;
        joint_attr_.position[1][0] = chn;
    } else {
        joint_attr_.position[0][0] = source_id_;
        joint_attr_.position[0][1] = chn;
    }
    auto ret = IMP_FrameSource_CreateJoint(&joint_attr_);
    MISC_CHK_STATUS_REPORT(ret, ret, "source %d %d IMP_FrameSource_CreateJoint failed with %#x\n", source_id_, chn, ret);
    if (source_id_ == 1 || source_id_ == 5) {
        ChnConfig.fs_chn_attr.picWidth = joint_attr_.width;
        ChnConfig.fs_chn_attr.picHeight = joint_attr_.height;
        ChnConfig.fs_chn_attr.pixFmt = PIX_FMT_NV12;
        ret = IMP_FrameSource_SetChnAttr(source_id_, &ChnConfig.fs_chn_attr);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d %d IMP_FrameSource_SetChnAttr failed with %#x\n", source_id_, chn, ret);
    }
    return ret;
}

ST_ERR DeviceFrameSource::DeJoin()
{
    auto ret = IMP_FrameSource_DestroyJoint(joint_attr_.handler);
    MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_DestroyJoint failed with %#x\n", source_id_, ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 视频源停止
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::Stop(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable == true) {
        ret = IMP_FrameSource_DisableChn(source_id_);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_DisableChn with %#x\n", source_id_, ret);
        LOG_DEBUG("source %d IMP_FrameSource_DisableChn success!\n", source_id_);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 视频源反初始化
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::DeInit(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable == true) {
        ret = IMP_FrameSource_DestroyChn(source_id_);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_DestroyChn with %#x\n", source_id_, ret);
    }
    // is_enable = false;
    // 恢复默认旋转
    EnableI2d(rotate_angle_);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: GetFrame
 * @brief: 获取视频帧
 * @param {IMPFrameInfo**} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::GetFrame(IMPFrameInfo** frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable) {
        ret = IMP_FrameSource_GetFrame(source_id_, frame);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_GetFrame failed with %#x\n", source_id_, ret);
        LOG_VERBOSE("source %d IMP_FrameSource_GetFrame success!\n", source_id_);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReleaseFrame
 * @brief: 释放视频帧
 * @param {IMPFrameInfo*} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::ReleaseFrame(IMPFrameInfo* frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable) {
        ret = IMP_FrameSource_ReleaseFrame(source_id_, frame);
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_ReleaseFrame failed with %#x\n", source_id_, ret);
        LOG_VERBOSE("source %d IMP_FrameSource_ReleaseFrame success!\n", source_id_);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SnapFrame
 * @brief: 快照视频帧
 * @param {void*} framedata 帧数据
 * @param {IMPFrameInfo*} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::SnapFrame(void* framedata, IMPFrameInfo* frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable) {
#ifdef KOKS_FEATURE_FAST_LDC
        ret = IMP_FrameSource_SnapFrame(source_id_, ChnConfig.fs_chn_attr.pixFmt, ChnConfig.fs_chn_attr.picWidth, ChnConfig.fs_chn_attr.picHeight, framedata, frame);
#else
        static std::vector<unsigned char> tmpdata(ChnConfig.fs_chn_attr.picWidth * ChnConfig.fs_chn_attr.picHeight * 3 / 2);
        ret = IMP_FrameSource_SnapFrame(source_id_, ChnConfig.fs_chn_attr.pixFmt, ChnConfig.fs_chn_attr.picWidth, ChnConfig.fs_chn_attr.picHeight, tmpdata.data(), frame);
        
#ifdef KOKS_PROJECT_M53D
        rotate270_nv12(tmpdata.data(), ChnConfig.fs_chn_attr.picWidth, ChnConfig.fs_chn_attr.picHeight, (unsigned char*)framedata);
#else
        rotate90(tmpdata.data(), ChnConfig.fs_chn_attr.picWidth, ChnConfig.fs_chn_attr.picHeight, (unsigned char*)framedata);
#endif

#endif
        MISC_CHK_STATUS_REPORT(ret, ret, "source %d IMP_FrameSource_SnapFrame failed with %#x\n", source_id_, ret);
        LOG_VERBOSE("source %d IMP_FrameSource_SnapFrame success!\n", source_id_);
        // LOG_INFO("source %d index %ld,ts %lld!\n", source_id_, frame->index, frame->timeStamp);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SaveFrame
 * @brief: 保存视频帧
 * @param {const std::string&} path 保存路径
 * @param {IMPFrameInfo&} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceFrameSource::SaveFrame(const std::string& path, IMPFrameInfo& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    FileGuard file = FileGuard(path);
    if (file.IsOpened()) {
        if (write(file.GetFd(), (void*)frame.virAddr, frame.size) != frame.size) {
            LOG_ERROR("write %s failed!\n", path.c_str());
            return ST_ERRNO_FAIL;
        }
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetIMPCell
 * @brief: 设置IMPCell
 * @return {*}
 */
void DeviceFrameSource::SetIMPCell(void)
{
    ChnConfig.SelfCell.deviceID = DEV_ID_FS;
    ChnConfig.SelfCell.groupID = source_id_;
    ChnConfig.SelfCell.outputID = 0;
}

/**
 * @name: GetIMPCell
 * @brief: 获取IMPCell
 * @return {*} IMPCell* IMPCell指针
 */
IMPCell* DeviceFrameSource::GetIMPCell(void)
{
    return &ChnConfig.SelfCell;
}

/**
 * @name: DeviceVenc
 * @brief: constructor
 * @param {int32_t} dev_id 设备ID
 * @return {*}
 */
DeviceVenc::DeviceVenc(int32_t dev_id) : Device(dev_id)
{
    group_id = dev_id;
}

/**
 * @name: DeviceVenc
 * @brief: destructor
 * @return {*}
 */
DeviceVenc::~DeviceVenc()
{
}

/**
 * @name: Init
 * @brief: 视频编码初始化
 * @param {Json&} config 配置文件
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::Init(const struct VencConfig& config)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    for (int i = 0; i < MAX_ENC_CHN; i++) {
        memset(&_VencChnConfig[i], 0, sizeof(VencChnConfig));
    }
    group_id = config.group;
    is_enable = config.enable;

    if (is_enable == true) {
#if 0
        ret = IMP_Encoder_CreateGroup(group_id);
        MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d IMP_Encoder_CreateGroup failed with %#x\n", group_id, ret);
#endif
        for (auto& iter : config.output) {
            auto chn_enable = iter.enable;
            if (chn_enable == true) {
                auto chn_id = iter.chn;
                auto enc_type = iter.type;

                _VencChnConfig[0].chn_id = chn_id;
                _VencChnConfig[0].is_enable = chn_enable;
                _VencChnConfig[0].type = GetEncType(enc_type);
                auto enc_attr = &_VencChnConfig[0].chn_attr.encAttr;
                enc_attr->enType = _VencChnConfig[0].type;
                enc_attr->picWidth = iter.width;
                enc_attr->picHeight = iter.height;

                enc_attr->crop.enable = iter.crop.enable;
                enc_attr->crop.x = iter.crop.left;
                enc_attr->crop.y = iter.crop.top;
                enc_attr->crop.w = iter.crop.width;
                enc_attr->crop.h = iter.crop.height;

                auto rc_attr = &_VencChnConfig[0].chn_attr.rcAttr;
                if (enc_attr->enType == PT_JPEG) {
                    auto qp = iter.quality;
                    if (qp < 0)
                        qp = 0;
                    if (qp > 98)
                        qp = 98;
                    enc_attr->bufSize = 0;
                    enc_attr->profile = 0;

                    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_FIXQP;
                    rc_attr->attrRcMode.attrJPEGFixQp.qp = qp;
                } else if (enc_attr->enType == PT_H264) {
                    enc_attr->profile = 1;
                    if (enc_attr->picHeight >= 1920) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3;
                    } else if (enc_attr->picHeight >= 1520) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    } else if (enc_attr->picHeight >= 1080) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    } else {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    }

                    rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                    rc_attr->attrHSkip.hSkipAttr.m = 3;
                    rc_attr->attrHSkip.hSkipAttr.n = 4;
                    rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
                    rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                    rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                    rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;

                    rc_attr->outFrmRate.frmRateNum = SENSOR_ENC_FRAME_RATE_NUM; // imp_chn_attr_tmp->outFrmRateNum;
                    rc_attr->outFrmRate.frmRateDen = 1;                         // imp_chn_attr_tmp->outFrmRateDen;
                    rc_attr->maxGop = rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;

                    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
#ifdef KOKS_FEATURE_KDS
                    rc_attr->attrRcMode.attrH264Cbr.maxQp = 51;
                    rc_attr->attrRcMode.attrH264Cbr.minQp = 28;
                    rc_attr->attrRcMode.attrH264Cbr.blkQpEn = 0;
                    rc_attr->attrRcMode.attrH264Cbr.initialQp = 35;
                    rc_attr->attrRcMode.attrH264Cbr.maxIQp = 48;
                    rc_attr->attrRcMode.attrH264Cbr.minIQp = 28;
                    rc_attr->attrRcMode.attrH264Cbr.outBitRate = BITRATE_720P_Kbs;
                    rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
                    rc_attr->attrRcMode.attrH264Cbr.IPfrmQPDelta = 8;
                    rc_attr->attrRcMode.attrH264Cbr.PPfrmQPDelta = 3;
                    rc_attr->attrRcMode.attrH264Cbr.staticTime = 2;
                    rc_attr->attrRcMode.attrH264Cbr.flucLvl = 2;
                    rc_attr->attrRcMode.attrH264Cbr.qualityLvl = 6;
                    rc_attr->attrRcMode.attrH264Cbr.maxIprop = 100;
                    rc_attr->attrRcMode.attrH264Cbr.minIprop = 1;
                    rc_attr->attrRcMode.attrH264Cbr.maxIPictureSize = 600;
                    rc_attr->attrRcMode.attrH264Cbr.maxPPictureSize = 500;
#elif defined(KOKS_VIDEO_RCMODE_VBR)
                    rc_attr->maxGop = 2 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
                    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
                    rc_attr->attrRcMode.attrH264Vbr.maxQp = 51;
                    rc_attr->attrRcMode.attrH264Vbr.minQp = 28;
                    rc_attr->attrRcMode.attrH264Vbr.blkQpEn = 0;
                    rc_attr->attrRcMode.attrH264Vbr.initialQp = 35;
                    rc_attr->attrRcMode.attrH264Vbr.maxIQp = 48;
                    rc_attr->attrRcMode.attrH264Vbr.minIQp = 28;
                    rc_attr->attrRcMode.attrH264Vbr.maxBitRate = BITRATE_720P_Kbs;
                    rc_attr->attrRcMode.attrH264Vbr.iBiasLvl = 0;
                    rc_attr->attrRcMode.attrH264Vbr.changePos = 75;
                    rc_attr->attrRcMode.attrH264Vbr.staticTime = 2;
                    rc_attr->attrRcMode.attrH264Vbr.qualityLvl = 6;
                    rc_attr->attrRcMode.attrH264Vbr.maxIprop = 100;
                    rc_attr->attrRcMode.attrH264Vbr.minIprop = 1;
                    rc_attr->attrRcMode.attrH264Vbr.maxIPictureSize = 560;
                    rc_attr->attrRcMode.attrH264Vbr.maxPPictureSize = 400;
#else // M2-4B
                    rc_attr->attrRcMode.attrH264Cbr.maxQp = 51;
                    rc_attr->attrRcMode.attrH264Cbr.minQp = 32;
                    rc_attr->attrRcMode.attrH264Cbr.blkQpEn = 0;
                    rc_attr->attrRcMode.attrH264Cbr.initialQp = 35;
                    rc_attr->attrRcMode.attrH264Cbr.maxIQp = 51;
                    rc_attr->attrRcMode.attrH264Cbr.minIQp = 32;
                    rc_attr->attrRcMode.attrH264Cbr.outBitRate = BITRATE_720P_Kbs;
                    rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
                    rc_attr->attrRcMode.attrH264Cbr.IPfrmQPDelta = 8;
                    rc_attr->attrRcMode.attrH264Cbr.PPfrmQPDelta = 3;
                    rc_attr->attrRcMode.attrH264Cbr.staticTime = 2;
                    rc_attr->attrRcMode.attrH264Cbr.flucLvl = 2;
                    rc_attr->attrRcMode.attrH264Cbr.qualityLvl = 6;
                    rc_attr->attrRcMode.attrH264Cbr.maxIprop = 100;
                    rc_attr->attrRcMode.attrH264Cbr.minIprop = 1;
                    rc_attr->attrRcMode.attrH264Cbr.maxIPictureSize = 504; // 63*8
                    rc_attr->attrRcMode.attrH264Cbr.maxPPictureSize = 420; // 504/1.2
#endif
                } else if (enc_attr->enType == PT_H265) {
                    enc_attr->profile = 1;
                    if (enc_attr->picHeight >= 1920) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3;
                    } else if (enc_attr->picHeight >= 1520) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    } else if (enc_attr->picHeight >= 1080) {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    } else {
                        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
                    }

                    rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
                    rc_attr->attrHSkip.hSkipAttr.m = 3;
                    rc_attr->attrHSkip.hSkipAttr.n = 4;
                    rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
                    rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
                    rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
                    rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;

                    rc_attr->outFrmRate.frmRateNum = SENSOR_ENC_FRAME_RATE_NUM; // imp_chn_attr_tmp->outFrmRateNum;
                    rc_attr->outFrmRate.frmRateDen = 1;                         // imp_chn_attr_tmp->outFrmRateDen;
#if defined(KOKS_VIDEO_RCMODE_VBR)
                    rc_attr->maxGop = 2 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
                    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
                    rc_attr->attrRcMode.attrH265Vbr.maxQp = 45;
                    rc_attr->attrRcMode.attrH265Vbr.minQp = 28;
                    rc_attr->attrRcMode.attrH265Vbr.blkQpEn = 0;
                    rc_attr->attrRcMode.attrH265Vbr.initialQp = 35;
                    rc_attr->attrRcMode.attrH265Vbr.maxIQp = 45;
                    rc_attr->attrRcMode.attrH265Vbr.minIQp = 28;
                    rc_attr->attrRcMode.attrH265Vbr.maxBitRate = BITRATE_720P_Kbs;
                    rc_attr->attrRcMode.attrH265Vbr.iBiasLvl = 0;
                    rc_attr->attrRcMode.attrH265Vbr.changePos = 75;
                    rc_attr->attrRcMode.attrH265Vbr.staticTime = 2;
                    rc_attr->attrRcMode.attrH265Vbr.qualityLvl = 6;
                    rc_attr->attrRcMode.attrH265Vbr.maxIprop = 100;
                    rc_attr->attrRcMode.attrH265Vbr.minIprop = 1;
                    rc_attr->attrRcMode.attrH265Vbr.maxIPictureSize = 560;
                    rc_attr->attrRcMode.attrH265Vbr.maxPPictureSize = 400;
#else
                    rc_attr->maxGop = 1 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
                    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
                    rc_attr->attrRcMode.attrH265Cbr.maxQp = 45;
                    rc_attr->attrRcMode.attrH265Cbr.minQp = 29;
                    rc_attr->attrRcMode.attrH265Cbr.blkQpEn = 0;
                    rc_attr->attrRcMode.attrH265Cbr.initialQp = 35;
                    rc_attr->attrRcMode.attrH265Cbr.maxIQp = 45;
                    rc_attr->attrRcMode.attrH265Cbr.minIQp = 29;
                    rc_attr->attrRcMode.attrH265Cbr.outBitRate = BITRATE_720P_Kbs;
                    rc_attr->attrRcMode.attrH265Cbr.iBiasLvl = 0;
                    rc_attr->attrRcMode.attrH265Cbr.IPfrmQPDelta = 8;
                    rc_attr->attrRcMode.attrH265Cbr.PPfrmQPDelta = 3;
                    rc_attr->attrRcMode.attrH265Cbr.staticTime = 2;
                    rc_attr->attrRcMode.attrH265Cbr.flucLvl = 2;
                    rc_attr->attrRcMode.attrH265Cbr.qualityLvl = 6;
                    rc_attr->attrRcMode.attrH265Cbr.maxIprop = 100;
                    rc_attr->attrRcMode.attrH265Cbr.minIprop = 1;
                    rc_attr->attrRcMode.attrH265Cbr.maxIPictureSize = rc_attr->attrRcMode.attrH265Cbr.outBitRate * 6 / 5;
                    rc_attr->attrRcMode.attrH265Cbr.maxPPictureSize = rc_attr->attrRcMode.attrH265Cbr.maxIPictureSize * 3 / 5;
#endif
                } else {
                    LOG_ERROR("invalid encode type!\n");
                    return ST_ERRNO_FAIL;
                }
#if 0
                if (_VencChnConfig[0].is_enable == true) {
                    ret = IMP_Encoder_CreateChn(chn_id, &_VencChnConfig[0].chn_attr);
                    MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_CreateChn failed with %#x\n", group_id, chn_id, ret);
                    LOG_DEBUG("venc group %d,chn %d enc type %s %d*%d IMP_Encoder_CreateChn success!\n", group_id, chn_id, enc_type.c_str(),
                              _VencChnConfig[0].chn_attr.encAttr.picWidth, _VencChnConfig[0].chn_attr.encAttr.picHeight);
                    ret = IMP_Encoder_RegisterChn(group_id, chn_id);
                    MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_RegisterChn failed with %#x\n", group_id, chn_id, ret);
                    LOG_DEBUG("venc group %d,chn %d enc type %s %d*%d IMP_Encoder_RegisterChn success!\n", group_id, chn_id, enc_type.c_str(),
                              _VencChnConfig[0].chn_attr.encAttr.picWidth, _VencChnConfig[0].chn_attr.encAttr.picHeight);
                }
#endif
            }
        }
        SetIMPCell();
        LOG_DEBUG("venc group %d,type Init success!\n", group_id);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: 视频编码启动
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::Start(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable == true) {
        for (int i = 0; i < MAX_ENC_CHN; i++) {
            ret = IMP_Encoder_StartRecvPic(_VencChnConfig[i].chn_id);
            MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_StartRecvPic failed with %#x\n", group_id, _VencChnConfig[i].chn_id, ret);
            // _VencChnConfig[i].sp_video_thread.first = true;
            // _VencChnConfig[i].sp_video_thread.second = std::make_shared<std::thread>(&VencChnConfig::VideoProcessStream, &_VencChnConfig[i], this);
        }
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReInit
 * @brief: 视频编码重启
 * @param {uint16_t} width 宽度
 * @param {uint16_t} height 高度
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::ReInit(const RestartAttr& param)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_Encoder_CreateGroup(group_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d IMP_Encoder_CreateGroup failed with %#x\n", group_id, ret);

    _VencChnConfig[0].chn_attr.encAttr.picWidth = param.width;
    _VencChnConfig[0].chn_attr.encAttr.picHeight = param.height;

    if (_VencChnConfig[0].is_enable == true) {
        ret = IMP_Encoder_CreateChn(_VencChnConfig[0].chn_id, &_VencChnConfig[0].chn_attr);
        MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_CreateChn failed with %#x\n", group_id, _VencChnConfig[0].chn_id, ret);
        ret = IMP_Encoder_RegisterChn(group_id, _VencChnConfig[0].chn_id);
        MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_RegisterChn failed with %#x\n", group_id, _VencChnConfig[0].chn_id, ret);

        IMPEncoderCropCfg cropCfg = {
            .enable = param.crop_enable, .x = param.crop_left, .y = param.crop_top, .w = param.crop_width, .h = param.crop_height
        };
        ret = IMP_Encoder_SetChnCrop(group_id, &cropCfg);
        MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_SetChnCrop failed with %#x\n", group_id, _VencChnConfig[0].chn_id, ret);
    }
    return ST_ERRNO_SUCCESS;
}

static int ConfigureEncoderByStandard(IMPEncoderAttr* enc_attr, IMPEncoderRcAttr* rc_attr, int codec_type) {
    if (enc_attr == NULL || rc_attr == NULL) {
        return ST_ERRNO_FAIL;
    }
    
    // 1. 设置编码类型和profile
    if (codec_type == PT_H264) {
        enc_attr->enType = PT_H264;
        enc_attr->profile = 1;
    } else if (codec_type == PT_H265) {
        enc_attr->enType = PT_H265;
        enc_attr->profile = 1;
    } else {
        LOG_ERROR("non support codec type %d\n", codec_type);
        return ST_ERRNO_FAIL;
    }
    
    // 2. 根据分辨率计算bufSize（H264和H265逻辑相同）
    if (enc_attr->picHeight >= 1920) {
        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3;
    } else if (enc_attr->picHeight >= 1520) {
        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
    } else if (enc_attr->picHeight >= 1080) {
        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
    } else {
        enc_attr->bufSize = enc_attr->picWidth * enc_attr->picHeight * 3 / 2;
    }
    
    // 3. 设置HSkip参数（H264和H265相同）
    rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N1X;
    rc_attr->attrHSkip.hSkipAttr.m = 3;
    rc_attr->attrHSkip.hSkipAttr.n = 4;
    rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 0;
    rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
    rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
    rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N1X;
    
    // 4. 设置帧率
    rc_attr->outFrmRate.frmRateNum = SENSOR_ENC_FRAME_RATE_NUM;
    rc_attr->outFrmRate.frmRateDen = 1;
    
    // 5. 设置码率控制模式为CBR
#if defined(KOKS_VIDEO_RCMODE_VBR)
    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
#else
    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
#endif
    // 6. 根据编码标准设置具体参数
    if (codec_type == PT_H264) {
        // H.264 GOP设置
        rc_attr->maxGop = rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
        
#ifdef KOKS_FEATURE_KDS
        // KDS版本的H.264参数
        rc_attr->attrRcMode.attrH264Cbr.maxQp = 51;
        rc_attr->attrRcMode.attrH264Cbr.minQp = 28;
        rc_attr->attrRcMode.attrH264Cbr.blkQpEn = 0;
        rc_attr->attrRcMode.attrH264Cbr.initialQp = 35;
        rc_attr->attrRcMode.attrH264Cbr.maxIQp = 48;
        rc_attr->attrRcMode.attrH264Cbr.minIQp = 28;
        rc_attr->attrRcMode.attrH264Cbr.outBitRate = BITRATE_720P_Kbs;
        rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Cbr.IPfrmQPDelta = 8;
        rc_attr->attrRcMode.attrH264Cbr.PPfrmQPDelta = 3;
        rc_attr->attrRcMode.attrH264Cbr.staticTime = 2;
        rc_attr->attrRcMode.attrH264Cbr.flucLvl = 2;
        rc_attr->attrRcMode.attrH264Cbr.qualityLvl = 6;
        rc_attr->attrRcMode.attrH264Cbr.maxIprop = 100;
        rc_attr->attrRcMode.attrH264Cbr.minIprop = 1;
        rc_attr->attrRcMode.attrH264Cbr.maxIPictureSize = 600;
        rc_attr->attrRcMode.attrH264Cbr.maxPPictureSize = 500;
#elif defined(KOKS_VIDEO_RCMODE_VBR)
        rc_attr->attrRcMode.attrH264Vbr.maxQp = 51;
        rc_attr->attrRcMode.attrH264Vbr.minQp = 28;
        rc_attr->attrRcMode.attrH264Vbr.blkQpEn = 0;
        rc_attr->attrRcMode.attrH264Vbr.initialQp = 35;
        rc_attr->attrRcMode.attrH264Vbr.maxIQp = 48;
        rc_attr->attrRcMode.attrH264Vbr.minIQp = 28;
        rc_attr->attrRcMode.attrH264Vbr.maxBitRate = BITRATE_720P_Kbs;
        rc_attr->attrRcMode.attrH264Vbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Vbr.changePos = 75;
        rc_attr->attrRcMode.attrH264Vbr.staticTime = 2;
        rc_attr->attrRcMode.attrH264Vbr.qualityLvl = 6;
        rc_attr->attrRcMode.attrH264Vbr.maxIprop = 100;
        rc_attr->attrRcMode.attrH264Vbr.minIprop = 1;
        rc_attr->attrRcMode.attrH264Vbr.maxIPictureSize = 560;
        rc_attr->attrRcMode.attrH264Vbr.maxPPictureSize = 400;
#else // M2-4B版本
        rc_attr->attrRcMode.attrH264Cbr.maxQp = 51;
        rc_attr->attrRcMode.attrH264Cbr.minQp = 32;
        rc_attr->attrRcMode.attrH264Cbr.blkQpEn = 0;
        rc_attr->attrRcMode.attrH264Cbr.initialQp = 35;
        rc_attr->attrRcMode.attrH264Cbr.maxIQp = 51;
        rc_attr->attrRcMode.attrH264Cbr.minIQp = 32;
        rc_attr->attrRcMode.attrH264Cbr.outBitRate = BITRATE_720P_Kbs;
        rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Cbr.IPfrmQPDelta = 8;
        rc_attr->attrRcMode.attrH264Cbr.PPfrmQPDelta = 3;
        rc_attr->attrRcMode.attrH264Cbr.staticTime = 2;
        rc_attr->attrRcMode.attrH264Cbr.flucLvl = 2;
        rc_attr->attrRcMode.attrH264Cbr.qualityLvl = 6;
        rc_attr->attrRcMode.attrH264Cbr.maxIprop = 100;
        rc_attr->attrRcMode.attrH264Cbr.minIprop = 1;
        rc_attr->attrRcMode.attrH264Cbr.maxIPictureSize = 504; // 63*8
        rc_attr->attrRcMode.attrH264Cbr.maxPPictureSize = 420; // 504/1.2
#endif
    } 
    else if (codec_type == PT_H265) {
#if defined(KOKS_VIDEO_RCMODE_VBR)
        rc_attr->maxGop = 2 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
        rc_attr->attrRcMode.attrH265Vbr.maxQp = 45;
        rc_attr->attrRcMode.attrH265Vbr.minQp = 28;
        rc_attr->attrRcMode.attrH265Vbr.blkQpEn = 0;
        rc_attr->attrRcMode.attrH265Vbr.initialQp = 35;
        rc_attr->attrRcMode.attrH265Vbr.maxIQp = 45;
        rc_attr->attrRcMode.attrH265Vbr.minIQp = 28;
        rc_attr->attrRcMode.attrH265Vbr.maxBitRate = BITRATE_720P_Kbs;
        rc_attr->attrRcMode.attrH265Vbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH265Vbr.changePos = 75;
        rc_attr->attrRcMode.attrH265Vbr.staticTime = 2;
        rc_attr->attrRcMode.attrH265Vbr.qualityLvl = 6;
        rc_attr->attrRcMode.attrH265Vbr.maxIprop = 100;
        rc_attr->attrRcMode.attrH265Vbr.minIprop = 1;
        rc_attr->attrRcMode.attrH265Vbr.maxIPictureSize = 560;
        rc_attr->attrRcMode.attrH265Vbr.maxPPictureSize = 400;
#else
        rc_attr->maxGop = 1 * rc_attr->outFrmRate.frmRateNum / rc_attr->outFrmRate.frmRateDen;
        rc_attr->attrRcMode.attrH265Cbr.maxQp = 45;
        rc_attr->attrRcMode.attrH265Cbr.minQp = 29;
        rc_attr->attrRcMode.attrH265Cbr.blkQpEn = 0;
        rc_attr->attrRcMode.attrH265Cbr.initialQp = 35;
        rc_attr->attrRcMode.attrH265Cbr.maxIQp = 45;
        rc_attr->attrRcMode.attrH265Cbr.minIQp = 29;
        rc_attr->attrRcMode.attrH265Cbr.outBitRate = BITRATE_720P_Kbs;
        rc_attr->attrRcMode.attrH265Cbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH265Cbr.IPfrmQPDelta = 8;
        rc_attr->attrRcMode.attrH265Cbr.PPfrmQPDelta = 3;
        rc_attr->attrRcMode.attrH265Cbr.staticTime = 2;
        rc_attr->attrRcMode.attrH265Cbr.flucLvl = 2;
        rc_attr->attrRcMode.attrH265Cbr.qualityLvl = 6;
        rc_attr->attrRcMode.attrH265Cbr.maxIprop = 100;
        rc_attr->attrRcMode.attrH265Cbr.minIprop = 1;
        rc_attr->attrRcMode.attrH265Cbr.maxIPictureSize = rc_attr->attrRcMode.attrH265Cbr.outBitRate * 6 / 5;
        rc_attr->attrRcMode.attrH265Cbr.maxPPictureSize = rc_attr->attrRcMode.attrH265Cbr.maxIPictureSize * 3 / 5;
#endif
    }
    
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceVenc::CodecTypeReInit(const int &codec_type)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    
    ret = ConfigureEncoderByStandard(&_VencChnConfig[0].chn_attr.encAttr, &_VencChnConfig[0].chn_attr.rcAttr, codec_type);
    if(ret != ST_ERRNO_SUCCESS){
        LOG_ERROR("ConfigureEncoderByStandard failed with %d\n", ret);
        return ST_ERRNO_FAIL;
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: 视频编码停止
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::Stop(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    if (is_enable == true) {
        for (int i = 0; i < MAX_ENC_CHN; i++) {
            // _VencChnConfig[i].sp_video_thread.first = false;
            // _VencChnConfig[i].sp_video_thread.second->join();
            // _VencChnConfig[i].sp_video_thread.second.reset();
            ret = IMP_Encoder_StopRecvPic(_VencChnConfig[i].chn_id);
            MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d chn %d IMP_Encoder_StopRecvPic failed with %#x\n", group_id, _VencChnConfig[i].chn_id, ret);
        }
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: 视频编码反初始化
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::DeInit(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPEncoderCHNStat chn_stat;
    if (is_enable == true) {
        for (int i = 0; i < MAX_ENC_CHN; i++) {
            if (_VencChnConfig[i].is_enable == true) {
                auto chn = _VencChnConfig[i].chn_id;
                memset(&chn_stat, 0, sizeof(IMPEncoderCHNStat));
                ret = IMP_Encoder_Query(chn, &chn_stat);
                MISC_CHK_STATUS_REPORT_NO_RET(ret, "IMP_Encoder_Query group %d chn %d fail with %d\n", group_id, chn, ret);
                ret = IMP_Encoder_UnRegisterChn(chn);
                MISC_CHK_STATUS_REPORT_NO_RET(ret, "IMP_Encoder_UnRegisterChn group %d chn %d fail with %d\n", group_id, chn, ret);
                ret = IMP_Encoder_DestroyChn(chn);
                MISC_CHK_STATUS_REPORT_NO_RET(ret, "IMP_Encoder_DestroyChn group %d chn %d fail with %d\n", group_id, chn, ret);
            }
        }
        ret = IMP_Encoder_DestroyGroup(group_id);
        MISC_CHK_STATUS_REPORT(ret, ret, "venc group %d IMP_Encoder_DestroyGroup failed with %#x\n", group_id, ret);
        LOG_DEBUG("enc group %d Deinit success!\n", group_id);
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetIMPCell
 * @brief: 设置IMPCell
 * @return {*}
 */
void DeviceVenc::SetIMPCell(void)
{
    SelfCell.deviceID = DEV_ID_ENC;
    SelfCell.groupID = group_id;
    SelfCell.outputID = 0;
}

/**
 * @name: GetIMPCell
 * @brief: 获取IMPCell
 * @return {*} IMPCell* IMPCell指针
 */
IMPCell* DeviceVenc::GetIMPCell(void)
{
    return &SelfCell;
}

/**
 * @name: GetFrame
 * @brief: 获取视频帧
 * @param {int} chn 通道号
 * @param {IMPEncoderStream&} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::GetFrame(IMPEncoderStream& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    uint32_t timeout = 1000;
    if (_VencChnConfig[0].type == PT_JPEG) {
        timeout = 10 * 1000;
    } else if (_VencChnConfig[0].type == PT_H264) {
        timeout = 1000;
    }
    auto chn = _VencChnConfig[0].chn_id;
    ret = IMP_Encoder_PollingStream(chn, 1000);
    MISC_CHK_STATUS_REPORT(ret, ret, "group %d chn %d IMP_Encoder_PollingStream failed with %#x\n", group_id, chn, ret);
    ret = IMP_Encoder_GetStream(chn, &frame, 1);
    MISC_CHK_STATUS_REPORT(ret, ret, "group %d chn %d IMP_Encoder_GetStream failed with %#x\n", group_id, chn, ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: ReleaseFrame
 * @brief: 释放视频帧
 * @param {int} chn 通道号
 * @param {IMPEncoderStream&} frame 视频帧
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::ReleaseFrame(IMPEncoderStream& frame)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto chn = _VencChnConfig[0].chn_id;
    ret = IMP_Encoder_ReleaseStream(chn, &frame);
    MISC_CHK_STATUS_REPORT(ret, ret, "group %d chn %d IMP_Encoder_ReleaseStream failed with %#x\n", group_id, chn, ret);
    return ST_ERRNO_SUCCESS;
}

int DeviceVenc::GetAndCopyFrame(char *stream_data, bool& is_key_frame)
{
    IMPEncoderStream frame;
    memset(&frame, 0, sizeof(frame));
    auto ret = GetFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("GetFrame failed\n");
        return -1;
    }

    int copy_length = 0;
    for (int i = 0; i < frame.packCount; i++) {
        memcpy(stream_data + copy_length, (void*)frame.pack[i].virAddr, frame.pack[i].length);
        copy_length += frame.pack[i].length;
    }
    is_key_frame = (frame.refType == IMP_Encoder_FSTYPE_IDR);
    ret = ReleaseFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("ReleaseFrame failed\n");
        return -1;
    }
    return copy_length;
}

int DeviceVenc::RemoveChromaDQTfromMjpeg(char* stream_date, IMPEncoderStream* stream)
{
    int i = 0;
    int ret = 0;
    int nr_pack = stream->packCount;
    unsigned char* data;
    int len;
    int pos;
    int dqt_len;
    int table_id;
    int chroma_dqt_removed = 0;
    int write_start;
    int index = 0;
    for (i = 0; i < nr_pack; i++) {
        data = (unsigned char*)stream->pack[i].virAddr;
        len = stream->pack[i].length;
        pos = 0;
        write_start = 0;
        chroma_dqt_removed = 0;
        while (pos < len - 1) {
            if (data[pos] == 0xFF && data[pos + 1] == 0xDB) {
                if (pos + 4 >= len) {
                    pos++;
                    continue;
                }
                dqt_len = (data[pos + 2] << 8) | data[pos + 3];
                table_id = data[pos + 4] & 0x0F;
                if (table_id == 1) {
                    if (chroma_dqt_removed) {
                        memcpy(stream_date + index, data + write_start, pos - write_start);
                        index += (pos - write_start);
                        pos += dqt_len + 2;
                        write_start = pos;
                        continue;
                    }
                    chroma_dqt_removed = 1;
                }
            }
            pos++;
        }
        if (write_start < len) {
            memcpy(stream_date + index, data + write_start, len - write_start);
            index += (len - write_start);
        }
    }
    return index;
}


int DeviceVenc::GetAndCopyFrameWithoutChromaDQT(char *stream_data)
{
    IMPEncoderStream frame;
    memset(&frame, 0, sizeof(frame));
    auto ret = GetFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("GetFrame failed\n");
        return -1;
    }
    int copy_length = RemoveChromaDQTfromMjpeg(stream_data,&frame);
    ret = ReleaseFrame(frame);
    if (ret != ST_ERRNO_SUCCESS) {
        LOG_ERROR("ReleaseFrame failed\n");
        return -1;
    }
    return copy_length;
}








/**
 * @name: SaveJpeg
 * @brief: 保存Jpeg图
 * @param {string&} path 保存路径
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::SaveJpeg(const std::string& path)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    int32_t chn = GetJpegChn();
    MISC_CHK_CONDITION_REPORT((chn >= 0), ST_ERRNO_FAIL, "GetJpegChn should >=0");

    RecvPicGuard reciver(chn);
    MISC_CHK_CONDITION_REPORT(reciver.IsSuccess() == true, ST_ERRNO_FAIL, "RecvPicGuard failed!\n");
    SpEncodeVideoFrameInfo sp_video_frame_info(new IMPEncoderStream, [=, &ret](IMPEncoderStream* stream) -> ST_ERR {
        ST_ERR inner_ret = ST_ERRNO_SUCCESS;
        if (stream == nullptr) {
            return ST_ERRNO_FAIL;
        }
        if (ret != ST_ERRNO_SUCCESS) {
            delete stream;
            return ST_ERRNO_FAIL;
        }
        inner_ret = ReleaseFrame(*stream);
        delete stream;
        MISC_CHK_STATUS_REPORT(inner_ret, inner_ret, "venc group %d, chn %d ReleaseFrame failed with %#x\n", group_id, chn, inner_ret);
        LOG_VERBOSE("venc group %d, chn %d ReleaseFrame frame success!\n", group_id, chn);
    });
    ret = GetFrame(*sp_video_frame_info);
    MISC_CHK_STATUS_REPORT(ret, ret, "group %d chn %d GetFrame failed with %#x\n", group_id, chn, ret);

    FileGuard file = FileGuard(path);
    if (file.IsOpened()) {
        ret = SaveStream(file.GetFd(), sp_video_frame_info.get());
        MISC_CHK_STATUS_REPORT(ret, ret, "SaveStream %s failed %d\n", path.c_str(), ret);
    }
    LOG_VERBOSE("SaveJpeg %s success!\n", path.c_str());
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetFrameProcCb
 * @brief: 设置视频帧处理回调
 * @param {int} chn 通道号
 * @param {VnecFrameProcCallback} Cb 回调函数
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceVenc::SetFrameProcCb(int32_t chn, VnecFrameProcCallback Cb)
{
    for (int i = 0; i < MAX_ENC_CHN; i++) {
        if (_VencChnConfig[i].chn_id == chn) {
            _VencChnConfig[i].frame_proc_cb = Cb;
            return ST_ERRNO_SUCCESS;
        }
        return ST_ERRNO_FAIL;
    }
    return ST_ERRNO_SUCCESS;
}

ST_ERR DeviceVenc::SetFrameIDR(void)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_Encoder_RequestIDR(_VencChnConfig[0].chn_id);
    MISC_CHK_STATUS_REPORT(ret, ret, "group %d chn %d IMP_Encoder_RequestIDR failed with %#x\n", group_id, _VencChnConfig[0].chn_id, ret);
    LOG_DEBUG("group %d chn %d IMP_Encoder_RequestIDR success!\n", group_id, _VencChnConfig[0].chn_id);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeviceOSD
 * @brief:
 * @param {int32_t} dev_id
 * @return {*}
 */
DeviceOSD::DeviceOSD(int32_t dev_id) : Device(dev_id)
{
    group = dev_id;
}

/**
 * @name: ~DeviceOSD
 * @brief:
 * @return {*}
 */
DeviceOSD::~DeviceOSD()
{
}

static std::mutex osd_mutex{};
static std::condition_variable osd_cv{};
/**
 * @name: Init
 * @brief: OSD初始化
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceOSD::Init()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPOSDRgnCreateStat stStatus;
    IMPOSDRgnRegisterStat stRigStatus;
    ret = IMP_OSD_CreateGroup(group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_CreateGroup failed with %#x\n", ret);
    LOG_DEBUG("IMP_OSD_Create success!\n");
    handle_ = IMP_OSD_CreateRgn(nullptr);
    if (INVHANDLE == handle_) {
        LOG_ERROR("IMP_OSD_CreateRgn failed!\n");
        return ST_ERRNO_FAIL;
    }

    memset(&stStatus, 0x0, sizeof(IMPOSDRgnCreateStat));
    ret = IMP_OSD_RgnCreate_Query(handle_, &stStatus);
    if (ret < 0) {
        LOG_ERROR("IMP_OSD_RgnCreate_Query failed with %#x\n", ret);
        return ST_ERRNO_FAIL;
    }
    ret = IMP_OSD_RegisterRgn(handle_, group, nullptr);
    if (ret < 0) {
        LOG_ERROR("IMP_OSD_RegisterRgn failed with %#x\n", ret);
        return ST_ERRNO_FAIL;
    }

    memset(&stRigStatus, 0x0, sizeof(IMPOSDRgnRegisterStat));
    ret = IMP_OSD_RgnRegister_Query(handle_, group, &stRigStatus);
    if (ret < 0) {
        LOG_ERROR("IMP_OSD_RgnRegister_Query failed with %#x\n", ret);
        return ST_ERRNO_FAIL;
    }

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    handle_cover_ = IMP_OSD_CreateRgn(NULL);
    if (INVHANDLE == handle_cover_) {
        LOG_ERROR("IMP_OSD_CreateRgn failed!\n");
        return ST_ERRNO_FAIL;
    }

	ret = IMP_OSD_RegisterRgn(handle_cover_, group, NULL);
    if (ret != 0) {
        LOG_ERROR("IMP_OSD_RegisterRgn failed with %#x\n", ret);
        return ST_ERRNO_FAIL;
    }
#endif

    SetIMPCell();
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Start
 * @brief: OSD启动
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceOSD::Start(uint32_t width, uint32_t height, uint16_t line_x, uint16_t line_y)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    IMPOSDGrpRgnAttr grAttrFont;
    memset(&rAttrFont, 0, sizeof(IMPOSDRgnAttr));
    rAttrFont.type = OSD_REG_BITMAP;
#ifdef KOKS_OSD_ON_LEFT
    rAttrFont.rect.p0.x = 30 ;
    rAttrFont.rect.p0.y = (height - OSD_LETTER_NUM * gBitmapHight) / 2;
    rAttrFont.rect.p1.x = rAttrFont.rect.p0.x + bitmapWidth - 1;
    rAttrFont.rect.p1.y = rAttrFont.rect.p0.y + OSD_LETTER_NUM * gBitmapHight - 1;
#elif defined(KOKS_OSD_ON_TOP)
    rAttrFont.rect.p0.x = (width - OSD_LETTER_NUM * bitmapWidth) / 2;
    rAttrFont.rect.p0.y = 20;
    rAttrFont.rect.p1.x =  rAttrFont.rect.p0.x + OSD_LETTER_NUM * bitmapWidth - 1;
    rAttrFont.rect.p1.y =  rAttrFont.rect.p0.y + gBitmapHight - 1;
#elif defined(KOKS_OSD_ON_TOP_LEFT)
    rAttrFont.rect.p0.x = 20;
    rAttrFont.rect.p0.y = 20;
    rAttrFont.rect.p1.x =  rAttrFont.rect.p0.x + OSD_LETTER_NUM * bitmapWidth - 1;
    rAttrFont.rect.p1.y =  rAttrFont.rect.p0.y + gBitmapHight - 1;
#endif
    rAttrFont.fmt = PIX_FMT_MONOWHITE;
    size_t bitmapDataSize = OSD_LETTER_NUM * bitmapWidth * gBitmapHight;
    rAttrFont.data.bitmapData = valloc(bitmapDataSize);
    if (rAttrFont.data.bitmapData == NULL) {
        LOG_ERROR("valloc failed!\n");
        return ST_ERRNO_FAIL;
    }
    memset(rAttrFont.data.bitmapData, 0, bitmapDataSize);
    ret = IMP_OSD_SetRgnAttr(handle_, &rAttrFont);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_SetRgnAttr failed with %#x\n", ret);
    ret = IMP_OSD_GetGrpRgnAttr(handle_, group, &grAttrFont);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_GetGrpRgnAttr failed with %#x\n", ret);
    memset(&grAttrFont, 0, sizeof(IMPOSDGrpRgnAttr));
    grAttrFont.show = 1;
    grAttrFont.gAlphaEn = 1;
    grAttrFont.fgAlhpa = 0xff;
    grAttrFont.layer = 3;
    ret = IMP_OSD_SetGrpRgnAttr(handle_, group, &grAttrFont);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_SetGrpRgnAttr failed with %#x\n", ret);

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    line_x_ = line_x;
    if (line_x > 0) {
        // 画竖线
        IMPOSDRgnAttr rAttrCover;
        memset(&rAttrCover, 0, sizeof(IMPOSDRgnAttr));
        const int line_width = 6;
        rAttrCover.type = OSD_REG_COVER;
        rAttrCover.rect.p0.x = line_x - line_width / 2;
        rAttrCover.rect.p0.y = 0;
        rAttrCover.rect.p1.x = rAttrCover.rect.p0.x + line_width -1;
        rAttrCover.rect.p1.y = rAttrCover.rect.p0.y + height -1 ;
        rAttrCover.fmt = PIX_FMT_BGRA;
        rAttrCover.data.coverData.color = OSD_IPU_BLACK;
        ret = IMP_OSD_SetRgnAttr(handle_cover_, &rAttrCover);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_SetRgnAttr failed with %#x\n", ret);
        IMPOSDGrpRgnAttr grAttrCover;
        ret = IMP_OSD_GetGrpRgnAttr(handle_cover_, group, &grAttrCover);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_GetGrpRgnAttr failed with %#x\n", ret);
        memset(&grAttrCover, 0, sizeof(IMPOSDGrpRgnAttr));
        grAttrCover.show = 1;
        grAttrCover.gAlphaEn = 0;
        grAttrCover.fgAlhpa = 0xff;
        grAttrCover.layer = 2;
        ret = IMP_OSD_SetGrpRgnAttr(handle_cover_, group, &grAttrCover);
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_SetGrpRgnAttr failed with %#x\n", ret);
    }
#endif

    ret = IMP_OSD_Start(group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_Start failed with %#x\n", ret);
    ret = IMP_OSD_ShowRgn(handle_, group, static_cast<int>(os_show::show));
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_ShowRgn failed with %#x\n", ret);

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    if (line_x > 0) {
        ret = IMP_OSD_ShowRgn(handle_cover_, group, static_cast<int>(os_show::show));
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_ShowRgn failed with %#x\n", ret);
    }
#endif

    sp_osd_thread.first = true;
    sp_osd_thread.second = std::make_shared<std::thread>(&DeviceOSD::OSDProcessStream, this);

    return ST_ERRNO_SUCCESS;
}

/**
 * @name: Stop
 * @brief: OSD停止
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceOSD::Stop()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    osd_cv.notify_all();
    sp_osd_thread.first = false;
    sp_osd_thread.second->join();
    sp_osd_thread.second.reset();
    ret = IMP_OSD_ShowRgn(handle_, group, static_cast<int>(os_show::hide));
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_ShowRgn failed with %#x\n", ret);

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    if (line_x_ > 0) {
        ret = IMP_OSD_ShowRgn(handle_cover_, group, static_cast<int>(os_show::hide));
        MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_ShowRgn failed with %#x\n", ret);
    }
#endif

    ret = IMP_OSD_Stop(group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_Stop failed with %#x\n", ret);
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: DeInit
 * @brief: OSD反初始化
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceOSD::DeInit()
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    ret = IMP_OSD_UnRegisterRgn(handle_, group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_UnRegisterRgn failed with %#x\n", ret);
    IMP_OSD_DestroyRgn(handle_);

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
    ret = IMP_OSD_UnRegisterRgn(handle_cover_, group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_UnRegisterRgn failed with %#x\n", ret);
    IMP_OSD_DestroyRgn(handle_cover_);
#endif

    ret = IMP_OSD_DestroyGroup(group);
    MISC_CHK_STATUS_REPORT(ret, ret, "IMP_OSD_DestroyGroup failed with %#x\n", ret);
    LOG_DEBUG("IMP_OSD_Destroy success!\n");
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: SetOsdText
 * @brief: 设置OSD文本
 * @param {const std::string&} text 文本内容
 * @return {*} ST_ERRNO_SUCCESS 成功，其他失败
 */
ST_ERR DeviceOSD::SetOsdText(const std::string& text)
{
    ST_ERR ret = ST_ERRNO_SUCCESS;
    void* dateData = nullptr;
    int penpos = 0;
    int penpos_t = 0;
    int fontadv = 0;
    int len = text.size();
    ret = IMP_OSD_GetRgnAttr(handle_, &rAttrFont);
    if (ret < 0) {
        LOG_ERROR("IMP_OSD_GetRgnAttr failed with %#x\n", ret);
        return ST_ERRNO_FAIL;
    }
    unsigned char* dstBase = static_cast<unsigned char*>(rAttrFont.data.bitmapData); //画框基地址
    int dstWidth = rAttrFont.rect.p1.x - rAttrFont.rect.p0.x + 1; //画框跨度
    int dstHeight = rAttrFont.rect.p1.y - rAttrFont.rect.p0.y + 1; //画框高度

    for (int i = 0; i < len; i++) {
        const bitmapinfo_t* bm = nullptr;
#if defined(KOKS_OSD_ON_TOP) || defined(KOKS_OSD_ON_TOP_LEFT)
        int index = i;
#else
        int index = len - 1 - i;
#endif
        switch (text[index]) {
        case '0' ... '9':
            bm = &gBitmap[text[index] - '0'];
            break;
        case '-':
            bm = &gBitmap[10];
            break;
        case ' ':
            bm = &gBitmap[11];
            break;
        case ':':
            bm = &gBitmap[12];
            break;
        default:
            break;
        }
        const unsigned char* src = static_cast<const unsigned char*>(bm->pdata);
        int w = bm->width;
        int h = gBitmapHight;
#if defined(KOKS_OSD_ON_TOP) || defined(KOKS_OSD_ON_TOP_LEFT)
        int start_pos_x = w * i;
        int start_pos_y = 0;
        for (int y = start_pos_y; y < start_pos_y + h; y++) {
            uint8_t * row_dst = dstBase + y * dstWidth + start_pos_x;
            memcpy((void*)row_dst, (void*)(src + (y - start_pos_y) * w), w);
        }
#else
        for (int j = 0; j < h; j++) {
            unsigned char* dstRow = dstBase + w * h * i + j * bitmapWidth;
            memcpy((void*)dstRow, (void*)(src + j * w), w);
        }
#endif
    }
    return ST_ERRNO_SUCCESS;
}

/**
 * @name: OSDProcessStream
 * @brief: OSD处理视频流
 * @return {*} void*
 */
void* DeviceOSD::OSDProcessStream(void)
{
    time_t currTime;
    ST_ERR ret = ST_ERRNO_SUCCESS;
    char DateStr[40];
    struct tm* currDate;
    while (sp_osd_thread.first == true) {
        time(&currTime);
        currDate = localtime(&currTime);
        std::unique_lock<std::mutex> lock_osd(osd_mutex);
        if(currDate->tm_year < 100) {
            osd_cv.wait_for(lock_osd, std::chrono::seconds(1));
            continue;
        }
        memset(DateStr, 0, 40);
        strftime(DateStr, 40, "%Y-%m-%d %H:%M:%S\0", currDate);
        ret = SetOsdText(DateStr);
        MISC_CHK_STATUS_REPORT_EXEC_NO_RET(ret, continue, "SetOsdText failed with %d\n", ret);
        osd_cv.wait_for(lock_osd, std::chrono::seconds(1));
    }
    return nullptr;
}
} // namespace sense_koks

} // namespace st
