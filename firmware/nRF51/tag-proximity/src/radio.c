/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
 *
 * Copyright 2013-2015 Milosch Meriac <meriac@openbeacon.de>
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
#include <tracker.h>
#include <adc.h>
#include <aes.h>
#include <rng.h>
#include <timer.h>
#include <openbeacon-proto.h>

static volatile uint32_t g_time, g_time_offset;
static volatile uint16_t g_ticks_offset, g_pkt_tracker_ticks;
static uint32_t g_next_listen_slot;
static uint8_t g_nrf_state;
static int8_t g_rssi;

static TBeaconNgProx g_pkt_prox ALIGN4;
static uint8_t g_pkt_prox_enc[sizeof(g_pkt_prox)] ALIGN4;

static TBeaconNgProx g_pkt_prox_rx ALIGN4;
static uint8_t g_pkt_prox_rx_enc[sizeof(g_pkt_prox_rx)] ALIGN4;

static uint8_t g_pkt_tracker_enc[sizeof(TBeaconNgTracker)] ALIGN4;

static int g_proximity_wait_pos;
static uint32_t g_proximity_wait[CONFIG_PROX_LISTEN_RATIO];

/* derived config values */
#define CONFIG_PROX_LISTEN MILLISECONDS(CONFIG_PROX_LISTEN_MS)
#define CONFIG_PROX_SPACING MILLISECONDS(CONFIG_PROX_SPACING_MS)

/* don't start DC/DC converter for voltages below 2.3V */
#define NRF_DCDC_STARTUP_VOLTAGE 23

#define NRF_MAC_SIZE 5UL
#define NRF_PROX_SIZE sizeof(TBeaconNgProx)
#define NRF_TRACKER_SIZE sizeof(TBeaconNgTracker)

#define NRF_STATE_IDLE            0
#define NRF_STATE_TX_PROX         1
#define NRF_STATE_RX_PROX         2
#define NRF_STATE_RX_PROX_PACKET  3
#define NRF_STATE_RX_PROX_BLINK   4
#define NRF_STATE_TX_TRACKER      5
#define NRF_STATE_TX_TRACKER_DONE 6

#define RADIO_TRACKER_TXADDRESS 1
#define RADIO_TRACKER_TXPOWER (TX_POWER_VALUE << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_TRACKER_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_MAXLEN_Pos)

#define RADIO_PROX_TXADDRESS 0
#define RADIO_PROX_TXPOWER (PX_POWER_VALUE << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_PROX_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Enabled  << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_MAXLEN_Pos)

static void radio_prox_tx(void)
{
	uint32_t ticks;

	/* update proximity time */
	g_pkt_prox.epoch = g_time+g_time_offset;
	/* adjust transmitted time by time
	 * it takes to encrypt the packet */
	ticks = NRF_RTC0->COUNTER;
	g_pkt_prox.ticks += (ticks+g_ticks_offset);
	/* encrypt data */
	aes_encr(
		&g_pkt_prox,
		&g_pkt_prox_enc,
		sizeof(g_pkt_prox_enc),
		CONFIG_SIGNATURE_SIZE
	);
	/* remember time it took to encrypt packet for next TX */
	g_pkt_prox.ticks = NRF_RTC0->COUNTER-ticks;

	/* set first packet pointer */
	NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox_enc;
	/* transmit proximity packet */
	NRF_RADIO->TASKS_TXEN = 1;
}

void RTC0_IRQ_Handler(void)
{
	uint32_t delta_t, start_t;

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

		if(!g_proximity_wait_pos)
		{
			/* calculate sum of all proximity slots */
			g_next_listen_slot = 0;
			/* populate proximity wait times randomly */
			while(g_proximity_wait_pos<CONFIG_PROX_LISTEN_RATIO)
			{
				delta_t =
					CONFIG_PROX_SPACING -
					(1<<(CONFIG_PROX_SPACING_RNG_BITS-1)) +
					rng(CONFIG_PROX_SPACING_RNG_BITS);
				g_next_listen_slot += delta_t;
				g_proximity_wait[g_proximity_wait_pos++] = delta_t;
			}

			/* correct last slot for listening time */
			g_next_listen_slot -= CONFIG_PROX_LISTEN;
			g_proximity_wait[0] -= CONFIG_PROX_LISTEN;

			/* schedule first proximity transmission */
			start_t = NRF_RTC0->COUNTER;
			NRF_RTC0->CC[1] =
				CONFIG_PROX_LISTEN +
				start_t +
				g_proximity_wait[--g_proximity_wait_pos];
			/* calculate next proximity RX slot */
			g_next_listen_slot += start_t;

			/* make sure to listen first */
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
		else
		{
			/* schedule next proximity event */
			NRF_RTC0->CC[1] += g_proximity_wait[g_proximity_wait_pos--];
			/* do proximity-tx-only for this event */
			g_nrf_state = NRF_STATE_TX_PROX;
		}

		/* start HF crystal oscillator */
		NRF_CLOCK->TASKS_HFCLKSTART = 1;
	}

	/* transmit proximity package after listening for proximity */
	if(NRF_RTC0->EVENTS_COMPARE[2])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[2] = 0;

		/* disable receive */
		NRF_RADIO->TASKS_RXEN = 0;

		/* transmit proximity event
		   on same frequency */
		g_nrf_state = NRF_STATE_TX_TRACKER;
		radio_prox_tx();
	}
}

