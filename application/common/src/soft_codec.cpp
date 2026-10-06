/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-08 11:56:49
 * @LastEditors: 洪照
 * @LastEditTime: 2025-03-27 17:17:34
 * @FilePath: /sense-koks/application/common/src/soft_codec.cpp
 */
#include "soft_codec.h"
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#ifdef __cplusplus
extern "C" {
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
}
#endif
namespace st
{
namespace sense_koks
{


/**
 * @name: ConvertNV12ToRGB
 * @brief: 将NV12格式转换为RGB格式
 * @param {unsigned char*} nv12Buffer NV12数据缓冲区
 * @param {unsigned char**} rgbBuffer RGB数据缓冲区
 * @param {int} width 图像宽度
 * @param {int} height 图像高度
 * @return {*} 0: 成功; -1: 失败
 */
int SoftCodec::ConvertNV12ToRGB(unsigned char* nv12Buffer, unsigned char** rgbBuffer, int width, int height)
{
    int rgbSize = width * height * 3;
    *rgbBuffer = (unsigned char*)malloc(rgbSize);
    if (!(*rgbBuffer)) {
        LOG_ERROR("Failed to allocate memory for RGB buffer\n");
        return -1;
    }

    unsigned char* yPlane = nv12Buffer;
    unsigned char* uvPlane = nv12Buffer + width * height;
    unsigned char* rgb = *rgbBuffer;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int y = yPlane[j * width + i];
            int u = uvPlane[(j / 2) * width + (i & ~1)];
            int v = uvPlane[(j / 2) * width + (i & ~1) + 1];

            int c = y - 16;
            int d = u - 128;
            int e = v - 128;

            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;

            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);

            rgb[(j * width + i) * 3] = r;
            rgb[(j * width + i) * 3 + 1] = g;
            rgb[(j * width + i) * 3 + 2] = b;
        }
    }
    return 0;
}
#ifdef KOKS_FEATURE_VIDEO
/**
 * @name: EncodeRGBToJPEG
 * @brief: 编码RGB数据为JPEG文件
 * @param {unsigned char*} rgbBuffer
 * @param {int} width 图片宽度
 * @param {int} height 图片高度
 * @param {char*} jpegFilePath JPEG文件路径
 * @param {int} quality 图片质量[0,100]
 * @return {*} 0: 成功; -1: 失败
 */
int SoftCodec::EncodeRGBToJPEG(unsigned char* rgbBuffer, int width, int height, const char* jpegFilePath, int quality)
{
    if (stbi_write_jpg(jpegFilePath, width, height, 3, rgbBuffer, quality) == 0) {
        LOG_ERROR("Failed to encode RGB to JPEG\n");
        return -1;
    }
    return 0;
}

int SoftCodec::SaveNV12ToFile(const char* filePath, unsigned char* nv12Buffer, int width, int height)
{
    FILE* file = fopen(filePath, "wb");
    if (!file) {
        LOG_ERROR("Failed to open file for writing: %s\n", filePath);
        return -1;
    }

    int nv12Size = width * height * 3 / 2;
    fwrite(nv12Buffer, 1, nv12Size, file);
    fclose(file);
    return 0;
}
#endif

/**
 * @name: ConvertYUV400ToRGB
 * @brief: 将YUV400格式转换为RGB格式
 * @param {unsigned char*} yuv400Buffer YUV400数据缓冲区
 * @param {unsigned char**} rgbBuffer RGB数据缓冲区
 * @param {int} width 图像宽度
 * @param {int} height 图像高度
 * @return {*} 0: 成功; -1: 失败
 */
int SoftCodec::ConvertYUV400ToRGB(unsigned char* yuv400Buffer, unsigned char** rgbBuffer, int width, int height)
{
    int rgbSize = width * height * 3;
    *rgbBuffer = (unsigned char*)malloc(rgbSize);
    if (!(*rgbBuffer)) {
        LOG_ERROR("Failed to allocate memory for RGB buffer\n");
        return -1;
    }

    unsigned char* yPlane = yuv400Buffer;
    unsigned char* rgb = *rgbBuffer;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int y = yPlane[j * width + i];

            // YUV400 only has Y component, U and V are assumed to be 128 (neutral color)
            int c = y - 16;
            int d = 0; // U is 128, so d = 128 - 128 = 0
            int e = 0; // V is 128, so e = 128 - 128 = 0

            int r = (298 * c + 128) >> 8;
            int g = (298 * c + 128) >> 8;
            int b = (298 * c + 128) >> 8;

            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);

            rgb[(j * width + i) * 3] = r;
            rgb[(j * width + i) * 3 + 1] = g;
            rgb[(j * width + i) * 3 + 2] = b;
        }
    }

    return 0;
}

/**
 * @name: EncodeRGBToJPEG
 * @brief: 编码RGB数据为JPEG内存
 * @param {unsigned char*} rgbBuffer
 * @param {int} width 图片宽度
 * @param {int} height 图片高度
 * @param {unsigned char**} jpegBuffer JPEG数据
 * @param {unsigned long*} jpegSize JPEG数据大小
 * @param {int} quality 图片质量[0,100]
 * @return {*} 0: 成功; -1: 失败
 */
int SoftCodec::EncodeRGBToJPEG(unsigned char* rgbBuffer, int width, int height, unsigned char** jpegBuffer, unsigned long* jpegSize, int quality)
{
    std::vector<unsigned char> data;
    auto writer = [](void* context, void* data, int size) {
        auto buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
        auto dataPtr = reinterpret_cast<unsigned char*>(data);
        buffer->insert(buffer->end(), dataPtr, dataPtr + size);
    };

    if (stbi_write_jpg_to_func(writer, (void*)(&data), width, height, 3, rgbBuffer, quality) == 0) {
        LOG_ERROR("Failed to encode RGB to JPEG\n");
        return -1;
    }
    *jpegSize = data.size();
    *jpegBuffer = (unsigned char*)malloc(*jpegSize);
    memcpy(*jpegBuffer, data.data(), *jpegSize);
    return 0;
}

} // namespace sense_koks

} // namespace st