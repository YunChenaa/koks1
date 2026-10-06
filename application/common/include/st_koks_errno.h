/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-16 17:16:12
 * @LastEditors: 洪照
 * @LastEditTime: 2024-12-16 17:16:14
 * @FilePath: /sense-koks/application/media_core/include/st_koks_errno.h
 */
#ifndef _ST_KOKS_ERRNO_H_
#define _ST_KOKS_ERRNO_H_

#include "stdint.h"

namespace st
{
namespace sense_koks
{

/// success
#define ST_ERRNO_SUCCESS 0

/// fail
#define ST_ERRNO_FAIL -1

typedef int32_t ST_ERR;

} // namespace sense_koks
} // namespace st

#endif