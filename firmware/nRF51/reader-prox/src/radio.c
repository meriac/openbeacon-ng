/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
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
#include <main.h>
#include <radio.h>
#include <timer.h>

#define NRF_MAC_SIZE 5UL

#define RADIO_TRACKER_TXADDRESS 0
#define RADIO_TRACKER_TXPOWER (RADIO_TXPOWER_TXPOWER_Neg16dBm << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_TRACKER_TXPOWER_NUM -16
#define RADIO_TRACKER_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_MAXLEN_Pos)

static volatile int g_pkt_count;
static volatile int g_pkt_pos_wr, g_pkt_pos_rd;
static int8_t g_rssi;
static TBeaconBuffer g_pkt[RADIO_MAX_PKT_BUFFERS];

void RADIO_IRQ_Handler(void)
{
	TBeaconBuffer *pkt;

	if(NRF_RADIO->EVENTS_END)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_END = 0;

		/* start RX */
		NRF_RADIO->TASKS_RXEN = 1;
	}

	if(NRF_RADIO->EVENTS_PAYLOAD)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_PAYLOAD = 0;

		/* set LED on every RX */
		if((NRF_RADIO->CRCSTATUS == 1) && (g_pkt_count < RADIO_MAX_PKT_BUFFERS))
		{
			pkt = &g_pkt[g_pkt_pos_wr++];
			if(g_pkt_pos_wr>=RADIO_MAX_PKT_BUFFERS)
				g_pkt_pos_wr = 0;
			/* set PACKETPTR to next slot */
			NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt[g_pkt_pos_wr].buf;

			pkt->rssi = g_rssi;
			g_pkt_count++;
		}

		/* reset RSSI */
		g_rssi = 0;
	}

	if(NRF_RADIO->EVENTS_RSSIEND)
	{
		g_rssi = -((int8_t)NRF_RADIO->RSSISAMPLE);

		/* acknowledge event */
		NRF_RADIO->EVENTS_RSSIEND = 0;

		/* disable RSSI measurement */
		NRF_RADIO->TASKS_RSSISTOP = 1;
	}
}

int radio_packet_count(void)
{
	return g_pkt_count;
}

BOOL radio_rx(TBeaconBuffer *buf)
{
	TBeaconBuffer *src;

	if(g_pkt_count>0)
	{
		/* remember last packet */
		src = &g_pkt[g_pkt_pos_rd++];
		if(g_pkt_pos_rd>=RADIO_MAX_PKT_BUFFERS)
			g_pkt_pos_rd = 0;

		memcpy(buf, src, sizeof(*buf));

		__disable_irq();
		g_pkt_count--;
		__enable_irq();

		return TRUE;
	}
	return FALSE;
}

void POWER_CLOCK_IRQ_Handler(void)
{
	/* always transmit proximity packet */
	if(NRF_CLOCK->EVENTS_HFCLKSTARTED)
	{
		/* acknowledge event */
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		/* start RX */
		NRF_RADIO->TASKS_RXEN = 1;
	}
}

void radio_init(void)
{
	/* reset counters */
	g_pkt_pos_wr = g_pkt_pos_rd = g_pkt_count = 0;
	memset(&g_pkt, 0, sizeof(g_pkt));

	/* setup default radio settings for proximity mode */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;

	/* reconfigure radio for tracker TX */
	NRF_RADIO->FREQUENCY = CONFIG_RADIO_CHANNEL;
	NRF_RADIO->TXPOWER = RADIO_TRACKER_TXPOWER;
	NRF_RADIO->TXADDRESS = RADIO_TRACKER_TXADDRESS;
	NRF_RADIO->PCNF1 = RADIO_TRACKER_PCNF1;

	/* generic radio setup */
	NRF_RADIO->PREFIX0 = NRF_TRACKER_PREFIX;
	NRF_RADIO->BASE0 = NRF_TRACKER_ADDRESS;

	NRF_RADIO->RXADDRESSES = 1;
	NRF_RADIO->PCNF0 = 0x0;
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos);
	NRF_RADIO->CRCINIT = 0xFFUL;
	NRF_RADIO->CRCPOLY = 0x107UL;

	NRF_RADIO->SHORTS = (
		(RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos)       |
		(RADIO_SHORTS_END_DISABLE_Enabled       << RADIO_SHORTS_END_DISABLE_Pos)       |
		(RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos) |
		(RADIO_SHORTS_DISABLED_RSSISTOP_Enabled << RADIO_SHORTS_DISABLED_RSSISTOP_Pos)
	);

	NRF_RADIO->INTENSET = (
		(RADIO_INTENSET_RSSIEND_Enabled         << RADIO_INTENSET_RSSIEND_Pos) |
		(RADIO_INTENSET_PAYLOAD_Enabled         << RADIO_INTENSET_PAYLOAD_Pos) |
		(RADIO_INTENSET_END_Enabled             << RADIO_INTENSET_END_Pos)
	);
	/* enabled radio IRQ */
	NVIC_SetPriority(RADIO_IRQn, IRQ_PRIORITY_RADIO);
	NVIC_EnableIRQ(RADIO_IRQn);

	/* setup HF-clock IRQ */
	NRF_CLOCK->INTENSET = (
		(CLOCK_INTENSET_HFCLKSTARTED_Enabled << CLOCK_INTENSET_HFCLKSTARTED_Pos)
	);
	NVIC_SetPriority(POWER_CLOCK_IRQn, IRQ_PRIORITY_POWER_CLOCK);
	NVIC_EnableIRQ(POWER_CLOCK_IRQn);

	/* start HF crystal oscillator */
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	/* start DC-DC converter */
	NRF_POWER->DCDCEN = (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);
}
