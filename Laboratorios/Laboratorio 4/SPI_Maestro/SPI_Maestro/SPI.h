/*
 * SPI.h
 *
 * Created: 17/11/2025 17:01:16
 *  Author: lucas
 */ 

#ifndef SPI_H_
#define SPI_H_

#pragma once
#include <stdint.h>
#include <avr/io.h>

// Pines del hardware SPI en ATmega328P
// MOSI = PB3 (Arduino D11)
// MISO = PB4 (Arduino D12)
// SCK  = PB5 (Arduino D13)
// SS   = PB2 (Arduino D10)
#define MOSI PB3
#define MISO PB4
#define SCK  PB5
#define SS   PB2

// Inicializar como maestro
void spi_master_init(void);

// Inicializar como esclavo
void spi_slave_init(void);

// Maestro: enviar dato (recibe al mismo tiempo)
uint8_t spi_write(uint8_t data);

// Maestro: enviar múltiples datos
void spi_write_buffer(const uint8_t *data, uint8_t length);

// Leer dato (en esclavo o dummy-read en maestro)
uint8_t spi_read(void);

// Esclavo: transmitir
void spi_slave_transmit(uint8_t data);

// Esclavo: recibir
uint8_t spi_slave_receive(void);

#endif /* SPI_H_ */
