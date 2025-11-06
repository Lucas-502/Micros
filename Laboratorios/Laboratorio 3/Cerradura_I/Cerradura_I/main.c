// /*
//  * main.c
//  *
//  * Created: 10/23/2025 1:23:14 PM
//  *  Author: lucas
//


#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#define PCF8574 0x27 

// Drivers base
#include "SPI.h"
#include "RC522.h"
#include "UART.h"
#include "twi.h"

extern void twi_lcd_init(void);
extern void twi_lcd_clear(void);
extern void twi_lcd_cmd(uint8_t cmd);
extern void twi_lcd_msg(const char *s);
extern void twi_lcd_dwr(uint8_t c);
#include "EEPROM.h"
#include "Mensajes.h"

#define BTN_A      (1<<PD4)
#define BTN_B      (1<<PD3)
#define LED_ROJO   (1<<PD5)
#define LED_VERDE  (1<<PD6)
#define LED_BLANCO (1<<PD7)
#define BUZ        (1<<PB1)

#define INTENTOS_MAX 5

typedef enum { ST_CERRADA=0, ST_VERIFICAR, ST_BLOQUEADA, ST_ABIERTA, ST_MENU, ST_RESET } ESTADO;
static volatile ESTADO estado_actual = ST_CERRADA;
static uint8_t intentos = 0;

static void InicializarHardware(void);
static void Estado_Cerrada(void);
static void Estado_Verificar(void);
static void Estado_Bloqueada(void);
static void Estado_Abierta(const uint8_t uid_ok[4]);
static void Estado_Menu(uint8_t uid_ok[4]);
static void Estado_Reset(void);
static void ProcesarOrdenesUART_Seguras(uint8_t contexto);
static uint8_t RFID_LeerUID4(uint8_t u4[4]);

typedef enum { EV_NONE, EV_A, EV_B, EV_AB } btn_ev_t;
static btn_ev_t Botones_LeerEvento(void);

// ============================ MAIN ====================================
int main(void)
{
	InicializarHardware();

	Accesos_Inicializar();
	intentos = LeerIntentos();

	if (LeerBloqueo()){
		UI_Bloqueada();
		Estado_Bloqueada();
		for(;;){}
	}
	Estado_Cerrada();
	for(;;){}
}


static void InicializarHardware(void){
	
	DDRD |= (LED_ROJO|LED_VERDE|LED_BLANCO);
	PORTD &= ~(LED_ROJO|LED_VERDE|LED_BLANCO);

	DDRB |= BUZ;
	PORTB &= ~BUZ;

	DDRD &= ~(BTN_A|BTN_B);
	PORTD |=  (BTN_A|BTN_B); // pull-ups

	twi_init();
	twi_lcd_init();
	twi_lcd_clear();

	uartInicio();
	Uart_MostrarAyuda();
	
	DDRB  |= (1<<PB2);       // SS alto antes del SPI
	PORTB |= (1<<PB2);
	spi_init();
	mfrc522_init();

	UI_Bienvenida();
}

static btn_ev_t Botones_LeerEvento(void){
	uint8_t saw_a=0, saw_b=0, saw_ab=0;

	for(uint8_t i=0;i<12;i++){
		uint8_t s = ~PIND;
		uint8_t a = s & BTN_A, b = s & BTN_B;
		if (a && b){ saw_ab=1; break; }
		if (a) saw_a=1;
		if (b) saw_b=1;
		_delay_ms(3);
	}
	if (!saw_ab && (saw_a ^ saw_b)){
		for(uint8_t i=0;i<10;i++){
			uint8_t s = ~PIND;
			if ( (s&BTN_A) && (s&BTN_B) ){ saw_ab=1; break; }
			_delay_ms(3);
		}
	}
	btn_ev_t ev = EV_NONE;
	if (saw_ab) ev=EV_AB; else if (saw_a) ev=EV_A; else if (saw_b) ev=EV_B;

	if (ev!=EV_NONE){
		do{ _delay_ms(2); }while( (~PIND) & (BTN_A|BTN_B) );
		_delay_ms(10);
	}
	return ev;
}

static uint8_t RFID_LeerUID4(uint8_t u4[4]){
	uint8_t uid[10], len=0;
	if (mfrc522_detectar(uid,&len) && len>=4){
		u4[0]=uid[0]; u4[1]=uid[1]; u4[2]=uid[2]; u4[3]=uid[3];
		return 1;
	}
	return 0;
}

