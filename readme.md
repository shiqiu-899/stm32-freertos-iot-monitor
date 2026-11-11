# STM32 FreeRTOS 物联网环境监测终端

基于 STM32F103C8T6 + FreeRTOS，采集温湿度与光照，本地 OLED 显示，
超阈值自动控制风扇与蜂鸣器，并通过 ESP8266 上报数据到巴法云。

## 功能

- DHT22 采集温度、湿度
- BH1750 采集光照强度（lux）
- 0.96 寸 OLED 实时显示三项数据
- 温度 > 【30】℃ 自动开启继电器（风扇）
- 光照 < 【50】lux 蜂鸣器报警
- ESP8266 每 【10】 秒通过 MQTT 上报 JSON 到巴法云
- 断网自动重连

## 硬件

| 器件 | 型号 | 接口 / 引脚 |
|---|---|---|
| 主控 | STM32F103C8T6 | 72MHz / 64KB Flash / 20KB RAM |
| 温湿度 | DHT22 | 单总线 PA6 |
| 光照 | BH1750 | I2C1，地址 0x23 |
| 显示屏 | SSD1306 0.96" | I2C1，地址 0x78 |
| WiFi | ESP8266（ESP-12F + CH340） | USART2，PA2/PA3 |
| 继电器 | 1 路，高电平触发 | PA5 |
| 蜂鸣器 | 有源，低电平触发 | PA4 |
| 调试 | USB-TTL | USART1，PA9/PA10，115200 |

## 软件架构

【在这里放你的任务表】

| 任务 | 优先级 | 栈(字) | 周期 | 职责 |
|---|---|---|---|---|
| Control | 【3】 | 【256】 | 【500ms】 | 【阈值判断、控制继电器与蜂鸣器】 |
| Sensor | 【2】 | 【256】 | 【2s】 | 【读 DHT22/BH1750，写入队列】 |
| Comm | 【2】 | 【512】 | 【1s】 | 【WiFi/MQTT 状态机，10s 上报】 |
| Display | 【1】 | 【256】 | 【事件驱动】 | 【数据变化时刷新 OLED】 |
| Log | 【1】 | 【256】 | 【事件驱动】 | 【报警事件打印串口】 |

【说明队列和互斥量：queue_sensor 长度1覆盖写、queue_alarm 长度5、sem_i2c 保护 I2C 总线】

## 关键设计

1. **历史值兜底**：DHT22/BH1750 读失败时沿用上一次成功值，避免界面闪烁。
2. **数据变化才重绘**：Display 任务比较新旧数据，只在变化时刷屏，解决屏幕周期性闪烁。
3. **【你自己再加一条，比如状态机重连 / 边沿检测】**

## 踩坑记录

1. I2C 总线冲突导致 OLED 花屏 → 加互斥量
2. DHT22 时序被任务切换打断 → 读之前 vTaskSuspendAll()
3. 【高优先级任务饿死低优先级 → 循环末尾必须 vTaskDelay】
4. 【xQueuePeek 不消费导致 CPU 空转 → 改为变化才重绘】
5. 【MQTT 发布偶发失败 → SendCommand 会清缓冲，改用 WaitForString】

## 编译运行

1. STM32CubeMX 打开 `FreeRTOS.ioc`，生成 MDK-ARM 工程
2. Keil 打开 `MDK-ARM/FreeRTOS.uvprojx`
3. 修改 `Core/Src/app_tasks.c` 顶部的 WiFi 账号与巴法云私钥
4. 编译下载（Options → Target 勾选 Use MicroLIB）

## 已知限制

- MQTT 只上行，不支持云端下发控制
- 无 RTC，日志时间为开机秒数
- 阈值写死在代码里，修改需重新编译