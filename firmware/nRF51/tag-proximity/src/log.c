/***************************************************************
 *
 * OpenBeacon.org - flash logging routines
 *
 * Copyright 2014 Ciro Cattuto <ciro.cattuto@isi.it>
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
#include <log.h>
#include <heatshrink_encoder.h> 
#include <flash.h>
#include <radio.h>


/* flash chip signature */
static uint8_t flash_signature[] = PROGRAM_VERSION;

/* RAM ring buffer */
static uint8_t buffer[BUF_SIZE];
static uint8_t *buf_head, *buf_tail;

static uint16_t current_page; /* FIXME */
static uint16_t current_block;
static uint16_t flash_error_count = 0;

#define BUF_LEN(HEAD_PTR,TAIL_PTR)			\
	(										\
	((HEAD_PTR) >= (TAIL_PTR)) ?			\
	((HEAD_PTR) - (TAIL_PTR)) :				\
	(BUF_SIZE - ((TAIL_PTR) - (HEAD_PTR)))	\
	)

/* block buffer */
static TLogBlock LogBlock ALIGN4;
static uint16_t seq = 1;

/* Heatshrink encoder */
// static heatshrink_encoder hse;


uint16_t flash_log(uint16_t len, uint8_t *data)
{
	uint8_t *my_tail = buf_tail;

	while (len--)
	{
		*buf_head++ = *data++;

		if (buf_head - buffer == BUF_SIZE)
			buf_head = buffer;

		/* bail out on buffer overflow */
		if (buf_head == my_tail)
			break;
	}

	return len;
}


static void block_init(void)
{
	memset((void *) &LogBlock, 0, sizeof(LogBlock));

	LogBlock.env.signature = BLOCK_SIGNATURE;
	LogBlock.env.log_version = 1;
	LogBlock.env.compressed = 1;
	LogBlock.env.seq = seq++;;
}


static uint8_t flash_log_block_write(uint8_t *buf)
{
	uint16_t page_addr = current_block*BLOCK_PAGES;
	uint8_t i;

	flash_wakeup();

	/* erase current block */
	flash_erase_block(page_addr);
	flash_wait_ready(1);

	/* write block, i.e., 8 pages */
	for (i=0; i<BLOCK_PAGES; i++, page_addr++)
	{
		flash_write_page_through_buffer(1, page_addr, 0, AT45D_PAGE_SIZE, 0, buf + AT45D_PAGE_SIZE*i);
		if (flash_wait_status(1) & AT45D_STATUS_EPE)
			break;

		/* check page content */
		if ( flash_compare_page_to_buffer(1, page_addr) )
			break;
	}

	flash_sleep_deep();

	if (i < BLOCK_PAGES)
	{
		flash_error_count++;
		return 1;
	}

	return 0;
}


static uint8_t flash_log_write(void)
{
	uint8_t *my_head = buf_head;
	uint16_t chunk_size;

	while ( (BUF_LEN(my_head,buf_tail) > 0) && (LogBlock.env.len < LOG_BLOCK_DATA_SIZE) )
	{
		if (buf_tail < my_head)
			chunk_size = my_head - buf_tail;
		else
			chunk_size = buffer + BUF_SIZE - buf_tail;

		if (chunk_size > (LOG_BLOCK_DATA_SIZE - LogBlock.env.len) )
			chunk_size = LOG_BLOCK_DATA_SIZE - LogBlock.env.len;

		/* copy into block buffer */
		memcpy(LogBlock.data + LogBlock.env.len, buf_tail, chunk_size);
		LogBlock.env.len += chunk_size;

		/* advance tail of ring buffer */
		buf_tail += chunk_size;
		if (buf_tail - buffer == BUF_SIZE)
			buf_tail = buffer;
	}

	/* if block buffer is full, write block to flash memory */
	if ( LogBlock.env.len == LOG_BLOCK_DATA_SIZE )
	{
		/* get timestamp */
		LogBlock.env.epoch = get_time();

		/* compute CRC, ignoring signature and CRC fields */
		LogBlock.env.crc = crc32( (void *) &LogBlock + 8, sizeof(LogBlock) - 8);

		/* write block */
		flash_log_block_write( (uint8_t *) &LogBlock );

		/* clean up block for next use */
		block_init();
	}

	return 0;
}



void flash_log_write_trigger(void)
{
	uint8_t *my_head = buf_head;

	if ( BUF_LEN(my_head,buf_tail) >= BUF_LEN_THRES )
		flash_log_write();
}


void flash_log_status(void)
{
	debug_printf("head: %i, tail: %i, block: %i, errors: %i\n\r", buf_head - buffer, buf_tail - buffer, current_block, flash_error_count);	
}


void flash_log_dump(void)
{
	uint8_t log_page[AT45D_PAGE_SIZE];
	uint16_t block_addr, page_addr;
	uint16_t i;

	flash_wakeup();

	for (block_addr=FLASH_LOG_FIRST_BLOCK; block_addr <= FLASH_LOG_LAST_BLOCK; block_addr++)
	{
		page_addr = block_addr*BLOCK_PAGES;
		flash_read_page(page_addr++, 0, AT45D_PAGE_SIZE, log_page);

		if ( *((uint32_t *) log_page) != BLOCK_SIGNATURE )
				break;

		//debug_printf("\r\nBLOCK %i", block_addr);

		while (page_addr <= block_addr*BLOCK_PAGES+8)
		{
			for (i=0; i<AT45D_PAGE_SIZE; i++)
			{
				if (i % 32 == 0)
					debug_printf("\n\r");

				debug_printf(" %02X", *(log_page+i));
			}
			debug_printf("\n\r");

			flash_read_page(page_addr++, 0, AT45D_PAGE_SIZE, log_page);
		}
	}

	debug_printf("\n\r");

	flash_sleep_deep();
}


uint8_t flash_setup_logging(uint32_t uid)
{
	uint8_t log_page[AT45D_PAGE_SIZE];
	uint16_t block_addr = FLASH_LOG_FIRST_BLOCK;

	/* init ring buffer */
	buf_head = buffer;
	buf_tail = buffer;
	current_page = 0; /* FIXME */

	/* init block buffer */
	block_init();
	LogBlock.env.uid = uid;
	
	/* wake up flash */
	flash_wakeup();

	/* if we get here with the key pressed, erase the entire flash memory */
	if (nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
	{
		debug_printf("\n\rERASING FLASH\n\r");
		flash_erase_chip();
		flash_wait_ready(1);
	}

	/* erase block 0 */
	flash_erase_block(0);
	flash_wait_ready(1);

	/* sign page 0 with PROGRAM_VERSION string */
	flash_write_page_through_buffer(1, 0, 0, sizeof(flash_signature), 0, flash_signature);
	flash_wait_ready(1);
	
	/* find first non-signed block */
	for (block_addr=FLASH_LOG_FIRST_BLOCK; block_addr<=FLASH_LOG_LAST_BLOCK; block_addr++)
	{
		// debug_printf("\n\rchecking block %i\n\r", block_addr);
		flash_read_page(block_addr*BLOCK_PAGES, 0, 4, log_page);

		if ( *((uint32_t *) log_page) != BLOCK_SIGNATURE )
			break;
	}

	/* return error if no block is available */
	if (block_addr > FLASH_LOG_LAST_BLOCK)
		return 1;

	flash_sleep_deep();

	current_block = block_addr;
	debug_printf("\n\rLogging starts at block %i\n\r", current_block);

	return 0;
}


