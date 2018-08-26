/***************************************************************
 *
 * OpenBeacon.org - log tag sigtings to external flash
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
#include <openbeacon-proto.h>

#include <log.h>
#include <timer.h>
#include <flash.h>
#include <heatshrink_encoder.h>

#ifndef CONFIG_LOG_BUFFER_COUNT
#define CONFIG_LOG_BUFFER_COUNT 32
#endif/*CONFIG_LOG_BUFFER_COUNT*/

#ifdef  LOG_TAG
static uint32_t g_page_count, g_page;
static TBeaconProxSightingPage g_log_page;
static uint32_t g_log_page_pos;
static bool g_first;
static heatshrink_encoder g_hse; 

typedef struct {
	TBeaconProxSighting buffer[CONFIG_LOG_BUFFER_COUNT];
	volatile int count;
	int head, tail;
} TLogBuffer;

/* Log all sightings in interrupt context - drain in main loop */
static TLogBuffer g_log;

static uint32_t log_scan_for_first_free_page(void)
{
	int i;
	uint8_t buffer[CONFIG_FLASH_PAGESIZE];
	uint32_t page, data, *p;

	/* find first page completely set to zero */ 
	for(page=0; page<g_page_count; page++)
	{
		/* read first dword in sector */
		flash_page_read (page, (uint8_t*)&data, sizeof(data));
		if( data!=0xFFFFFFFFUL )
			continue;

		/* verify complete sector */
		flash_page_read (page, buffer, sizeof(buffer));
		p = (uint32_t*)&buffer;
		for(i=0; i<(CONFIG_FLASH_PAGESIZE/4); i++)
			if(*p++ != 0xFFFFFFFFUL)
				continue;

		/* bail out if sector is truly empty */
		break;
	}

	return page;
}
#endif/*LOG_TAG*/

uint8_t log_init(uint32_t tag_id)
{
	(void)tag_id;

	/* initialize flash */
	if(flash_init())
		return 1;

#ifdef  LOG_TAG
	uint32_t t;

	/* initialize compression library */
	heatshrink_encoder_reset(&g_hse); 

	/* wake up flash */
	flash_sleep(0);

	/* issue flag for first sector */
	g_first = true;

	/* print detected flash memory size */
	t = flash_size();
	g_page_count = t / CONFIG_FLASH_PAGESIZE;
	debug_printf("- Initialized %u bytes of log memory\n\r", t);

	/* find first free page in external flash memory */
	g_page = log_scan_for_first_free_page();
	debug_printf("- Log size %u page%c...\n\r", g_page, g_page==1 ? '.':'s');

	/* put flash to sleep again */
	flash_sleep(1);

#endif/*LOG_TAG*/
	return 0;
}

void log_sighting(uint32_t epoch_local, uint32_t epoch_remote,
	uint32_t tag_id, uint8_t power, int8_t angle)
{
#ifndef LOG_TAG
	(void)epoch_local;
	(void)epoch_remote;
	(void)tag_id;
	(void)power;
	(void)angle;
#else /*LOG_TAG*/
	TBeaconProxSighting *log;

	if(g_log.count<CONFIG_LOG_BUFFER_COUNT)
	{
		log = &g_log.buffer[g_log.head++];
		if(g_log.head>=CONFIG_LOG_BUFFER_COUNT)
			g_log.head = 0;

		/* remember sighting */
		log->epoch_local = epoch_local;
		log->epoch_remote = epoch_remote;
		log->power = power;
		log->angle = angle;
		log->tag_id = tag_id;
		/* update counter */
		g_log.count++;
	}
#endif/*LOG_TAG*/
}

#ifdef  LOG_TAG
static uint8_t log_read(TBeaconProxSighting *log)
{
	if(!g_log.count)
		return 0;

	/* get buffered entry */
	memcpy(log, &g_log.buffer[g_log.tail++], sizeof(*log));
	if(g_log.tail>=CONFIG_LOG_BUFFER_COUNT)
		g_log.tail = 0;

	__disable_irq();
	g_log.count--;
	__enable_irq();

	return 1;
}
#endif/*LOG_TAG*/

void log_dump_escaped(uint8_t type, const uint8_t* data, uint32_t size)
{
	uint8_t c;

	/* issue frame start */
	default_putchar(0xFF);
	default_putchar(type);

	while(size--)
	{
		c = *data++;
		default_putchar(c);
		/* escape FF by issuing code 0x00 */
		if(c == 0xFF)
			default_putchar(0x00);
	}
}

