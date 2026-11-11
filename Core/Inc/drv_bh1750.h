#ifndef __DRV_BH1750_H
#define __DRV_BH1750_H

#include "main.h"
#include "i2c.h"

/* GY-302 模块就是 BH1750 传感器
 * 地址由 ADDR 引脚决定：
 *   ADDR 接 GND -> 0x23
 *   ADDR 接 VCC -> 0x5C
 * 默认 0x23（你的模块如果没焊接 ADDR 上拉电阻，就是 0x23）
 */
#define BH1750_ADDR         (0x23 << 1)   // HAL 库需要左移 1 位
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_H_RES    0x10          // 连续高分辨率模式，1 lux 精度，120ms

void BH1750_Init(void);
uint16_t BH1750_ReadLux(void);

#endif
