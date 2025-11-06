// /*
//  * main.c
//  *
//  * Created: 10/23/2025 11:17:16 PM
//  *  Author: lucas
//  */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#include "UART.h"                     
#include "Joystick/joystick.h"        
#include "Matriz WS2812B/matriz.h"       


#define PASO_MS       200    // velocidad fija por paso 
#define INVERTIR_X      0    
#define INVERTIR_Y      0   

volatile uint32_t g_millis = 0;
ISR(TIMER0_OVF_vect){ g_millis++; }
static void bt_init(void){
    TCCR0A = 0; TCCR0B = (1<<CS01)|(1<<CS00); TIMSK0 = (1<<TOIE0);
    sei();
}
static inline uint32_t millis_safe(void){
    uint32_t m; uint8_t s=SREG; cli(); m=g_millis; SREG=s; return m;
}

int main(void){


    init_matriz();
    cursor_iniciar_xy(3,3, 0,0,255);   // empieza en el centro (sup-izq) color azul 


    JoystickConfig joy = {
        .ch_x=0, .ch_y=1,
        .btn_pinr=&PINC, .btn_ddr=&DDRC, .btn_port=&PORTC, .btn_bit=PC2,
        .centro_x=512, .centro_y=512,
        .zona_muerta=120, .histeresis=20,
        .ema_shift=2, .escala_A=100,
        .habilitar_pullup=true, .inicializar_adc=true
    };
    joystick_iniciar(&joy);


    bt_init();
    _delay_ms(20);
    JoystickEstado j0 = joystick_leer();
   azar_fijar( (uint16_t)( TCNT0 ^ (g_millis & 0xFFFF) ) );

    bool btn_ant = false;
    uint32_t t_ultimo = millis_safe();

    while(1){
        JoystickEstado js = joystick_leer();

        bool btn = js.boton_presionado;
        if (!btn_ant && btn){
            cursor_color_aleatoria();
        }
        btn_ant = btn;

        int8_t dx = js.dir_x;    // ?1,0,+1
        int8_t dy = js.dir_y;    // ?1,0,+1
        #if INVERTIR_X
            dx = -dx;
        #endif
        #if INVERTIR_Y
            dy = -dy;
        #endif

        if (millis_safe() - t_ultimo >= PASO_MS){
            if (dx < 0)      mover_izquierda();
            else if (dx > 0) mover_derecha();

            if (dy < 0)      mover_arriba();
            else if (dy > 0) mover_abajo();

            t_ultimo = millis_safe();
        }

        _delay_ms(3);
    }
}
