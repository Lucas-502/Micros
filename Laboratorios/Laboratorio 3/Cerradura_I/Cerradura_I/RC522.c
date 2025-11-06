/*
 * RFIDmain.c  (RC522 con tu UART)
 *
 * Created: 22/10/2025 18:36:47
 *  Author: lucas
 */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>       

#include "SPI.h"       // ? para spi_transfer()
#include "UART.h"     // ? tu librería UART (uartTxt/uartChar/...)
#include "RC522.h"

#ifndef RC522_C
#define RC522_C

static inline void uart_print(const char* s) { uartTxt(s); }
static inline void uart_print_hex(uint8_t b) {
    static const char HEX[] = "0123456789ABCDEF";
    uartChar(HEX[(b >> 4) & 0x0F]);
    uartChar(HEX[b & 0x0F]);
}
/* ----------------------------------------------------- */

void mfrc522_resetPinInit() {
    DDRB |= (1<<RST_PIN);
    PORTB &= ~(1<<RST_PIN);
    _delay_ms(10);
    PORTB |= (1<<RST_PIN);
    _delay_ms(50);
}

void mfrc522_write(uint8_t reg, uint8_t value) {
    SS_LOW();
    // pequeño margen tras CS bajo (algunos clones lo requieren)
    __asm__ __volatile__("nop\n\tnop\n\t");
    spi_transfer((reg<<1) & 0x7E);
    spi_transfer(value);
    SS_HIGH();
}

uint8_t mfrc522_read(uint8_t reg) {
    uint8_t val;
    SS_LOW();
    __asm__ __volatile__("nop\n\tnop\n\t");
    spi_transfer(((reg<<1)&0x7E) | 0x80);
    val = spi_transfer(0x00);
    SS_HIGH();
    return val;
}

void mfrc522_setBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read(reg);
    mfrc522_write(reg, tmp | mask);
}

void mfrc522_clearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read(reg);
    mfrc522_write(reg, tmp & (~mask));
}

void mfrc522_printRegister(const char* name, uint8_t reg) {
    uart_print(name);
    uart_print(": ");
    uart_print_hex(mfrc522_read(reg));
    uart_print("\r\n");
}


void mfrc522_reset() {
    mfrc522_write(CommandReg, (1<<4));
    _delay_ms(50);
}

void mfrc522_init() {
	mfrc522_reset();

	uart_print("Configurando temporizadores y modulacion...\r\n");
	mfrc522_write(TModeReg, 0x8D);
	mfrc522_write(TPrescalerReg, 0x3E);
	mfrc522_write(TReloadRegL, 30);
	mfrc522_write(TReloadRegH, 0);

	mfrc522_write(TxASKReg, 0x40);
	mfrc522_write(ModeReg,  0x3D);

	mfrc522_write(RFCfgReg, 0x48);            // ganancia media (más estable)

	mfrc522_setBitMask(TxControlReg, 0x03);   // Antena ON
	_delay_ms(5);
}


void mfrc522_debug_init() {
    mfrc522_reset();
    mfrc522_write(TModeReg, 0x8D);
    mfrc522_write(TPrescalerReg, 0x3E);
    mfrc522_write(TReloadRegL, 30);
    mfrc522_write(TReloadRegH, 0);

    // Configuracion de la modulacion
    mfrc522_write(TxASKReg, 0x40);
    mfrc522_write(ModeReg, 0x3D);

    // Configurar ganancia del receptor
    mfrc522_write(RFCfgReg, 0x70);

    // Activar la antena
    mfrc522_write(TxControlReg, 0x03);
    _delay_ms(5);
}

