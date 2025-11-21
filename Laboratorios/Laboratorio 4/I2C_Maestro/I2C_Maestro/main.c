/*
 * main.c
 *
 * Created: 11/20/2025 6:08:00 PM
 *  Author: lucas
 */ 
// main_maestro.c - Maestro I2C
// DHT11 + LCD + Pot + Botón
// Envía al esclavo: servo_val (0..255) y comando de canción (bit0)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "twi.h"
#include "twi_lcd.h"
#include "dht11.h"

// Dirección del esclavo (la misma que usaste en el esclavo)
#define SLAVE_ADDR          0x20

// Bit de comando para pedir al esclavo que toque la canción 1
#define CMD_PLAY_SONG_BIT   0x01

// ---------------- Prototipos ----------------
static void    adc_init(void);
static uint16_t adc_read_0(void);

static void   button_init(void);
static uint8_t button_read_logico(void); // 1 = apretado (GND), 0 = suelto

static uint8_t i2c_send_servo_and_cmd(uint8_t servo_val, uint8_t cmd);

// ---------------- main ----------------

int main(void)
{
    uint8_t temperatura = 0;
    uint8_t humedad = 0;
    uint8_t dht_error = 0;

    uint16_t pot_raw = 0;
    uint16_t pot_display = 0;    // valor solo para mostrar en LCD

    uint8_t  servo_val = 0;      // 0..255 para el esclavo

    uint8_t button = 0;          // 1 = apretado, 0 = suelto
    uint8_t prev_button = 0;
    uint8_t play_flag = 0;

    uint8_t dht_counter = 0;     // para leer DHT11 ~cada 1s
    uint8_t lcd_counter = 0;     // para refrescar LCD cada ~200 ms
    uint8_t i2c_status = 0;

    char linea1[17];
    char linea2[17];

    // --- Inicialización de hardware ---
    twi_master_init();   // Maestro I2C
    twi_lcd_init();      // LCD 16x2 I2C
    dht11_init();        // DHT11 en PD2
    adc_init();          // Potenciómetro en ADC0 (PC0 / A0)
    button_init();       // Botón en PD3 (a GND, con pull-up interno)

    lcd_2lineas("Maestro I2C", "DHT+POT+BTN");
    _delay_ms(1000);

    while (1)
    {
        // 1) Leer DHT11 cada ~1 segundo
        //    dht_counter usa loop de 20 ms -> 50 * 20 ms = 1000 ms
        if (dht_counter == 0)
        {
            dht_error = dht11_read(&temperatura, &humedad);
            dht_counter = 50;
        }
        else
        {
            dht_counter--;
        }

        // 2) Leer pot SIN filtro (que oscile lo que quiera)
        pot_raw = adc_read_0();    // 0..1023 idealmente
        if (pot_raw > 1023) pot_raw = 1023;

        // Mapear 0..1023 -> 0..255 para el servo del esclavo
        servo_val = (uint32_t)pot_raw * 255 / 1023;

        // 3) Leer botón y detectar flanco de subida (0 -> 1)
        button = button_read_logico();   // 1 = apretado (pin en GND), 0 = suelto

        play_flag = 0;
        if (button && !prev_button)
        {
            // Se acaba de apretar el botón -> pedimos canción
            play_flag = 1;
        }
        prev_button = button;

        // 4) Enviar por I2C al esclavo: servo_val y cmd (bit0 = play)
        i2c_status = i2c_send_servo_and_cmd(
                         servo_val,
                         play_flag ? CMD_PLAY_SONG_BIT : 0x00
                     );

        // 5) Actualizar LCD SOLO cada ~200 ms
        if (lcd_counter == 0)
        {
            // Línea 1: DHT11
            if (dht_error == 0)
            {
                snprintf(linea1, sizeof(linea1),
                         "T:%2uC H:%2u%%", temperatura, humedad);
            }
            else
            {
                snprintf(linea1, sizeof(linea1),
                         "DHT ERR:%u", dht_error);
            }

            // Línea 2: Pot + Botón o error I2C
            if (i2c_status == 0)
            {
                // Para la pantalla, si estamos muy cerca del máximo,
                // mostramos 1023 "a mano"
                pot_display = pot_raw;
                if (pot_display > 1015)
                    pot_display = 1023;

                snprintf(linea2, sizeof(linea2),
                         "P:%4u B:%s", pot_display, (button ? "ON " : "OFF"));
            }
            else
            {
                snprintf(linea2, sizeof(linea2),
                         "I2C ERR:%3u", i2c_status);
            }

            linea1[16] = '\0';
            linea2[16] = '\0';

            lcd_2lineas(linea1, linea2);

            lcd_counter = 10;   // 10 * 20 ms = 200 ms
        }
        else
        {
            lcd_counter--;
        }

        // 6) Loop rápido (~20ms) 
        _delay_ms(20);
    }

    return 0;
}

// ---------------- ADC: potenciómetro en ADC0 (PC0/A0) ----------------

static void adc_init(void)
{
    // Referencia AVcc, canal ADC0
    ADMUX = (1 << REFS0);   // REFS0=1 (AVcc), MUX=0000 (ADC0)

    // Habilitar ADC, prescaler 128 -> 16MHz/128 = 125kHz
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static uint16_t adc_read_0(void)
{
    // Asegurar canal 0
    ADMUX &= 0xF0;  // limpia MUX[3:0]

    // Iniciar conversión
    ADCSRA |= (1 << ADSC);

    // Esperar fin de conversión
    while (ADCSRA & (1 << ADSC));

    return ADC;   
}

// ---------------- Botón en PD3, a GND con pull-up ----------------

static void button_init(void)
{
    // PD3 como entrada, con pull-up interno
    DDRD  &= ~(1 << PD3);   // entrada
    PORTD |=  (1 << PD3);   // pull-up interno ON
}

// Devuelve 1 = apretado (pin en GND), 0 = suelto
static uint8_t button_read_logico(void)
{
    uint8_t raw = (PIND & (1 << PD3)) ? 1 : 0; // HIGH=1, LOW=0
    return (raw == 0) ? 1 : 0;                 // invertimos: GND -> 1
}

// ---------------- I2C: enviar servo y comando al esclavo ----------------

static uint8_t i2c_send_servo_and_cmd(uint8_t servo_val, uint8_t cmd)
{
    uint8_t status;

    // START
    status = twi_start();
    if (status != 0) {
        twi_stop();
        return status;
    }

    // SLA+W (bit R/W = 0)
    status = twi_write_address((SLAVE_ADDR << 1) | TWI_WRITE);
    if (status != 0) {
        twi_stop();
        return status;
    }

    // Byte 0: servo_val
    status = twi_write_data(servo_val);
    if (status != 0) {
        twi_stop();
        return status;
    }

    // Byte 1: cmd (bit0 puede ser CMD_PLAY_SONG_BIT)
    status = twi_write_data(cmd);
    if (status != 0) {
        twi_stop();
        return status;
    }

    // STOP
    twi_stop();
    return 0;
}
