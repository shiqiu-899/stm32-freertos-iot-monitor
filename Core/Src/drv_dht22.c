#include "drv_dht22.h"
#include "dwt_delay.h"

// DHT22 驱动，从之前 OLED 工程搬过来的

static void DHT22_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void DHT22_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

void DHT22_Init(void)
{
    DHT22_IO_OUT();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    DWT_Init();          // DWT 做 us 级延时
}

static uint8_t DHT22_Read_Bit(void)
{
    uint32_t start;
    uint32_t timeout_ticks = (SystemCoreClock / 1000000) * 1000; // 1ms 超时

    start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
        if ((DWT->CYCCNT - start) > timeout_ticks) {
            return 0xFF; // 超时，返回 0xFF
        }
    }

    delay_us(40);
    uint8_t bit = HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN);

    start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if ((DWT->CYCCNT - start) > timeout_ticks) {
            return 0xFF;
        }
    }
    return bit;
}

static uint8_t DHT22_Read_Byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte <<= 1;
        uint8_t bit = DHT22_Read_Bit();
        if (bit == 0xFF) return 0xFF; // 把超时错误传上去
        if (bit == GPIO_PIN_SET)
            byte |= 0x01;
    }
    return byte;
}
//判断是否读取成功
uint8_t DHT22_Read_Data(int16_t *temp_x10, uint16_t *humi_x10)
{
    uint8_t buf[5];

    DHT22_IO_OUT();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);                          // 拉低 20ms 发启动信号
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30);

    DHT22_IO_IN();
    delay_us(40);

    if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET)
    {
        delay_us(80);
        if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET)
        {
            delay_us(80);
            for (int i = 0; i < 5; i++) {
                buf[i] = DHT22_Read_Byte();
                if (buf[i] == 0xFF) return 1; // 位读取超时
            }

            if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
            {
                uint16_t humi_raw = ((uint16_t)buf[0] << 8) | buf[1];
                int16_t  temp_raw = ((uint16_t)buf[2] << 8) | buf[3];

                if (temp_raw & 0x8000)
                    temp_raw = -(temp_raw & 0x7FFF);

                *humi_x10 = humi_raw;
                *temp_x10 = temp_raw;
                return 0; // 成功
            }
        }
    }
    return 1; // 失败
}
