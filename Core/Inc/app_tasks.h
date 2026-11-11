#ifndef __APP_TASKS_H
#define __APP_TASKS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"

/* ---------- 数据结构 ---------- */
typedef struct {
    float temperature;   // 温度，单位 °C
    float humidity;      // 湿度，单位 %
    uint16_t light;      // 光照，单位 lux
    uint8_t valid;       // 1=有效，0=无效
} SensorData_t;

typedef struct {
    uint8_t type;        // 1=温度高，3=光线暗
    uint8_t status;      // 1=触发，0=解除
    uint32_t timestamp;  // 开机后的秒数
} AlarmEvent_t;

/* WiFi 状态机（仅通信任务内部使用） */
typedef enum {
    WIFI_INIT,      // 初始化模块
    WIFI_CONNECT,   // 连接WiFi
    MQTT_CONNECT,   // 连接MQTT服务器
    MQTT_PUBLISH,   // 发送数据
    WIFI_ERROR      // 出错重试
} WiFiState_t;

/* ---------- 全局句柄（在 freertos.c 中创建） ---------- */
extern osMessageQId queue_sensorHandle;   // 传感器数据队列（长度 1，用于覆盖最新值）
extern osMessageQId queue_alarmHandle;    // 报警事件队列
extern osMutexId    sem_i2cHandle;        // I2C 总线互斥量

/* ---------- 任务函数声明 ---------- */
void vTaskSensorCollect(void const *pvParameters);
void vTaskDisplay      (void const *pvParameters);
void vTaskControl      (void const *pvParameters);
void vTaskCommunication(void const *pvParameters);
void vTaskLog          (void const *pvParameters);
#endif
