#include "twi.h"
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>

void twi_master_init(void)
{
	TWCR &= ~(1 << TWEN);   // Apagar TWI para configurar
	TWSR = 0x00;            // Prescaler = 1
	TWBR = (uint8_t)BITRATE_REG;
	TWCR = (1 << TWEN);     // Encender TWI
	_delay_us(10);
}

void twi_slave_init(uint8_t address)
{
	TWAR = (address << 1);     // Dirección propia
	TWCR = (1 << TWEA) |
	(1 << TWEN) |
	(1 << TWINT);       // Listo para recibir
}

//   FUNCIONES PARA MAESTRO

uint8_t twi_start(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return (TW_STATUS == TW_START) ? 0 : TW_STATUS;
}

uint8_t twi_repeated_start(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return (TW_STATUS == TW_REP_START) ? 0 : TW_STATUS;
}

uint8_t twi_write_address(uint8_t address)
{
	TWDR = address;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));

	// Aceptar ACK tanto en modo transmisión como en recepción
	if (TW_STATUS == TW_MT_SLA_ACK || TW_STATUS == TW_MR_SLA_ACK)
	return 0;

	return TW_STATUS;
}

uint8_t twi_write_data(uint8_t data)
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return (TW_STATUS == TW_MT_DATA_ACK) ? 0 : TW_STATUS;
}

uint8_t twi_read_data_ack(uint8_t *data)
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));
	if (TW_STATUS != TW_MR_DATA_ACK) return TW_STATUS;
	*data = TWDR;
	return 0;
}

uint8_t twi_read_data_nack(uint8_t *data)
{
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	if (TW_STATUS != TW_MR_DATA_NACK) return TW_STATUS;
	*data = TWDR;
	return 0;
}

//   STOP (común)

void twi_stop(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	while (TWCR & (1 << TWSTO));
}

//   FUNCIONES PARA ESCLAVO

uint8_t twi_slave_listen(void)
{
	while (1)
	{
		TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
		while (!(TWCR & (1 << TWINT)));

		uint8_t status = TW_STATUS;

		if (status == TW_SR_SLA_ACK || status == TW_ST_SLA_ACK)
		return status;
	}
}

uint8_t twi_slave_receive(uint8_t *data)
{
	TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
	if (TW_STATUS != TW_SR_DATA_ACK) return TW_STATUS;
	*data = TWDR;
	return 0;
}

uint8_t twi_slave_transmit(uint8_t data)
{
	TWDR = data;
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
	if (TW_STATUS != TW_ST_DATA_ACK) return TW_STATUS;
	return 0;
}
