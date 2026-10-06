/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-13 23:09:10
 * @LastEditors: 洪照
 * @LastEditTime: 2025-02-19 19:43:04
 * @FilePath: /sense-koks/application/communication/include/crypto.h
 */
#ifndef __SENSE_CRYPTO_H__
#define __SENSE_CRYPTO_H__
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CryptoMode { AES_ENCRYPT = 0, AES_DECRYPT } CryptoMode;

#define JZAES_IOC_MAGIC 'A'
#define IOCTL_AES_GET_PARA _IOW(JZAES_IOC_MAGIC, 110, unsigned int)
#define IOCTL_AES_PROCESSING _IOW(JZAES_IOC_MAGIC, 111, unsigned int)
#define IOCTL_AES_INIT _IOW(JZAES_IOC_MAGIC, 112, unsigned int)
#define IOCTL_AES_SG_PROCESSING _IOW(JZAES_IOC_MAGIC, 117, unsigned int)

typedef enum jz_aes_status {
    JZ_AES_STATUS_PREPARE = 0,
    JZ_AES_STATUS_WORKING,
    JZ_AES_STATUS_DONE,
} JZ_AES_STATUS;

typedef enum IN_UNF_CIPHER_WORK_MODE_E {
    IN_UNF_CIPHER_WORK_MODE_ENC_ECB = 0x0,
    IN_UNF_CIPHER_WORK_MODE_DEC_ECB = 0x1,
    IN_UNF_CIPHER_WORK_MODE_ENC_CBC = 0x2,
    IN_UNF_CIPHER_WORK_MODE_DEC_CBC = 0x3,
    IN_UNF_CIPHER_WORK_MODE_OTHER = 0x4,
} IN_UNF_CIPHER_WORK_MODE;

struct aes_para {
	unsigned int status;
	unsigned int enworkmode;
	unsigned int datalen; // it should be 16bytes aligned
	unsigned int aeskey[8];
	unsigned int aesiv[4]; // when work mode is cbc, the parameter must be setted.
	unsigned char *src;
	unsigned char *dst;
	unsigned int donelen; // The length of the processed data.
	int keybits;
};

class crypto
{
    public:
    static crypto& GetInstance();
    int Init();
    int Deinit();
    int aes_crypto(unsigned char* in, int len, unsigned char* out, char* key, int mode);

    private:
    crypto(){};
    ~crypto(){};
    crypto(const crypto&) = delete;
    crypto& operator=(const crypto&) = delete;

    private:
    int handle{ -1 };
    const std::string aes_node{ "/dev/aes" };
    struct aes_para para;
    std::mutex aes_lock;
};
// ecrypted message adapter
typedef struct {
    uint16_t header; // the message id
    uint16_t size;   // 16bit big endian size,
    uint8_t data[0];
} s_enc_msg;

extern int aes_crypto(unsigned char* in, int len, unsigned char* out, char* key, int mode);

#ifdef __cplusplus
}
#endif

#endif //__SENSE_CRYPTO_H__