void mfrc522_debug_REQA() {
    uint8_t req[1] = {PICC_REQIDL};
    uint8_t buffer[16];
    uint8_t bufferLength = sizeof(buffer);
    uint8_t backBits = 0; (void)backBits;   // evitar warning
    uint8_t status    = 0; (void)status;    // evitar warning

    // Preparar registro de bits y FIFO
    mfrc522_write(BitFramingReg, 0x07); // 7 bits para REQA
    mfrc522_write(CommIrqReg, 0x7F);    // Limpiar IRQ
    mfrc522_write(FIFOLevelReg, 0x80);  // Limpiar FIFO

    uart_print("\r\n=== Enviando REQA ===\r\n");

    // Escribir FIFO
    for (uint8_t i=0; i<1; i++) {
        mfrc522_write(FIFODataReg, req[i]);
    }

    // Iniciar transaccion
    mfrc522_write(CommandReg, PCD_TRANSCEIVE);
    mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend

    // Esperar IRQ (RxIRq o Timeout)
    uint16_t count = 1000; // Reducir el tiempo de espera si no anda
    uint8_t irq;
    do {
        irq = mfrc522_read(CommIrqReg);
        count--;
    } while (!(irq & 0x30) && count); // RxIRq o IdleIRq

    mfrc522_clearBitMask(BitFramingReg, 0x80);



    if (count == 0) {
        uart_print("Timeout REQA, tarjeta no detectada\r\n");
    } else {
        uint8_t fifoLevel = mfrc522_read(FIFOLevelReg);
        for (uint8_t i=0; i<fifoLevel; i++) {
            uint8_t val = mfrc522_read(FIFODataReg);
            if (i < bufferLength) buffer[i] = val;
        }

        // Si hay datos, intentar leer el UID
        if (fifoLevel > 0) {
            uart_print("Tarjeta detectada! Intentando leer UID...\r\n");

            // Anticollision
            mfrc522_write(BitFramingReg, 0x00);
            mfrc522_write(CommIrqReg, 0x7F);
            mfrc522_write(FIFOLevelReg, 0x80);

            // Escribir comando Anticollision en FIFO
            mfrc522_write(FIFODataReg, PICC_ANTICOLL);
            mfrc522_write(FIFODataReg, 0x20); // NVB

            // Iniciar transaccion
            mfrc522_write(CommandReg, PCD_TRANSCEIVE);
            mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend

            // Esperar IRQ
            count = 1000;
            do {
                irq = mfrc522_read(CommIrqReg);
                count--;
            } while (!(irq & 0x30) && count);

            mfrc522_clearBitMask(BitFramingReg, 0x80);

            if (count == 0) {
                uart_print("Timeout Anticollision\r\n");
            } else {
                fifoLevel = mfrc522_read(FIFOLevelReg);
                uart_print("UID leido: ");
                for (uint8_t i=0; i<fifoLevel; i++) {
                    uint8_t val = mfrc522_read(FIFODataReg);
                    uart_print_hex(val);
                    uart_print(" ");
                }
                uart_print("\r\n");
            }
        }
    }
}

uint8_t mfrc522_detectar(uint8_t *uid, uint8_t *uid_len)
{
	// --- REQA (7 bits) ---
	mfrc522_write(BitFramingReg, 0x07);
	mfrc522_write(CommIrqReg,    0x7F);
	mfrc522_write(FIFOLevelReg,  0x80);
	mfrc522_write(FIFODataReg, PICC_REQIDL);

	mfrc522_write(CommandReg, PCD_TRANSCEIVE);
	mfrc522_setBitMask(BitFramingReg, 0x80);  // StartSend

	uint16_t to=2000; uint8_t irq;
	do { irq = mfrc522_read(CommIrqReg); } while(!(irq & 0x30) && --to);
	mfrc522_clearBitMask(BitFramingReg, 0x80);

	if (!to) return 0;                      // timeout
	if (!(irq & 0x20)) return 0;            // debe ser RxIRq

	uint8_t fifo = mfrc522_read(FIFOLevelReg);
	if (fifo != 2) return 0;                // ATQA = 2 bytes exactos

	// leer ATQA (y descartarlo)
	(void)mfrc522_read(FIFODataReg);
	(void)mfrc522_read(FIFODataReg);

	// --- ANTICOLL Nivel 1 ---
	mfrc522_write(BitFramingReg, 0x00);
	mfrc522_write(CommIrqReg,    0x7F);
	mfrc522_write(FIFOLevelReg,  0x80);
	mfrc522_write(FIFODataReg, PICC_ANTICOLL);
	mfrc522_write(FIFODataReg, 0x20);       // NVB

	mfrc522_write(CommandReg, PCD_TRANSCEIVE);
	mfrc522_setBitMask(BitFramingReg, 0x80);

	to=2000;
	do { irq = mfrc522_read(CommIrqReg); } while(!(irq & 0x30) && --to);
	mfrc522_clearBitMask(BitFramingReg, 0x80);
	if (!to) return 0;

	fifo = mfrc522_read(FIFOLevelReg);
	if (fifo < 5) return 0;                 // UID[4]+BCC mínimo

	// Leer 5 bytes: 4 UID + 1 BCC
	uint8_t tmp[5];
	for (uint8_t i=0; i<5; i++) tmp[i] = mfrc522_read(FIFODataReg);

	// Verificar BCC: XOR de UID[0..3] debe igualar tmp[4]
	uint8_t bcc = tmp[0] ^ tmp[1] ^ tmp[2] ^ tmp[3];
	if (bcc != tmp[4]) return 0;

	// OK ? devolver UID (4 bytes) y longitud
	for (uint8_t i=0; i<4; i++) uid[i] = tmp[i];
	if (uid_len) *uid_len = 4;
	return 1;
}


