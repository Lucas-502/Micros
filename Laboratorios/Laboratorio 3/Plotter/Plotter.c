
#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#include "Figuras.h"



//________________Calibración para futuro movimiento en milímetros_________________

#define PASOS_POR_MM_VERTICAL    19.9 f   // Eje X (arriba/abajo)
#define PASOS_POR_MM_HORIZONTAL  17.9 f   // Eje Y (derecha/izquierda)

#define RelojX     PB3
#define DirX       PB4
#define EnableX    PB5

#define Solenoide  PC0

#define RelojY     PC3
#define DirY       PC4
#define EnableY    PC5

#define LED        PD5

#define EN_ACTIVO_ALTO         1
#define SOLENOIDE_ACTIVO_BAJO  1

// 2 ms LOW + 2 ms HIGH → ~250 Hz
#define STEP_HIGH_MS   2
#define STEP_LOW_MS    2

/* ==== Eliminado pinHigh/pinLow y pasado a PORTx directo ==== */

static inline void habilitarX(void){
	#if EN_ACTIVO_ALTO
	PORTB |=  (1 << EnableX);
	#else
	PORTB &= ~(1 << EnableX);
	#endif
}
static inline void deshabilitarX(void){
	#if EN_ACTIVO_ALTO
	PORTB &= ~(1 << EnableX);
	#else
	PORTB |=  (1 << EnableX);
	#endif
}
static inline void habilitarY(void){
	#if EN_ACTIVO_ALTO
	PORTC |=  (1 << EnableY);
	#else
	PORTC &= ~(1 << EnableY);
	#endif
}
static inline void deshabilitarY(void){
	#if EN_ACTIVO_ALTO
	PORTC &= ~(1 << EnableY);
	#else
	PORTC |=  (1 << EnableY);
	#endif
}

static inline void bajarSolenoide(void){
	#if SOLENOIDE_ACTIVO_BAJO
	PORTC &= ~(1 << Solenoide);
	#else
	PORTC |=  (1 << Solenoide);
	#endif
}
static inline void subirSolenoide(void){
	#if SOLENOIDE_ACTIVO_BAJO
	PORTC |=  (1 << Solenoide);
	#else
	PORTC &= ~(1 << Solenoide);
	#endif
}

static inline void dirArribaX(void)    { PORTB |=  (1 << DirX); }
static inline void dirAbajoX(void)     { PORTB &= ~(1 << DirX); }
static inline void dirDerechaY(void)   { PORTC |=  (1 << DirY); }
static inline void dirIzquierdaY(void) { PORTC &= ~(1 << DirY); }


static inline void stepX(void){
	PORTB &= ~(1 << RelojX);
	_delay_ms(STEP_HIGH_MS);
	PORTB |=  (1 << RelojX);
	_delay_ms(STEP_LOW_MS);
}
static inline void stepY(void){
	PORTC &= ~(1 << RelojY);
	_delay_ms(STEP_HIGH_MS);
	PORTC |=  (1 << RelojY);
	_delay_ms(STEP_LOW_MS);
}
static inline void stepXY_pulse(void){
	PORTB &= ~(1 << RelojX);
	PORTC &= ~(1 << RelojY);
	_delay_ms(STEP_HIGH_MS);
	PORTB |=  (1 << RelojX);
	PORTC |=  (1 << RelojY);
	_delay_ms(STEP_LOW_MS);
}

// Movimiento en pasos de los motores
void arriba_pasos(int pasos){
	deshabilitarY();
	habilitarX();
	dirArribaX();
	_delay_ms(10);
	for (int i=0;i<pasos;i++) stepX();
	deshabilitarX();
	_delay_ms(50);
}
void abajo_pasos(int pasos){
	deshabilitarY();
	habilitarX();
	dirAbajoX();
	_delay_ms(10);
	for (int i=0;i<pasos;i++) stepX();
	deshabilitarX();
	_delay_ms(50);
}
void derecha_pasos(int pasos){
	deshabilitarX();
	habilitarY();
	dirDerechaY();
	_delay_ms(10);
	for (int i=0;i<pasos;i++) stepY();
	deshabilitarY();
	_delay_ms(50);
}
void izquierda_pasos(int pasos){
	deshabilitarX();
	habilitarY();
	dirIzquierdaY();
	_delay_ms(10);
	for (int i=0;i<pasos;i++) stepY();
	deshabilitarY();
	_delay_ms(50);
}

//_________________________________________- Funciones para movimiento en milímetros___________________________________________

void arriba(float mm){    arriba_pasos((int)(mm*PASOS_POR_MM_VERTICAL   + 0.5f)); }
void abajo(float mm){     abajo_pasos ((int)(mm*PASOS_POR_MM_VERTICAL   + 0.5f)); }
void derecha(float mm){   derecha_pasos((int)(mm*PASOS_POR_MM_HORIZONTAL + 0.5f)); }
void izquierda(float mm){ izquierda_pasos((int)(mm*PASOS_POR_MM_HORIZONTAL + 0.5f)); }

void bajar_lapiz(void){ bajarSolenoide(); _delay_ms(300); }
void subir_lapiz(void){ subirSolenoide(); _delay_ms(300); }


