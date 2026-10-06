/*
 * Copyright (c) 2026. Sunvision.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Sunvision.
 */

#ifndef __OVD_OTA_H__
#define __OVD_OTA_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 用文件进行OTA升级。md5必须是长度为33的字符串数组。
// 返回值：0：成功；其他：失败。
int ovd_ota_run(const char* file_path, const char* md5);

#ifdef __cplusplus
}
#endif

#endif /* __OVD_OTA_H__ */