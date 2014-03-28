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
#include <aes.h>

#define NRF_MAC_SIZE 5UL
#define NRF_PROX_SIZE sizeof(TBeaconNgProx)
#define NRF_TRACKER_SIZE sizeof(TBeaconNgTracker)

#define RADIO_TRACKER_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_MAXLEN_Pos)

#define RADIO_PROX_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_MAXLEN_Pos)

void radio_init(uint32_t uid)
{
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
	NRF_RADIO->FREQUENCY = CONFIG_TRACKER_CHANNEL;
	NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
	NRF_RADIO->PREFIX0 = 0x80D7UL;
	NRF_RADIO->BASE0 = 0xEA8AF0B1UL;
	NRF_RADIO->BASE1 = 0x40C04080UL;
	NRF_RADIO->RXADDRESSES = 2;
	NRF_RADIO->TXADDRESS = 0;
	NRF_RADIO->PCNF0 = 0x0;
	NRF_RADIO->PCNF1 = RADIO_PROX_PCNF1;
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos);
	NRF_RADIO->CRCINIT = 0xFFUL;
	NRF_RADIO->CRCPOLY = 0x107UL;
	NRF_RADIO->SHORTS = (
		(RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos)       |
		(RADIO_SHORTS_END_DISABLE_Enabled       << RADIO_SHORTS_END_DISABLE_Pos)       |
		(RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos) |
		(RADIO_SHORTS_DISABLED_RSSISTOP_Enabled << RADIO_SHORTS_DISABLED_RSSISTOP_Pos)
	);

	/* initialize AES encryption engine */
	aes_init(uid);
}

