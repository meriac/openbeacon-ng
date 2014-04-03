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

	/* start radio */
	debug_printf("\n\rInitializing Radio @24%02iMHz ....\n\r", CONFIG_TRACKER_CHANNEL);
	debug_printf("FICR:\n\r"
		"  CONFIGID       = 0x%08X\n\r"
		"  DEVICEID[0]    = 0x%08X\n\r"
		"  DEVICEID[1]    = 0x%08X\n\r"
		"  DEVICEADDRTYPE = %s\n\r"
		"  DEVICEADDR[0]  = 0x%08X\n\r"
		"  DEVICEADDR[1]  = 0x%04X\n\r"
		,
		NRF_FICR->CONFIGID,NRF_FICR->DEVICEID[0],NRF_FICR->DEVICEID[1],
		(NRF_FICR->DEVICEADDRTYPE & 1)? "random":"public",
		NRF_FICR->DEVICEADDR[0],(uint16_t)NRF_FICR->DEVICEADDR[1]);
	radio_init(0x12345678);

	/* enter main loop */
	nrf_gpio_pin_clear(CONFIG_LED_PIN);
	while(TRUE)
		__WFI();
}
