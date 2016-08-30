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
#include <adc.h>
#include <flash.h>
#include <radio.h>
#include <timer.h>

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

static void run_mode_beacon(uint32_t tag_id)
{
	uint8_t blink;

	/* start radio */
	radio_init(tag_id);
	debug_printf("- Start radio\n\r");

	/* power down USART */
	uart_power(FALSE);

	/* turn off LED to indicate operation */
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

	/* enter main loop */
	blink = 0;
	while(TRUE)
	{
		/* get tag angle once per second */
		acc_magnitude(&g_tag_angle);
		timer_wait(MILLISECONDS(1000));

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

static void run_mode_datadump(uint32_t tag_id)
{
	int i;
	uint32_t addr, length;
	uint8_t page[CONFIG_FLASH_PAGESIZE];
	(void) tag_id;

	/* print detected flash memory size */
	length = flash_size();
	debug_printf("- Initialized %i bytes log memory\n\r", length);

	while(1)
	{
		/* wait for button press to start data dump */
		debug_printf("- Press [BUTTON] to start data dump...\n\r");
		while(!nrf_gpio_pin_read(CONFIG_SWITCH_PIN));

		/* turn off LED to indicate operation */
		nrf_gpio_pin_clear(CONFIG_LED_PIN);
		debug_printf("- Start dumping data...\n\r");

		addr = 0;
		while(addr<length)
		{
			/* blink acknowledgement */
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			/* read page from flash */
			flash_read (addr, sizeof(page), page);
			nrf_gpio_pin_clear(CONFIG_LED_PIN);

			/* check for last page: all bytes are 0xFF */
			for(i=0; i<CONFIG_FLASH_PAGESIZE; i++)
				if(page[i]!=0xFF)
					break;
			/* terminate if we've found an empty page */
			if(i==CONFIG_FLASH_PAGESIZE)
				break;

			/* read page from flash */
			debug_printf("\n\rPage[%05i]\n\r", addr/CONFIG_FLASH_PAGESIZE);

			/* dump page on UART */
			hex_dump (page, 0, sizeof(page));
			addr += CONFIG_FLASH_PAGESIZE;
		}

		/* wait if dump period is too short, wait for button release */
		while(nrf_gpio_pin_read(CONFIG_SWITCH_PIN));

		/* turn LED on again to indicate end of operation */
		nrf_gpio_pin_set(CONFIG_LED_PIN);
	}
}

void main_entry(void)
{
	int voltage;
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
	tag_id = crc32((void*)&NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));
#if defined(MARKER_TAG_NEAR) || defined(MARKER_TAG_FAR)
	tag_id |= MARKER_TAG_BIT;
#else /*MARKER_TAG*/
	tag_id &= MARKER_TAG_BIT-1;
#endif/*MARKER_TAG*/
	debug_printf("\n\rInitializing Tag[%08X] v" PROGRAM_VERSION " @24%02iMHz ...\n\r",
		tag_id,
		CONFIG_TRACKER_CHANNEL);

	/* initialize ADC battery voltage measurements */
	adc_init();
	voltage = (int)adc_bat_sync() * 100;
	debug_printf("- Supply voltage of %imV\n\r", voltage);

	/* initialize external flash */
	if(flash_init())
		halt(2);

	/* initialize accelerometer */
	if(acc_init())
		halt(3);

	/* for voltages above 3.2V assume to be in reader */
	if(voltage >= 3200)
		run_mode_datadump(tag_id);
	else
		run_mode_beacon(tag_id);
}
