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
#include <timer.h>

extern const uint8_t audio_start, audio_end;
uint32_t audio_len;

static int8_t g_tag_angle;

int8_t tag_angle(void)
{
	return g_tag_angle;
}

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

void main_entry(void)
{
	/* disabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_PULLUP);

	/* start timer */
	timer_init();

	/* determine audio length */
	audio_len = &audio_end - &audio_start;

	/* enter main loop */
	while(TRUE)
	{
		blink(1);
		timer_wait(SECONDS(1));
	}
}
