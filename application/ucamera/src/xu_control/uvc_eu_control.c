#include <stdio.h>
#include <usbcamera.h>
#include <string.h>
#include <linux/usb/video.h>

static struct Ucamera_Video_EU_Control eu_cs13 = {
        .type = UVC_EU_CMD_USR13,

	.data[UVC_INFO] = {
                0x03,
        },

        .data[UVC_CUR] = {
                /*0x01, 0x03, 0x01,*/
                0x00, 0x00,
        },

        .data[UVC_DEF] = {
                0x00, 0x00,
        },

};

int sample_video1_eu_get(int cmd, int req, void *data)
{
	/* TODO: */
	uint8_t ret = 1;
	uint8_t len = 1;
	uint8_t *data_cpy = NULL;
	uint8_t index = 0;
	data_cpy = (uint8_t *)data;

	switch(cmd){
		/*实现客户自己的get_cur的功能,下面实现的为升级功能*/
		case UVC_EU_CMD_USR13:
			/*memset(data, 2, len);*/
			{

				switch(req) {
				case UVC_GET_LEN:
					len = 0x3c;
					memset(data_cpy, 0, 60);
					memcpy(data_cpy, &len, sizeof(len));
					break;
				case UVC_GET_MIN:
				case UVC_GET_DEF:
				case UVC_GET_CUR:
					len = 0x3c;
					index = (req & 0x7f);
					memset(data_cpy, 0, 60);
					memcpy(data_cpy, eu_cs13.data[index - 1], len);
					if (req == UVC_GET_CUR) {
						for(int i = 0; i < len; i++) {
							printf("UVC2: GET_CUR CS6: data_cpy[%d] = 0x%x\n", i, data_cpy[i]);
						}
					}

					break;
				case UVC_GET_MAX:
					len = 0x3c;
					memset(data_cpy, 0xff, len);
					break;

				case UVC_GET_RES:
					memset(data_cpy, 0, 60);
					break;
				}
			}
			break;
	default:
		memcpy(data, &len, sizeof(len));
		break;
	}
	return ret;
}

int sample_video1_eu_set(int cmd, void *data, int len)
{
    /* TODO: */
    unsigned char data_cpy[len];
    int value_mode = 0;
    memcpy(data_cpy, (char *)data, len);
    switch (cmd) {
    /* ota 升级用 */
    case UVC_EU_CMD_USR13:
        if (data_cpy[0] == 0x06 && data_cpy[1] == 0x01) {
            memcpy(&value_mode, data_cpy, 4);
            IF_Ucam_Set_RecoveryMode(value_mode);
        } else {
            printf("Upgarde cmd value error! \n");
        }

        break;
    default:
        break;
    }
    return 0;
}

#ifndef WINHELLO_ENABLE
int sample_video2_eu_get(int cmd, int req, void *data)
{
	/* TODO: */
	uint8_t ret = 1;
	uint8_t len = 1;
	uint8_t *data_cpy = NULL;
	uint8_t index = 0;
	data_cpy = (uint8_t *)data;

	switch(cmd){
		/* 示例：实现客户自己的get_cur的功能,下面实现的为升级功能*/
		case UVC_EU_CMD_USR13:
			{

				switch(req) {
				case UVC_GET_LEN:
					len = 0x3c;
					memset(data_cpy, 0, 60);
					memcpy(data_cpy, &len, sizeof(len));
					break;
				case UVC_GET_MIN:
				case UVC_GET_DEF:
				case UVC_GET_CUR:
					len = 0x3c;
					index = (req & 0x7f);
					memset(data_cpy, 0, 60);
					memcpy(data_cpy, eu_cs13.data[index - 1], len);
					if (req == UVC_GET_CUR) {
						for(int i = 0; i < len; i++) {
							printf("UVC2: GET_CUR CS6: data_cpy[%d] = 0x%x\n", i, data_cpy[i]);
						}
					}

					break;
				case UVC_GET_MAX:
					len = 0x3c;
					memset(data_cpy, 0xff, len);
					break;

				case UVC_GET_RES:
					memset(data_cpy, 0, 60);
					break;
				}
			}
			break;
	default:
		memcpy(data, &len, sizeof(len));
		break;
	}
	return ret;
}

int sample_video2_eu_set(int cmd, void *data, int len)
{
	/* TODO: */
	switch(cmd) {
		/* ota 升级用 */
		case UVC_EU_CMD_USR13:
			/*TODO: */
			break;
		default:
			break;

	}
	return 0;
}
#endif
