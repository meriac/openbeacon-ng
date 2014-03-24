/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
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
#include <radio.h>

TBeaconEnvelope g_Beacon;

typedef struct {
	uint8_t aeskey[AES_KEY_SIZE];
	uint8_t cleart[AES_KEY_SIZE];
	uint8_t cipher[AES_KEY_SIZE];
} PACKED TCryptoEngine;

static TCryptoEngine g_Crypto;

void radio_init(void)
{
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
	NRF_RADIO->FREQUENCY = CONFIG_TRACKER_CHANNEL;
	NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
	NRF_RADIO->PREFIX0 = 0x80UL;
	NRF_RADIO->BASE0 = 0x40C04080UL;
	NRF_RADIO->RXADDRESSES = 1;
	NRF_RADIO->PCNF0 = 0x0;
	NRF_RADIO->PCNF1 =
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |
		(NRF_PKT_SIZE                 << RADIO_PCNF1_STATLEN_Pos) |
		(NRF_PKT_SIZE                 << RADIO_PCNF1_MAXLEN_Pos);
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos);
	NRF_RADIO->CRCINIT = 0xFFUL;
	NRF_RADIO->CRCPOLY = 0x107UL;
	NRF_RADIO->SHORTS = (
		(RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos)       |
		(RADIO_SHORTS_END_DISABLE_Enabled       << RADIO_SHORTS_END_DISABLE_Pos)       |
		(RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos) |
		(RADIO_SHORTS_DISABLED_RSSISTOP_Enabled << RADIO_SHORTS_DISABLED_RSSISTOP_Pos)
	);

	/* initialize AES engine */
	memset(&g_Crypto, 0, sizeof(g_Crypto));
	NRF_ECB->ECBDATAPTR = (uint32_t)&g_Crypto;
}

