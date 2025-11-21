/*
 * main.c
 *
 * Created: 11/21/2025
 * Author: lucas
 */

// Maestro SPI: DHT11 + LCD + Pot + Botón
// Envía por SPI: servo_val y comando (bit0 para canción)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "SPI.h"
#include "twi.h"
#include "twi_lcd.h"
#include "dht11.h"

#define CMD_PLAY_SONG_BIT   0x01
#define SS_PIN              PB2

// ---------------- Prototipos ----------------
static void    adc_init(void);
static uint16_t adc_read_0(void);

static void   button_init(void);
static uint8_t button_read_logico(void);

static void spi_send_servo_and_cmd(uint8_t servo_val, uint8_t cmd);

// ---------------- Función de envío SPI ----------------

static void spi_send_servo_and_cmd(uint8_t servo_val, uint8_t cmd)
{
	PORTB &= ~(1 << SS_PIN);    // SS LOW
	_delay_us(5);               // Protección mínima

	spi_write(servo_val);       // Dato 1: posición
	spi_write(cmd);             // Dato 2: comando

	_delay_us(5);
	PORTB |= (1 << SS_PIN);     // SS HIGH
}

// ---------------- main ----------------

int main(void)
{
	uint8_t temperatura = 0;
	uint8_t humedad = 0;
	uint8_t dht_error = 0;

	uint16_t pot_raw = 0;
	uint16_t pot_filtrado = 0;
	uint8_t  servo_val = 0;

	uint8_t button = 0;
	uint8_t prev_button = 0;
	uint8_t play_flag = 0;

	uint8_t dht_counter = 0;
	uint8_t lcd_counter = 0;

	char linea1[17];
	char linea2[17];

	// --- Inicialización ---
	spi_master_init();       // SPI como maestro
	twi_master_init();       // LCD I2C
	twi_lcd_init();
	dht11_init();
	adc_init();
	button_init();

	DDRB |= (1 << SS_PIN);   // SS como salida
	PORTB |= (1 << SS_PIN);  // Inicialmente en HIGH

	lcd_2lineas("Maestro SPI", "DHT+POT+BTN");
	_delay_ms(1000);

	while (1)
	{
		// 1) Leer DHT11 cada ~1 seg
		if (dht_counter == 0)
		{
			dht_error = dht11_read(&temperatura, &humedad);
			dht_counter = 50;  // 20ms * 50 = 1000ms
		}
		else
		{
			dht_counter--;
		}

		// 2) Leer potenciómetro y filtrar
		{
			uint16_t pot_med = adc_read_0();
			pot_filtrado = (pot_filtrado * 3 + pot_med) / 4;
			pot_raw = pot_filtrado;
			if (pot_raw > 1023) pot_raw = 1023;
		}

		servo_val = (uint32_t)pot_raw * 255 / 1023;

		// 3) Leer botón
		button = button_read_logico();
		play_flag = 0;
		if (button && !prev_button)
		{
			play_flag = 1;
		}
		prev_button = button;

		// 4) Enviar por SPI: posición servo + comando
		if (play_flag)
		{
			spi_send_servo_and_cmd(servo_val, CMD_PLAY_SONG_BIT);  // Enviar con bit de canción
			_delay_ms(2);  // Pequeña pausa para que el esclavo lo procese
			spi_send_servo_and_cmd(servo_val, 0x00);               // Bajar el bit inmediatamente
		}
		else
		{
			spi_send_servo_and_cmd(servo_val, 0x00);               // Envío normal
		}
		// 5) Actualizar LCD cada ~200 ms
		if (lcd_counter == 0)
		{
			if (dht_error == 0)
				snprintf(linea1, sizeof(linea1),
				         "T:%2uC H:%2u%%", temperatura, humedad);
			else
				snprintf(linea1, sizeof(linea1),
				         "DHT ERR:%u", dht_error);

			snprintf(linea2, sizeof(linea2),
			         "P:%4u B:%s", pot_raw, (button ? "ON " : "OFF"));

			linea1[16] = '\0';
			linea2[16] = '\0';
			lcd_2lineas(linea1, linea2);

			lcd_counter = 10;
		}
		else
		{
			lcd_counter--;
		}

		_delay_ms(20);
	}

	return 0;
}

// ---------------- ADC en ADC0 ----------------

static void adc_init(void)
{
	ADMUX = (1 << REFS0);  // AVcc
	ADCSRA = (1 << ADEN) |
	         (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // prescaler 128
}

static uint16_t adc_read_0(void)
{
	ADMUX &= 0xF0;             // Canal 0
	ADCSRA |= (1 << ADSC);     // Iniciar conversión
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

// ---------------- Botón en PD3 ----------------

static void button_init(void)
{
	DDRD  &= ~(1 << PD3);  // entrada
	PORTD |=  (1 << PD3);  // pull-up
}

static uint8_t button_read_logico(void)
{
	uint8_t raw = (PIND & (1 << PD3)) ? 1 : 0;
	return (raw == 0) ? 1 : 0;
}
