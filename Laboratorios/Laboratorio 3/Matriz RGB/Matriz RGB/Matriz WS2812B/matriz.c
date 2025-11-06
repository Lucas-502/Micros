/*
 * CFile1.c
 *
 * Created: 27/10/2025 11:35:25
 *  Author: lucas
 */ 
#include "matriz.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>      // ? srand(), rand()

uint8_t leds[NUM_LEDS][3];

static uint16_t _cursor_idx = 0;
static uint8_t  _curR = 0, _curG = 0, _curB = 255;
//_____________________________________________________________Aleatorio
void azar_fijar(uint16_t base) {
	if (base == 0) base = 1;
	srand((unsigned int)base);
}

uint8_t azar_u8(void) {
	return (uint8_t)(rand() % 256);
}


static inline void enviarBit(uint8_t bitVal);
static void enviarByte(uint8_t byte);
static void _cursor_pintar_unico(void);
static void _cursor_xy(uint8_t *x, uint8_t *y); 


void init_matriz(void) {
    MATRIZ_DDR  |=  matriz;
    MATRIZ_PORT &= ~matriz;
    limpiar();
}

void limpiar(void) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i][0] = 0;
        leds[i][1] = 0;
        leds[i][2] = 0;
    }
}

void setTodos(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i][0] = g;
        leds[i][1] = r;
        leds[i][2] = b;
    }
}

void setLed(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
    if (i >= NUM_LEDS) return;
    leds[i][0] = g;
    leds[i][1] = r;
    leds[i][2] = b;
}

void setLedXY(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    setLed(indiceXY(x, y), r, g, b);
}

void colores_aleatorios(void) {
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		leds[i][0] = azar_u8();  // G
		leds[i][1] = azar_u8();  // R
		leds[i][2] = azar_u8();  // B
	}
}

void mostrar(void) {
    uint8_t sreg_bak = SREG;
    cli();

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        enviarByte(leds[i][0]); // G 
        enviarByte(leds[i][1]); // R 
        enviarByte(leds[i][2]); // B 
    }

    SREG = sreg_bak;
    _delay_us(60); 
}

void cursor_iniciar_xy(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b){
    _cursor_idx = indiceXY(x, y);
    _curR = r; _curG = g; _curB = b;
    _cursor_pintar_unico();
}

void cursor_iniciar_idx(uint16_t idx, uint8_t r, uint8_t g, uint8_t b){
    if (idx >= NUM_LEDS) idx = 0;
    _cursor_idx = idx;
    _curR = r; _curG = g; _curB = b;
    _cursor_pintar_unico();
}

void cursor_get_xy(uint8_t *x, uint8_t *y){
    if (x && y) _cursor_xy(x, y);
}

uint16_t cursor_get_idx(void){
    return _cursor_idx;
}

void cursor_set_color(uint8_t r, uint8_t g, uint8_t b){
    _curR = r; _curG = g; _curB = b;
    _cursor_pintar_unico();
}

void cursor_color_aleatoria(void){
	uint8_t r = azar_u8();
	uint8_t g = azar_u8();
	uint8_t b = azar_u8();
	if (r==_curR && g==_curG && b==_curB) { r ^= 0x55; g ^= 0xAA; } // evita repetición exacta
	cursor_set_color(r,g,b);
}

void mover_izquierda(void){
    uint8_t x, y; _cursor_xy(&x,&y);
    if (x > 0) { _cursor_idx -= 1; _cursor_pintar_unico(); }
}
void mover_derecha(void){
    uint8_t x, y; _cursor_xy(&x,&y);
    if (x < (Columnas-1)) { _cursor_idx += 1; _cursor_pintar_unico(); }
}
void mover_arriba(void){
    uint8_t x, y; _cursor_xy(&x,&y);
    if (y > 0) { _cursor_idx -= Columnas; _cursor_pintar_unico(); }
}
void mover_abajo(void){
    uint8_t x, y; _cursor_xy(&x,&y);
    if (y < (Filas-1)) { _cursor_idx += Columnas; _cursor_pintar_unico(); }
}

static void _cursor_pintar_unico(void){
    /* limpia todo y pinta solo el cursor */
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i][0] = leds[i][1] = leds[i][2] = 0;
    }
    setLed(_cursor_idx, _curR, _curG, _curB);
    mostrar();
}

static void _cursor_xy(uint8_t *x, uint8_t *y){
    uint16_t idx = _cursor_idx;
    #if ORIGEN_ABAJO_IZQUIERDA
        uint8_t yy = (uint8_t)(idx / Columnas);
        *x = (uint8_t)(idx % Columnas);
        *y = (uint8_t)(Filas - 1U - yy);
    #else
        *y = (uint8_t)(idx / Columnas);
        *x = (uint8_t)(idx % Columnas);
    #endif
}

static void enviarByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        enviarBit( (byte & (1U << (7 - i))) != 0U );
    }
}

static inline void enviarBit(uint8_t bitVal) {
    if (bitVal) {
        asm volatile(
            "sbi  %[port], %[bit]\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "nop\n\t""nop\n\t""nop\n\t"
            "cbi  %[port], %[bit]\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            :
            : [port] "I" (MATRIZ_PORT_IO_ADDR), [bit] "I" (MATRIZ_BIT_NUM)
        );
    } else {
        asm volatile(
            "sbi  %[port], %[bit]\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "cbi  %[port], %[bit]\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
            :
            : [port] "I" (MATRIZ_PORT_IO_ADDR), [bit] "I" (MATRIZ_BIT_NUM)
        );
    }
}
