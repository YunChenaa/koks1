/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-05-29 17:26:42
 * @LastEditors: 洪照
 * @LastEditTime: 2025-06-05 18:06:17
 * @FilePath: /sense-koks/application/entry/sdk_test.cpp
 */
#include "st_face.h"
#include "st_face_data.h"
#include "st_face_module.h"
#include <unistd.h>
#define MODULE_AES_KEY "Y6nkaNQkW+jAAUe="

static st_module_config_t st_module_config_640_480 = { .template_save_path = (char*)"/data",
                                                       .resize_image = ST_FACE_RESIZE_IMG_640x480,
                                                       .liveness_LOGIc_method = 2,
                                                       .load_verify = true,
                                                       .aes_enc_key = (char*)MODULE_AES_KEY,
                                                       .calc_pose = nullptr,
                                                       .face_rect = nullptr,
                                                       .algo_abort = nullptr,
                                                       .aes_crypto = nullptr };

int main(int argc, char** argv)
{
    int sdk_init_result = st_face_module_init(&st_module_config_640_480);
    if (ST_OK != sdk_init_result) {
        return -1;
    }
    while (1) {
        sleep(1);
    }
}