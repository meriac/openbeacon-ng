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
#include <main.h> 
#include <log.h>
#include <heatshrink_encoder.h> 
#include <flash.h>
#include <radio.h>
#include <timer.h>


/* flash chip signature */
static uint8_t flash_signature[] = PROGRAM_VERSION;

/* RAM ring buffer */
static uint8_t buffer[BUF_SIZE];
static uint8_t *buf_head, *buf_tail;

#define BUF_LEN(HEAD_PTR,TAIL_PTR)			\
	(										\
	((HEAD_PTR) >= (TAIL_PTR)) ?			\
	((HEAD_PTR) - (TAIL_PTR)) :				\
	(BUF_SIZE - ((TAIL_PTR) - (HEAD_PTR)))	\
	)

/* block buffer */
static TLogBlock LogBlock ALIGN4;
static uint16_t current_block;
static uint16_t flash_log_last_block;
static uint16_t seq = 1;

/* logging status */
static uint16_t flash_error_count = 0;
static uint8_t log_running = 0;
static uint16_t log_wrap_count = 0;
static uint16_t log_buffer_overrun_count = 0;
static uint16_t log_compression_error = 0;

#if FLASH_LOG_COMPRESSION
/* Heatshrink encoder */
static heatshrink_encoder hse;
#endif


uint16_t flash_log(uint16_t len, uint8_t *data)
{
	uint8_t *my_tail = buf_tail;

	/* check for buffer overflow */
	if ( BUF_SIZE - BUF_LEN(buf_head,my_tail) <= len )
	{
		log_buffer_overrun_count++;
		status_flags |= ERROR_LOG_BUF_OVERRUN;
		return 0;
	}

	/* copy to ring buffer */
	while (len--)
	{
		*buf_head++ = *data++;

		if (buf_head - buffer == BUF_SIZE)
			buf_head = buffer;
	}

	return len;
}


static void block_init(void)
{
	memset(LogBlock.data, 0, LOG_BLOCK_DATA_SIZE);

	LogBlock.env.len = 0;
	LogBlock.env.seq = seq++;;
}


static uint8_t flash_log_block_write(uint16_t block_addr, uint8_t *buf)
{
	uint16_t page_addr = block_addr * BLOCK_PAGES;
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
		return 1;

	return 0;
}


static void flash_log_block_commit(void)
{
	uint8_t err;

	/* if block is empty, ignore it */
	if (LogBlock.env.len == 0)
		return;

	/* get timestamp */
	LogBlock.env.epoch = get_time();

	/* compute CRC, ignoring signature and CRC fields */
	LogBlock.env.crc = crc32( ((void *) &LogBlock) + 8, sizeof(LogBlock) - 8);

	/* write block */
	err = flash_log_block_write(current_block, (uint8_t *) &LogBlock );
	if (err) {
		flash_error_count++;
		status_flags |= ERROR_FLASH_WRITE;
	}

	current_block++;
	/* depending on configuration,
	   wrap around or stop logging on flash full */
	if (current_block > flash_log_last_block)
	{
		log_wrap_count++;
#if LOG_WRAPAROUND
		current_block = FLASH_LOG_FIRST_BLOCK;
#else
		log_running = 0;
#endif
	}

	/* if a write error occurred, try writing into next block */
	if (err && log_running)
	{
		if ( flash_log_block_write(current_block, (uint8_t *) &LogBlock ) )
		{
			flash_error_count++;
			status_flags |= ERROR_FLASH_WRITE;
		}

		current_block++;
		if (current_block > flash_log_last_block)
		{
			log_wrap_count++;
#if LOG_WRAPAROUND
			current_block = FLASH_LOG_FIRST_BLOCK;
#else
			log_running = 0;
			status_flags |= ERROR_FLASH_FULL;
#endif
		}
	}

	/* clean up block for next use */
	block_init();
}


#if FLASH_LOG_COMPRESSION

