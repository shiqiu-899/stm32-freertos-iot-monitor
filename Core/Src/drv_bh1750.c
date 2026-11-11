#include "FreeRTOS.h"
#include "cmsis_os.h"      // ← 加这行
#include "semphr.h"
#include "drv_bh1750.h"

// I2C 互斥量，由 CubeMX 在 freertos.c 中创建
extern osMutexId sem_i2cHandle;

void BH1750_Init(void)
{
    uint8_t cmd;

        cmd = BH1750_CMD_POWER_ON; // 开机
    HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR, &cmd, 1, 100);
    HAL_Delay(10);

        cmd = BH1750_CMD_H_RES; // 连续高分辨率模式
    HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR, &cmd, 1, 100);

        // 第一次测量要等 180ms 才准
    HAL_Delay(180);
}

uint16_t BH1750_ReadLux(void)
{
    uint8_t buf[2];
    uint16_t raw;

        // 抢 I2C 锁，最多等 200ms
    if(sem_i2cHandle != NULL) {
        if(xSemaphoreTake(sem_i2cHandle, pdMS_TO_TICKS(200)) != pdTRUE) {
            return 0; // 超时，返回 0 表示失败
        }
    }

        // 读 2 字节
    if(HAL_I2C_Master_Receive(&hi2c1, BH1750_ADDR, buf, 2, 100) != HAL_OK) {
        if(sem_i2cHandle != NULL) xSemaphoreGive(sem_i2cHandle);
        return 0;
    }

    if(sem_i2cHandle != NULL) xSemaphoreGive(sem_i2cHandle);

        // 大端格式，lux = raw / 1.2
    raw = ((uint16_t)buf[0] << 8) | buf[1];
    return (uint16_t)(raw / 1.2f);
}
