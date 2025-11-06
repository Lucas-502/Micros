/*
 * IncFile1.h
 *
 * Created: 24/10/2025 12:59:00
 *  Author: lucas
 */ 


#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

typedef struct {
	uint8_t ch_x;
	uint8_t ch_y;

	volatile uint8_t *btn_pinr;  // p.ej. &PINC
	volatile uint8_t *btn_ddr;   // p.ej. &DDRC
	volatile uint8_t *btn_port;  // p.ej. &PORTC
	uint8_t btn_bit;             // p.ej. PC2

	// Calibración y filtros
	uint16_t centro_x;           // centro nominal (?512)
	uint16_t centro_y;           // centro nominal (?512)
	uint16_t zona_muerta;        // ± zona muerta alrededor del centro (p.ej. 160)
	uint16_t histeresis;         // margen extra para evitar vaivén (p.ej. 40)
	uint8_t  ema_shift;          // 3??=1/8, 4??=1/16 (suavizado)
	int16_t  escala_A;           // A (salida normalizada en [-A..+A])
	bool     habilitar_pullup;   // true: activa pull-up interno del botón

	bool     inicializar_adc;    // true: la lib hace adc_init()
} JoystickConfig;


typedef struct {
	
	// Lecturas
	uint16_t raw_x, raw_y;   // 0..1023 (sin filtrar)
	uint16_t fil_x, fil_y;   // 0..1023 (filtradas)

	// Normalizados a [-A..+A] según escala_A
	int16_t nx, ny;


	int8_t dir_x;            // -1 izquierda, 0 centro, +1 derecha
	int8_t dir_y;            // -1 arriba,    0 centro, +1 abajo


	bool centro;
	bool horizontal;
	bool vertical;
	bool diagonal;

	// Botón
	bool boton_presionado;   // activo en LOW si se usa pull-up
} JoystickEstado;


void joystick_iniciar(const JoystickConfig *cfg);


void joystick_calibrar_centro(uint16_t cx, uint16_t cy);
void joystick_set_zona_muerta(uint16_t zm);
void joystick_set_histeresis(uint16_t h);
void joystick_set_ema_shift(uint8_t s);
void joystick_set_escala(int16_t A);


JoystickEstado joystick_leer(void);


static inline bool joystick_izquierda(const JoystickEstado *e){ return e->dir_x < 0 && e->dir_y == 0; }
static inline bool joystick_derecha  (const JoystickEstado *e){ return e->dir_x > 0 && e->dir_y == 0; }
static inline bool joystick_arriba   (const JoystickEstado *e){ return e->dir_y < 0 && e->dir_x == 0; }
static inline bool joystick_abajo    (const JoystickEstado *e){ return e->dir_y > 0 && e->dir_x == 0; }
static inline bool joystick_horizontal(const JoystickEstado *e){ return e->horizontal; }
static inline bool joystick_vertical  (const JoystickEstado *e){ return e->vertical; }
static inline bool joystick_diagonal  (const JoystickEstado *e){ return e->diagonal; }
static inline bool joystick_centro    (const JoystickEstado *e){ return e->centro; }
static inline bool joystick_boton     (const JoystickEstado *e){ return e->boton_presionado; }

#endif /* JOYSTICK_H */
