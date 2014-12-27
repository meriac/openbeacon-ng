/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
 * Modified by Ciro Cattuto <ciro.cattuto@isi.it>
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
#include <acc.h>

#if CONFIG_FLASH_LOGGING
#include <log.h>
#endif

#define RXTX_BASELOSS -4.0
#define BALUN_INSERT_LOSS -2.25
#define BALUN_RETURN_LOSS -10.0
#define ANTENNA_GAIN 0.5
#define RX_LOSS ((RXTX_BASELOSS/2.0)+BALUN_RETURN_LOSS+ANTENNA_GAIN)
#define TX_LOSS ((RXTX_BASELOSS/2.0)+BALUN_INSERT_LOSS+ANTENNA_GAIN)

/* proximity power sequence */
static uint32_t prox_txpower_sequence[] = {
	RADIO_TXPOWER_TXPOWER_Neg20dBm,
	RADIO_TXPOWER_TXPOWER_Neg20dBm,
	RADIO_TXPOWER_TXPOWER_Neg20dBm,
	RADIO_TXPOWER_TXPOWER_Neg12dBm,
	RADIO_TXPOWER_TXPOWER_Neg4dBm
};
#define PROX_TXPOWER_SEQUENCE_LENGTH (sizeof(prox_txpower_sequence) / sizeof(uint32_t))

static volatile uint32_t g_time, g_time_offset, g_pkt_tracker_ticks;
static volatile uint16_t g_ticks_offset;
static volatile uint8_t g_request_tx;
static uint8_t g_listen_ratio;
static uint8_t g_nrf_state;
static int8_t g_rssi;
static uint8_t prox_txpower_index;
static uint8_t payload_received;

#define STATUS_FORCE_REPORT_PERIOD	300
static uint32_t g_time_status_reported;

#if CONFIG_FLASH_LOGGING
#define STATUS_FORCE_LOG_PERIOD		900
static uint32_t g_time_status_logged;
#endif

static TBeaconNgProx g_pkt_prox ALIGN4;
static uint8_t g_pkt_prox_enc[sizeof(g_pkt_prox)] ALIGN4;

static TBeaconNgProx g_pkt_prox_rx ALIGN4;
static uint8_t g_pkt_prox_rx_enc[sizeof(g_pkt_prox_rx)] ALIGN4;

static TBeaconNgTracker g_pkt_tracker ALIGN4;
static uint8_t g_pkt_tracker_enc[sizeof(g_pkt_tracker)] ALIGN4;

/* don't start DC/DC converter for voltages below 2.3V */
#define NRF_DCDC_STARTUP_VOLTAGE 23

#define NRF_MAC_SIZE 5UL
#define NRF_PROX_SIZE sizeof(TBeaconNgProx)
#define NRF_TRACKER_SIZE sizeof(TBeaconNgTracker)

#define NRF_STATE_IDLE           0
#define NRF_STATE_TX_PROX        1
#define NRF_STATE_RX_PROX        2
#define NRF_STATE_RX_PROX_PACKET 3
#define NRF_STATE_RX_PROX_BLINK  4
#define NRF_STATE_TX_TRACKER     5

#define RADIO_TRACKER_TXADDRESS 1
#define RADIO_TRACKER_TXPOWER (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_TRACKER_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_TRACKER_SIZE             << RADIO_PCNF1_MAXLEN_Pos)

#define RADIO_PROX_TXADDRESS 0
#define RADIO_PROX_TXPOWER(PX_POWER_VALUE) ((PX_POWER_VALUE) << RADIO_TXPOWER_TXPOWER_Pos)
#define RADIO_PROX_PCNF1 \
		(RADIO_PCNF1_WHITEEN_Enabled  << RADIO_PCNF1_WHITEEN_Pos) |\
		(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |\
		((NRF_MAC_SIZE-1UL)           << RADIO_PCNF1_BALEN_Pos)   |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_STATLEN_Pos) |\
		(NRF_PROX_SIZE                << RADIO_PCNF1_MAXLEN_Pos)


inline uint32_t get_time(void)
{
	return g_time;
}

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

#if !CONFIG_ACCEL_SLEEP
		if (!hibernate)
#else
		if (!hibernate && !sleep)
