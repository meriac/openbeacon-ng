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
#include <main.h>
#include <radio.h>
#include <adc.h>
#include <aes.h>
#include <rng.h>
#include <timer.h>

static TBeaconNgMarker g_pkt_tracker ALIGN4;
static uint8_t g_pkt_tracker_enc[sizeof(g_pkt_tracker)] ALIGN4;

#define NRF_MAC_SIZE 5UL
#define NRF_TRACKER_SIZE sizeof(g_pkt_tracker)

#define RADIO_PKTS_PER_SEC 10

#define RADIO_TRACKER_TXADDRESS 0
#define RADIO_TRACKER_TXPOWER (RADIO_TXPOWER_TXPOWER_Neg16dBm << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_TRACKER_TXPOWER_NUM -16
#define RADIO_TRACKER_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_MAXLEN_Pos)

void RTC0_IRQ_Handler(void)
{
	if(NRF_RTC0->EVENTS_COMPARE[0])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		/* start HF crystal oscillator */
		NRF_CLOCK->TASKS_HFCLKSTART = 1;

		/* re-trigger */
		NRF_RTC0->CC[0] =
			NRF_RTC0->COUNTER + (LF_FREQUENCY/RADIO_PKTS_PER_SEC) + rng(8);
	}
}

void POWER_CLOCK_IRQ_Handler(void)
{
	/* always transmit proximity packet */
	if(NRF_CLOCK->EVENTS_HFCLKSTARTED)
	{
		/* acknowledge event */
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		/* update packet */
		g_pkt_tracker.proto = RFBPROTO_BEACON_NG_MARKER;
		g_pkt_tracker.counter++;
		g_pkt_tracker.voltage = adc_bat();

		/* encrypt packet */
		aes_encr(
			&g_pkt_tracker,
			&g_pkt_tracker_enc,
			sizeof(g_pkt_tracker_enc),
			CONFIG_SIGNATURE_SIZE
		);

		/* set first packet pointer */
		NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_tracker_enc;
		/* transmit proximity packet */
		NRF_RADIO->TASKS_TXEN = 1;
	}
}

void RADIO_IRQ_Handler(void)
{
	if(NRF_RADIO->EVENTS_DISABLED)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_DISABLED = 0;

		/* stop HF clock */
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
	}
}

void radio_init(uint32_t uid)
{
	/* initialize tracker packet */
	memset(&g_pkt_tracker, 0, sizeof(g_pkt_tracker));
	g_pkt_tracker.tx_power = RADIO_TRACKER_TXPOWER_NUM;
	g_pkt_tracker.uid = uid;

	/* start random number genrator */
	rng_init();
	/* initialize AES encryption engine */
	aes_init(uid);
	/* initialize ADC battery voltage measurements */
	adc_init();

	/* setup default radio settings for proximity mode */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;

	/* reconfigure radio for tracker TX */
	NRF_RADIO->FREQUENCY = CONFIG_TRACKER_CHANNEL;
	NRF_RADIO->TXPOWER = RADIO_TRACKER_TXPOWER;
	NRF_RADIO->TXADDRESS = RADIO_TRACKER_TXADDRESS;
	NRF_RADIO->PCNF1 = RADIO_TRACKER_PCNF1;

	/* generic radio setup */
	NRF_RADIO->PREFIX0 = 0x80UL;
	NRF_RADIO->BASE0 = 0x40C04080UL;
	NRF_RADIO->RXADDRESSES = 1;
	NRF_RADIO->PCNF0 = 0x0;
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos);
	NRF_RADIO->CRCINIT = 0xFFUL;
	NRF_RADIO->CRCPOLY = 0x107UL;
	NRF_RADIO->SHORTS = (
		(RADIO_SHORTS_READY_START_Enabled       << RADIO_SHORTS_READY_START_Pos)|
		(RADIO_SHORTS_END_DISABLE_Enabled       << RADIO_SHORTS_END_DISABLE_Pos)
	);
	NRF_RADIO->INTENSET = (
		(RADIO_INTENSET_DISABLED_Enabled        << RADIO_INTENSET_DISABLED_Pos)
	);
	NVIC_SetPriority(RADIO_IRQn, IRQ_PRIORITY_RADIO);
	NVIC_EnableIRQ(RADIO_IRQn);

	/* setup HF-clock IRQ */
	NRF_CLOCK->INTENSET = (
		(CLOCK_INTENSET_HFCLKSTARTED_Enabled << CLOCK_INTENSET_HFCLKSTARTED_Pos)
	);
	NVIC_SetPriority(POWER_CLOCK_IRQn, IRQ_PRIORITY_POWER_CLOCK);
	NVIC_EnableIRQ(POWER_CLOCK_IRQn);

	/* setup radio timer */
	NRF_RTC0->TASKS_STOP = 1;
	NRF_RTC0->COUNTER = 0;
	NRF_RTC0->PRESCALER = 0;
	NRF_RTC0->CC[0] = LF_FREQUENCY*2;
	NRF_RTC0->INTENSET = (
		(RTC_INTENSET_COMPARE0_Enabled   << RTC_INTENSET_COMPARE0_Pos)
	);
	NVIC_SetPriority(RTC0_IRQn, IRQ_PRIORITY_RTC0);
	NVIC_EnableIRQ(RTC0_IRQn);
	NRF_RTC0->TASKS_START = 1;
}
