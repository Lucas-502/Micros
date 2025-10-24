/*
 * main.c
 *
 * Created: 10/23/2025 1:23:14 PM
 *  Author: lucas
 * Este programa es para ver si detecta alguna targeta o pin cerca
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "SPI.h"
#include "RC522.h"
#include "UART.h"

#define LED (1<<PB0)

int main(void)
{
	DDRB |= LED; 
	PORTB &= ~LED;
	
	uartInicio();

	spi_init();
	PORTB |= (1<<PB2);   //SDA 
	mfrc522_init();

	uint8_t ver = mfrc522_read(VersionReg);

	uint8_t uid[10], uid_len=0;
	uint8_t ok_streak=0, fail_streak=0;

	while(1){
		if (mfrc522_detectar(uid, &uid_len)) {
			ok_streak++; 
			fail_streak=0;
			if (ok_streak >= 3) 
			PORTB |= LED; 
			} else {
			fail_streak++; 
			if (fail_streak >= 5){ 
				PORTB &= ~LED; ok_streak=0; 
				}
		}
		_delay_ms(50);
	}
}
