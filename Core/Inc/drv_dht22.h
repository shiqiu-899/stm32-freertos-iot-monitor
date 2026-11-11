#ifndef __DRV_DHT22_H
#define __DRV_DHT22_H

#include "main.h"

#define DHT22_PIN   GPIO_PIN_6
#define DHT22_PORT  GPIOA

void DHT22_Init(void);
uint8_t DHT22_Read_Data(int16_t *temp_x10, uint16_t *humi_x10);

#endif
