/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
 *
 * Copyright 2013-2015 Milosch Meriac <meriac@openbeacon.org>
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
#include <radio.h>
#include <timer.h>

static int8_t g_tag_angle;

int8_t tag_angle(void)
{
	return g_tag_angle;
}

static void led_set(int led)
{
	if(led)
		NRF_GPIO->OUTSET = LED_MASK_ROWS;
	else
		NRF_GPIO->OUTCLR = LED_MASK_ROWS;
}

void blink(uint8_t times)
{
	while(times--)
	{
		led_set(1);
		timer_wait(MILLISECONDS(10));
		led_set(0);
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
	int i;
	uint8_t *p, data;
	TBeaconBuffer pkt;
	uint32_t tag_id;

	/* enable LED output */
	led_set(1);
    for(i=LED_PIN_OFFSET;i<(LED_PIN_OFFSET+LED_COUNT);i++)
		nrf_gpio_cfg_output(i);

	/* enabled input pins */
	nrf_gpio_cfg_input(CONFIG_BTN_A, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(CONFIG_BTN_B, NRF_GPIO_PIN_NOPULL);

	/* initialize UART */
	uart_init();

	/* start timer */
	timer_init();

	/* calculate tag ID from NRF_FICR->DEVICEID */
	tag_id = crc32((void*)&NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));

	/* start radio */
	debug_printf("\n\rInitializing Reader[%08X] (CH:%02i) v" PROGRAM_VERSION "\n\r",
		tag_id, CONFIG_RADIO_CHANNEL);
	radio_init();

	/* enter main loop */
	blink(3);
	led_set(0);
	while(TRUE)
	{
		if(!radio_rx(&pkt))
			__WFE();
		else
		{
#ifdef  RSSI_FILTERING
			if(pkt.rssi<=RSSI_FILTERING)
				continue;
#endif/*RSSI_FILTERING*/

			led_set(1);

			/* output packet, escape 0xFF's by appending 0x01's */
			p = (uint8_t*)&pkt;
			for(i=0; i<(int)sizeof(pkt); i++)
			{
				data = *p++;
				default_putchar(data);
				/* if data is 0xFF, emit control character */
				if(data == 0xFF)
					default_putchar(0x00);
			}

			/* issue frame termination indicator */
			default_putchar(0xFF);
			default_putchar(0x01);
			led_set(0);
		}
	}
}
