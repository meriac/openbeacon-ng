/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Reader Communication Handling
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
#include <tracker.h>
#include <flash.h>
#include <radio.h>
#include <acc.h>
#include <timer.h>
#include <openbeacon-proto.h>

static TBeaconNgTracker g_pkt_tracker ALIGN4;
static int8_t g_acc_channel[3];

const void* tracker_transmit(uint16_t tx_delay_us, uint16_t listen_wait_ms)
{
	(void)tx_delay_us;

	/* announce next listening slot */
	g_pkt_tracker.listen_wait_ms = listen_wait_ms;

	/* update counter to guarantee uniquely encrypted packets */
	g_pkt_tracker.counter++;

	/* return pointer to packet - needs to be in SRAM */
	return &g_pkt_tracker;
}

void tracker_receive(uint32_t uid, int tx_power, int rx_power)
{
	(void)uid;
	(void)tx_power;
	(void)rx_power;
}

void tracker_init(uint32_t uid)
{
	memset(&g_pkt_tracker, 0, sizeof(g_pkt_tracker));
	g_pkt_tracker.uid = uid;
}

uint8_t tracker_loop(void)
{
	int blink = 0;
	ACC_CHANNELS acc_ch;

	nrf_gpio_pin_clear(CONFIG_LED_PIN);
	while(TRUE)
	{
		/* get tag angle once per second */
		acc_channels(&acc_ch);

		/* dump readings into 8 bit buffer, ensure atomicity */
		__disable_irq();
		g_acc_channel[0] = acc_ch[0]/0x100;
		g_acc_channel[1] = acc_ch[1]/0x100;
		g_acc_channel[2] = acc_ch[2]/0x100;
		__enable_irq();

		/* blink every 5 seconds */
		timer_wait(MILLISECONDS(1000));
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

	/* should never return */
	return 4;
}

