/*
 * Mensajes.c
 *
 * Created: 4/11/2025 13:56:06
 *  Author: lucas
 */ 
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>


#define BTN_A      (1<<PD4)
#define BTN_B      (1<<PD3)
#define LED_ROJO   (1<<PD5)
#define LED_VERDE  (1<<PD6)
#define LED_BLANCO (1<<PD7)
#define BUZ        (1<<PB1)


#include "UART.h"   /* uartInicio, uartTxt, uartChar, etc. */
#include "twi.h"    /* twi_init, twi_start(), etc. */

#ifndef PCF8574
#define PCF8574 0x27       
#endif

#include "twi_lcd.h"

#include "Mensajes.h"


static void lcd_2lineas(const char* l1, const char* l2){
	twi_lcd_clear();
	twi_lcd_cmd(0x80); twi_lcd_msg(l1 ? l1 : "");
	twi_lcd_cmd(0xC0); twi_lcd_msg(l2 ? l2 : "");
}

/* Pantallas fijas */
void UI_Bienvenida(void){ lcd_2lineas("Bienvenido al","sistema RFID");_delay_ms(400);}
void UI_Acerque(void){ lcd_2lineas("Acerque la ","tarjeta :)"); }
void UI_Bloqueada(void){ lcd_2lineas("Cerradura","BLOQUEADA"); }
void UI_Cerrando(void){ lcd_2lineas("Cerrando..."," "); _delay_ms(400); }
void UI_CerraduraAbierta(void){ lcd_2lineas("Cerradura","ABIERTA  A+B"); }
void UI_MenuPrincipal(void){ lcd_2lineas("A:Nuevo  B:Borrar","A+B Cerrar"); }
void UI_SinTarjetaPrevia(void){ lcd_2lineas("Sin tarjeta","previa"); _delay_ms(700); }
void UI_BorrarRequiereNueva(void){ lcd_2lineas("Borrar requiere","nueva tarjeta"); _delay_ms(700); }
void UI_PedirNuevaTarjeta(void){ lcd_2lineas("Acerca nueva","tarjeta..."); }
void UI_TarjetaAgregadaOK(void){ lcd_2lineas("Tarjeta","AGREGADA"); _delay_ms(700); }
void UI_TarjetaBorradaOK(void){ lcd_2lineas("Tarjeta","BORRADA"); _delay_ms(700); }
void UI_ErrorAgregando(void){ lcd_2lineas("Error","Agregando UID"); _delay_ms(900); }
void UI_ResetPregunta(void){ lcd_2lineas("Volver a fabrica","A=OK   B=NO"); }
void UI_ResetCancelado(void){ lcd_2lineas("Reset cancelado",""); _delay_ms(700); }
void UI_ResetProgreso(void){ lcd_2lineas("Presente tarjeta","valida (3s)..."); }
void UI_ResetCompletado(void){ lcd_2lineas("Restaurado","OK"); _delay_ms(800); }

void UI_IntentoFallido(uint8_t n, uint8_t maxn){
	
	UI_Leds(1, 0, 0);
	UI_BeepERR();
	twi_lcd_clear();
	twi_lcd_cmd(0x80);
	twi_lcd_msg("Intento fallido");
	twi_lcd_cmd(0xC0);
	twi_lcd_msg("Intentos ");
	twi_lcd_dwr( '0' + (n % 10));
	twi_lcd_msg("/");
	twi_lcd_dwr('0' + (maxn % 10));
	_delay_ms(600);
	UI_Leds(0, 0, 0);
	
}
void UI_BloqueadaPorIntentos(uint8_t n, uint8_t maxn){
	
	twi_lcd_cmd(0xC0);
	UI_Alarma();
	twi_lcd_msg("Intentos ");
	twi_lcd_dwr('0' + (n % 10));
	twi_lcd_msg("/");
	twi_lcd_dwr('0' + (maxn % 10));	
	twi_lcd_msg("  :( ");
	UI_Leds(1, 0, 0);
	_delay_ms(200);
	twi_lcd_cmd(0xC0);
	lcd_2lineas("Cerradura","BLOQUEADA");


}
void UI_AccesoConcedido(void){
	
	UI_Leds(0, 1, 0);     // Verde
	UI_BeepOK();          // Beep
	twi_lcd_clear();
	twi_lcd_cmd(0x80); twi_lcd_msg("Acceso Concedido");
	_delay_ms(300);
	UI_Leds(0, 0, 0);     // Apagar LEDs
}

void UI_Leds(uint8_t r, uint8_t v, uint8_t b){ 
	    PORTD &= ~(LED_ROJO | LED_VERDE | LED_BLANCO);
	    if (r) PORTD |= LED_ROJO;
	    if (v) PORTD |= LED_VERDE;
	    if (b) PORTD |= LED_BLANCO;
	 }
