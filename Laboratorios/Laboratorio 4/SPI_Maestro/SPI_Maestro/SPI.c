/*
 * SPI.c
 *
 * Created: 17/11/2025 17:01:34
 *  Author: lucas
 */ 


#include "SPI.h"
#include <avr/io.h>
#include <util/delay.h>

void spi_master_init(void)
{
    // MOSI, SCK y SS como salida; MISO como entrada
    DDRB |= (1 << MOSI) | (1 << SCK) | (1 << SS);
    DDRB &= ~(1 << MISO);

    // SS en alto para mantener modo maestro
    PORTB |= (1 << SS);

    // SPI habilitado, modo maestro, MODO 3 (CPOL=1, CPHA=1), prescaler fosc/16
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << CPOL) | (1 << CPHA);
    SPSR &= ~(1 << SPI2X);  // No doble velocidad

    _delay_us(10);
}

void spi_slave_init(void)
{
    // MISO como salida; MOSI, SCK y SS como entrada
    DDRB |= (1 << MISO);
    DDRB &= ~((1 << MOSI) | (1 << SCK) | (1 << SS));

    // SPI habilitado en modo esclavo
    SPCR = (1 << SPE);
}

uint8_t spi_write(uint8_t data)
{
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

void spi_write_buffer(const uint8_t *data, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++) {
        spi_write(data[i]);
    }
}

uint8_t spi_read(void)
{
    return spi_write(0xFF);  // Dummy write para leer
}

void spi_slave_transmit(uint8_t data)
{
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

uint8_t spi_slave_receive(void)
{
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}
