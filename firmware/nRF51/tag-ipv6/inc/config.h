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

#define CONFIG_UART_BAUDRATE UART_BAUDRATE_BAUDRATE_Baud38400
#define CONFIG_UART_TXD_PIN  9
#ifdef  CONFIG_UART_RX
#define CONFIG_UART_RXD_PIN  8
#else
#define CONFIG_GPIO3_PIN     8
#endif/*CONFIG_UART_RX*/

#define CONFIG_LED_PIN       17
#define CONFIG_SWITCH_PIN    29

#endif/*__CONFIG_H__*/
