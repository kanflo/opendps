#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

char  _bootcom_start[16];


void scb_reset_system(void)
{
	printf("scb_reset_system!\n");
}

uint32_t get_ticks(void)
{
	return 0;
}

void delay_ms(uint32_t t)
{
	(void) t;
}

void usart_send_blocking(uint32_t usart, char ch)
{
	printf("TX 0x%02x ('%c')\n", ch, ch);
}