void flash_log_write(uint8_t flush_buf)
{
	uint8_t *my_head = buf_head;
	uint8_t *tail_copy = buf_tail;
	uint16_t chunk_size;

    HSE_sink_res sres = 0;
    HSE_poll_res pres = 0;
    HSE_finish_res fres = 0;
    size_t sink_sz;
    size_t poll_sz;

	/* proceed until there is data in the ring buffer
	   and there is space in the block buffer */
	while (	(BUF_LEN(my_head,buf_tail) > 0) &&
			(LOG_BLOCK_DATA_SIZE - LogBlock.env.len > COMPRESS_CHUNK_SIZE) )
	{
		/* set the size of the next _contiguous_ chunk to compress */
		if (buf_tail < my_head)
			chunk_size = my_head - buf_tail;
		else
			chunk_size = buffer + BUF_SIZE - buf_tail;

		/* trim it down to the compression chunk size */
		if (chunk_size > COMPRESS_CHUNK_SIZE )
			chunk_size = COMPRESS_CHUNK_SIZE;

		/* feed it to the encoder */
		sres = heatshrink_encoder_sink(&hse, buf_tail, chunk_size, &sink_sz);
		if (sres < 0)
			break;

		/* pull out the compressed stream */
		do {
            pres = heatshrink_encoder_poll(
            	&hse,
            	LogBlock.data + LogBlock.env.len,
            	LOG_BLOCK_DATA_SIZE - LogBlock.env.len,
            	&poll_sz);

            LogBlock.env.len += poll_sz;
        } while (pres == HSER_POLL_MORE);

        if (pres < 0)
        	break;

		/* advance tail of ring buffer */
		buf_tail += sink_sz;
		if (buf_tail >= buffer+BUF_SIZE)
			buf_tail -= BUF_SIZE;
	}

	/* on error, bail out and revert buffer tail to original position */
	if (pres < 0 || sres < 0)
	{
		log_compression_error++;
		status_flags |= ERROR_LOG_COMPRESS;
		buf_tail = tail_copy;
		return;
	}

	/* if block buffer is almost full, or if we are flushing the ring buffer,
	   flush the encoder and commit compressed data to flash memory */
	if ( (LOG_BLOCK_DATA_SIZE - LogBlock.env.len <= COMPRESS_CHUNK_SIZE ) || flush_buf )
		{
		/* signal encoder that we are done */
		fres = heatshrink_encoder_finish(&hse);

		/* on error, bail out and revert buffer tail to original position */
		if (fres < 0)
		{
			log_compression_error++;
			status_flags |= ERROR_LOG_COMPRESS;
			buf_tail = tail_copy;
			return;
		}

		/* if necessary, pull out remaining compressed data */
		if (fres == HSER_FINISH_MORE)
		{
			do {
    			pres = heatshrink_encoder_poll(
    				&hse,
        			LogBlock.data + LogBlock.env.len,
            		LOG_BLOCK_DATA_SIZE - LogBlock.env.len,
            		&poll_sz);

    			/* update block length */
        		LogBlock.env.len += poll_sz;
        		if (LogBlock.env.len >= LOG_BLOCK_DATA_SIZE)
        		{
					log_compression_error++;
					status_flags |= ERROR_LOG_COMPRESS;
					buf_tail = tail_copy;
					return;
        		}
    		} while (pres == HSER_POLL_MORE);
    	}

		/* on error, bail out and revert buffer tail to original position */
    	if (pres < 0)
		{
			log_compression_error++;
			status_flags |= ERROR_LOG_COMPRESS;
			buf_tail = tail_copy;
			return;
		}

		/* reset encoder */
		heatshrink_encoder_reset(&hse);

		/* commit block to flash */
		flash_log_block_commit();
	}
}

#else

