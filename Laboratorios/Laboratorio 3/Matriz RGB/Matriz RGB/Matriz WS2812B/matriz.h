/*
 * matriz.h
 *
 * Created: 27/10/2025 11:35:06
 *  Author: lucas
 */ 
#ifndef MATRIZ_H
#define MATRIZ_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdint.h>
#include <avr/io.h>

#define Filas     8
#define Columnas  8
#define NUM_LEDS  (Filas * Columnas)

#define matriz              (1 << PD6)
#define MATRIZ_PORT         PORTD
#define MATRIZ_DDR          DDRD
#define MATRIZ_BIT_NUM      PD6
#define MATRIZ_PORT_IO_ADDR 0x0B   

#define ORIGEN_ABAJO_IZQUIERDA   0  

extern uint8_t leds[NUM_LEDS][3];

void init_matriz(void);
void mostrar(void);
void setLed(uint16_t i, uint8_t r, uint8_t g, uint8_t b);
void setTodos(uint8_t r, uint8_t g, uint8_t b);
void limpiar(void);

void setLedXY(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);

void azar_fijar(uint16_t base);
uint8_t azar_u8(void);
void colores_aleatorios(void);

void cursor_iniciar_xy(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void cursor_iniciar_idx(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
void cursor_get_xy(uint8_t *x, uint8_t *y);
uint16_t cursor_get_idx(void);
void cursor_set_color(uint8_t r, uint8_t g, uint8_t b);
void cursor_color_aleatoria(void);

void mover_izquierda(void);
void mover_derecha(void);
void mover_arriba(void);
void mover_abajo(void);

static inline uint16_t indiceXY(uint8_t x, uint8_t y) {
    if (x >= Columnas) x = Columnas - 1;
    if (y >= Filas)    y = Filas - 1;
    #if ORIGEN_ABAJO_IZQUIERDA
        uint8_t yy = (uint8_t)(Filas - 1U - y);
        return (uint16_t)yy * Columnas + x;
    #else
        return (uint16_t)y * Columnas + x;
    #endif
}

#endif /* MATRIZ_H */