#endif
		{
			/* schedule tracker TX */
			if(!g_request_tx)
				/* wait for random(2^5) slots */
				g_request_tx = rng(5);

			/* measure battery voltage once per second */
			adc_start();
		}
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

#if !CONFIG_ACCEL_SLEEP
		if (!hibernate)
#else
		if (!hibernate && !sleep)
#endif	
		{
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
	}

	/* listen for CONFIG_PROX_WINDOW_MS every second */
	if(NRF_RTC0->EVENTS_COMPARE[2])
	{
		/* acknowledge event */
		NRF_RTC0->EVENTS_COMPARE[2] = 0;

		/* stop HF clock */
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;

#if CONFIG_PROXIMITY_BLINK
		if(g_nrf_state == NRF_STATE_RX_PROX_BLINK)
		{
			g_nrf_state = NRF_STATE_IDLE;
			/* light LED */
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			/* retrigger LED blink */
			NRF_RTC0->CC[2] = NRF_RTC0->COUNTER + MILLISECONDS(1);
		}
		else
#endif /* CONFIG_PROXIMITY_BLINK */
		{
			/* set next state */
			g_nrf_state = NRF_STATE_IDLE;
			/* stop radio */
			NRF_RADIO->TASKS_DISABLE = 1;
			/* disable DC-DC converter */
			NRF_POWER->DCDCEN = 0;

#if CONFIG_PROXIMITY_BLINK
			/* disable LED */
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
#endif /* CONFIG_PROXIMITY_BLINK */
		}
	}
}


void POWER_CLOCK_IRQ_Handler(void)
{
	uint32_t ticks;

	/* always transmit proximity packet */
	if(NRF_CLOCK->EVENTS_HFCLKSTARTED)
	{
		/* acknowledge event */
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		/* set & record proximity TX power */
		NRF_RADIO->TXPOWER = RADIO_PROX_TXPOWER(prox_txpower_sequence[prox_txpower_index]);
		g_pkt_prox.p.prox.tx_power =
			(int8_t) (prox_txpower_sequence[prox_txpower_index] & 0xFF);
		if (++prox_txpower_index == PROX_TXPOWER_SEQUENCE_LENGTH)
			prox_txpower_index = 0;

		/* update proximity time */
		g_pkt_prox.p.prox.epoch = g_time;
		/* adjust transmitted time by time
		 * it takes to encrypt the packet */
		ticks = NRF_RTC0->COUNTER;
		g_pkt_prox.p.prox.ticks += (ticks+g_ticks_offset);
		/* encrypt data */
		aes_encr(
			&g_pkt_prox,
			&g_pkt_prox_enc,
			sizeof(g_pkt_prox_enc),
			CONFIG_SIGNATURE_SIZE
		);
		/* remember time it took to encrypt packet for next TX */
		g_pkt_prox.p.prox.ticks = NRF_RTC0->COUNTER-ticks;

		/* set first packet pointer */
		NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_prox_enc;
		/* transmit proximity packet */
		NRF_RADIO->TASKS_TXEN = 1;
	}
}


static void radio_on_prox_packet(uint16_t delta_t)
{
	int i;
	TBeaconNgSighting *slot;

	/* ignore replayed packets from myself */
	if(g_pkt_prox_rx.p.prox.uid == g_pkt_prox.p.prox.uid)
		return;

#if CONFIG_EPOCH_PROXIMITY
	/* if we lack a valid epoch time, get it from proximity packet */
	if ( g_time < VALID_EPOCH_THRES && g_pkt_prox_rx.p.prox.epoch > VALID_EPOCH_THRES )
	{
		g_time = g_pkt_prox_rx.p.prox.epoch;
		status_flags |= FLAG_TIME_RESET;
	}
#endif /* CONFIG_EPOCH_PROXIMITY */

	/* maintain epoch time offset */
	if(g_pkt_prox_rx.p.prox.epoch > (g_time+g_time_offset))
	{
		g_time_offset = g_pkt_prox_rx.p.prox.epoch - g_time;

		/* adjust fine-grained time by decryption time */
		g_ticks_offset =
			(delta_t + g_pkt_prox_rx.p.prox.ticks) -
			((uint16_t)NRF_RTC0->COUNTER);
	}

	/* remember proximity sighting */
	slot = g_pkt_tracker.p.sighting;
	for(i=0; i<CONFIG_SIGHTING_SLOTS; i++)
	{
		/* a tag we have already seen */
		if (slot->uid == g_pkt_prox_rx.p.prox.uid) {
			/* overwrite older reading with higher TX power */
			if ( g_pkt_prox_rx.p.prox.tx_power < slot->tx_power )
			{
				slot->tx_power = g_pkt_prox_rx.p.prox.tx_power;
				slot->rx_power = g_rssi;
			}
			break;
		}

		/* use first free entry */
		if (!slot->uid)
		{
			slot->uid = g_pkt_prox_rx.p.prox.uid;
			slot->tx_power = g_pkt_prox_rx.p.prox.tx_power;
			slot->rx_power = g_rssi;
			break;
		}

		slot++;
	}
}


