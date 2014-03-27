/***************************************************************
 *
 * OpenBeacon.org - nRF51 USART Serial Handler
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
#ifndef __UART_H__
#define __UART_H__

#ifndef CONFIG_UART_BUFFER
#define CONFIG_UART_BUFFER 128
#endif/*CONFIG_UART_BUFFER*/

#ifdef  CONFIG_UART_BAUDRATE
extern void uart_init(void);
extern BOOL uart_tx(uint8_t data);
extern int uart_rx(void);
#endif/*CONFIG_UART_BAUDRATE*/

#endif/*__UART_H__*/
