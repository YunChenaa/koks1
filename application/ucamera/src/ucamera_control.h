#ifndef __UVC_CONTROL_H__
#define __UVC_CONTROL_H__

#include <global_config.h>
#include <usbcamera.h>

int inlock_ucamera_stream_on(int);
int inlock_ucamera_impinited(void);
void inlock_ucamera_get_frame(uint32_t *frame_width, uint32_t *frame_height);
int inlock_ucamera_init(void *param);
void inlock_ucamera_deinit();
void *inlock_ucamera_getCtHandler(void);
void *inlock_ucamera_getPuHandler(void);
void inlock_ucamera_setCtHandler(void *ct);
void inlock_ucamera_setPuHandler(void *pu);
#endif

