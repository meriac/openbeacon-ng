/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
 *
 * Copyright 2015 Milosch Meriac <milosch@meriac.com>
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
#include <math.h>
#include <timer.h>

#define SAMPLE_OVS 2
#define CLOCK_DIVIDER (16000000UL / (SAMPLING_RATE*SAMPLE_OVS))

static uint32_t g_seq_counter;

extern const uint8_t audio_start, audio_end;
const uint8_t *g_audio;
uint32_t g_buffer[SAMPLE_OVS];
uint32_t g_buffer_pos, g_buffer_ovs;

void blink(uint8_t times)
{
	while(times--)
	{
		nrf_gpio_pin_clear(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(10));
		nrf_gpio_pin_set(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(490));
	}
}

void halt(uint8_t times)
{
	while(TRUE)
	{
		blink(times);
		timer_wait(SECONDS(3));
	}
}

void SOUND_IRQ_Handler(void)
{
	uint32_t next, data;

	if(SOUND->EVENTS_COMPARE[2])
	{
		SOUND->EVENTS_COMPARE[2] = 0;

		next = SOUND->CC[2] + CLOCK_DIVIDER;
		SOUND->CC[2] = next;

		data = (*g_audio)*4;
		g_buffer_ovs -= g_buffer[g_buffer_pos];
		g_buffer_ovs += data;
		g_buffer[g_buffer_pos] = data;
		g_buffer_pos++;
		if(g_buffer_pos >= SAMPLE_OVS)
		{
			g_buffer_pos = 0;

			g_audio++;
			if(g_audio >= &audio_end)
				g_audio = &audio_start;
		}

		SOUND->CC[g_seq_counter & 1] = next + (g_buffer_ovs / SAMPLE_OVS);
		g_seq_counter++;
	}
}

static void sound_init(void)
{
	g_audio=&audio_start;

	/* setup sound output */
	SOUND->TASKS_CLEAR = 1;
	SOUND->MODE = TIMER_MODE_MODE_Timer;
	SOUND->PRESCALER = 0;
	SOUND->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	SOUND->CC[0] = 0;
	SOUND->CC[1] = 0;
	SOUND->CC[2] = CLOCK_DIVIDER;
	SOUND->INTENSET = TIMER_INTENSET_COMPARE2_Msk;
	SOUND->SHORTS = 0;

	/* update driver strength */
	NRF_GPIO->PIN_CNF[CONFIG_PWM_PIN_A] = NRF_GPIO->PIN_CNF[CONFIG_PWM_PIN_B] =
		(GPIO_PIN_CNF_DIR_Output       << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos);

	/* create tasks for both GPIO pins */
	nrf_gpiote_task_config(0, CONFIG_PWM_PIN_A,
		GPIOTE_CONFIG_POLARITY_Toggle, GPIOTE_CONFIG_OUTINIT_Low);
	nrf_gpiote_task_config(1, CONFIG_PWM_PIN_B,
		GPIOTE_CONFIG_POLARITY_Toggle, GPIOTE_CONFIG_OUTINIT_High);

	/* wire up PWM to output pins */
	NRF_PPI->CHENCLR = 0x1F;
	NRF_PPI->CH[0].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[0];
	NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
	NRF_PPI->CH[1].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[1];
	NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];

	NRF_PPI->CH[2].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[0];
	NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];
	NRF_PPI->CH[3].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[1];
	NRF_PPI->CH[3].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

	NRF_PPI->CH[4].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[2];
	NRF_PPI->CH[4].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
	NRF_PPI->CH[5].EEP = (uint32_t)&SOUND->EVENTS_COMPARE[2];
	NRF_PPI->CH[5].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

	NRF_PPI->CHEN = 0x3F;
	NRF_PPI->CHG[0] = 0x3F;
	NRF_PPI->TASKS_CHG[0].EN = 1;

	NVIC_SetPriority(SOUND_IRQn, IRQ_PRIORITY_SOUND);
	NVIC_EnableIRQ(SOUND_IRQn);

	/* start audio handling */
	SOUND->TASKS_START = 1;
}

void main_entry(void)
{
	/* disabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_PULLUP);

	/* start timer */
	timer_init();

	/* init sound driver */
	sound_init();

	/* enter main loop */
	while(TRUE)
	{
		blink(1);
		timer_wait(SECONDS(1));
	}
}
