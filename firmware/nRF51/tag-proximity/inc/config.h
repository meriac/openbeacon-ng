/***************************************************************
 *
 * OpenBeacon.org - nRF51 board config files
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

 This file is part of the OpenBeacon.org active RFID firmware

 OpenBeacon is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenBeacon is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef __CONFIG_H__
#define __CONFIG_H__

/* log to flash */
#define CONFIG_FLASH_LOGGING        1

/* blink on receiving a proximity packet */
#define CONFIG_PROXIMITY_BLINK      0

/* if epoch is invalid, pick up time from proximity packet */
#define CONFIG_EPOCH_PROXIMITY      1

/* double blink on invalid epoch time */
#define CONFIG_EPOCH_INVALID_BLINK  1

/* every CONFIG_PROX_SPACING-RANDOM(2^CONFIG_PROX_SPACING_RNG_BITS)
 * listen for CONFIG_PROX_LISTEN - all based on LF_FREQUENCY ticks */
#define CONFIG_PROX_SPACING_RNG_BITS 9
#define CONFIG_PROX_SPACING MILLISECONDS(25)
#define CONFIG_PROX_LISTEN_RATIO 10
#define CONFIG_PROX_LISTEN MILLISECONDS(5)

#define CONFIG_UART_BAUDRATE UART_BAUDRATE_BAUDRATE_Baud115200
#define CONFIG_UART_TXD_PIN  9
#ifdef  CONFIG_UART_RX
#define CONFIG_UART_RXD_PIN  8
#else
#define CONFIG_GPIO3_PIN     8
#endif/*CONFIG_UART_RX*/

#define CONFIG_FLASH_MISO    16
#define CONFIG_FLASH_MOSI    15
#define CONFIG_FLASH_SCK     14
#define CONFIG_FLASH_nRESET  13
#define CONFIG_FLASH_nCS     12
#define SPI_FLASH            NRF_SPI0

#define CONFIG_ADC0          1
#define CONFIG_ADC1          2

#define CONFIG_ACC_INT1_CH   0
#define CONFIG_ACC_INT1      3
#define CONFIG_ACC_nCS       4
#define CONFIG_ACC_MISO      5
#define CONFIG_ACC_MOSI      6
#define CONFIG_ACC_SCK       7
#define SPI_ACC              NRF_SPI1

#define CONFIG_LED_PIN       17
#define CONFIG_SWITCH_PIN    29

/* only two priority bits available ! */

#define IRQ_PRIORITY_HIGH        0
#define IRQ_PRIORITY_AES         (IRQ_PRIORITY_HIGH)
#define IRQ_PRIORITY_RNG         (IRQ_PRIORITY_HIGH+1)
#define IRQ_PRIORITY_POWER_CLOCK (IRQ_PRIORITY_HIGH+1)
#define IRQ_PRIORITY_RADIO       (IRQ_PRIORITY_HIGH+2)

#define IRQ_PRIORITY_LOW         (IRQ_PRIORITY_HIGH+3)
#define IRQ_PRIORITY_RTC0        (IRQ_PRIORITY_LOW)
#define IRQ_PRIORITY_RTC1        (IRQ_PRIORITY_LOW)
#define IRQ_PRIORITY_ADC         (IRQ_PRIORITY_LOW)
#define IRQ_PRIORITY_UART0       (IRQ_PRIORITY_LOW)

#endif/*__CONFIG_H__*/
