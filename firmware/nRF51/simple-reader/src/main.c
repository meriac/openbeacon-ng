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

/* Default TEA encryption key of the tag - MUST CHANGE ! */
static const uint32_t xxtea_key[XXTEA_BLOCK_COUNT] = {
	0x00112233,
	0x44556677,
	0x8899AABB,
	0xCCDDEEFF
};

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

static void init_hardware(void)
{
	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_NOPULL);

	/* reset LED */
	nrf_gpio_pin_clear(CONFIG_LED_PIN);

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
	radio_init();
}

void main_entry(void)
{
	uint16_t crc;
	int strength;
	init_hardware();

	/* start 16MHz crystal oscillator */
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

	/* enter RX loop */
	while(true)
	{
		/* briefly blink LED */
		nrf_gpio_pin_set(CONFIG_LED_PIN);
		timer_wait(MILLISECONDS(25));
		nrf_gpio_pin_clear(CONFIG_LED_PIN);

		/* set packet start address */
		NRF_RADIO->PACKETPTR = (uint32_t)&g_Beacon;
		debug_printf("RX: ");

		/* enable & start RX */
		NRF_RADIO->EVENTS_END = 0;
		NRF_RADIO->TASKS_RXEN = 1;

		/* wait for packet end */
		while(!NRF_RADIO->EVENTS_END);

		/* verify CRC */
		if(NRF_RADIO->CRCSTATUS == 1)
		{
			/* adjust byte order and decode */
			xxtea_decode ((uint32_t*)&g_Beacon.block, XXTEA_BLOCK_COUNT, xxtea_key);

			/* verify the CRC checksum */
			crc = crc16 ((uint8_t*)&g_Beacon.byte,
				sizeof (g_Beacon) - sizeof (g_Beacon.pkt.crc));

			if (ntohs (g_Beacon.pkt.crc) != crc)
				debug_printf("CRC_ERROR_ENCR\n\r");
			else
			{
				switch(g_Beacon.pkt.proto)
				{
					case RFBPROTO_PROXREPORT:
					case RFBPROTO_PROXREPORT_EXT:
						strength = 3;
						break;
					case RFBPROTO_BEACONTRACKER:
						strength = g_Beacon.pkt.p.tracker.strength;
						break;
					default:
						strength = -1;
				}
				if(strength>=0)
					debug_printf("RSSI[%i@-%03idBm] (%08us) ", strength, NRF_RADIO->RSSISAMPLE, timer_s());
				hex_dump((uint8_t*)&g_Beacon.byte, 0, sizeof(g_Beacon.byte));
			}
		}
		else
			debug_printf("CRC_ERROR_PLAIN\n\r");
	}
}
