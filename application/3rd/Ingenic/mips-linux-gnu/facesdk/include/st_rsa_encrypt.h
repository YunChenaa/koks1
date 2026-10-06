#ifndef ST_RSA_ENCRYPT_H_
#define ST_RSA_ENCRYPT_H_

#include "st_utils.h"

/// @brief 对数据进行rsa加密
/// @param[in]  data 待加密的数据
/// @param[in]  data_len 带加密数据的长度
/// @param[in]  n 公钥中的n
/// @param[in]  nlen n的长度
/// @param[in]  e 公钥中的e
/// @param[in]  elen e的长度
ST_SDK_API
int st_face_module_rsa_encrypt(unsigned char *data, int data_len,
                               unsigned char *n, int nlen, unsigned char *e,
                               int elen);

#endif  // ST_RSA_ENCRYPT_H_