void linea(float mm, float ang_deg){
	if (mm <= 0.0f) return;

	float rad  = ang_deg * (float)M_PI / 180.0f;
	float dxmm = cosf(rad) * mm;  // +derecha / -izquierda → Y
	float dymm = sinf(rad) * mm;  // +arriba  / -abajo     → X

	int pasosX = (int)(fabsf(dymm) * PASOS_POR_MM_VERTICAL   + 0.5f);
	int pasosY = (int)(fabsf(dxmm) * PASOS_POR_MM_HORIZONTAL + 0.5f);
	if (pasosX == 0 && pasosY == 0) return;

	if (dymm >= 0) dirArribaX(); else dirAbajoX();
	if (dxmm >= 0) dirDerechaY(); else dirIzquierdaY();

	habilitarX();
	habilitarY();
	_delay_ms(5);

	int max  = (pasosX > pasosY) ? pasosX : pasosY;
	int errX = 0, errY = 0;

	for (int i=0; i<max; i++){
		uint8_t doX = 0, doY = 0;

		errX += pasosX; if (errX >= max){ errX -= max; doX = 1; }
		errY += pasosY; if (errY >= max){ errY -= max; doY = 1; }

		if (doX && doY){
			stepXY_pulse();   // ambos a la vez
			}else if (doX){
			stepX();
			}else if (doY){
			stepY();
			}else{
			_delay_ms(STEP_HIGH_MS + STEP_LOW_MS);
		}
	}

	deshabilitarX();
	deshabilitarY();
	_delay_ms(50);
}


static void dibuja_LUT(const cmd_t *lut, uint16_t n, float escala){
	if (escala <= 0.0f) escala = 1.0f;

	uint8_t pen_down = 0;
	subir_lapiz();

	for (uint16_t i = 0; i < n; i++) {
		uint16_t L10 = pgm_read_word(&lut[i].L10);
		uint16_t A   = pgm_read_word(&lut[i].A);

		if (L10 == 0) continue;  // segmento nulo, lo saltamos

		if (!pen_down){
			bajar_lapiz();
			pen_down = 1;
		}

		float L_mm = (L10 / 10.0f) * escala;  // décimas → mm
		linea(L_mm, (float)A);
	}

	subir_lapiz();
}


// __________________________________________________________________________ FIGURAS_______________________________________________________
void Cuadrado30(void){
	
	bajar_lapiz();
	arriba(30.0f);
	derecha(30.0f);
	abajo(30.0f);
	izquierda(30.0f);
	subir_lapiz();
}


void triangulo_isosceles(float lado_mm){
	if (lado_mm <= 0.0f) return;

	const float base = 1.7320508f * lado_mm; // √3

	bajar_lapiz();
	linea(lado_mm, 210.0f); // abajo-izquierda
	linea(base,    0.0f);   // derecha (base)
	linea(lado_mm, 150.0f); // arriba-izquierda
	_delay_ms(40);
	subir_lapiz();
}


void Cruz(float lado_mm){
	
	if (lado_mm <= 0.0f) return;

	const float base = 0.70710f * lado_mm ; //   coseno de (45) = √2/2

	bajar_lapiz();
	linea(lado_mm, 45.0f); // abajo-izquierda
	subir_lapiz();
	linea(base,    270.0f);
	bajar_lapiz();
	linea(lado_mm, 135.0f); // arriba-izquierda
	_delay_ms(40);
	subir_lapiz();
	
}

void circulo(float radio_mm){
	if (radio_mm <= 0.0f) return;

	const int N = 72;
	float lado;

	#if POLY_SIDE_FROM_INSCRIBED
	
	lado = 2.0f * radio_mm * sinf((float)M_PI / (float)N);
	#else
	
	lado = (2.0f * (float)M_PI * radio_mm) / (float)N;
	#endif

	float ang = 0.0f;                 // 0° = derecha; incrementa 360/N por lado
	const float dAng = 360.0f / (float)N;

	bajar_lapiz();
	
	_delay_ms(200);
	
	for (int i = 0; i < N; i++){
		linea(lado, ang);
		_delay_ms(100);
		ang += dAng;
	}
	_delay_ms(40);
	subir_lapiz();
}


void zorro(float escala){
	dibuja_LUT(ZORRO_LUT, ZORRO_N, escala);
}

void murcielago(float escala){
	dibuja_LUT(MURCIELAGO_LUT, MURCIELAGO_N, escala);
}


// ____________________________________________________ MAIN _______________________________________________________
int main(void){
	

	DDRB |= (1<<RelojX)|(1<<DirX)|(1<<EnableX);
	DDRC |= (1<<Solenoide)|(1<<RelojY)|(1<<DirY)|(1<<EnableY);
	DDRD |= (1<<LED);

	subirSolenoide();
	deshabilitarX();
	deshabilitarY();
	/* idle HIGH en relojes */
	PORTB |= (1 << RelojX);
	PORTC |= (1 << RelojY);

	_delay_ms(1000);

	// “beep” con LED: 3 parpadeos
	for(int i=0;i<3;i++){
		PORTD |=  (1<<LED); _delay_ms(200);
		PORTD &= ~(1<<LED); _delay_ms(200);
	}

	PORTD |= (1<<LED);

	//Cuadrado30();   // Para calibrar

	triangulo_isosceles(30.0f);
	
	_delay_ms(100);
	
	derecha(50.0f);

	_delay_ms(100);
	
	circulo(25.0f);
	
	_delay_ms(100);
	
	abajo(50.0f);
	
	Cruz(30.0f);
	
	abajo(50.0f);
	
	zorro(0.5f);
	
	izquierda(50.0f);
	
	abajo(50.0f);
	
	murcielago(0.5f);
	
	PORTD &= ~(1<<LED);

	// Fin: parpadeo lento del led
	while(1){
		PORTD ^= (1<<LED);
		_delay_ms(1000);
	}
}
