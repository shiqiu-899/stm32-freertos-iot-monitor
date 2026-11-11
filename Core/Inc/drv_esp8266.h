#ifndef __DRV_ESP8266_H
#define __DRV_ESP8266_H

#include "main.h"
#include "usart.h"

/* ESP-01S 模块驱动（基于标准 AT 固件）
 * 通过 USART2 (PA2/PA3) 通信，波特率 115200
 */

void ESP8266_Init(void);
uint8_t ESP8266_SendCommand(const char* cmd, const char* expect, uint32_t timeout_ms);
uint8_t ESP8266_ConnectWiFi(const char* ssid, const char* password);
uint8_t ESP8266_ConnectMQTT(const char* host, uint16_t port, const char* client_id);
uint8_t ESP8266_MQTTPublish(const char* topic, const char* payload);

#endif
