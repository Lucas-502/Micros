/*
 * EEPROM.h
 *
 * Created: 29/10/2025 6:23:56
 *  Author: lucas
 */ 

#ifndef EEPROM_H
#define EEPROM_H

#pragma once
#include <stdint.h>
#include <stdbool.h>


#define EEPROMAccesosOk          0x40
#define EEPROMCantidadAccesos    0x41
#define EEPROMAccesos            0x42  // base de tabla de UIDs (4B c/u)
#define EEPROMTarjetaViejaFlag   0x30
#define EEPROMTarjetaVieja       0x31
#define EEPROMCerraduraBloqueada 0x60
#define EEPROM_DIR_INTENTOS  0x21

#define MAX_UIDS                 100

void Accesos_Inicializar(void);            
uint8_t Accesos_Cantidad(void);
void Accesos_GuardarCantidad(uint8_t c);
void Accesos_Leer(uint8_t idx, uint8_t u[4]);
void Accesos_Escribir(uint8_t idx, const uint8_t u[4]);
int8_t  Accesos_Buscar(const uint8_t u[4]);
uint8_t Accesos_Agregar(const uint8_t u[4]);
uint8_t Accesos_BorrarPorIndice(uint8_t idx);
void GuardarIntentos(uint8_t n);
uint8_t LeerIntentos(void);

void    GuardarBloqueo(uint8_t activo);    // 1=bloqueada
uint8_t LeerBloqueo(void);                 // 1=bloqueada

static inline uint8_t cantidadAccesos(void){ return Accesos_Cantidad(); }
static inline void     guardarCantidadAccesos(uint8_t c){ Accesos_GuardarCantidad(c); }
static inline void     leerAcceso(uint8_t idx, uint8_t u[4]){ Accesos_Leer(idx,u); }


#endif
