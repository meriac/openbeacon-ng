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
	uint32_t tag_id;
	int16_t xyz[3];
	int8_t acc[6], delta, t;

	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_NOPULL);

	/* initialize UART */
	uart_init();

	/* start timer */
	timer_init();

	/* initialize flash */
	if(flash_init())
		halt(2);

	/* initialize accelerometer */
	if(acc_init())
		halt(3);

	/* calculate tag ID from NRF_FICR->DEVICEID */
	tag_id = crc32(&NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));

	/* start radio */
	debug_printf("\n\rInitializing Tag[%08X] @24%02iMHz ....\n\r",
		tag_id,
		CONFIG_TRACKER_CHANNEL);
	radio_init(tag_id);

	/* enter main loop */
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

	memset(xyz, 0, sizeof(xyz));
	while(TRUE)
	{
		/* briefly turn on accelerometer */
		acc_write(ACC_REG_CTRL_REG1, 0x9F);
		nrf_gpio_pin_set(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(1));
		nrf_gpio_pin_clear(CONFIG_LED_PIN);
		acc_read(ACC_REG_OUT_X, sizeof(acc), (uint8_t*)&acc);
		acc_write(ACC_REG_CTRL_REG1, 0x00);

		/* get largest accelerometer reading */
		delta = abs(acc[1]-xyz[0]);
		if((t = abs(acc[3]-xyz[1])) > delta)
			delta = t;
		if((t = abs(acc[5]-xyz[2])) > delta)
			delta = t;
		/* remember previous values */
		xyz[0] = acc[1];
		xyz[1] = acc[3];
		xyz[2] = acc[5];

		debug_printf("delta=%i\n\r", delta);
		timer_wait(MILLISECONDS(1000));
	}
}
