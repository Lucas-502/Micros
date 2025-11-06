/*
 * CFile1.c
 *
 * Created: 24/10/2025 12:59:11
 *  Author: lucas
 */ 
#include "joystick.h"
#include <avr/io.h>

static void adc_init_local(void){
	ADMUX  = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}
static uint16_t adc_read_local(uint8_t ch){
	ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)) { }
	return ADC;
}

static JoystickConfig G;
static int32_t ema_x_q7; // filtro IIR en Q7
static int32_t ema_y_q7;

void joystick_iniciar(const JoystickConfig *cfg){
	G = *cfg;

	if (G.inicializar_adc){
		adc_init_local();
	}

	if (G.btn_ddr)  *G.btn_ddr  &= ~(1 << G.btn_bit);
	if (G.btn_port){
		if (G.habilitar_pullup) *G.btn_port |=  (1 << G.btn_bit);
		else                    *G.btn_port &= ~(1 << G.btn_bit);
	}

	ema_x_q7 = ((int32_t)G.centro_x) << 7;
	ema_y_q7 = ((int32_t)G.centro_y) << 7;
}

void joystick_calibrar_centro(uint16_t cx, uint16_t cy){
	G.centro_x = cx;
	G.centro_y = cy;
	ema_x_q7 = ((int32_t)cx) << 7;
	ema_y_q7 = ((int32_t)cy) << 7;
}
void joystick_set_zona_muerta(uint16_t zm){ G.zona_muerta = zm; }
void joystick_set_histeresis(uint16_t h){  G.histeresis   = h;  }
void joystick_set_ema_shift(uint8_t s){    G.ema_shift    = s;  }
void joystick_set_escala(int16_t A){       G.escala_A     = A;  }

static inline bool leer_boton(void){
	
	if (!G.btn_pinr) return false;
	return ((*G.btn_pinr & (1 << G.btn_bit)) == 0);
}

static inline int16_t clamp_i16(int32_t v, int16_t lo, int16_t hi){
	if (v < lo) return lo;
	if (v > hi) return hi;
	return (int16_t)v;
}

JoystickEstado joystick_leer(void){
	JoystickEstado E = {0};


	E.raw_x = adc_read_local(G.ch_x);
	E.raw_y = adc_read_local(G.ch_y);


	ema_x_q7 += ((((int32_t)E.raw_x) << 7) - ema_x_q7) >> G.ema_shift;
	ema_y_q7 += ((((int32_t)E.raw_y) << 7) - ema_y_q7) >> G.ema_shift;

	E.fil_x = (uint16_t)(ema_x_q7 >> 7);
	E.fil_y = (uint16_t)(ema_y_q7 >> 7);


	int16_t low  = (int16_t)G.centro_x - (int16_t)G.zona_muerta;
	int16_t high = (int16_t)G.centro_x + (int16_t)G.zona_muerta;

	int16_t lowH  = low  - (int16_t)G.histeresis;
	int16_t highH = high + (int16_t)G.histeresis;

	// X
	if (E.fil_x < lowH)       E.dir_x = -1;
	else if (E.fil_x > highH) E.dir_x = +1;
	else                      E.dir_x =  0;


	low  = (int16_t)G.centro_y - (int16_t)G.zona_muerta;
	high = (int16_t)G.centro_y + (int16_t)G.zona_muerta;
	lowH  = low  - (int16_t)G.histeresis;
	highH = high + (int16_t)G.histeresis;

	if (E.fil_y < lowH)       E.dir_y = -1; // arriba
	else if (E.fil_y > highH) E.dir_y = +1; // abajo
	else                      E.dir_y =  0;


	int16_t A = G.escala_A;
	int16_t dx = (int16_t)E.fil_x - (int16_t)G.centro_x;
	int16_t dy = (int16_t)E.fil_y - (int16_t)G.centro_y;

	int16_t denom = (int16_t)G.zona_muerta + (int16_t)G.histeresis + 256; // margen
	if (denom < 1) denom = 1;

	int32_t nx = ((int32_t)A * dx) / denom;
	int32_t ny = ((int32_t)A * dy) / denom;

	E.nx = clamp_i16(nx, -A, +A);
	E.ny = clamp_i16(ny, -A, +A);


	E.centro     = (E.dir_x == 0 && E.dir_y == 0);
	E.diagonal   = (E.dir_x != 0 && E.dir_y != 0);
	E.horizontal = (E.dir_x != 0 && E.dir_y == 0);
	E.vertical   = (E.dir_x == 0 && E.dir_y != 0);


	E.boton_presionado = leer_boton();

	return E;
}
