/***************************************************************
 *
 * OpenBeacon.org - nRF51 Random Number Generation
 *
 * Copyright 2014 Milosch Meriac <meriac@openbeacon.de>
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
#include <rng.h>

/* allow overriding RNG pool size */
#ifndef CONFIG_RNG_POOL_SIZE
#define CONFIG_RNG_POOL_SIZE 32
#endif/*CONFIG_RNG_POOL_SIZE*/

static uint8_t g_rng_pool[CONFIG_RNG_POOL_SIZE];
static uint16_t g_rng_pool_write, g_rng_pool_read;
static volatile uint16_t g_rng_pool_count;
static volatile uint8_t g_rng_pool_stopped;

void RNG_IRQ_Handler(void)
{
	if(NRF_RNG->EVENTS_VALRDY)
	{
		/* remember value */
		g_rng_pool[g_rng_pool_write++] = (uint8_t)NRF_RNG->VALUE;
		if(g_rng_pool_write==CONFIG_RNG_POOL_SIZE)
			g_rng_pool_write=0;

		/* disable once pool is filled */
		g_rng_pool_count++;
		if(g_rng_pool_count==CONFIG_RNG_POOL_SIZE)
			NRF_RNG->TASKS_STOP = 1;

		/* acknowledge event */
		NRF_RNG->EVENTS_VALRDY = 0;
	}
}

uint32_t rng(uint8_t bits)
{
	uint8_t bytes,*p;
	uint32_t data;

	if(!bits)
		return 0;
	if(bits>32)
		bits=32;

	/* calculate bytes and mask */
	bytes = (bits+7)/8;
	/* wait for pool to fill up */
	if(bytes>g_rng_pool_count)
	{
		/* start refilling pool */
		NRF_RNG->TASKS_START = 1;
		/* wait till minimum bytes were aquired */
		while(bytes>g_rng_pool_count)
			__WFI();
	}

	data = 0;
	p = (uint8_t*)&data;
	/* protect counter */
	__disable_irq();

	/* extract number of bytes needed */
	g_rng_pool_count-=bytes;
	while(bytes--)
	{
		*p++ = g_rng_pool[g_rng_pool_read++];
		if(g_rng_pool_read==CONFIG_RNG_POOL_SIZE)
			g_rng_pool_read=0;
	}

	/* leave protection */
	__enable_irq();

	/* refill pool */
	NRF_RNG->TASKS_START = 1;

	/* mask needed bytes */
	return data & ((1UL<<bits)-1);
}

void rng_init(void)
{
	g_rng_pool_write = g_rng_pool_read = g_rng_pool_count = 0;

	NRF_RNG->CONFIG = 0;
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->INTENSET = (
		(RNG_INTENSET_VALRDY_Enabled << RNG_INTENSET_VALRDY_Pos)
	);
	NVIC_SetPriority(RNG_IRQn, IRQ_PRIORITY_RNG);
	NVIC_EnableIRQ(RNG_IRQn);
}
