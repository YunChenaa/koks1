/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-08 11:56:40
 * @LastEditors: 洪照
 * @LastEditTime: 2025-03-27 15:00:38
 * @FilePath: /sense-koks/application/common/include/soft_codec.h
 */
#pragma once
#include "sys_logger.h"
namespace st
{
namespace sense_koks
{
class SoftCodec
{
    public:
    static int ConvertYUV400ToRGB(unsigned char* yuv400Buffer, unsigned char** rgbBuffer, int width, int height);
    static int EncodeRGBToJPEG(unsigned char* rgbBuffer, int width, int height, unsigned char** jpegBuffer, unsigned long* jpegSize, int quality);
    static int ConvertNV12ToRGB(unsigned char* nv12Buffer, unsigned char** rgbBuffer, int width, int height);
#ifdef KOKS_FEATURE_VIDEO
    static int EncodeRGBToJPEG(unsigned char* rgbBuffer, int width, int height, const char* jpegFilePath, int quality);
    static int SaveNV12ToFile(const char* filePath, unsigned char* nv12Buffer, int width, int height);
#endif
};
} // namespace sense_koks

} // namespace st