void UI_BeepOK(void){
	    PORTB |= BUZ;
	    _delay_ms(100);
	    PORTB &= ~BUZ;
}
void UI_BeepERR(void){
	    for(uint8_t i = 0; i < 2; i++){
		    PORTB |= BUZ;
		    _delay_ms(200);
		    PORTB &= ~BUZ;
		    _delay_ms(200);
	    }
}
void UI_Alarma(void){
	    for(uint8_t i = 0; i < 4; i++){
		    PORTB |= BUZ;
		    _delay_ms(200);
		    PORTB &= ~BUZ;
		    _delay_ms(130);
	    }
}


static volatile char g_cmd = 0;
static volatile uint8_t g_clave_ok = 0;
static const char clave_target[] = "amen";
static uint8_t idx_key = 0;

static inline char to_lower_fast(char c){   //Operador ternario para que pase de mayúscula a minuscula y si no es mayuscula, manda la que era
	return (c>='A' && c<='Z') ? (char)(c - 'A' + 'a') : c;
}

void Uart_MostrarAyuda(void){
	uartTxt("Se bloquea y resetea en cualquier momento\r\n");
	uartTxt("Se Ingresan o borran tarjetas sólo en el menú\r\n");
	uartTxt("Se Cierra si está abierta previamente\r\n");
	
	uartTxt("h : ayuda\r\n");
	uartTxt("k : bloquear cerradura \r\n");
	uartTxt("c : cerrar \r\n");
	uartTxt("r : reset a fabrica \r\n");
	uartTxt("n : nueva tarjeta \r\n");
	uartTxt("b : borrar ultima usada \r\n");
	uartTxt("m : Ingresar manualmente (Si está cerrada)\r\n");
	uartTxt("p : ingrese código manual (Si está abierta) \r\n ");
}

void Uart_ProcesarEntrada(void){
	while (UCSR0A & (1<<RXC0)){
		char cr = UDR0;
		char c  = to_lower_fast(cr); 

		// Clave maestra: detectar secuencia "amen"
		if (c == clave_target[idx_key]) {
			idx_key++;
			if (clave_target[idx_key] == '\0') {
				g_clave_ok = 1;
				idx_key = 0;
				uartTxt("Clave maestra detectada\r\n");
				continue;
			}
			} else {
			// Si el caracter es el inicio de la clave, reiniciar desde 1
			if (c == clave_target[0]) {
				idx_key = 1;
				} else {
				idx_key = 0;
			}
		}

		// Si no estamos en medio de ingreso de clave, aceptar comandos
		if (idx_key == 0) {
			if (c=='h'||c=='n'||c=='b'||c=='r'||c=='k'||c=='c'||c=='m'|| c=='p' ) {
				g_cmd = c;
			}
		}
	}
}



bool Uart_ClaveMaestraListo(void){
	if (!g_clave_ok) return false;
	g_clave_ok = 0;
	uartTxt("Clave maestra ingresada\r\n");
	UI_Leds(0, 0, 0);
	return true;
}

bool Uart_TomarComando(char *cmd){
	if (!g_cmd) return false;
	*cmd = g_cmd; g_cmd = 0;
	return true;
}

uint8_t Uart_LeerUidManual(uint8_t u4[4]){
	uartTxt("UID manual (8 dígitos): ");
	uint8_t dig=0, val=0, ok=0;
	for(uint8_t i=0;i<8;i++){
		while(!(UCSR0A & (1<<RXC0)));
		char c = UDR0; uartChar(c);
		if(c>='0'&&c<='9'){ ok=1; val=(val<<4)|(uint8_t)(c-'0'); }
		else if(c>='A'&&c<='F'){ ok=1; val=(val<<4)|(uint8_t)(10 + c-'A'); }
		else if(c>='a'&&c<='f'){ ok=1; val=(val<<4)|(uint8_t)(10 + c-'a'); }
		else { ok=0; }
		if(!ok){ uartTxt("\r\n clave invalida\r\n"); return 0; }
		if((i&1)==1){ if(dig<4) u4[dig++]=val; val=0; }
	}
	uartTxt("\r\n");
	return (dig==4);
}

void Uart_InyectarComando(char c){ g_cmd = c; }


void Log_ClaveMaestraOK(void){ uartTxt("Clave maestra OK \r\n"); }
void Log_Bloqueada(void){ uartTxt("Cerradura bloqueada\r\n"); }
void Log_Cerrada(void){ uartTxt("Cerradura Cerrada\r\n"); }
void Log_TarjetaAgregadaOK(void){ uartTxt("Tarjeta agregada\r\n"); }
void Log_TarjetaBorrada(void){ uartTxt("Tarjeta borrada\r\n"); }
void Log_ErrorAgregando(void){ uartTxt("Error agregando UID\r\n"); }
void Log_AccesoConcedido(void){ uartTxt("Acceso concedido\r\n"); }
void Log_BloqueadaPorIntentos(void){ uartTxt("Bloqueada por multiples intentos\r\n"); }
void Log_CerraduraAbierta(void){ uartTxt("Cerradura abierta\r\n"); }
void Log_ResetCompletado(void){ uartTxt("Reset completado\r\n"); }
void Log_ResetCancelado(void){ uartTxt("Reset cancelado (tiempo vencido)\r\n"); }
