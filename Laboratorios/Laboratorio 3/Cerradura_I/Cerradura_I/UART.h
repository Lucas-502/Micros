/*
 *
 * Created: 22/10/2025 15:15:20
 *  Author: lucas
 *
 */ 

#ifndef UART_H
#define UART_H

#include <stdint.h>

void uartInicio(void);
void uartChar(char c);
void uartTxt(const char* s);
void uartNum(uint16_t n);
char uartLeer(void);


void uartCRLF(void);
void uartHex(uint8_t b);

char    uartLeerNoBloq(void);




#endif