//          contexto: 0=cerrada,1=verificar,2=bloqueada,3=abierta,4=menu,5=reset 
static void ProcesarOrdenesUART_Seguras(uint8_t contexto)
{
	Uart_ProcesarEntrada();

	if (Uart_ClaveMaestraListo()){
		GuardarBloqueo(0);
		intentos = 0;
		Log_ClaveMaestraOK();
		uint8_t vacio[4]={0};
		Estado_Abierta(vacio);
		return;
	}

	char cmd;
	if (!Uart_TomarComando(&cmd)) return;

	if (cmd=='h'){ Uart_MostrarAyuda(); return; }

	if (cmd=='k'){
		GuardarBloqueo(1);
		UI_Bloqueada();
		Log_Bloqueada();
		Estado_Bloqueada();
		return;
	}

	if (cmd=='c'){
		if (contexto == 3 || contexto == 4) {  // ST_ABIERTA o ST_MENU
			UI_Cerrando();
			Log_Cerrada();
			Estado_Cerrada();
		}
		return;
	}

	if (cmd=='r'){
		Estado_Reset();
		return;
	}
	if (cmd=='p' && contexto == 0){
		uint8_t u4[4];
		if (Uart_LeerUidManual(u4)){
			if (Accesos_Buscar(u4) >= 0){
				intentos = 0;
				Log_AccesoConcedido();
				UI_AccesoConcedido();
				Estado_Abierta(u4);
				return;
				} else {
				intentos++;
				UI_IntentoFallido(intentos, INTENTOS_MAX);
				if (intentos >= INTENTOS_MAX){
					UI_BloqueadaPorIntentos(intentos, INTENTOS_MAX);
					GuardarBloqueo(1);
					Log_BloqueadaPorIntentos();
					Estado_Bloqueada();
					return;
				}
				UI_Acerque();
			}
		}
		return;
	}

	if (cmd=='m' && (contexto == 3 || contexto == 4)){
		uint8_t u4[4];
		if (Uart_LeerUidManual(u4)){
			if (Accesos_Agregar(u4)){
				UI_TarjetaAgregadaOK();
				Log_TarjetaAgregadaOK();
				} else {
				UI_ErrorAgregando();
				Log_ErrorAgregando();
			}
		}
		UI_MenuPrincipal(); // si estás en menú, redibuja
		return;
	}

	if ((cmd=='n' || cmd=='b') && (contexto==3 || contexto==4)){
		Uart_InyectarComando(cmd=='n' ? 'A' : 'D');
		if (contexto==3){
			UI_CerraduraAbierta();
			uint8_t vacio[4]={0};
			Estado_Menu(vacio);
		}
		return;
	}
}

static void Estado_Cerrada(void)
{
	estado_actual = ST_CERRADA;

	// Delay inicial para evitar lecturas falsas de RFID al arrancar
	_delay_ms(500);

	// Evitar entrada automática si se arrancó con un intento previo
	if (intentos >= INTENTOS_MAX) {
		UI_BloqueadaPorIntentos(intentos, INTENTOS_MAX);
		GuardarBloqueo(1);
		Log_BloqueadaPorIntentos();
		Estado_Bloqueada();
		return;
	}

	UI_Acerque();

	for(;;){
		ProcesarOrdenesUART_Seguras(0);

		// Reset por mantener A+B 2s
		if ((~PIND)&(BTN_A|BTN_B)){
			uint16_t hold=0;
			while( ((~PIND)&BTN_A) && ((~PIND)&BTN_B) && hold<2000 ){ _delay_ms(1); hold++; }
			if (hold>=2000){ (void)Botones_LeerEvento(); Estado_Reset(); return; }
		}

		// Ventana RFID 40 ms
		uint8_t u4[4];
		if (RFID_LeerUID4(u4)){
			Estado_Verificar();
			return;
		}
		_delay_ms(40);
	}
}

static void Estado_Verificar(void)
{
	estado_actual = ST_VERIFICAR;

	for(;;){
		ProcesarOrdenesUART_Seguras(1);

		// AB para cerrar
		btn_ev_t evc = Botones_LeerEvento();
		if (evc == EV_AB){
			UI_Cerrando();
			Estado_Cerrada();
			return;
		}

		// Ventana RFID 300ms
		uint8_t u4[4]={0};
		uint8_t got=0;
		for(uint16_t to=300; to && !got; --to){
			if (RFID_LeerUID4(u4)) got=1;
			_delay_ms(1);
		}
		if (!got){ UI_Acerque(); return; }

		if (Accesos_Buscar(u4) >= 0){
			intentos = 0;
			GuardarIntentos(0);
			UI_AccesoConcedido();
			Log_AccesoConcedido();
			Estado_Abierta(u4);
			return;
			} else {
			intentos++;
			GuardarIntentos(intentos);

			UI_IntentoFallido(intentos, INTENTOS_MAX);
			if (intentos >= INTENTOS_MAX){
				UI_BloqueadaPorIntentos(intentos, INTENTOS_MAX);
				GuardarBloqueo(1);
				Log_BloqueadaPorIntentos();
				Estado_Bloqueada();
				return;
				} else {
				UI_Acerque();
			}
		}
	}
}




static void Estado_Bloqueada(void)
{
	estado_actual = ST_BLOQUEADA;

	for(;;){
		ProcesarOrdenesUART_Seguras(2);

		// Mantener A+B 2s -> Reset
		btn_ev_t ev = Botones_LeerEvento();
		if (ev == EV_AB){
			uint16_t hold=0;
			while( ((~PIND)&BTN_A) && ((~PIND)&BTN_B) && hold<2000 ){ _delay_ms(1); hold++; }
			if (hold>=2000){ Estado_Reset(); return; }
		}
		_delay_ms(20);
	}
}





