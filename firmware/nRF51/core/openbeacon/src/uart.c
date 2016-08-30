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

#ifndef CONFIG_UART_FORCE_POWERED
#define CONFIG_UART_FORCE_POWERED 0
#endif/*CONFIG_UART_FORCE_POWERED*/

static uint8_t g_uart_buffer[CONFIG_UART_BUFFER];
static uint16_t g_uart_buffer_head, g_uart_buffer_tail;
static volatile uint16_t g_uart_buffer_count;

/* allow to override default putchar output from serial to something else */
BOOL default_putchar (uint8_t data) ALIAS(uart_tx);

void uart_init(void)
{
	g_uart_buffer_count = 0;
	g_uart_buffer_head = g_uart_buffer_tail = 0;

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

	/* enable UART IRQ */
#ifdef  IRQ_PRIORITY_UART0
	NVIC_SetPriority(UART0_IRQn, IRQ_PRIORITY_UART0);
#endif
	NVIC_EnableIRQ(UART0_IRQn);
	NRF_UART0->INTENSET =
		(UART_INTENSET_TXDRDY_Enabled << UART_INTENSET_TXDRDY_Pos);

	/* start UART */
#ifdef  CONFIG_UART_RXD_PIN
	NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
#else
	NRF_UART0->ENABLE = CONFIG_UART_FORCE_POWERED ? (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos) : 0;
#endif/*CONFIG_UART_RXD_PIN*/
}

#if CONFIG_UART_FORCE_POWERED
void uart_power(uint8_t enable)
{
	/* if disabling - wait for ongoing UART transmissions to finish */
	if(!enable)
		while(g_uart_buffer_count)
			__WFE();

	/* enable/disable USART */
	NRF_UART0->ENABLE = enable ? (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos) : 0;
}
#endif/*CONFIG_UART_FORCE_POWERED*/

#ifdef  CONFIG_UART_TXD_PIN
BOOL uart_tx(uint8_t data)
{
	/* wait for buffer */
	while(g_uart_buffer_count==CONFIG_UART_BUFFER)
		__WFE();

	/* temporarily disable IRQs ... */
	__disable_irq();

	/* ...and push data */
	if(g_uart_buffer_count)
	{
		g_uart_buffer[g_uart_buffer_head++] = data;
		if(g_uart_buffer_head == CONFIG_UART_BUFFER)
			g_uart_buffer_head = 0;
	}
	else
	{
		/* enable UART for sending out first byte */
#if !CONFIG_UART_FORCE_POWERED
		NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
#endif/*CONFIG_UART_FORCE_POWERED*/
		NRF_UART0->TASKS_STARTTX = 1;
		NRF_UART0->TXD = data;
	}
	g_uart_buffer_count++;

	/* enable IRQs again */
	__enable_irq();

	return TRUE;
}
#endif/*CONFIG_UART_TXD_PIN*/

#ifdef  CONFIG_UART_RXD_PIN
int uart_rx(void)
{
	return -1;
}
#endif/*CONFIG_UART_RXD_PIN*/

void UART0_IRQ_Handler(void)
{
	if(NRF_UART0->EVENTS_TXDRDY)
	{
		NRF_UART0->EVENTS_TXDRDY = 0;
		g_uart_buffer_count--;
		if(!g_uart_buffer_count)
		{
			NRF_UART0->TASKS_STOPTX = 1;
#if !CONFIG_UART_FORCE_POWERED
			NRF_UART0->ENABLE = 0;
#endif
		}
		else
		{
			NRF_UART0->TXD = g_uart_buffer[g_uart_buffer_tail++];
			if(g_uart_buffer_tail == CONFIG_UART_BUFFER)
				g_uart_buffer_tail = 0;
		}
	}
}

#endif/*CONFIG_UART_BAUDRATE*/

