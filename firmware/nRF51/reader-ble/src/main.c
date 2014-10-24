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
#include <radio.h>
#include <timer.h>

#define BT_PREFIX 2
#define BT_MAC_SIZE 6
#define BT_IBEACON_HDR_SIZE sizeof(g_iBeacon_sig)
#define BT_IB_PREFIX_SIZE (BT_PREFIX+BT_MAC_SIZE+BT_IBEACON_HDR_SIZE)

static const uint8_t g_iBeacon_sig[] = {
	0x02,0x01,0x06,0x1A,0xFF,0x4C,0x00,0x02,0x15
};
typedef struct {
	uint8_t guid[16];
	uint16_t major;
	uint16_t minor;
	int8_t txpower;
} PACKED TiBeacon;

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

static void print_hex(const uint8_t* data, uint16_t len)
{
	uint8_t d;

	while(len>0)
	{
		d = *data++;
		default_putchar(hex_char(d>>4));
		default_putchar(hex_char(d&0xF));
		len--;
	}
}

static void print_guid(const uint8_t* data)
{
	print_hex(data +  0, 4);
	default_putchar('-');
	print_hex(data +  4, 2);
	default_putchar('-');
	print_hex(data +  6, 2);
	default_putchar('-');
	print_hex(data +  8, 2);
	default_putchar('-');
	print_hex(data + 10, 6);
}

void main_entry(void)
{
	TBeaconBuffer pkt;
	const TiBeacon* ib = (TiBeacon*)&pkt.buf[BT_IB_PREFIX_SIZE];
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

	/* start radio */
	debug_printf("\n\rInitializing Reader[%08X] v" PROGRAM_VERSION "\n\r",
		tag_id);
	radio_init();

	/* enter main loop */
	nrf_gpio_pin_clear(CONFIG_LED_PIN);
	while(TRUE)
	{
		if(!radio_rx(&pkt))
			__WFE();
		else
			/* check for iBeacon signature */
			if( (pkt.buf[1] >= 36) && !memcmp(
					&pkt.buf[BT_PREFIX+BT_MAC_SIZE],
					&g_iBeacon_sig,
					sizeof(g_iBeacon_sig)
				) )
			{
				print_guid(ib->guid);
				debug_printf(",%i,%i,0x%04X,0x%04X,%i\n\r",
					pkt.rssi,
					ib->txpower,
					ntohs(ib->major),
					ntohs(ib->minor),
					pkt.channel
					);
			}
	}
}
