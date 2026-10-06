#ifndef __OVD_DEMO_FirmwareUpgrade_h
#define __OVD_DEMO_FirmwareUpgrade_h
#include "OVD_OpenAPI.h"


OVD_int32 OVD_DEMO_FirmwareBinUpgrade(OVD_int32 channel_id, OVD_Upgrade_cmd_e cmd, void* data, size_t size, void* user_data);
OVD_int32 OVD_DEMO_FirmwareUpgradeStatusCb(OVD_int32 channel_id, OVD_UpgradeConfirmInfo_t *info);

#endif