void mfrc522_standard(uint8_t *card_uid) {
    uint8_t req[1] = {PICC_REQIDL};
    uint8_t buffer[16];
    uint8_t bufferLength = sizeof(buffer);
    uint8_t backBits = 0; (void)backBits;  // evitar warning
    uint8_t status    = 0; (void)status;   // evitar warning

    // Preparar registro de bits y FIFO
    mfrc522_write(BitFramingReg, 0x07); // 7 bits para REQA
    mfrc522_write(CommIrqReg, 0x7F);    // Limpiar IRQ
    mfrc522_write(FIFOLevelReg, 0x80);  // Limpiar FIFO

    // uart_print("\r\n=== Enviando REQA ===\r\n");

    // Escribir FIFO
    mfrc522_write(FIFODataReg, req[0]);

    // Iniciar transaccion
    mfrc522_write(CommandReg, PCD_TRANSCEIVE);
    mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend

    // Esperar IRQ (RxIRq o Timeout)
    uint16_t count = 1000; // Reducir el tiempo de espera
    uint8_t irq;
    do {
        irq = mfrc522_read(CommIrqReg);
        count--;
    } while (!(irq & 0x30) && count); // RxIRq o IdleIRq

    mfrc522_clearBitMask(BitFramingReg, 0x80);

    if (count == 0) {
        uart_print("Timeout REQA, tarjeta no detectada\r\n");
        memset(card_uid, 0, 16);
    } else {
        uint8_t fifoLevel = mfrc522_read(FIFOLevelReg);
        for (uint8_t i=0; i<fifoLevel; i++) {
            uint8_t val = mfrc522_read(FIFODataReg);
            if (i < bufferLength) buffer[i] = val;
        }

        // Si hay datos, intentar leer el UID
        if (fifoLevel > 0) {
            uart_print("Tarjeta detectada! Intentando leer UID...\r\n");

            // Anticollision
            mfrc522_write(BitFramingReg, 0x00);
            mfrc522_write(CommIrqReg, 0x7F);
            mfrc522_write(FIFOLevelReg, 0x80);

            // Escribir comando Anticollision en FIFO
            mfrc522_write(FIFODataReg, PICC_ANTICOLL);
            mfrc522_write(FIFODataReg, 0x20); // NVB

            // Iniciar transaccion
            mfrc522_write(CommandReg, PCD_TRANSCEIVE);
            mfrc522_setBitMask(BitFramingReg, 0x80); // StartSend

            // Esperar IRQ
            count = 1000;
            do {
                irq = mfrc522_read(CommIrqReg);
                count--;
            } while (!(irq & 0x30) && count);

            mfrc522_clearBitMask(BitFramingReg, 0x80);

            if (count == 0) {
                uart_print("Timeout Anticollision\r\n");
            } else {
                fifoLevel = mfrc522_read(FIFOLevelReg);
                uart_print("UID leido!");
                for (uint8_t i=0; i<fifoLevel; i++) {
                    uint8_t val = mfrc522_read(FIFODataReg);
                    // uart_print_hex(val);
                    // uart_print(" ");
                    card_uid[i] = val;
                }
                uart_print("\r\n");
            }
        }
    }
}

#endif
