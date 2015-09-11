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
#include <adc.h>
#include <rng.h>
#include <timer.h>
#include <openbeacon-proto.h>

/* derived config values */
#define CONFIG_TRACKER_SLOW_INFO_RATIO ((uint32_t)((CONFIG_TRACKER_SLOW_INFO_SECONDS*1000UL)/(CONFIG_PROX_SPACING_MS*CONFIG_PROX_LISTEN_RATIO)))

typedef struct {
	uint32_t uid, count;
	int rx_power, tx_power;
} TProximityEntry;

static volatile uint32_t g_time, g_time_offset;
static TBeaconNgProx g_pkt_prox ALIGN4;
static TBeaconNgTracker g_pkt_tracker ALIGN4;
static int8_t g_acc_channel[3];
static uint8_t g_boot_count;
static uint32_t g_slow_ratio_counter;
static uint16_t g_tracker_flags;
static TProximityEntry g_sightings[CONFIG_SIGHTING_SLOTS];
static uint32_t g_sightings_sent, g_sightings_count;

void tracker_second_tick(void)
{
	/* increment time */
	g_time++;

	/* measure battery voltage once per second */
	adc_start();
}

const void* tracker_px(uint16_t listen_wait_ms)
{
	g_pkt_prox.epoch = g_time;
	g_pkt_prox.listen_wait_ms = listen_wait_ms;
	g_pkt_prox.tx_power = PX_POWER;
	return &g_pkt_prox;
}

const void* tracker_tx(uint16_t listen_wait_ms, uint16_t tx_delay_us)
{
	int i;
	(void)tx_delay_us;
	TBeaconNgSighting *dst;
	TProximityEntry *src;

	/* announce next listening slot */
	g_pkt_tracker.listen_wait_ms = listen_wait_ms;
	/* update counter to guarantee uniquely encrypted packets */
	g_pkt_tracker.counter++;

	/* ensure to send out a status regularly */
	g_sightings_sent++;
	if(!g_sightings_count || (g_sightings_sent>=CONFIG_TRACKER_STATUS_RATIO))
		g_sightings_sent = 0;
	else
	{
		/* send out sightings */
		g_pkt_tracker.proto = RFBPROTO_BEACON_NG_SIGHTING;

		/* erase empty slots */
		i = g_sightings_count;
		memset(&g_pkt_tracker.p.sighting[i],
			0, sizeof(g_pkt_tracker.p.sighting[0])*(CONFIG_SIGHTING_SLOTS-i));

		/* copy proimity data to tracker package */
		dst = g_pkt_tracker.p.sighting;
		src = g_sightings;
		while(g_sightings_count--)
		{
			/* store average readings */
			dst->uid = src->uid;
			dst->tx_power = src->tx_power;
			dst->rx_power = src->rx_power / src->count;
			/* process next slot */
			dst++;
			src++;
		}
		/* reset sigthings buffer */
		memset(&g_sightings, 0, sizeof(g_sightings));

		/* return pointer to packet - needs to be in SRAM */
		return &g_pkt_tracker;
	}

	/* prepare slow tracker status information */
	if(!g_slow_ratio_counter)
	{
		g_pkt_tracker.proto = RFBPROTO_BEACON_NG_SSTATUS;
		g_pkt_tracker.p.sstatus.flags = g_tracker_flags;
		g_pkt_tracker.p.sstatus.px_power = (int16_t)((PX_POWER*100)+0.5);
		g_pkt_tracker.p.sstatus.tx_power = (int16_t)((TX_POWER*100)+0.5);
		g_pkt_tracker.p.sstatus.tx_loss = (int16_t)((TX_LOSS*100)+0.5);
		g_pkt_tracker.p.sstatus.rx_loss = (int16_t)((RX_LOSS*100)+0.5);
		g_pkt_tracker.p.sstatus.git_hash_32 = 0;
		g_pkt_tracker.p.sstatus.firmware_version = 0;
		g_pkt_tracker.p.sstatus.flash_log_blocks_total = 0;
	}
	else
	{
		g_pkt_tracker.proto = RFBPROTO_BEACON_NG_FSTATUS;
		g_pkt_tracker.p.fstatus.flags = g_tracker_flags;
		g_pkt_tracker.p.fstatus.voltage = adc_bat();
		g_pkt_tracker.p.fstatus.boot_count = g_boot_count;
		g_pkt_tracker.p.fstatus.epoch = g_time;
		memcpy(&g_pkt_tracker.p.fstatus.acc, g_acc_channel,
				sizeof(g_pkt_tracker.p.fstatus.acc));
	}

	/* increment slow ratio counter */
	g_slow_ratio_counter++;
	if(g_slow_ratio_counter >= CONFIG_TRACKER_SLOW_INFO_RATIO)
		g_slow_ratio_counter = 0;

	/* return pointer to packet - needs to be in SRAM */
	return &g_pkt_tracker;
}

void tracker_receive(uint32_t uid, int8_t tx_power, int8_t rx_power)
{
	int i;
	TProximityEntry *sighting = g_sightings;

	/* store sightings *, aggregate same readings */
	for(i=0; i<CONFIG_SIGHTING_SLOTS; i++, sighting++)
		if(!sighting->uid)
		{
			sighting->uid = uid;
			sighting->count = 1;
			sighting->tx_power = tx_power;
			sighting->rx_power = rx_power;
			g_sightings_count++;
		}
		else
			/* aggregate entries with the same tx power */
			if((sighting->uid==uid) && (sighting->tx_power==tx_power))
			{
				sighting->count++;
				sighting->rx_power+= rx_power;
			}
}

void tracker_init(uint32_t uid)
{
	/* initialize variables */
	g_slow_ratio_counter = 0;
	g_time = g_time_offset = 0;
	memset(&g_pkt_tracker, 0, sizeof(g_pkt_tracker));
	g_pkt_tracker.uid = uid;
	memset(&g_pkt_prox, 0, sizeof(g_pkt_prox));
	g_pkt_prox.uid = uid;

	/* update boot count */
	g_boot_count = (uint8_t)(NRF_POWER->GPREGRET++);

	/* ensure that tracker pkt counter starts counting
	   at random offset after power cycle */
	g_pkt_tracker.counter = (uint16_t)rng(16);
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