static void Estado_Abierta(const uint8_t uid_ok[4])
{
	(void)uid_ok;
	estado_actual = ST_ABIERTA;

	GuardarBloqueo(0);
	UI_CerraduraAbierta();
	Log_CerraduraAbierta();

	Estado_Menu((uint8_t*)uid_ok);
}




static void Estado_Menu(uint8_t uid_ok[4])
{
	estado_actual = ST_MENU;
	int8_t  last_index = Accesos_Buscar(uid_ok);
	uint8_t pendiente_borrar = 0;
	uint8_t uid_prev[4]={0};

	REDIBUJAR:
	UI_MenuPrincipal();

	for(;;){
		ProcesarOrdenesUART_Seguras(4);

		btn_ev_t ev = EV_NONE;
		char cmd;
		if (Uart_TomarComando(&cmd)){
			if (cmd=='A') ev=EV_A; else if (cmd=='D') ev=EV_B;
			} else {
			ev = Botones_LeerEvento();
		}

		if (ev == EV_AB){
			UI_Cerrando();
			Log_Cerrada();
			Estado_Cerrada();
			return;
		}

		if (ev == EV_B){
			uint8_t c = Accesos_Cantidad();

			if (last_index < 0){
				UI_SinTarjetaPrevia();
				goto REDIBUJAR;
			}

			if (c > 1){
				int8_t idx = Accesos_Buscar(uid_ok);
				if (idx >= 0){ Accesos_BorrarPorIndice(idx); }
				UI_TarjetaBorradaOK();
				Log_TarjetaBorrada();
				last_index = Accesos_Buscar(uid_ok);
				goto REDIBUJAR;
				} else {
				uid_prev[0]=uid_ok[0]; uid_prev[1]=uid_ok[1];
				uid_prev[2]=uid_ok[2]; uid_prev[3]=uid_ok[3];
				pendiente_borrar = 1;
				UI_BorrarRequiereNueva();
				ev = EV_A; // fuerza flujo a ?nuevo?
			}
		}

		if (ev == EV_A){
			UI_PedirNuevaTarjeta();
			uint8_t ok=0, nu4[4];

			for(uint16_t to=4000; to && !ok; --to){
				ProcesarOrdenesUART_Seguras(4);
				if (RFID_LeerUID4(nu4)){
					if (Accesos_Buscar(nu4)<0) ok = Accesos_Agregar(nu4);
					else ok = 1;
				}
				_delay_ms(1);
			}

			if (!ok){
				UI_ErrorAgregando();
				Log_ErrorAgregando();
				pendiente_borrar = 0;
				goto REDIBUJAR;
				} else {
				UI_TarjetaAgregadaOK();
				Log_TarjetaAgregadaOK();

				if (pendiente_borrar){
					int8_t idx = Accesos_Buscar(uid_prev);
					if (idx >= 0 && Accesos_Cantidad() >= 2){
						Accesos_BorrarPorIndice(idx);
						UI_TarjetaBorradaOK();
						Log_TarjetaBorrada();
					}
					pendiente_borrar = 0;
				}
				goto REDIBUJAR;
			}
		}

		_delay_ms(20);
	}
}




static void Estado_Reset(void){
	ESTADO estado_anterior = estado_actual;
	estado_actual = ST_RESET;
	UI_ResetPregunta();

	uint8_t u4[4];
	uint8_t ok = 0;

	for (uint16_t ms = 0; ms < 3000 && !ok; ms++) {
		ProcesarOrdenesUART_Seguras(5);
		btn_ev_t ev = Botones_LeerEvento();

		if (ev == EV_B) {
			UI_ResetCancelado();
			Log_ResetCancelado();
			break;  // Salir del bucle si el usuario cancela
		}

		if (RFID_LeerUID4(u4)) {
			if (Accesos_Buscar(u4) >= 0) {
				ok = 1;
			}
		}
		_delay_ms(1);
	}

	if (ok) {
		Accesos_GuardarCantidad(0);
		GuardarBloqueo(0);
		UI_ResetCompletado();
		Log_ResetCompletado();

		// Agregar UID fijo: "12345689" = 0x12 0x34 0x56 0x89
		uint8_t master_uid[4] = {0x12, 0x34, 0x56, 0x89};
		Accesos_Agregar(master_uid);
		} else if (!ok) {
		UI_ResetCancelado();
		Log_ResetCancelado();
	}
	
	
	if (estado_anterior == ST_CERRADA) {
		UI_Acerque();
		Estado_Cerrada();
		} else if (estado_anterior == ST_BLOQUEADA) {
		UI_Bloqueada();
		Estado_Bloqueada();
		} else if (estado_anterior == ST_ABIERTA) {
		UI_CerraduraAbierta();
		Estado_Abierta((uint8_t*)"\0\0\0\0");
		} else if (estado_anterior == ST_MENU) {
		Estado_Menu((uint8_t*)"\0\0\0\0");
		} else {
		UI_Acerque();
		Estado_Cerrada();
	}
}
