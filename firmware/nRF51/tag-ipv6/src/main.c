/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
 *
 * Copyright 2013-2014 Milosch Meriac <meriac@openbeacon.de>
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
#include <nrf_sdm.h>

void main_entry(void)
{
	uint32_t res;
	uint8_t enabled;
	volatile int t;

	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_NOPULL);

	/* initialize UART */
	uart_init();

	while(1)
	{
		res = sd_softdevice_is_enabled(&enabled);

		debug_printf("Hello World (0x%08X, %i)\n", res, enabled);
		for(t=0; t<100000; t++);

		if(nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
			nrf_gpio_pin_set(CONFIG_LED_PIN);
		else
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
	}
}
