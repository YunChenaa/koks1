#include "factory.h"
#include "st_koks_led.h"
#include "app_flash_otp.h"
#include "rota_common.h"
#include "uvc_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <sys/mman.h>

extern int audio_test(float threshold);

volatile bool g_FactoryTest = false;
static int g_uvc_switch_mode = 0;

int get_uvc_switch_mode(void) {
    return g_uvc_switch_mode;
}

void set_uvc_switch_mode(int mode) {
    g_uvc_switch_mode = mode;
}

static int g_camera_isp_mode = 0;
int get_camera_isp_mode(void) {
    return g_camera_isp_mode;
}

void set_camera_isp_mode(int mode) {
    g_camera_isp_mode = mode;
}

static cam_calibration_param_t g_cam_params;

void copy_cal_params(float *dest, float *src, size_t count)
{
    for(int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

bool process_cam_cal_params(camera_cal_params params) {
    static uint32_t mask = 0;
    if(params.type == CAL_KL) {
        copy_cal_params(g_cam_params.KL, params.param, CAM_PARAM_SIZE_K);
        mask |= params.type;
    }
    else if(params.type == CAL_KR) {
        copy_cal_params(g_cam_params.KR, params.param, CAM_PARAM_SIZE_K);
        mask |= params.type;
    }
    else if(params.type == CAL_R) {
        copy_cal_params(g_cam_params.R, params.param, CAM_PARAM_SIZE_R);
        mask |= params.type;
    }
    else if(params.type == CAL_T) {
        copy_cal_params(g_cam_params.T, params.param, CAM_PARAM_SIZE_RT);
        mask |= params.type;
    }
    else if(params.type == CAL_DL) {
        copy_cal_params(g_cam_params.dL, params.param, CAM_PARAM_SIZE_D);
        mask |= params.type;
    }
    else if(params.type == CAL_DR) {
        copy_cal_params(g_cam_params.dR, params.param, CAM_PARAM_SIZE_D);
        mask |= params.type;
    }

    if(mask == CAL_ALL) {
        mask = 0;
        return app_flash_write_cam(&g_cam_params);
    }

    return true;
}

int factory_process(factory_req_t* req, factory_ack_t* ack) {
    if (!req || !ack) {
        return -1;
    }

    int len = 3;
    bool ret = false;

    ack->mid = req->mid;
    ack->type = MSG_TYPE_ACK;
    printf("req mid = %d, type = %d\n", req->mid, req->type);
    switch (req->mid) {
    case CMD_ID_FW_VERSION:
        len += 1 + sprintf(ack->data.version, "%s", rota_get_version());//KOK1_VERSION);
        ret = true;
        break;
    case CMD_ID_SERIAL_NO:
        if (req->type == MSG_TYPE_SET) {
            ret = app_flash_write_sn((uint8_t*)req->params.sn.serialNo, req->params.sn.serialNoLen);
            len += 1;
        } else if (req->type == MSG_TYPE_GET) {
            ret = app_flash_read_sn((uint8_t*)ack->data.sn);
            if (ret) {
                len += sizeof(ack->data.sn);
            }
        }
        break;
    case CMD_ID_STATION:
        if (req->type == MSG_TYPE_SET) {
            if (req->params.station.total_num == 0 || req->params.station.total_num > FACTORY_STATION_SIZE) {
                break;
            }
            char station_buffer[FACTORY_STATION_SIZE] = { 0 };
            ret = app_flash_read_station((uint8_t*)station_buffer);
            if (station_buffer[0] != req->params.station.total_num || req->params.station.cur_station == 1) {
                memset(station_buffer, 'N', sizeof(station_buffer));
                station_buffer[0] = req->params.station.total_num;
            }
            station_buffer[req->params.station.cur_station] = req->params.station.result[0];
            ret = app_flash_write_station((uint8_t*)station_buffer, req->params.station.total_num + 1);
        } else if (req->type == MSG_TYPE_GET) {
            ret = app_flash_read_station((uint8_t*)ack->data.station);
            if (ret) {
                int n = ack->data.station[0];
                ack->data.station[1 + n] = 0;
                len += 2 + n;
            }
        }
        break;
    case CMD_ID_UVC_SWITCH:
        g_uvc_switch_mode = req->params.camera.sensor_type;
        ret = true;
        break;
    case CMD_ID_IRLED_SWITCH:
        st::sense_koks::LedProxy::SetAiMode(req->params.irled.switch_state == 1);
        ret = true;
        break;
    case CMD_ID_CPUID:
        if (req->type == MSG_TYPE_GET) {
            uint32_t cid[3] = {0};
            ret = app_flash_get_chipid(cid) == 0;
            if (ret) {
                len += 1 + sprintf(ack->data.chipid, "%08x%08x%08x", cid[0], cid[1], cid[2]);
            }
        }
        break;
    case CMD_ID_LICENSE:
        if (req->type == MSG_TYPE_SET) {
            printf("write license\n");
            ret = req->params.license.licenseLen < sizeof(req->params.license.license)
                && app_flash_write_licence((uint8_t*)req->params.license.license, req->params.license.licenseLen);
        } else if (req->type == MSG_TYPE_GET) {
            ret = app_flash_read_licence((uint8_t*)ack->data.license);
            if (ret) {
                len += 1 + strlen(ack->data.license);
            }
        }
        break;
    case CMD_ID_CAMERA_ISP_MODE:
        if (req->type == MSG_TYPE_SET) {
            if (req->params.camera_isp_mode.mode < 0 || req->params.camera_isp_mode.mode > 2) {
                break;
            }
            // Set camera ISP mode here
            set_camera_isp_mode(req->params.camera_isp_mode.mode);
            ret = true;
        } else if (req->type == MSG_TYPE_GET) {
            ack->data.isp_mode = get_camera_isp_mode();
            len += ack->data.isp_mode;
            ret = true;
        }
        break;
    case CMD_ID_CAMERA_CALIB_PARAM:
        if (req->type == MSG_TYPE_SET) {
            // 参数已经在 app_rxdata_parse 中写入全局 g_cam_params
            ret = process_cam_cal_params(req->params.cal_params);
        } else if (req->type == MSG_TYPE_GET) {
            ret = app_flash_read_cam((p_cam_calibration_param_t)ack->data.calib_val);
            if (ret) len += sizeof(ack->data.calib_val);
        }
        break;
    case CMD_ID_AUDIO_TEST:
        if (req->type == MSG_TYPE_SET) {
            ret = audio_test(req->params.audio_test.threshold / 1000) == 0;
        }
        break;
#ifndef KOKS_NO_UVC
    case CMD_ID_UVC_STREAM:
        if (req->type == MSG_TYPE_GET) {
            len += sizeof(ack->data.resolutions);
            memcpy(ack->data.resolutions, resolutions, sizeof(resolutions));
            ret = true;
        }
        break;
#endif
    default:
        ack->err_code = 2;
        return len;
    }
    ack->err_code = !ret;
    return len;
}
