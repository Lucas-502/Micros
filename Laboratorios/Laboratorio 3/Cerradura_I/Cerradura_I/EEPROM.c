/*
 * EEPROM.c
 *
 * Created: 29/10/2025 6:24:10
 *  Author: lucas
 */ 
#include <avr/eeprom.h>
#include <stdint.h>
#include "EEPROM.h"

#include <avr/eeprom.h>
#include <string.h>

#define EE_OLD_MAGIC 0x30
#define EE_OLD_UID0  0x31

static inline void eew(uint16_t a, uint8_t d){ eeprom_update_byte((uint8_t*)a, d); }
static inline uint8_t eer(uint16_t a){ return eeprom_read_byte((uint8_t*)a); }

void Accesos_Inicializar(void){
	
	uint8_t new_magic = eer(EEPROMAccesosOk);
	uint8_t old_magic = eer(EE_OLD_MAGIC);
	if (new_magic!=0xA5){
		if (old_magic==0xA5){
			uint8_t u[4]={ eer(EE_OLD_UID0+0), eer(EE_OLD_UID0+1),
			eer(EE_OLD_UID0+2), eer(EE_OLD_UID0+3) };
			uint16_t a = EEPROMAccesos;
			eew(a+0,u[0]); eew(a+1,u[1]); eew(a+2,u[2]); eew(a+3,u[3]);
			eew(EEPROMCantidadAccesos,1);
			} else {
			eew(EEPROMCantidadAccesos,0);
		}
		eew(EEPROMAccesosOk,0xA5);
		} else {


		if (eer(EEPROMCantidadAccesos)==0 && old_magic==0xA5){
			uint8_t u[4]={ eer(EE_OLD_UID0+0), eer(EE_OLD_UID0+1),
			eer(EE_OLD_UID0+2), eer(EE_OLD_UID0+3) };
			uint16_t a = EEPROMAccesos;
			eew(a+0,u[0]); eew(a+1,u[1]); eew(a+2,u[2]); eew(a+3,u[3]);
			eew(EEPROMCantidadAccesos,1);
		}
	}
}

uint8_t Accesos_Cantidad(void){
	return (eer(EEPROMAccesosOk)==0xA5) ? eer(EEPROMCantidadAccesos) : 0;
}
void Accesos_GuardarCantidad(uint8_t c){
	eew(EEPROMCantidadAccesos,c);
	eew(EEPROMAccesosOk,0xA5);
}
void Accesos_Leer(uint8_t idx, uint8_t u[4]){
	uint16_t a = EEPROMAccesos + (uint16_t)idx*4;
	u[0]=eer(a+0); u[1]=eer(a+1); u[2]=eer(a+2); u[3]=eer(a+3);
}
void Accesos_Escribir(uint8_t idx, const uint8_t u[4]){
	uint16_t a = EEPROMAccesos + (uint16_t)idx*4;
	eew(a+0,u[0]); eew(a+1,u[1]); eew(a+2,u[2]); eew(a+3,u[3]);
}
int8_t Accesos_Buscar(const uint8_t u[4]){
	uint8_t c = Accesos_Cantidad(), t[4];
	for(uint8_t i=0;i<c;i++){ Accesos_Leer(i,t);
		if(!memcmp(t,u,4)) return (int8_t)i;
	}
	return -1;
}
uint8_t Accesos_Agregar(const uint8_t u[4]){
	uint8_t c = Accesos_Cantidad();
	if (c>=MAX_UIDS) return 0;
	if (Accesos_Buscar(u)>=0) return 1;
	Accesos_Escribir(c,u);
	Accesos_GuardarCantidad(c+1);
	return 1;
}
uint8_t Accesos_BorrarPorIndice(uint8_t idx){
	uint8_t c = Accesos_Cantidad(); if (idx>=c) return 0;
	if (idx != c-1){
		uint8_t last[4]; Accesos_Leer(c-1,last); Accesos_Escribir(idx,last);
	}
	Accesos_GuardarCantidad(c-1);
	return 1;
}

// Memoria para intentos y bloqueo 

void GuardarBloqueo(uint8_t activo){ 
	eew(EEPROMCerraduraBloqueada, activo?0xA5:0x00); }
uint8_t LeerBloqueo(void){ 
	return eer(EEPROMCerraduraBloqueada)==0xA5; }


void GuardarIntentos(uint8_t n) {
	eeprom_write_byte((uint8_t*)EEPROM_DIR_INTENTOS, n);
}

uint8_t LeerIntentos(void) {
	return eeprom_read_byte((uint8_t*)EEPROM_DIR_INTENTOS);
}
