/*
 * Pantalla.h
 *
 * Created: 14/11/2025 17:55:26
 *  Author: lucas
 */ 


#ifndef PANTALLA_H
#define PANTALLA_H

#include <stdint.h>

void pantalla_iniciar(void);
void pantalla_limpiar(void);
void pantalla_actualizar(void);
void pixel(uint8_t x, uint8_t y, uint8_t color);
void set_interlineado(uint8_t px);
void cursor(int16_t x, int16_t y);
void letra(char c);
void texto(const char *msg);
void tam_texto(uint8_t tam);
#endif