/*
 * DHT11.h
 *
 * Created: 11/11/2025 15:50:40
 *  Author: lucas
 */ 


#ifndef DHT11_H_
#define DHT11_H_

#include <avr/io.h>
#include <stdint.h>

// Cambiá este define según el pin que uses
#define DHT11_DDR  DDRD
#define DHT11_PORT PORTD
#define DHT11_PIN  PIND
#define DHT11_INPUT_PIN PD2  // Por defecto: pin digital 2

void dht11_init(void);
uint8_t dht11_read(uint8_t *temperature, uint8_t *humidity);

#endif
