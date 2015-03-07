/***************************************************************
 *
 * OpenBeacon.org - nRF51 board config files
 *
 * Copyright 2013-2015 Milosch Meriac <meriac@openbeacon.org>
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

#define CONFIG_UART_BAUDRATE UART_BAUDRATE_BAUDRATE_Baud1M
#define CONFIG_UART_FORCE_POWERED 1
#define CONFIG_UART_TXD_PIN  9
#ifdef  CONFIG_UART_RX
#define CONFIG_UART_RXD_PIN  8
#else
#define CONFIG_GPIO3_PIN     8
#endif/*CONFIG_UART_RX*/

#define CONFIG_LED_PIN       17
#define CONFIG_SWITCH_PIN    29

/* only two priority bits available ! */

#define IRQ_PRIORITY_HIGH        0
#define IRQ_PRIORITY_RADIO       (IRQ_PRIORITY_HIGH)
#define IRQ_PRIORITY_UART0       (IRQ_PRIORITY_HIGH+1)
#define IRQ_PRIORITY_POWER_CLOCK (IRQ_PRIORITY_HIGH+2)

#define IRQ_PRIORITY_LOW         (IRQ_PRIORITY_HIGH+3)
#define IRQ_PRIORITY_RTC0        (IRQ_PRIORITY_LOW)
#define IRQ_PRIORITY_RTC1        (IRQ_PRIORITY_LOW)

#endif/*__CONFIG_H__*/
