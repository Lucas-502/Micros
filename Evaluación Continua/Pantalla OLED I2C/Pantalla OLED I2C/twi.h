/*		
		"TWI.h"	
*/


#ifndef TWI_H_
#define TWI_H_

#pragma once
#include <stdint.h>

#define F_CPU 16000000UL
#define TWI_FREQ 100000UL
#define BITRATE_REG ((F_CPU / TWI_FREQ) - 16) / 2

// Inicializar como maestro
void twi_master_init(void);

// Inicializar como esclavo
void twi_slave_init(uint8_t address);

// Maestro: START, REP-START, STOP
uint8_t twi_start(void);
uint8_t twi_repeated_start(void);
void    twi_stop(void);

// Maestro: escribir
uint8_t twi_write_address(uint8_t address);
uint8_t twi_write_data(uint8_t data);

// Maestro: leer
uint8_t twi_read_data_ack(uint8_t *data);
uint8_t twi_read_data_nack(uint8_t *data);

// Esclavo: operación
uint8_t twi_slave_listen(void);
uint8_t twi_slave_receive(uint8_t *data);
uint8_t twi_slave_transmit(uint8_t data);


#endif // TWI_H_
