#ifndef __DRV_OLED_H
#define __DRV_OLED_H

#include "main.h"
#include "i2c.h"

#define OLED_ADDR 0x78

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr);
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no);

#endif
