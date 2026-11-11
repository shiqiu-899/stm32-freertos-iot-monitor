#include "app_tasks.h"
#include "drv_dht22.h"
#include "drv_bh1750.h"
#include "drv_oled.h"
#include "drv_esp8266.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include "stdio.h"

// 温度/光照报警阈值
#define TEMP_THRESHOLD   30.0f   // 温度超过 30°C 就报警
#define LIGHT_THRESHOLD  50      // 光照低于 50 lux 就报警

// WiFi 和 MQTT 配置，记得改成自己的
#define WIFI_SSID       "your WIFI"
#define WIFI_PASSWORD   "aaaaaaaa"
#define MQTT_HOST       "bemfa.com"
#define MQTT_PORT       9501
#define MQTT_CLIENT_ID  "your MQTT_CLIENT_ID"
#define MQTT_TOPIC      "freertos"

// 任务1：读传感器
void vTaskSensorCollect(void const *pvParameters)
	{
     SensorData_t data;
     int16_t temp_x10;
     uint16_t humi_x10;

     // 初始化传感器
     HAL_TIM_Base_Start(&htim2);
     DHT22_Init();
     BH1750_Init();

         // dht22 偶尔抽风读不出来，先初始定义
     static float last_temp = 25.0f;
     static float last_humi = 60.0f;
		static uint16_t last_light = 0;      // ← 加这行

     for(;;) {
                 vTaskDelay(pdMS_TO_TICKS(2000));

        //读dht22
         uint8_t dht_ok = 0;
         for(int retry = 0; retry < 3; retry++) {
             vTaskSuspendAll(); //任务切换会打断，先暂停调度器
             dht_ok = (DHT22_Read_Data(&temp_x10, &humi_x10) == 0);
					 //成功dht_ok就是1
             xTaskResumeAll();
             if(dht_ok) break;
             vTaskDelay(pdMS_TO_TICKS(100)); // 歇 100ms 再试
         }
				 //读取成功后才刷新，失败就什么都不做
         if(dht_ok) {
             last_temp = temp_x10 / 10.0f;
             last_humi = humi_x10 / 10.0f;
         }

         data.temperature = last_temp;
         data.humidity    = last_humi;
				 
				 //读bh1750
        uint16_t lux = BH1750_ReadLux();
        if(lux > 0) 
				{
										last_light = lux;  // 只在读到有效值时更新

				}
        data.light = last_light; 			// 无论成功失败，都用"最后一次有效值"
        data.valid = (data.light > 0) ? 1 : 0;
        printf("[SENSOR] lx=%u valid=%d dht_ok=%d raw_t=%d raw_h=%u\r\n",
               data.light, data.valid, dht_ok, temp_x10, humi_x10);
				//这一句是给串口的
        xQueueOverwrite(queue_sensorHandle, &data);  //句柄＋地址，最新数据塞进信箱发送
     }
 }