void POWER_CLOCK_IRQ_Handler(void)
{
	uint32_t ticks;

	/* acknowledge event */
	if(!NRF_CLOCK->EVENTS_HFCLKSTARTED)
		return;
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

	/* process state machine */
	switch(g_nrf_state)
	{
		case NRF_STATE_RX_PROX:
		{
			/* set next state */
			g_nrf_state = NRF_STATE_RX_PROX_PACKET;
			/* reset rssi measurement */
			g_rssi = 0;
			/* set first packet pointer */
			NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox_rx_enc;
			/* start listening */
			NRF_RADIO->TASKS_RXEN = 1;

			/* retrigger listening stop */
			NRF_RTC0->CC[2] = NRF_RTC0->COUNTER + CONFIG_PROX_LISTEN;
			break;
		}

		case NRF_STATE_TX_PROX:
		{
			/* update proximity time */
			g_pkt_prox.epoch = g_time+g_time_offset;
			/* adjust transmitted time by time
			 * it takes to encrypt the packet */
			ticks = NRF_RTC0->COUNTER;
			g_pkt_prox.ticks += (ticks+g_ticks_offset);
			/* encrypt data */
			aes_encr(
				&g_pkt_prox,
				&g_pkt_prox_enc,
				sizeof(g_pkt_prox_enc),
				CONFIG_SIGNATURE_SIZE
			);
			/* remember time it took to encrypt packet for next TX */
			g_pkt_prox.ticks = NRF_RTC0->COUNTER-ticks;

			/* set first packet pointer */
			NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox_enc;
			/* transmit proximity packet */
			NRF_RADIO->TASKS_TXEN = 1;
			break;
		}
	}
}

static void radio_on_prox_packet(uint16_t delta_t)
{
	/* ignore replayed packets from myself */
	if(g_pkt_prox_rx.uid == g_pkt_prox.uid)
		return;

	/* adjust epoch time if booted freshly */
	if(!g_time_offset && (g_pkt_prox_rx.epoch > (g_time+g_time_offset)))
	{
		g_time_offset = g_pkt_prox_rx.epoch - g_time;

		/* adjust fine-grained time by decryption time */
		g_ticks_offset =
			(delta_t + g_pkt_prox_rx.ticks) -
			((uint16_t)NRF_RTC0->COUNTER);
	}

	/* process proximity package */
	tracker_receive(g_pkt_prox_rx.uid, g_pkt_prox_rx.tx_power, g_rssi);
}

