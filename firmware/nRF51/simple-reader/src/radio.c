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
#include <adc.h>
#include <aes.h>
#include <rng.h>
#include <timer.h>

static volatile uint32_t g_time;
static volatile uint32_t g_rxed;
static uint8_t g_listen_ratio;
static uint8_t g_nrf_state;
static TBeaconNgProx g_pkt_prox;

/* don't start DC/DC converter for voltages below 2.3V */
#define NRF_DCDC_STARTUP_VOLTAGE 23

#define NRF_MAC_SIZE 5UL
#define NRF_PROX_SIZE sizeof(TBeaconNgProx)
#define NRF_TRACKER_SIZE sizeof(TBeaconNgTracker)

#define NRF_STATE_IDLE           0
#define NRF_STATE_TX_PROX        1
#define NRF_STATE_RX_PROX        2
#define NRF_STATE_RX_PROX_PACKET 3

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

void RTC0_IRQ_Handler(void)
{
	uint32_t delta_t;

	/* run every second */
	if(NRF_RTC0->EVENTS_COMPARE[0])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		/* re-trigger in one second */
		NRF_RTC0->CC[0]+= LF_FREQUENCY;

		/* increment time */
		g_time++;

		/* measure battery voltage once per second */
		adc_start();
	}

	if(NRF_RTC0->EVENTS_COMPARE[1])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[1] = 0;

		/* re-trigger next RX/TX-slot */
		delta_t =
			CONFIG_PROX_SPACING -
			(1<<(CONFIG_PROX_SPACING_RNG_BITS-1)) +
			rng(CONFIG_PROX_SPACING_RNG_BITS);
		NRF_RTC0->CC[1] = NRF_RTC0->COUNTER + delta_t;

		/* start HF crystal oscillator */
		NRF_CLOCK->TASKS_HFCLKSTART = 1;

		/* listen every CONFIG_PROX_LISTEN_RATIO slots */
		g_listen_ratio++;
		if(g_listen_ratio<CONFIG_PROX_LISTEN_RATIO)
			g_nrf_state = NRF_STATE_TX_PROX;
		else
		{
			g_listen_ratio = 0;
			g_nrf_state = NRF_STATE_RX_PROX;

			/* only start DC/DC converter for
			 * RX & higher battery voltages */
			if(adc_bat()>=NRF_DCDC_STARTUP_VOLTAGE)
			{
				/* start DC-DC converter */
				NRF_POWER->DCDCEN = (
					(POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos)
				);
			}
		}
	}

	/* listen for CONFIG_PROX_WINDOW_MS every second */
	if(NRF_RTC0->EVENTS_COMPARE[2])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[2] = 0;

		/* set next state */
		g_nrf_state = NRF_STATE_IDLE;
		/* stop radio */
		NRF_RADIO->TASKS_DISABLE = 1;
		/* stop HF clock */
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
		/* disable DC-DC converter */
		NRF_POWER->DCDCEN = 0;
	}
}

void POWER_CLOCK_IRQ_Handler(void)
{
	/* always transmit proximity packet */
	if(NRF_CLOCK->EVENTS_HFCLKSTARTED)
	{
		/* acknowledge event */
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		/* set first packet pointer */
		NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox;
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

		/* process state machine */
		switch(g_nrf_state)
		{
			case NRF_STATE_RX_PROX:
			{
				/* set next state */
				g_nrf_state = NRF_STATE_RX_PROX_PACKET;

				/* set first packet pointer */
				NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox;
				/* start listening */
				NRF_RADIO->TASKS_RXEN = 1;

				/* retrigger listening stop */
				NRF_RTC0->CC[2] = NRF_RTC0->COUNTER + CONFIG_PROX_LISTEN;
				break;
			}

			case NRF_STATE_TX_PROX:
			{
				/* set next state */
				g_nrf_state = NRF_STATE_IDLE;

				/* stop HF clock */
				NRF_CLOCK->TASKS_HFCLKSTOP = 1;
				/* disable DC-DC converter */
				NRF_POWER->DCDCEN = 0;
			}
		}
	}

	if(NRF_RADIO->EVENTS_END)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_END = 0;

		/* received packet */
		if(g_nrf_state == NRF_STATE_RX_PROX_PACKET)
		{
			if(NRF_RADIO->CRCSTATUS == 1)
				g_rxed++;
		}
	}

	if(NRF_RADIO->EVENTS_RSSIEND)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_RSSIEND = 0;

		/* disable RSSI measurement */
		NRF_RADIO->TASKS_RSSISTOP = 1;
	}
}

void radio_init(uint32_t uid)
{
	/* reset variables */
	g_time = 0;
	g_listen_ratio = 0;
	g_nrf_state = 0;

	/* start random number genrator */
	rng_init();
	/* initialize AES encryption engine */
	aes_init(uid);
	/* initialize ADC battery voltage measurements */
	adc_init();

	/* setup default radio settings */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
	NRF_RADIO->FREQUENCY = CONFIG_PROX_CHANNEL;
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
	NRF_RADIO->INTENSET = (
		(RADIO_INTENSET_RSSIEND_Enabled         << RADIO_INTENSET_RSSIEND_Pos)  |
		(RADIO_INTENSET_DISABLED_Enabled        << RADIO_INTENSET_DISABLED_Pos) |
		(RADIO_INTENSET_END_Enabled             << RADIO_INTENSET_END_Pos)
	);
	NVIC_EnableIRQ(RADIO_IRQn);

	/* setup HF-clock IRQ */
	NRF_CLOCK->INTENSET = (
		(CLOCK_INTENSET_HFCLKSTARTED_Enabled << CLOCK_INTENSET_HFCLKSTARTED_Pos)
	);
	NVIC_EnableIRQ(POWER_CLOCK_IRQn);

	/* setup radio timer */
	NRF_RTC0->TASKS_STOP = 1;
	NRF_RTC0->COUNTER = 0;
	NRF_RTC0->PRESCALER = 0;
	NRF_RTC0->CC[0] = LF_FREQUENCY;
	NRF_RTC0->CC[1] = LF_FREQUENCY*2;
	NRF_RTC0->CC[2] = 0;
	NRF_RTC0->INTENSET = (
		(RTC_INTENSET_COMPARE0_Enabled   << RTC_INTENSET_COMPARE0_Pos) |
		(RTC_INTENSET_COMPARE1_Enabled   << RTC_INTENSET_COMPARE1_Pos) |
		(RTC_INTENSET_COMPARE2_Enabled   << RTC_INTENSET_COMPARE2_Pos)
	);
	NVIC_EnableIRQ(RTC0_IRQn);
	NRF_RTC0->TASKS_START = 1;
}
