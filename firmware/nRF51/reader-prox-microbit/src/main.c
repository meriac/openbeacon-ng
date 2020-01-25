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
#include <clock.h>
#include <aes.h>
#include <../../tag-proximity/inc/openbeacon-proto.h>

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

static void port_rx(const void* pkt_encrypted, int rssi)
{
	int proto, i;
	int status;
	TBeaconNgTracker pkt;

	/* decrypt valid packet */
	if((status = aes_decr(pkt_encrypted, &pkt, sizeof(pkt), CONFIG_SIGNATURE_SIZE))!=0)
	{
		debug_printf("error: failed decrypting packet with error [%i]\n\r", status);
		return;
	}

	/* white-list supported protocols */
	proto = pkt.proto & RFBPROTO_PROTO_MASK;
	if(!((proto == 30) || (proto == 31)))
	{
		debug_printf("error: uknnown protocol %i\n\r", proto);
		return;
	}

	debug_printf(
		"{ \"uid\":\"0x%08X\", \"time_local_s\":%8u, \"time_remote_s\":%8u, \"rssi\":%3i, \"angle\":%3i, \"voltage\":%2i, \"tx_power\":%i",
		pkt.uid,
		clock_get(),
		pkt.epoch,
		rssi,
		pkt.angle,
		pkt.voltage,
		pkt.tx_power);

	/* optionally decode button */
	if(pkt.proto & RFBPROTO_PROTO_BUTTON)
		debug_printf(", \"button\": 1");

	switch(proto)
	{
		case 30:
			if(pkt.p.sighting[0].uid)
			{
				debug_printf(", \"sighting\": ");
				for(i=0; i<CONFIG_SIGHTING_SLOTS; i++)
					if(pkt.p.sighting[i].uid)
						debug_printf("%c{\"uid\":\"0x%08X\",\"rssi\":%i}",
							i ? ',':'[',
							pkt.p.sighting[i].uid,
							pkt.p.sighting[i].rx_power
						);
				debug_printf("]");
			}
			break;
	}
	debug_printf("}\n\r");
}

void main_entry(void)
{
	int i, decode;
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
	/* start clock */
	clock_init();

	/* calculate tag ID from NRF_FICR->DEVICEID */
	tag_id = crc32((void*)&NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));

	/* Initialize AES decryption */
	aes_init(tag_id);

	/* start radio */
	debug_printf("\n\rInitializing Reader[%08X] (CH:%02i) v" PROGRAM_VERSION "\n\r",
		tag_id, CONFIG_RADIO_CHANNEL);
	radio_init();

	/* enter main loop */
	blink(5);
	led_set(0);
	decode = 0;
	while(TRUE)
	{

		if(!nrf_gpio_pin_read(CONFIG_BTN_A))
		{
			blink(2);
			timer_wait(MILLISECONDS(1000));
			decode = 1;
		}

		if(!nrf_gpio_pin_read(CONFIG_BTN_B))
		{
			blink(3);
			timer_wait(MILLISECONDS(1000));
			decode = 0;
		}

		if(radio_rx(&pkt))
		{
#ifdef  RSSI_FILTERING
			if(pkt.rssi<=RSSI_FILTERING)
				continue;
#endif/*RSSI_FILTERING*/

			led_set(1);

			/* output packet, escape 0xFF's by appending 0x01's */
			if(decode)
				port_rx(&pkt.buf, pkt.rssi);
			else
			{
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
			}
			led_set(0);
		}
	}
}
