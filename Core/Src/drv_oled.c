#include "drv_oled.h"
#include "cmsis_os.h"      // ← 加这行
#include "oledfont.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define OLED_CMD  0
#define OLED_DATA 1

// I2C 互斥量句柄
extern osMutexId sem_i2cHandle;
static void OLED_I2C_Send(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, buf, 2, 100);
}

static void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
        // 操作 I2C 前先加锁，防止和 BH1750 抢
    if(sem_i2cHandle != NULL) {
        xSemaphoreTake(sem_i2cHandle, portMAX_DELAY);
    }

    if(cmd == OLED_CMD)
        OLED_I2C_Send(0x00, dat);
    else
        OLED_I2C_Send(0x40, dat);

    if(sem_i2cHandle != NULL) {
        xSemaphoreGive(sem_i2cHandle);
    }
}

void OLED_Init(void)
{
    HAL_Delay(100);
    OLED_WR_Byte(0xAE, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xB0, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(0xFF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);
    OLED_WR_Byte(0xA6, OLED_CMD);
    OLED_WR_Byte(0xA8, OLED_CMD);
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_WR_Byte(0xD3, OLED_CMD);
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);
    OLED_WR_Byte(0xF0, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);
    OLED_WR_Byte(0x22, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
    OLED_Clear();
}

void OLED_Clear(void)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xB0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for(n = 0; n < 128; n++)
            OLED_WR_Byte(0x00, OLED_DATA);
    }
}

static void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_WR_Byte(0xB0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xF0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte(x & 0x0F, OLED_CMD);
}

void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t t;
    OLED_SetPos(x, y);
    for(t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[no][t], OLED_DATA);
    OLED_SetPos(x, y + 1);
    for(t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[no][t + 16], OLED_DATA);
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr)
{
    uint8_t j = 0;
    while(chr[j] != '\0')
    {
        OLED_SetPos(x, y);
        for(uint8_t i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[chr[j] - 32][i], OLED_DATA);
        x += 6;
        if(x > 120) { x = 0; y++; }
        j++;
    }
}
