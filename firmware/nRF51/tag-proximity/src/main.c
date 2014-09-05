/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
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
#include <acc.h>
#include <flash.h>
#include <radio.h>
#include <timer.h>

#if CONFIG_FLASH_LOGGING
#include <log.h>
#endif

uint8_t hibernate = 1;
static int8_t g_tag_angle;


int8_t tag_angle(void)
{
	return g_tag_angle;
}

void blink(uint8_t times)
{
	while(times--)
	{
		nrf_gpio_pin_set(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(10));
		nrf_gpio_pin_clear(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(490));
	}
}

void blink_fast(uint8_t times)
{
	while(times--)
	{
		nrf_gpio_pin_set(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(10));
		nrf_gpio_pin_clear(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(90));
	}
}

uint16_t blink_wait_release(void) {
	uint16_t counter = 0;
	const uint8_t delta_t = 10;

	nrf_gpio_pin_set(CONFIG_LED_PIN);
	timer_wait(MILLISECONDS(10));
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

	while (nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
	{
		timer_wait(MILLISECONDS(delta_t));
		counter++;
	}

	return counter * delta_t;
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
	uint32_t tag_id;
	uint8_t blink_counter = 0;
	uint16_t keypress_duration;

	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_NOPULL);

	/* initialize UART */
	uart_init();

	/* start timer */
	timer_init();

	/* calculate tag ID from NRF_FICR->DEVICEID */
	tag_id = crc32(&NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));

	/* initialize flash */
	if(flash_init())
		halt(2);

#if CONFIG_FLASH_LOGGING
	flash_setup_logging(tag_id);
#endif

	/* initialize accelerometer */
	if(acc_init())
		halt(3);

	/* start radio */
	debug_printf("\n\rInitializing Tag[%08X] v" PROGRAM_VERSION " @24%02iMHz ...\n\r",
		tag_id,
		CONFIG_TRACKER_CHANNEL);
	radio_init(tag_id);

	/* enter main loop */

	while(TRUE)
	{
		if (!hibernate)
		{
			/* get tag angle once per second */
			acc_magnitude(&g_tag_angle);

			/* trigger flash write */
#if CONFIG_FLASH_LOGGING
			flash_log_write_trigger();
			//flash_log_status();
#endif
		}

		/* key press */
		if(nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
		{
			keypress_duration = blink_wait_release();

			/* long key press while tag is hibernating triggers log dump */
			if (hibernate && (keypress_duration > 2000))
			{
#if CONFIG_FLASH_LOGGING
				blink_fast(10);

				/* dump log data & status to serial */
				flash_log_dump();
				flash_log_status();
#endif /* CONFIG_FLASH_LOGGING */
			} else if (keypress_duration > 500)
			{
			/* short key press toggle hibernation */
				hibernate ^= 1;

				/* if hibernating, flush log buffers to flash */
				if (hibernate)
					flash_log_flush();

				blink_fast(hibernate ? 3 : 6);
				debug_printf("\n\rhibernate -> %i", hibernate);
			}
		}

		timer_wait(MILLISECONDS(1000));

		/* blink every 5 seconds, unless hibernating */
		if( ((!hibernate) && blink_counter<5) || (hibernate && blink_counter<12) )
			blink_counter++;
		else
		{
			blink_counter = 0;
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			timer_wait(MILLISECONDS(1));
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
		}
	}
}
