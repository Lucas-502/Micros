/*
 * Pantalla.c
 *
 * Created: 14/11/2025 17:56:11
 *  Author: lucas
 */ 
#include "pantalla.h"
#include "twi.h"

//Fuentes de texto

//#include "Picopixel.h"
//static const GFXfont *fuente = &Picopixel;
#include "FreeSans9pt7b.h"
static const GFXfont *fuente = &FreeSans9pt7b;
 


#include <avr/pgmspace.h>

#define DIR_I2C_OLED 0x3C
#define CMD  0x00
#define DATA 0x40

#define ANCHO 128
#define ALTO 64


static uint8_t buffer[ANCHO * ALTO / 8];
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 4;
static uint8_t escala = 1;
static uint8_t interlineado = 1;
uint8_t cursor_es_inicial = 1;




static void enviar_comando(uint8_t c) {
	twi_start();
	twi_write_address((DIR_I2C_OLED << 1));
	twi_write_data(CMD);
	twi_write_data(c);
	twi_stop();
}

static void enviar_dato(uint8_t d) {
	twi_start();
	twi_write_address((DIR_I2C_OLED << 1));
	twi_write_data(DATA);
	twi_write_data(d);
	twi_stop();
}

void pantalla_iniciar(void)
{
	twi_master_init();

	enviar_comando(0xAE);            // Display OFF (pág.34)

	enviar_comando(0xD5);            // Clock div (pág.40)
	enviar_comando(0x80);

	enviar_comando(0xA8);            // Multiplex 1/64 (pág.37)
	enviar_comando(0x3F);

	enviar_comando(0xD3);            // Display offset 0 (pág.37)
	enviar_comando(0x00);

	enviar_comando(0x40);            // Start line 0 (pág.36)

	enviar_comando(0xA1);            // Segment remap (pág.36)
	enviar_comando(0xC8);            // COM scan dir invert (pág.37)

	enviar_comando(0xDA);            // COM pins config (pág.40)
	enviar_comando(0x12);

	enviar_comando(0x81);            // Contrast (pág.36)
	enviar_comando(0x7F);

	enviar_comando(0xD9);            // Pre-charge (pág.40)
	enviar_comando(0xF1);

	enviar_comando(0xDB);            // VCOMH (pág.43)
	enviar_comando(0x40);

	enviar_comando(0xA4);            // Enable display from RAM (pág.37)
	enviar_comando(0xA6);            // Normal mode (pág.37)

	enviar_comando(0x20);            // Addressing mode (pág.34)
	enviar_comando(0x00);            // Horizontal

	enviar_comando(0xAF);            // Display ON (pág.27)
}


void pantalla_limpiar(void) {
	for (uint16_t i = 0; i < sizeof(buffer); i++) {
		buffer[i] = 0x00;
	}
}

void pantalla_actualizar(void) {
	for (uint8_t pag = 0; pag < 8; pag++) {
		enviar_comando(0xB0 + pag);
		enviar_comando(0x00);
		enviar_comando(0x10);
		for (uint8_t col = 0; col < ANCHO; col++) {
			uint16_t pos = pag * ANCHO + col;
			enviar_dato(buffer[pos]);
		}
	}
}

void pixel(uint8_t x, uint8_t y, uint8_t color) {
	if (x >= ANCHO || y >= ALTO) return;

	uint16_t i = x + (y / 8) * ANCHO;
	if (color)
	buffer[i] |= (1 << (y % 8));
	else
	buffer[i] &= ~(1 << (y % 8));
}

void cursor(int16_t x, int16_t y)
{
	cursor_x = x;

	if (cursor_es_inicial)
	{
		// usar una letra con altura real, la 'A'
		uint8_t idx = 'A' - fuente->first;
		const GFXglyph *gA = &fuente->glyph[idx];

		int16_t min_y = -(gA->yOffset * escala);
		cursor_y = y + min_y;

		if (cursor_y < 0)
		cursor_y = 0;

		cursor_es_inicial = 0;   // <-- Ya no volverá a corregir
	}
	else
	{
		cursor_y = y;
	}
}

void set_interlineado(uint8_t px) {
	interlineado = px;
}

void letra(char c) {
	if (c == '\n') {
		cursor_x = 0;
		cursor_y += (fuente->yAdvance * escala) + interlineado;
		return;
	}

	if (c < fuente->first || c > fuente->last)
	c = '?';

	uint8_t i = c - fuente->first;
	const GFXglyph *g = &fuente->glyph[i];

	uint16_t bo = g->bitmapOffset;
	uint8_t  w  = g->width;
	uint8_t  h  = g->height;
	int8_t   xo = g->xOffset;
	int8_t   yo = g->yOffset;
	uint8_t  xa = g->xAdvance;
	
	int16_t ancho_real = (xo + w) * escala;
	
	// Salto automático SI NO ENTRA
	if (cursor_x + ancho_real >= ANCHO) {
		cursor_x = 0;
		cursor_y += fuente->yAdvance * escala + interlineado;
	}



	uint8_t bit = 0;
	uint8_t bits = 0;

	for (uint8_t yy = 0; yy < h; yy++) {
		for (uint8_t xx = 0; xx < w; xx++) {

			if (!(bit & 7)) {
				bits = pgm_read_byte(&fuente->bitmap[bo++]);
			}

			if (bits & 0x80) {
				for (uint8_t dx = 0; dx < escala; dx++)
				for (uint8_t dy = 0; dy < escala; dy++)
				pixel(cursor_x + (xo + xx)*escala + dx,
				cursor_y + (yo + yy)*escala + dy,
				1);
			}

			bits <<= 1;
			bit++;
		}
	}

	cursor_x += xa * escala;
}



void texto(const char *s) {

	while (*s) {

		if (*s == '\n') {
			cursor_x = 0;
			cursor_y += (fuente->yAdvance * escala) + interlineado;
			s++;            
			continue;       
		}

		letra(*s);
		s++;
	}
}
void tam_texto(uint8_t t) {
	if (t < 1) t = 1;
	escala = t;
}


