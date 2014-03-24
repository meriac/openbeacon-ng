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
#include <openbeacon.h>
#include "uart.h"

#ifdef  CONFIG_UART_BAUDRATE

/* allow to override default putchar output from serial to something else */
BOOL default_putchar (uint8_t data) ALIAS(uart_tx);

void uart_init(void)
{
#ifdef  CONFIG_UART_RXD_PIN
	nrf_gpio_cfg_input(CONFIG_UART_RXD_PIN, NRF_GPIO_PIN_NOPULL);
	NRF_UART0->PSELRXD = CONFIG_UART_RXD_PIN;
#endif/*CONFIG_UART_RXD_PIN*/

#ifdef  CONFIG_UART_TXD_PIN
	nrf_gpio_cfg_output(CONFIG_UART_TXD_PIN);
	NRF_UART0->PSELTXD = CONFIG_UART_TXD_PIN;
#endif/*CONFIG_UART_TXD_PIN*/

	/* Optionally enable UART flow control */
#if defined(CONFIG_UART_RTS_PIN) || defined(CONFIG_UART_CTS_PIN)

	/* Clear-To-Send */
	nrf_gpio_cfg_input(CONFIG_UART_CTS_PIN, NRF_GPIO_PIN_NOPULL);
	NRF_UART0->PSELCTS = CONFIG_UART_CTS_PIN;

	/* Ready-To-Send */
	nrf_gpio_cfg_output(CONFIG_UART_RTS_PIN);
	NRF_UART0->PSELRTS = CONFIG_UART_RTS_PIN;

	/* enable hardware flow control */
	NRF_UART0->CONFIG = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
#endif/* flow control */

	/* set baud rate */
	NRF_UART0->BAUDRATE = (CONFIG_UART_BAUDRATE << UART_BAUDRATE_BAUDRATE_Pos);

	/* start UART */
	NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
#ifdef  CONFIG_UART_TXD_PIN
	NRF_UART0->TASKS_STARTTX = 1;
#endif/*CONFIG_UART_TXD_PIN*/
#ifdef  CONFIG_UART_RXD_PIN
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
#endif/*CONFIG_UART_RXD_PIN*/
}

#ifdef  CONFIG_UART_TXD_PIN
BOOL uart_tx(uint8_t data)
{
	NRF_UART0->TXD = data;
	/* wait for TX */
	while (!NRF_UART0->EVENTS_TXDRDY);
	/* reset TX event */
	NRF_UART0->EVENTS_TXDRDY = 0;

	return TRUE;
}
#endif/*CONFIG_UART_TXD_PIN*/

#ifdef  CONFIG_UART_RXD_PIN
int uart_rx(void)
{
	return -1;
}
#endif/*CONFIG_UART_RXD_PIN*/

#endif/*CONFIG_UART_BAUDRATE*/