// 任务2：刷屏幕
void vTaskDisplay(void const *pvParameters)
	{
		SensorData_t data;      // 这次从队列读到的数据
		SensorData_t last;      // 上次画到屏幕上的数据（用来比较）
		uint8_t hasLast = 0;    // 有没有"上次"（第一次运行时没有）
		char line[22];          // 拼字符串用的缓冲区
		int t_i, t_d, h_i, h_d; // 温度/湿度的整数部分和小数部分
	
    OLED_Init();
    OLED_Clear();

    for(;;) {
        if(xQueuePeek(queue_sensorHandle, &data, pdMS_TO_TICKS(100)) == pdTRUE) {

            /* 只有数据真的变了才重绘，避免无脑刷屏，脏检查 */
            if(!hasLast || 
							 data.valid != last.valid ||
               data.light != last.light ||
               data.temperature != last.temperature ||
               data.humidity != last.humidity) 
						{
								 
								 //数据拆成整数和小数，valid可用进行
                if(data.valid) {
                    // 温度
                    t_i = (int)data.temperature;
                    t_d = (int)((data.temperature - t_i) * 10);
                    if(t_d < 0) t_d = -t_d;
                    snprintf(line, sizeof(line), ":%d.%dC  ", t_i, t_d);
                    OLED_ShowCHinese(0,  0, 4);
                    OLED_ShowCHinese(16, 0, 5);
                    OLED_ShowString(40, 1, (uint8_t*)line);

                    // 湿度
                    h_i = (int)data.humidity;
                    h_d = (int)((data.humidity - h_i) * 10);
                    if(h_d < 0) h_d = -h_d;
                    snprintf(line, sizeof(line), ":%d.%d%%  ", h_i, h_d);
                    OLED_ShowCHinese(0,  2, 6);
                    OLED_ShowCHinese(16, 2, 5);
                    OLED_ShowString(40, 3, (uint8_t*)line);

                    // 光照
                    snprintf(line, sizeof(line), ":%d lx  ", data.light);
                    OLED_ShowCHinese(0,  4, 3);
                    OLED_ShowCHinese(16, 4, 5);
                    OLED_ShowString(40, 5, (uint8_t*)line);
                }
								else {
                    OLED_Clear();
                    OLED_ShowString(0, 0, (uint8_t*)"Sensor Error!   ");
                }

                last = data;
                hasLast = 1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));  //把100ms换成tick
    }
} 	
// 任务3：控制继电器和蜂鸣器
void vTaskControl(void const *pvParameters)
	{
    SensorData_t data;
    AlarmEvent_t alarm;
    uint8_t last_temp_alarm = 0;
    uint8_t last_light_alarm = 0;
	//上电先静音0.
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); 

    for(;;) {
                if(xQueuePeek(queue_sensorHandle, &data, pdMS_TO_TICKS(3000)) == pdTRUE) {
            if(data.valid) {
                                // 温度高了开风扇
                if(data.temperature > TEMP_THRESHOLD) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // 继电器吸合，风扇转
                    if(last_temp_alarm == 0) {
                        last_temp_alarm = 1;
                        alarm.type = 1;
                        alarm.status = 1;
                        alarm.timestamp = xTaskGetTickCount() / 1000;
                        xQueueSend(queue_alarmHandle, &alarm, 0);
                    }
                } else {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // 继电器断开
                    if(last_temp_alarm == 1) {
                        last_temp_alarm = 0;
                        alarm.type = 1;
                        alarm.status = 0;
                        alarm.timestamp = xTaskGetTickCount() / 1000;
                        xQueueSend(queue_alarmHandle, &alarm, 0);
                    }
                }

                                // 光线暗了报警
                if(data.light < LIGHT_THRESHOLD) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // 蜂鸣器响
                    if(last_light_alarm == 0) {
                        last_light_alarm = 1;
                        alarm.type = 3;
                        alarm.status = 1;
                        alarm.timestamp = xTaskGetTickCount() / 1000;
                        xQueueSend(queue_alarmHandle, &alarm, 0);
                    }
                } else {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // 蜂鸣器停
                    if(last_light_alarm == 1) {
                        last_light_alarm = 0;
                        alarm.type = 3;
                        alarm.status = 0;
                        alarm.timestamp = xTaskGetTickCount() / 1000;
                        xQueueSend(queue_alarmHandle, &alarm, 0);
                    }
                }
            }
        }
                // 必须加 delay，不然高优先级饿死别的任务（血的教训）
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// 任务4：WiFi 发数据
void vTaskCommunication(void const *pvParameters)
	{
    SensorData_t data;
    WiFiState_t state = WIFI_INIT;
    char mqtt_payload[128];
    uint32_t lastPubTime = 0;
    ESP8266_Init();

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        switch(state) {
            case WIFI_INIT:
                if(ESP8266_SendCommand("AT\r\n", "OK", 1000)) {
                    printf("[WiFi] Module OK\r\n");
                    state = WIFI_CONNECT;
                } else {
                    printf("[WiFi] Module No Response\r\n");
                }
                break;

            case WIFI_CONNECT:
                printf("[WiFi] Connecting to %s...\r\n", WIFI_SSID);
                if(ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
                    printf("[WiFi] Connected\r\n");
                    state = MQTT_CONNECT;
                } else {
                    printf("[WiFi] Connect Failed\r\n");
                    state = WIFI_ERROR;
                }
                break;

            case MQTT_CONNECT:
                printf("[MQTT] Connecting...\r\n");
                if(ESP8266_ConnectMQTT(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID)) {
                    printf("[MQTT] Connected\r\n");
                    state = MQTT_PUBLISH;
                } else {
                    printf("[MQTT] Connect Failed\r\n");
                    state = WIFI_ERROR;
                }
                break;

            case MQTT_PUBLISH:
                                if((xTaskGetTickCount() - lastPubTime) > pdMS_TO_TICKS(10000)) {
                                        if(xQueuePeek(queue_sensorHandle, &data, 0) == pdTRUE) {
                        if(data.valid) {
                            int t_i = (int)data.temperature;
                            int t_d = (int)((data.temperature - t_i) * 10);
                            if(t_d < 0) t_d = -t_d;
                            int h_i = (int)data.humidity;
                            int h_d = (int)((data.humidity - h_i) * 10);
                            if(h_d < 0) h_d = -h_d;

                            snprintf(mqtt_payload, sizeof(mqtt_payload),
                                "{\"温度\":%d.%d,\"湿度\":%d.%d,\"照度\":%d}",
                                t_i, t_d, h_i, h_d, data.light);

                            printf("[MQTT] Publish: %s\r\n", mqtt_payload);

                            if(ESP8266_MQTTPublish(MQTT_TOPIC, mqtt_payload)) {
                                lastPubTime = xTaskGetTickCount();
                            } else {
                                printf("[MQTT] Publish Failed, reconnect...\r\n");
                                state = WIFI_ERROR;
                            }
                        }
                    }
                }
                break;

            case WIFI_ERROR:
                printf("[WiFi] Error, retry in 5s...\r\n");
                vTaskDelay(pdMS_TO_TICKS(5000));
                state = WIFI_INIT;
                break;
        }
    }
}

// 任务5：串口打印日志
void vTaskLog(void const *pvParameters)
{
    AlarmEvent_t alarm;
    char log_buf[80];

    for(;;) {
                if(xQueueReceive(queue_alarmHandle, &alarm, portMAX_DELAY) == pdTRUE) {
        	const char* type_str = (alarm.type == 1) ? "TEMP" : "LIGHT";
            const char* status_str = (alarm.status == 1) ? "TRIGGER" : "CLEAR";

            snprintf(log_buf, sizeof(log_buf),
            		"[LOG] Time:%lus Type:%s Status:%s\r\n",
                (unsigned long)alarm.timestamp, type_str, status_str);

                        HAL_UART_Transmit(&huart1, (uint8_t*)log_buf, strlen(log_buf), 100);
        }
    }
}
