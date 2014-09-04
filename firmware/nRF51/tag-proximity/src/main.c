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
	uint8_t blink;
	uint32_t tag_id;

	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

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
	blink = 0;
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

	while(TRUE)
	{
		/* get tag angle once per second */
		acc_magnitude(&g_tag_angle);
		timer_wait(MILLISECONDS(1000));

#if CONFIG_FLASH_LOGGING
		flash_log_write_trigger();
		//flash_log_status();

		/* dump log data & status upon key press */
		if(nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
		{
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			timer_wait(MILLISECONDS(100));

			flash_log_flush(); /* flush log buffer to flash */
			flash_log_dump();
			flash_log_status();

			nrf_gpio_pin_clear(CONFIG_LED_PIN);
		}
#endif /* CONFIG_FLASH_LOGGING */

		/* blink every 5 seconds */
		if(blink<5)
			blink++;
		else
		{
			blink = 0;
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			timer_wait(MILLISECONDS(1));
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
		}
	}
}
