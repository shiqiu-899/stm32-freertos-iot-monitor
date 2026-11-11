#include "dwt_delay.h"

#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)
#define DWT_CONTROL (*(volatile uint32_t *)0xE0001000)
#define SCB_DEMCR (*(volatile uint32_t *)0xE000EDFC)

void DWT_Init(void){
	SCB_DEMCR |=0x01000000;
	DWT_CONTROL |=1;
}
void delay_us(uint32_t us){
	uint32_t cycles=us*(SystemCoreClock/1000000);
	uint32_t start=DWT_CYCCNT;
	while((DWT_CYCCNT-start)<cycles);
}
