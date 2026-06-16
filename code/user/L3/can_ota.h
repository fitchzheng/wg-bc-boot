#ifndef __CAN_OTA_H__
#define __CAN_OTA_H__

#include <stdint.h>

#define CAN_OTA_BLOCK_SIZE              224U
#define CAN_OTA_FRAMES_PER_BLOCK        28U

void can_ota_task(void);
void can_ota_reset_session(void);
void can_ota_prepare_boot_recovery(void);

#endif
