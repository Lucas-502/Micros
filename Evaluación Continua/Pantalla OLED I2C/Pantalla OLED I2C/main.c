/*
 * main.c
 *
 * Created: 11/11/2025 11:45:50 AM
 * Author : Un fiel servidor
 */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "pantalla.h"
#include <util/delay.h>


int main(void) {
	
	pantalla_iniciar();
	pantalla_limpiar();
	
	tam_texto(1);
	set_interlineado(1);
	
	cursor(0, 0);
	texto("Utec.\nTec. en Microprocesamiento!!! ");
	
	for (uint8_t i = 0; i < 128; i++){         //Líneas horizontales del largo de la pantalla
		
		pixel(i, 17, 1);   // (x, y, color)
		pixel(i, 16, 1);
		pixel(i, 15, 1);
	}
	
// 	cursor(0, 0);
// 	texto("Una cosa.");
// 	cursor(0,31);
// 	texto("Otra cosa. ");

	pantalla_actualizar();

	while (1); 
}