void RADIO_IRQ_Handler(void)
{
	uint32_t ticks;

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

				/* reset rssi measurement */
				g_rssi = 0;

				/* reset payload received flag */
				payload_received = 0;

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
				/* set default next state */
				g_nrf_state = NRF_STATE_IDLE;
				/* transmit pending tracker packets in a random slot */
				if(g_request_tx)
				{
					g_request_tx--;
					if(!g_request_tx)
						/* divert to tracker TX state */
						g_nrf_state = NRF_STATE_TX_TRACKER;
				}

				if(g_nrf_state == NRF_STATE_TX_TRACKER)
				{
					/* reconfigure radio for tracker TX */
					NRF_RADIO->FREQUENCY = CONFIG_TRACKER_CHANNEL;
					NRF_RADIO->TXPOWER = RADIO_TRACKER_TXPOWER;
					NRF_RADIO->TXADDRESS = RADIO_TRACKER_TXADDRESS;
					NRF_RADIO->PCNF1 = RADIO_TRACKER_PCNF1;

					/* set protocol and epoch */
					g_pkt_tracker.proto = RFBPROTO_BEACON_NG_SIGHTING;
					g_pkt_tracker.epoch = g_time;

					/* if the tracker packet contains no sightings,
					   or if we have not reported status for too long,
					   prepare a status packet */
					if( (!g_pkt_tracker.p.sighting[0].uid) ||
						 (g_time - g_time_status_reported >= STATUS_FORCE_REPORT_PERIOD) )
					{
						g_pkt_tracker.proto = RFBPROTO_BEACON_NG_STATUS;

						g_pkt_tracker.p.status.ticks = NRF_RTC0->COUNTER + g_ticks_offset + g_pkt_tracker_ticks;						
						g_pkt_tracker.p.status.rx_loss = (int16_t)((RX_LOSS*100)+0.5);
						g_pkt_tracker.p.status.tx_loss = (int16_t)((TX_LOSS*100)+0.5);
						g_pkt_tracker.p.status.acc_x = acc_get(0);
						g_pkt_tracker.p.status.acc_y = acc_get(1);
						g_pkt_tracker.p.status.acc_z = acc_get(2);
						g_pkt_tracker.p.status.voltage = adc_bat();
						g_pkt_tracker.p.status.boot_count = boot_count;

						g_pkt_tracker.p.status.flags = status_flags;

#if CONFIG_FLASH_LOGGING
						g_pkt_tracker.p.status.flash_log_free_blocks = flash_log_free_blocks();
						if (!flash_log_running())
							g_pkt_tracker.p.status.flags |= FLAG_LOG_STOPPED;
#endif /* CONFIG_FLASH_LOGGING */

#if CONFIG_ACCEL_SLEEP
						if (moving)
							g_pkt_tracker.p.status.flags |= FLAG_MOVING;
#endif /* CONFIG_ACCEL_SLEEP */

						/* if we have just rebooted, set upper byte of flags to reset reason */
						if (status_flags & FLAG_BOOT)
						{
							g_pkt_tracker.p.status.flags &= 0x00FF;
							g_pkt_tracker.p.status.flags |=
							  (uint8_t) ( (reset_reason & 0x0F) | ((reset_reason >> 12) & 0x70) ) << 8;
						}

#if !CONFIG_FLASH_LOGGING
/* if we do not log to flash, reset status flags here.
   otherwise only reset when we log a status packet to flash */
						status_flags = 0;
#endif /* !CONFIG_FLASH_LOGGING */

						g_time_status_reported = g_time;
					}
					
#if CONFIG_FLASH_LOGGING
					/* log sightings to flash,
					   and log status if long enough time has elapsed */
					if ( (g_pkt_tracker.proto == RFBPROTO_BEACON_NG_SIGHTING) ||
						 (g_pkt_tracker.proto == RFBPROTO_BEACON_NG_STATUS &&
						  g_time - g_time_status_logged >= STATUS_FORCE_LOG_PERIOD) )
					{
						flash_log(sizeof(g_pkt_tracker) - CONFIG_SIGNATURE_SIZE, (uint8_t *) &g_pkt_tracker);

						if (g_pkt_tracker.proto == RFBPROTO_BEACON_NG_STATUS)
						{
							g_time_status_logged = g_time;
							status_flags = 0;
						}
					}
#endif /* CONFIG_FLASH_LOGGING */
					
					/* measure encryption time */
					ticks = NRF_RTC0->COUNTER;

					/* encrypt packet */
					aes_encr(
						&g_pkt_tracker,
						&g_pkt_tracker_enc,
						sizeof(g_pkt_tracker_enc),
						CONFIG_SIGNATURE_SIZE
					);
					/* set first packet pointer */
					NRF_RADIO->PACKETPTR = (uint32_t)&g_pkt_tracker_enc;
					/* start tracker TX */
					NRF_RADIO->TASKS_TXEN = 1;

					/* update encryption time */
					g_pkt_tracker_ticks = NRF_RTC0->COUNTER - ticks;
				}
				else
				{
					/* stop HF clock */
					NRF_CLOCK->TASKS_HFCLKSTOP = 1;
					/* disable DC-DC converter */
					NRF_POWER->DCDCEN = 0;
				}
				break;
			}

			case NRF_STATE_TX_TRACKER:
			{
				/* set next state */
				g_nrf_state = NRF_STATE_IDLE;

				/* reconfigure radio back to proximity */
				NRF_RADIO->FREQUENCY = CONFIG_PROX_CHANNEL;
				NRF_RADIO->TXADDRESS = RADIO_PROX_TXADDRESS;
				NRF_RADIO->PCNF1 = RADIO_PROX_PCNF1;

				/* stop HF clock */
				NRF_CLOCK->TASKS_HFCLKSTOP = 1;
				/* disable DC-DC converter */
				NRF_POWER->DCDCEN = 0;

				/* confirm tracker transmission */
				memset(&g_pkt_tracker.p, 0, sizeof(g_pkt_tracker.p));
				g_request_tx = 0;
				break;
			}
		}
	}

	if(NRF_RADIO->EVENTS_END)
	{
		/* acknowledge event */
		NRF_RADIO->EVENTS_END = 0;

		/* received packet */
		if(	(g_nrf_state == NRF_STATE_RX_PROX_PACKET) && 
			payload_received &&
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

	if (NRF_RADIO->EVENTS_PAYLOAD)
	{
		payload_received = 1;

		/* acknowledge event */
		NRF_RADIO->EVENTS_PAYLOAD = 0;
	}
}


void radio_init(uint32_t uid)
{
	/* reset variables */
	g_time = g_time_offset = 0;
	g_pkt_tracker_ticks = 0;
	g_ticks_offset = 0;
	g_listen_ratio = 0;
	g_nrf_state = 0;
	g_request_tx = 0;
	g_rssi = 0;
	prox_txpower_index = 0;

	/* initialize proximity packet */
	memset(&g_pkt_prox, 0, sizeof(g_pkt_prox));
	g_pkt_prox.p.prox.uid = uid;

	/* initialize tracker packet */
	memset(&g_pkt_tracker, 0, sizeof(g_pkt_tracker));
	g_pkt_tracker.uid = uid;

	/* start random number genrator */
	rng_init();
	/* initialize AES encryption engine */
	aes_init(uid);
	/* initialize ADC battery voltage measurements */
	adc_init();

	/* setup default radio settings for proximity mode */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos;
	NRF_RADIO->FREQUENCY = CONFIG_PROX_CHANNEL;
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
		(RADIO_INTENSET_END_Enabled             << RADIO_INTENSET_END_Pos)      |
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
