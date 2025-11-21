#ifndef TWI_LCD_H_
#define TWI_LCD_H_

#include "twi.h"
#include <avr/io.h>
#include <util/delay.h>

#define PCF8574_ADDR  0x27   // si alguna vez deja de verse, probá 0x3F
#define LCD_BACKLIGHT 0x08

// lcd_control: RS (bit0), RW(bit1=0), EN(bit2), BACKLIGHT(bit3)
static uint8_t lcd_control = LCD_BACKLIGHT;

// Enviar un byte crudo al PCF8574
static void PCF8574_write(uint8_t data)
{
	twi_start();
	twi_write_address((PCF8574_ADDR << 1) | TWI_WRITE); // SLA+W
	twi_write_data(data);
	twi_stop();
}

// Pulso de enable sobre los 4 bits altos
static void lcd_pulse_enable(uint8_t data)
{
	PCF8574_write(data | 0x04); // EN = 1
	_delay_us(1);
	PCF8574_write(data & ~0x04); // EN = 0
	_delay_us(50);
}

// Escribe un nibble (4 bits altos) al LCD
static void lcd_write_4bits(uint8_t nibble)
{
	nibble |= lcd_control;      // agrega RS + backlight
	PCF8574_write(nibble);
	lcd_pulse_enable(nibble);
}

// Comando al LCD (RS=0)
static void twi_lcd_cmd(uint8_t cmd)
{
	lcd_control &= ~0x01; // RS = 0
	lcd_write_4bits(cmd & 0xF0);
	lcd_write_4bits((cmd << 4) & 0xF0);

	if (cmd == 0x01 || cmd == 0x02)
	_delay_ms(2); // clear/home tardan más
}

// Dato al LCD (RS=1)
static void twi_lcd_data(uint8_t data)
{
	lcd_control |= 0x01; // RS = 1
	lcd_write_4bits(data & 0xF0);
	lcd_write_4bits((data << 4) & 0xF0);
}

// Clear completo (lo usamos solo al inicio, no en cada refresco)
static void twi_lcd_clear(void)
{
	twi_lcd_cmd(0x01);
}

static void lcd_print_line(uint8_t line, const char *txt)
{
	uint8_t addr = (line == 0) ? 0x80 : 0xC0;  // 1ª o 2ª línea
	twi_lcd_cmd(addr);

	// Calculamos longitud real del texto (máx 16)
	uint8_t len = 0;
	if (txt) {
		while (len < 16 && txt[len] != '\0') {
			len++;
		}
	}

	for (uint8_t i = 0; i < 16; i++)
	{
		char c = ' ';
		if (txt && i < len)      // solo usamos txt[i] si está dentro del texto
		c = txt[i];
		twi_lcd_data(c);
	}
}

// Función cómoda para escribir las 2 líneas
static void lcd_2lineas(const char* l1, const char* l2)
{
	lcd_print_line(0, l1);
	lcd_print_line(1, l2);
}

// Inicialización del LCD en modo 4 bits
static void twi_lcd_init(void)
{
	_delay_ms(50);

	// Secuencia de inicio en 8 bits (tres veces 0x30) y luego 4 bits
	lcd_write_4bits(0x30);
	_delay_ms(5);

	lcd_write_4bits(0x30);
	_delay_us(150);

	lcd_write_4bits(0x30);
	_delay_us(150);

	lcd_write_4bits(0x20); // 4-bit mode

	twi_lcd_cmd(0x28); // 4 bits, 2 líneas, 5x8
	twi_lcd_cmd(0x0C); // display ON, cursor OFF
	twi_lcd_clear();
	twi_lcd_cmd(0x06); // entry mode
	twi_lcd_cmd(0x80); // situarse en primera línea
}

#endif
