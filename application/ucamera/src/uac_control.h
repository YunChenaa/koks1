#ifndef __MODULE_UAC_CONTROL_H__
#define __MODULE_UAC_CONTROL_H__

#include <global_config.h>
#include <usbcamera.h>

#define MODULE_UAC_CONTROL_NAME	"uac_control"

/**
 * init and deinit.
 */

int uac_control_init(void *param);
void uac_control_deinit();
#endif /* __MODULE_UAC_CONTROL_H__ */
