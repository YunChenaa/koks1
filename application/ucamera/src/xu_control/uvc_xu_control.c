#include <stdio.h>
#include <usbcamera.h>
#include <string.h>
#include <linux/usb/video.h>

static struct Ucamera_Video_EU_Control eu_cs6 = {
        .type = UVC_EU_CMD_USR6,
        .data[UVC_CUR] = {
                /*0x01, 0x03, 0x01,*/
                0x01, 0x03, 0x01,
        },

        .data[UVC_MIN] = {
                0x00,
        },

        .data[UVC_MAX] = {
                0x01, 0x03, 0x03 ,
        },

        .data[UVC_DEF] = {
                0x01, 0x03, 0x01,
        },
};

static struct Ucamera_Video_EU_Control eu_cs9 = {
	.type = UVC_EU_CMD_USR9,
	.data[UVC_CUR] = {
		0x00, 0x00, 0x00,0x00,
	},

	.data[UVC_MIN] = {
		0x00, 0x00, 0x00,0x00,
	},

	.data[UVC_MAX] = {
		0x10, 0x00, 0x00,0x00,
	},

	.data[UVC_DEF] = {
		0x00, 0x00, 0x00,0x00,
	},
};

int sample_video2_eu_get(int cmd, int req, void *data)
{
	uint8_t len = 1;
	uint8_t *data_cpy = NULL;
	uint8_t index = 0;
	data_cpy = (uint8_t *)data;

	switch (cmd) {
	case UVC_EU_CMD_USR6:
		{
			switch(req){
			case UVC_GET_LEN:
				len = 0x09;
				memset(data_cpy, 0, 60);
				memcpy(data_cpy, &len, sizeof(len));
				break;
			case UVC_GET_MIN:
			case UVC_GET_MAX:
			case UVC_GET_DEF:
			case UVC_GET_CUR:
				len = 0x09;
				index = (req & 0x7f);
				memset(data_cpy, 0, 60);
				memcpy(data_cpy, eu_cs6.data[index - 1], len);
				if (req == UVC_GET_CUR) {
					for(int i = 0; i < len; i++) {
						printf("UVC2: GET_CUR CS6: data_cpy[%d] = 0x%x\n", i, data_cpy[i]);
					}
				}

				break;

			case UVC_GET_RES:
				len = 0x09;
				memset(data_cpy, 0, 60);
				break;
			}

		}
		break;
	case UVC_EU_CMD_USR9:
		{
			switch(req){
			case UVC_GET_LEN:
				len = 0x04;
				memset(data_cpy, 0, 60);
				memcpy(data_cpy, &len, sizeof(len));
				break;
			case UVC_GET_MIN:
			case UVC_GET_MAX:
			case UVC_GET_DEF:
			case UVC_GET_CUR:
				len = 0x04;
				index = (req & 0x7f);
				memset(data_cpy, 0, 60);
				memcpy(data_cpy, eu_cs9.data[index - 1], len);
				break;

			case UVC_GET_RES:
				len = 0x04;
				memset(data_cpy, 0, 60);
				break;
			}

		}
		break;
	}
	return 0;
}

int sample_video2_eu_set(int cmd, void *data, int len)
{
	/* TODO: */
	unsigned char data_cpy[9] = {0};
	memcpy(data_cpy, (char *)data, len);
	switch(cmd) {
		case UVC_EU_CMD_USR6:
			memcpy(eu_cs6.data[UVC_CUR], data_cpy, len);
			break;
		case UVC_EU_CMD_USR9:
			for(int i = 0; i < len; i++) {
				printf("UVC2: SET_CUR CS9: data_cpy[%d] = 0x%x\n", i, data_cpy[i]);
			}
			memcpy(eu_cs9.data[UVC_CUR], data_cpy, len);
			break;
	}

	return 0;
}
