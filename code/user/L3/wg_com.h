#ifndef __WG_COM_H
#define __WG_COM_H

#include "stdint.h"

#define FRAME_HEAD 0x5A
#define FRAME_TAIL 0x0D
#define DEVICE_ADDR 0x01
#define MAX_FRAME_SIZE 64

#define HOST_ADDR 0x01      // 主机地址
#define BROADCAST_ADDR 0x99 // 广播地址

void wg_com_send(uint8_t cmd, uint8_t *data, uint32_t len);

#endif
