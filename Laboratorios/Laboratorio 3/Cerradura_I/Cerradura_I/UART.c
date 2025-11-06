/*
 *
 * Created: 22/10/2025 15:19:28
 *  Author: lucas
 *
 */
 
#include <avr/io.h>
#include <stdint.h>
#include "UART.h"

void uartInicio(void){
	UBRR0H = 0;
	UBRR0L = 103;
	UCSR0A = 0;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void uartChar(char c){
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void uartTxt(const char* s){
	while(*s) uartChar(*s++);
}

void uartNum(uint16_t n){
	if(n==0){ uartChar('0'); return; }
	char buf[8]; uint8_t i=0;
	while(n){ buf[i++] = '0' + (n%10); n/=10; }
	while(i--) uartChar(buf[i]);
}
char uartLeer(void){
	while(!(UCSR0A & (1<<RXC0)));               // espera un byte
	return UDR0;
}

void uartCRLF(void){
	uartChar('\r'); uartChar('\n');
}

static void _hexNibble(uint8_t v){
	v &= 0x0F;
	uartChar( v<10 ? ('0'+v) : ('A'+(v-10)) );
}

void uartHex(uint8_t b){
	_hexNibble(b>>4);
	_hexNibble(b);
}


uint8_t uartHayDato(void){
	return (UCSR0A & (1<<RXC0)) ? 1 : 0;
}

char uartLeerNoBloq(void){
	return UDR0;
}