int log_process(void)
{
#ifdef  LOG_TAG
	int res;
	size_t ipos;
	size_t in,out;
	HSE_poll_res pres;
	TBeaconProxSighting log;

	res = 0;

	while(log_read(&log) && (g_page<g_page_count))
	{
		ipos = 0;

		do {
			if(heatshrink_encoder_sink(&g_hse, ((uint8_t*)&log)+ipos, sizeof(log)-ipos, &in)<0)
				goto error;

			ipos += in;

			do {
				out = 0;
				if((pres = heatshrink_encoder_poll(
					&g_hse,
					&g_log_page.buffer[g_log_page_pos],
					sizeof(g_log_page.buffer) - g_log_page_pos,
					&out))<0)
					goto error;

				g_log_page_pos += out;

				if( g_log_page_pos>=sizeof(g_log_page.buffer) )
				{
					g_log_page.length = g_log_page_pos;

					/* mark first page */
					if(g_first)
					{
						g_first = false;
						g_log_page.length |= BEACON_PROXSIGHTING_PAGE_MARKER;
					}

					g_log_page.crc32 = crc32(&g_log_page, sizeof(g_log_page)-sizeof(g_log_page.crc32));

					/* write to flash */
					flash_sleep(0);
					timer_wait(MILLISECONDS(1));
					flash_page_write(g_page++, (uint8_t*)&g_log_page, sizeof(g_log_page));
					timer_wait(MILLISECONDS(10));
					flash_sleep(1);

					/* start over again */
					g_log_page_pos = 0;

					/* increment return packet counter */
					res++;
				}
			} while( pres == HSER_POLL_MORE );
		} while ( ipos < sizeof(log) );
	}
	return res;

	/* restart encoder upon error */
error:
	/* initialize compression library */
	heatshrink_encoder_reset(&g_hse);
	/* reset settings */
	g_log_page_pos = 0;
	/* issue new marker */
	g_first = true;

#endif/*LOG_TAG*/
	/* return empty */
	return 0;
}

void log_dump(uint32_t tag_id)
{
#ifndef LOG_TAG
	(void)tag_id;
#else /*LOG_TAG*/
	int i;
	uint32_t page;
	uint8_t buffer[CONFIG_FLASH_PAGESIZE];

	/* wake up flash */
	flash_sleep(0);

	while(1)
	{
		/* wait for button press to start data dump */
		debug_printf("- Press [BUTTON] shortly to start data dump, or 3 seconds to erase device [0x%08X] ...\n\r", tag_id);
		while(!nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
		{
			/* issue transmission start */
			log_dump_escaped(0x01, (uint8_t*)&tag_id, sizeof(tag_id));
			/* issue transmission end */
			default_putchar(0xFF);
			default_putchar(0x03);

			timer_wait(MILLISECONDS(749));
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			timer_wait(MILLISECONDS(1));
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
		}
		/* turn off LED */
		nrf_gpio_pin_clear(CONFIG_LED_PIN);

		/* measure button press */
		i = 0;
		do {
			/* turn off LED to indicate operation */
			timer_wait(MILLISECONDS(500));
			nrf_gpio_pin_set(CONFIG_LED_PIN);
			timer_wait(MILLISECONDS(250));
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
			/* cancel after three seconds */
			if(++i>=3)
			{
				/* erase flash */
				flash_erase();
				/* wait for erasing to terminate */
				do {
					timer_wait(MILLISECONDS(90));
					nrf_gpio_pin_set(CONFIG_LED_PIN);
					timer_wait(MILLISECONDS(10));
					nrf_gpio_pin_clear(CONFIG_LED_PIN);
				} while (!(flash_status()&0x80));

				/* put flash to sleep again */
				flash_sleep(1);
				debug_printf(" [DONE]\n\r");
			}
		} while (nrf_gpio_pin_read(CONFIG_SWITCH_PIN));

		debug_printf("- Start dumping data...\n\r");

		/* issue transmission start */
		log_dump_escaped(0x01, (uint8_t*)&tag_id, sizeof(tag_id));
		/* issue transmission end */
		default_putchar(0xFF);
		default_putchar(0x03);

		/* iterate over all pages */
		for(page=0; page<g_page; page++)
		{
			/* blink acknowledgement for every 16th page */
			if(!(page&0xF))
				nrf_gpio_pin_set(CONFIG_LED_PIN);
			/* read page from flash */
			flash_page_read(page, buffer, sizeof(buffer));
			nrf_gpio_pin_clear(CONFIG_LED_PIN);

			/* dump page on UART */
			log_dump_escaped(0x02, buffer, sizeof(buffer));
		}
		/* issue transmission end */
		default_putchar(0xFF);
		default_putchar(0x03);

		/* print confirmation */
		debug_printf("\n\r[DONE]\n\r");

		/* wait if dump period is too short, wait for button release */
		while(nrf_gpio_pin_read(CONFIG_SWITCH_PIN));

		/* turn LED on again to indicate end of operation */
		nrf_gpio_pin_set(CONFIG_LED_PIN);
	}
#endif/*LOG_TAG*/
}
