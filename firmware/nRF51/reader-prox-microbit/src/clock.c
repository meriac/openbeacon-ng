/***************************************************************
 *
 * OpenBeacon.org - nRF51 Clock Routines
 *
 * Copyright 2020 - Milosch Meriac <meriac@openbeacon.org>
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
#include <timer.h>
#include <clock.h>

static volatile uint32_t g_clock_time;

void RTC0_IRQ_Handler(void)
{
	if(NRF_RTC0->EVENTS_COMPARE[0])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		/* re-trigger in one second */
		NRF_RTC0->CC[0]+= LF_FREQUENCY;

		/* increment time */
		g_clock_time++;
	}
}

uint32_t clock_get(void)
{
	return g_clock_time;
}

void clock_init(void)
{
	/* reset wall time */
	g_clock_time = 0;

	/* setup radio timer */
	NRF_RTC0->TASKS_STOP = 1;
	NRF_RTC0->COUNTER = 0;
	NRF_RTC0->PRESCALER = 0;
	NRF_RTC0->CC[0] = LF_FREQUENCY;
	NRF_RTC0->CC[1] = 0;
	NRF_RTC0->CC[2] = 0;
	NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Enabled << RTC_INTENSET_COMPARE0_Pos;

	NVIC_SetPriority(RTC0_IRQn, IRQ_PRIORITY_RTC0);
	NVIC_EnableIRQ(RTC0_IRQn);
	NRF_RTC0->TASKS_START = 1;
}
