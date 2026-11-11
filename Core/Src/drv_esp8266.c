#include "drv_esp8266.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stdio.h"

// ring buffer
#define ESP_RX_BUF_SIZE 1024
static uint8_t  esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_write_idx = 0;
static uint16_t esp_rx_read_idx = 0;

// 单字节接收缓冲
static uint8_t esp_rx_byte;


// 清空接收缓冲区
static void ESP8266_ClearBuffer(void)
{
    esp_rx_read_idx = 0;
    esp_rx_write_idx = 0;
    memset((char*)esp_rx_buf, 0, ESP_RX_BUF_SIZE);
}

// 获取缓冲区有效长度
static uint16_t ESP8266_GetBufferLen(void)
{
    uint16_t wi = esp_rx_write_idx;
    uint16_t ri = esp_rx_read_idx;
    if(wi >= ri) return wi - ri;
    return ESP_RX_BUF_SIZE - (ri - wi);
}

// 在缓冲区里找指定字符串
static uint8_t ESP8266_FindString(const char* str)
{
    uint16_t len = ESP8266_GetBufferLen();
    if(len < strlen(str)) return 0;

    uint16_t str_len = (uint16_t)strlen(str);
    for(uint16_t i = 0; i <= len - str_len; i++) {
        uint8_t match = 1;
        for(uint16_t j = 0; j < str_len; j++) {
            uint16_t idx = (esp_rx_read_idx + i + j) % ESP_RX_BUF_SIZE;
            if((char)esp_rx_buf[idx] != str[j]) {
                match = 0;
                break;
            }
        }
        if(match) return 1;
    }
    return 0;
}

// 串口接收中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2) {
        esp_rx_buf[esp_rx_write_idx++] = esp_rx_byte;
        if(esp_rx_write_idx >= ESP_RX_BUF_SIZE) {
            esp_rx_write_idx = 0;
        }
                // 再次启动单字节接收中断
        HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
    }
}


void ESP8266_Init(void)
{
    ESP8266_ClearBuffer();
        // 启动第一次中断接收
    HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
}

// 发 AT 指令并等回应
uint8_t ESP8266_SendCommand(const char* cmd, const char* expect, uint32_t timeout_ms)
{
    ESP8266_ClearBuffer();
    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, (uint16_t)strlen(cmd), 100);

    uint32_t start = HAL_GetTick();
    while((HAL_GetTick() - start) < timeout_ms) {
        if(ESP8266_FindString(expect)) {
            return 1; // 成功
        }
        if(ESP8266_FindString("ERROR") || ESP8266_FindString("FAIL")) {
            return 0; // 明确失败
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // 阻塞一下，别忙等
    }
    return 0; // 超时
}

// 不清缓冲区，直接等字符串出现
static uint8_t ESP8266_WaitForString(const char* str, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while((HAL_GetTick() - start) < timeout_ms) {
        if(ESP8266_FindString(str)) {
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    return 0;
}
uint8_t ESP8266_ConnectWiFi(const char* ssid, const char* password)
{
    char cmd[64];

        // 关闭回显，少收点垃圾数据
    ESP8266_SendCommand("ATE0\r\n", "OK", 1000);

        // 设成 Station 模式
    if(!ESP8266_SendCommand("AT+CWMODE=1\r\n", "OK", 2000)) {
        return 0;
    }

        // 连接 WiFi
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    return ESP8266_SendCommand(cmd, "OK", 15000); // WiFi 连接可能要 10 秒以上
}

// 拼装 MQTT CONNECT 报文
static uint8_t ESP8266_MQTT_SendConnect(const char* client_id)
{
    uint8_t pkt[128];
    uint16_t cid_len = (uint16_t)strlen(client_id);
    uint16_t rem_len = 10 + 2 + cid_len; // 10 = 可变头固定长度

    uint16_t idx = 0;
    pkt[idx++] = 0x10;                   // CONNECT 固定报头
    pkt[idx++] = (uint8_t)rem_len;       // 剩余长度（简化：假设 < 127）
    pkt[idx++] = 0x00; pkt[idx++] = 0x04;
    pkt[idx++] = 'M'; pkt[idx++] = 'Q'; pkt[idx++] = 'T'; pkt[idx++] = 'T';
    pkt[idx++] = 0x04;                   // 协议级别 3.1.1
    pkt[idx++] = 0x02;                   // Clean Session
    pkt[idx++] = 0x00; pkt[idx++] = 0x3C; // Keep Alive = 60s
    pkt[idx++] = (uint8_t)(cid_len >> 8);
    pkt[idx++] = (uint8_t)(cid_len & 0xFF);
    memcpy(&pkt[idx], client_id, cid_len);
    idx += cid_len;

        // 发 TCP 数据长度指令
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", idx);
    if(!ESP8266_SendCommand(cmd, ">", 2000)) {
        return 0;
    }

        // 发原始二进制数据
    HAL_UART_Transmit(&huart2, pkt, idx, 1000);
    return ESP8266_WaitForString("SEND OK", 5000);
}

// 拼装 MQTT PUBLISH 报文
static uint8_t ESP8266_MQTT_SendPublish(const char* topic, const char* payload)
{
    uint8_t pkt[256];
    uint16_t tlen = (uint16_t)strlen(topic);
    uint16_t plen = (uint16_t)strlen(payload);
    uint16_t rem_len = 2 + tlen + plen;

    uint16_t idx = 0;
    pkt[idx++] = 0x30;                   // PUBLISH 固定报头 QoS0
    pkt[idx++] = (uint8_t)rem_len;       // 剩余长度（简化：假设 < 127）
    pkt[idx++] = (uint8_t)(tlen >> 8);
    pkt[idx++] = (uint8_t)(tlen & 0xFF);
    memcpy(&pkt[idx], topic, tlen);
    idx += tlen;
    memcpy(&pkt[idx], payload, plen);
    idx += plen;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", idx);
    if(!ESP8266_SendCommand(cmd, ">", 2000)) {
        return 0;
    }

    HAL_UART_Transmit(&huart2, pkt, idx, 1000);
    return ESP8266_WaitForString("SEND OK", 5000);
}


uint8_t ESP8266_ConnectMQTT(const char* host, uint16_t port, const char* client_id)
{
    char cmd[64];

        // 关闭透传模式
    ESP8266_SendCommand("AT+CIPMODE=0\r\n", "OK", 1000);

        // 建立 TCP 连接
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", host, port);
    if(!ESP8266_SendCommand(cmd, "CONNECT", 10000)) {
        return 0;
    }

        // 发 MQTT CONNECT
    if(!ESP8266_MQTT_SendConnect(client_id)) {
        return 0;
    }

        // 等 CONNACK（其实不等也行，保险起见延时 500ms）
    vTaskDelay(pdMS_TO_TICKS(500));
    return 1;
}

uint8_t ESP8266_MQTTPublish(const char* topic, const char* payload)
{
    return ESP8266_MQTT_SendPublish(topic, payload);
}