void flash_log_write(uint8_t flush_buf)
{
	uint8_t *my_head = buf_head;
	uint16_t chunk_size;

	/* proceed until there is data in the ring buffer
	   and there is space in the block buffer */
	while ( (BUF_LEN(my_head,buf_tail) > 0) && (LogBlock.env.len < LOG_BLOCK_DATA_SIZE) )
	{
		/* set the size of the next chunk to copy */
		if (buf_tail < my_head)
			chunk_size = my_head - buf_tail;
		else
			chunk_size = buffer + BUF_SIZE - buf_tail;

		/* trim it according to available space in block buffer */
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

	/* if block buffer is full, or if we are flushing the ring buffer,
	   commit block to flash memory */
	if ( (LogBlock.env.len == LOG_BLOCK_DATA_SIZE) || flush_buf )
		flash_log_block_commit();
}

#endif /* FLASH_LOG_COMPRESSION */


void flash_log_write_trigger(void)
{
	uint8_t *my_head = buf_head;

	if ( log_running && (BUF_LEN(my_head,buf_tail) >= BUF_LEN_THRES) )
		flash_log_write(0);
}


void flash_log_flush(void)
{
	uint8_t *my_head = buf_head;

	/* flush ring buffer */
	while (BUF_LEN(my_head,buf_tail) > 0)
		flash_log_write(0);

	/* flush block buffer */
	flash_log_write(1);
}


void flash_log_status(void)
{
	debug_printf(
		"\n\rflash log status:\n\rrunning %i, wrapped %i, block: %i, block len: %i, head: %i, tail: %i, errors: %i, overruns: %i, compression: %i\n\r",
		log_running,
		log_wrap_count,
		current_block,
		LogBlock.env.len,
		buf_head - buffer, buf_tail - buffer,
		flash_error_count,
		log_buffer_overrun_count,
		log_compression_error
		);
}


/*
 base64 log dump:
 by running uudecode(1) on the dumped data
 we obtain a binary log file named after the tag ID
*/

char base64_char(uint8_t b)
{
	if (b < 26)
		return 'A' + b;
	else if (b < 52)
		return 'a' + (b - 26);
	else if (b < 62)
		return '0' + (b - 52);
	else if (b == 62)
		return '+';
	else if (b == 63)
		return '/';
	else
		return 0;
}

#define CHARS_PER_LINE 64

void base64_dump(uint16_t len, uint8_t *data)
{
	uint16_t i, j, printed_chars = 1;
	char c[4];

	for (i=0; i<len; i+=3,data+=3)
	{
		c[0] = base64_char( data[0] >> 2 );
		c[1] = base64_char( (data[0] & 0x03) << 4 | (data[1] & 0xF0) >> 4 );
		c[2] = base64_char( (data[1] & 0x0F) << 2 | (data[2] & 0xC0) >> 6 );
		c[3] = base64_char( (data[2] & 0x3F) );

		for (j=0; j<4; j++, printed_chars++)
		{
			debug_printf("%c", c[j]);
			if (printed_chars % CHARS_PER_LINE == 0)
				debug_printf("\n\r");
		}
	}

	if (len % 3 == 2)
	{
		c[0] = base64_char( data[0] >> 2 );
		c[1] = base64_char( (data[0] & 0x03) << 4 | (data[1] & 0x0F) );
		c[2] = base64_char( (data[1] & 0xF0) >> 2 );
		c[3] = '=';
	} else if (len % 3 == 1) {
		c[0] = base64_char( data[0] >> 2 );
		c[1] = base64_char( (data[0] & 0x03) << 4 );
		c[2] = '=';
		c[3] = '=';
	}

	if (len % 3)
	{
		for (j=0; j<4; j++, printed_chars++)
		{
			debug_printf("%c", c[j]);
			if (printed_chars % CHARS_PER_LINE == 0)
				debug_printf("\n\r");
		}
	}

	if (printed_chars % CHARS_PER_LINE)
		debug_printf("\n\r");
}


void flash_log_dump(void)
{
	uint8_t log_page[AT45D_PAGE_SIZE];
	uint16_t block_addr, page_addr;

	flash_wakeup();

	/* read and print flash chip signature */
	flash_read_page(FLASH_LOG_CONFIG_PAGE, 0, AT45D_PAGE_SIZE, log_page);
	log_page[AT45D_PAGE_SIZE-1] = 0;
	debug_printf("\n\r\n\rversion: %s", log_page);

	debug_printf("\n\r\n\rbegin-base64 0644 %08X.dat\n\r", LogBlock.env.uid);

	/* loop over blocks and dump them, stopping at the first invalid signature */
	for (block_addr=FLASH_LOG_FIRST_BLOCK; block_addr <= flash_log_last_block; block_addr++)
	{
		page_addr = block_addr*BLOCK_PAGES;
		flash_read_page(page_addr++, 0, AT45D_PAGE_SIZE, log_page);

		if ( *((uint32_t *) log_page) != BLOCK_SIGNATURE )
			break;

		base64_dump(AT45D_PAGE_SIZE, log_page);

		while ( page_addr < block_addr*BLOCK_PAGES+8 )
		{
			flash_read_page(page_addr++, 0, AT45D_PAGE_SIZE, log_page);
			base64_dump(AT45D_PAGE_SIZE, log_page);
		}
	}

	flash_sleep_deep();

	debug_printf("====\n\r\n\r");
}


uint16_t flash_log_free_blocks(void)
{
#if LOG_WRAPAROUND
	/* if we wrap around on flash full,
	   always report total number of log blocks */
	return flash_log_last_block;
#else
	return flash_log_last_block - current_block + 1;
#endif
}


uint8_t flash_log_running(void)
{
	return log_running;
}


uint8_t flash_setup_logging(uint32_t uid)
{
	uint8_t log_page[AT45D_PAGE_SIZE];
	uint16_t block_addr = FLASH_LOG_FIRST_BLOCK;

	/* init ring buffer */
	buf_head = buffer;
	buf_tail = buffer;

	/* init block buffer */
	block_init();
	LogBlock.env.signature = BLOCK_SIGNATURE;
	LogBlock.env.log_version = 1;
	LogBlock.env.compressed = FLASH_LOG_COMPRESSION;
	LogBlock.env.uid = uid;

	/* wake up flash */
	flash_wakeup();

	/* if we get here with the key pressed
	   and we release it after more than 1 second,
	   then erase the entire flash memory */
	if (nrf_gpio_pin_read(CONFIG_SWITCH_PIN) && blink_wait_release() > 1000)
	{
		blink_fast(5);
		debug_printf("\n\rERASING FLASH\n\r");
		flash_erase_chip();
		flash_wait_ready(1);

		hibernate = 1;
	}

	/* erase block of configuration page */
	flash_erase_block(FLASH_LOG_CONFIG_PAGE);
	flash_wait_ready(1);

	/* sign page FLASH_LOG_CONFIG_PAGE with PROGRAM_VERSION string */
	flash_write_page_through_buffer(1, FLASH_LOG_CONFIG_PAGE, 0, sizeof(flash_signature), 0, flash_signature);
	flash_wait_ready(1);

	/* set index of last block */
	flash_log_last_block = flash_get_num_pages() / BLOCK_PAGES - 1;
	
	/* find first non-signed block */
	for (block_addr=FLASH_LOG_FIRST_BLOCK; block_addr<=flash_log_last_block; block_addr++)
	{
		// debug_printf("\n\rchecking block %i\n\r", block_addr);
		flash_read_page(block_addr*BLOCK_PAGES, 0, 4, log_page);

		if ( *((uint32_t *) log_page) != BLOCK_SIGNATURE )
			break;
	}

	current_block = block_addr;
	flash_sleep_deep();

	/* return error if no block is available */
	if (block_addr > flash_log_last_block)
		return 1;

#if FLASH_LOG_COMPRESSION
	/* initialize data compressor */
	heatshrink_encoder_reset(&hse);
#endif

	log_running = 1;
	debug_printf("\n\rLogging starts at block %i of %i\n\r", current_block, flash_log_last_block);

	return 0;
}