void RADIO_IRQ_Handler(void)
{
	uint32_t ticks;
	const void* tracker_pkt;

	if(NRF_RADIO->EVENTS_DISABLED)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_DISABLED = 0;

		/* process state machine */
		switch(g_nrf_state)
		{
			case NRF_STATE_TX_PROX:
			{
				/* set default next state */
				g_nrf_state = NRF_STATE_IDLE;
				/* stop HF clock */
				NRF_CLOCK->TASKS_HFCLKSTOP = 1;
				/* disable DC-DC converter */
				NRF_POWER->DCDCEN = 0;
				break;
			}

			case NRF_STATE_TX_TRACKER:
			{
				/* set next state */
				g_nrf_state = NRF_STATE_TX_TRACKER_DONE;

				/* reconfigure radio for tracker TX */
				NRF_RADIO->FREQUENCY = CONFIG_TRACKER_CHANNEL;
				NRF_RADIO->TXPOWER = RADIO_TRACKER_TXPOWER;
				NRF_RADIO->TXADDRESS = RADIO_TRACKER_TXADDRESS;
				NRF_RADIO->PCNF1 = RADIO_TRACKER_PCNF1;

				/* get tracker packet, correct for transmission time,
				   calculate delta time till next transmission */
				tracker_pkt = tracker_transmit(
					(g_pkt_tracker_ticks * 1000000UL)/LF_FREQUENCY,
					(((g_next_listen_slot-NRF_RTC0->COUNTER-g_pkt_tracker_ticks)
						& RTC_COUNTER_COUNTER_Msk) * 1000UL)/LF_FREQUENCY
				);
				/* update encryption time */
				g_pkt_tracker_ticks = NRF_RTC0->COUNTER;
				/* encrypt+sign packet */
				aes_encr(
					tracker_pkt,
					&g_pkt_tracker_enc,
					sizeof(g_pkt_tracker_enc),
					CONFIG_SIGNATURE_SIZE
				);
				/* set first packet pointer */
				NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_tracker_enc;
				/* start tracker TX */
				NRF_RADIO->TASKS_TXEN = 1;
				break;
			}

			case NRF_STATE_TX_TRACKER_DONE:
			{
				/* calculate TX delay */
				g_pkt_tracker_ticks = NRF_RTC0->COUNTER - g_pkt_tracker_ticks;

				/* set next state */
				g_nrf_state = NRF_STATE_IDLE;

				/* stop HF clock */
				NRF_CLOCK->TASKS_HFCLKSTOP = 1;

				/* reconfigure radio back to proximity */
				NRF_RADIO->FREQUENCY = CONFIG_PROX_CHANNEL;
				NRF_RADIO->TXPOWER = RADIO_PROX_TXPOWER;
				NRF_RADIO->TXADDRESS = RADIO_PROX_TXADDRESS;
				NRF_RADIO->PCNF1 = RADIO_PROX_PCNF1;

				/* disable DC-DC converter */
				NRF_POWER->DCDCEN = 0;
				break;
			}
		}
	}

	if(NRF_RADIO->EVENTS_PAYLOAD)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_PAYLOAD = 0;

		/* received packet */
		if(	(g_nrf_state == NRF_STATE_RX_PROX_PACKET) && 
			(NRF_RADIO->CRCSTATUS == 1) )
		{
			/* measure decryption time */
			ticks = NRF_RTC0->COUNTER;

			/* decrypt and verify packet */
			if(!aes_decr(
				&g_pkt_prox_rx_enc,
				&g_pkt_prox_rx,
				sizeof(g_pkt_prox_rx),
				CONFIG_SIGNATURE_SIZE))
			{
				g_nrf_state = NRF_STATE_RX_PROX_BLINK;
				radio_on_prox_packet(NRF_RTC0->COUNTER-ticks);
			}
		}
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

void radio_init(uint32_t uid)
{
	/* reset variables */
	g_time = g_time_offset = 0;
	g_pkt_tracker_ticks = 0;
	g_ticks_offset = 0;
	g_nrf_state = 0;
	g_rssi = 0;
	g_proximity_wait_pos = 0;

	/* initialize proximity packet */
	memset(&g_pkt_prox, 0, sizeof(g_pkt_prox));
	g_pkt_prox.uid = uid;

	/* start random number genrator */
	rng_init();
	/* initialize AES encryption engine */
	aes_init(uid);
	/* initialize ADC battery voltage measurements */
	adc_init();

	/* setup default radio settings for proximity mode */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
	NRF_RADIO->FREQUENCY = CONFIG_PROX_CHANNEL;
	NRF_RADIO->TXPOWER = RADIO_PROX_TXPOWER;
	NRF_RADIO->TXADDRESS = RADIO_PROX_TXADDRESS;
	NRF_RADIO->PCNF1 = RADIO_PROX_PCNF1;
	/* generic radio setup */
	NRF_RADIO->PREFIX0 = 0x46D7UL;
	NRF_RADIO->BASE0 = 0xEA8AF0B1UL;
	NRF_RADIO->BASE1 = 0xCC864569UL;
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
		(RADIO_INTENSET_RSSIEND_Enabled         << RADIO_INTENSET_RSSIEND_Pos)  |
		(RADIO_INTENSET_DISABLED_Enabled        << RADIO_INTENSET_DISABLED_Pos) |
		(RADIO_INTENSET_PAYLOAD_Enabled         << RADIO_INTENSET_PAYLOAD_Pos)
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
	NRF_RTC0->CC[0] = LF_FREQUENCY;
	NRF_RTC0->CC[1] = LF_FREQUENCY*2;
	NRF_RTC0->CC[2] = 0;
	NRF_RTC0->INTENSET = (
		(RTC_INTENSET_COMPARE0_Enabled   << RTC_INTENSET_COMPARE0_Pos) |
		(RTC_INTENSET_COMPARE1_Enabled   << RTC_INTENSET_COMPARE1_Pos) |
		(RTC_INTENSET_COMPARE2_Enabled   << RTC_INTENSET_COMPARE2_Pos)
	);
	NVIC_SetPriority(RTC0_IRQn, IRQ_PRIORITY_RTC0);
	NVIC_EnableIRQ(RTC0_IRQn);
	NRF_RTC0->TASKS_START = 1;
}
