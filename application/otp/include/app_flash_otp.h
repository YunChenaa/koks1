#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CAM_PARAM_SIZE_K 9
#define CAM_PARAM_SIZE_R 9
#define CAM_PARAM_SIZE_RT 3
#define CAM_PARAM_SIZE_D 5

#pragma pack(push, 1)
typedef struct _cam_calibration_params
{
    float KL[CAM_PARAM_SIZE_K];
    float KR[CAM_PARAM_SIZE_K];
    float R[CAM_PARAM_SIZE_R];
    float T[CAM_PARAM_SIZE_RT];
    float dL[CAM_PARAM_SIZE_D];
    float dR[CAM_PARAM_SIZE_D];
}cam_calibration_param_t, *p_cam_calibration_param_t;
#pragma pack(pop)

extern const int FACTORY_STATION_SIZE;

#ifdef __cplusplus
extern "C" {
#endif

bool app_flash_write_sn(uint8_t *sn, uint32_t len);
bool app_flash_read_sn(uint8_t *sn);
bool app_flash_write_licence(uint8_t *licence, uint32_t len);
bool app_flash_read_licence(uint8_t *licence);

bool app_flash_write_station(uint8_t *station, uint32_t len);
bool app_flash_read_station(uint8_t *station);

bool app_flash_write_cam(p_cam_calibration_param_t cam);
bool app_flash_read_cam(p_cam_calibration_param_t cam);

int app_flash_get_chipid(uint32_t chipid[3]);

bool app_flash_enter_factory_mode(void);
bool app_flash_exit_factory_mode(void);

#ifdef __cplusplus
}
#endif
