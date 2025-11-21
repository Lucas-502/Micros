/*
 * DHT11.c
 *
 * Created: 11/11/2025 15:50:55
 *  Author: lucas
 */ 




#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>
#include <avr/io.h>
#include "dht11.h"




static void dht11_request(void)
{
	DHT11_DDR |= (1 << DHT11_INPUT_PIN);    // Pin como salida
	DHT11_PORT &= ~(1 << DHT11_INPUT_PIN);  // LOW por 18 ms
	_delay_ms(18);

	DHT11_PORT |= (1 << DHT11_INPUT_PIN);   // Pull-up
	_delay_us(30);                          // Tiempo mínimo

	DHT11_DDR &= ~(1 << DHT11_INPUT_PIN);   // Pin como entrada
}

static uint8_t dht11_wait_response(void)
{
	uint16_t timeout = 0;

	// Espera LOW
	while (DHT11_PIN & (1 << DHT11_INPUT_PIN)) {
		if (++timeout > 200) return 1;
		_delay_us(1);
	}

	// Espera HIGH
	timeout = 0;
	while (!(DHT11_PIN & (1 << DHT11_INPUT_PIN))) {
		if (++timeout > 200) return 2;   // <--- ESTE es tu error
		_delay_us(1);
	}

	// Espera que termine respuesta
	timeout = 0;
	while (DHT11_PIN & (1 << DHT11_INPUT_PIN)) {
		if (++timeout > 200) return 3;
		_delay_us(1);
	}

	return 0;
}

static uint8_t dht11_read_bit(void)
{
	uint16_t width = 0;

	// Espera inicio del bit (LOW)
	while (!(DHT11_PIN & (1 << DHT11_INPUT_PIN)));

	// Comienza HIGH, mide cuánto dura
	while (DHT11_PIN & (1 << DHT11_INPUT_PIN)) {
		_delay_us(1);
		if (++width > 100) break;
	}

	// HIGH > 40us es un 1, si no es 0
	return (width > 40) ? 1 : 0;
}

static uint8_t dht11_read_byte(void)
{
	uint8_t i, byte = 0;

	for (i = 0; i < 8; i++) {
		byte <<= 1;
		byte |= dht11_read_bit();
	}

	return byte;
}

void dht11_init(void)
{
	DHT11_DDR |= (1 << DHT11_INPUT_PIN);
	DHT11_PORT |= (1 << DHT11_INPUT_PIN);  // Pull-up
}

uint8_t dht11_read(uint8_t *temperature, uint8_t *humidity)
{
	uint8_t hum_int, hum_dec;
	uint8_t temp_int, temp_dec;
	uint8_t checksum;

	dht11_request();
	uint8_t error = dht11_wait_response();
	if (error) return error;

	hum_int  = dht11_read_byte();
	hum_dec  = dht11_read_byte();
	temp_int = dht11_read_byte();
	temp_dec = dht11_read_byte();
	checksum = dht11_read_byte();

	if ((hum_int + hum_dec + temp_int + temp_dec) != checksum)
	return 4;  // checksum inválido

	*humidity = hum_int;
	*temperature = temp_int;

	return 0;
}
