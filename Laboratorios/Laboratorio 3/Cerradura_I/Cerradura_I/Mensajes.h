/*
 * Mensajes.h
 *
 * Created: 4/11/2025 13:55:53
 *  Author: lucas
 */ 


#ifndef MENSAJES_H_
#define MENSAJES_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define BTN_A      (1<<PD4)
#define BTN_B      (1<<PD3)
#define LED_ROJO   (1<<PD5)
#define LED_VERDE  (1<<PD6)
#define LED_BLANCO (1<<PD7)
#define BUZ        (1<<PB1)

void UI_Bienvenida(void);
void UI_Acerque(void);
void UI_Bloqueada(void);
void UI_Cerrando(void);
void UI_CerraduraAbierta(void);
void UI_MenuPrincipal(void);
void UI_SinTarjetaPrevia(void);
void UI_BorrarRequiereNueva(void);
void UI_PedirNuevaTarjeta(void);
void UI_TarjetaAgregadaOK(void);
void UI_TarjetaBorradaOK(void);
void UI_ErrorAgregando(void);
void UI_ResetPregunta(void);
void UI_ResetCancelado(void);
void UI_ResetProgreso(void);
void UI_ResetCompletado(void);


void UI_IntentoFallido(uint8_t n, uint8_t maxn);
void UI_BloqueadaPorIntentos(uint8_t n, uint8_t maxn);
void UI_AccesoConcedido(void);

void Uart_MostrarAyuda(void);
void Uart_ProcesarEntrada(void);
bool Uart_ClaveMaestraListo(void);
bool Uart_TomarComando(char *cmd);      /* h n b r k c m */
uint8_t Uart_LeerUidManual(uint8_t u4[4]);
void Uart_InyectarComando(char c);

void Log_ClaveMaestraOK(void);
void Log_Bloqueada(void);
void Log_Cerrada(void);
void Log_TarjetaAgregadaOK(void);
void Log_TarjetaBorrada(void);
void Log_ErrorAgregando(void);
void Log_AccesoConcedido(void);
void Log_BloqueadaPorIntentos(void);
void Log_CerraduraAbierta(void);
void Log_ResetCompletado(void);
void Log_ResetCancelado(void);




#endif /* MENSAJES_H_ */