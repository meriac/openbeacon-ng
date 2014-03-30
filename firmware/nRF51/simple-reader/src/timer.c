/***************************************************************
 *
 * OpenBeacon.org - nRF51 Timer Routines
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
#include <timer.h>

static volatile uint8_t g_timer_wait;

void timer_wait(uint32_t ticks)
{
	g_timer_wait = TRUE;

	/* configure timeout */
	NRF_RTC1->CC[0] = ticks;
	/* start timer */
	NRF_RTC1->TASKS_START = 1;

	while(g_timer_wait)
		__WFI();
}

void RTC1_IRQ_Handler(void)
{
	/* allow wait loop to exit */
	g_timer_wait = FALSE;

	/* stop timer */
	NRF_RTC1->TASKS_STOP = 1;
	NRF_RTC1->TASKS_CLEAR = 1;
	/* acknowledge interrupt */
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
}

void timer_init(void)
{
	/* start 32kHz crystal oscillator */
	NRF_CLOCK->LFCLKSRC =
		(CLOCK_LFCLKSRCCOPY_SRC_Xtal << CLOCK_LFCLKSRCCOPY_SRC_Pos);
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);

	/* setup delay routine */
	NRF_RTC1->COUNTER = 0;
	NRF_RTC1->PRESCALER = 0;
	NRF_RTC1->TASKS_STOP = 1;
	NRF_RTC1->INTENSET =
		(RTC_INTENSET_COMPARE0_Enabled << RTC_INTENSET_COMPARE0_Pos);
	NVIC_EnableIRQ(RTC1_IRQn);
}
