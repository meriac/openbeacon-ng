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
#include <flash.h>

/* RAM ring buffer */
static uint8_t buffer[BUF_SIZE];
static uint8_t *buf_head, *buf_tail;

static uint16_t current_page;
static uint16_t flash_error_count = 0;

#define BUF_LEN(HEAD_PTR,TAIL_PTR)			\
	(										\
	((HEAD_PTR) >= (TAIL_PTR)) ?			\
	((HEAD_PTR) - (TAIL_PTR)) :				\
	(BUF_SIZE - ((TAIL_PTR) - (HEAD_PTR)))	\
	)


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


static void flash_log_write(void)
{
	uint8_t *my_head = buf_head;

	flash_wakeup();

	while (	BUF_LEN(my_head,buf_tail) >= AT45D_PAGE_SIZE )
		{
			if (buffer + BUF_SIZE - buf_tail >= AT45D_PAGE_SIZE)
			{
				flash_write_buffer(1, 0, AT45D_PAGE_SIZE, buf_tail);
			} else {
				flash_write_buffer(1, 0, buffer + BUF_SIZE - buf_tail, buf_tail);
				flash_write_buffer(1, buffer + BUF_SIZE - buf_tail, AT45D_PAGE_SIZE - (buffer + BUF_SIZE - buf_tail), buffer);
			}

			flash_write_buffer_to_page(1, current_page++, 0);
			if (flash_wait_status(1) & AT45D_STATUS_EPE)
				flash_error_count++;

			buf_tail += AT45D_PAGE_SIZE;
			if (buf_tail - buffer >= BUF_SIZE)
				buf_tail -= BUF_SIZE;
		}

	flash_sleep_deep();
}


void flash_log_write_trigger(void)
{
	uint8_t *my_head = buf_head;

	if ( BUF_LEN(my_head,buf_tail) >= BUF_LEN_THRES )
		flash_log_write();
}


void flash_log_status(void)
{
	debug_printf("head: %i, tail: %i, page: %i, errors: %i\n\r", buf_head - buffer, buf_tail - buffer, current_page, flash_error_count);	
}


void flash_dump(void)
{
	uint8_t log_page[AT45D_PAGE_SIZE];
	uint16_t page_addr;
	uint16_t i;

	for (page_addr=FLASH_LOG_FIRST_PAGE; page_addr<FLASH_LOG_LAST_PAGE; page_addr++)
	{
		flash_read_page(page_addr, 0, AT45D_PAGE_SIZE, log_page);

		/* stop at the first page which is still erased */
		if (*log_page == 0xFF)
			break;

		debug_printf("\r\nPAGE %i", page_addr);

		for (i=0; i<AT45D_PAGE_SIZE; i++) {
			if (i % 32 == 0)
				debug_printf("\r\n");

			debug_printf(" %02X", *(log_page+i));
		}

		debug_printf("\r\n");
	}

	debug_printf("\r\n");

}


uint8_t flash_setup_logging(void)
{
	uint8_t FLASH_SIGNATURE[] = "OpenBeacon Flash Log"; 
	uint8_t signature[sizeof(FLASH_SIGNATURE)] = {0x00, };
	uint16_t i;

	flash_wakeup();

	/* check signature at the beginning of page FLASH_CONFIG_PAGE */

	flash_read_page(FLASH_LOG_CONFIG_PAGE, 0, sizeof(FLASH_SIGNATURE), signature);

	for (i=0; i<sizeof(FLASH_SIGNATURE); i++)
		if (FLASH_SIGNATURE[i] != signature[i])
			break;

	if (i == sizeof(FLASH_SIGNATURE)) {
		debug_printf("\r\nFound OpenBeacon signature. Dumping data to serial.\r\n");

		flash_dump();
	} else
		debug_printf("\r\nDid not find OpenBeacon signature.\r\n");

	debug_printf("Erasing 1st sector & signing...\r\n");

	/* erase 1st sector (pages 1024-2047) */
	flash_erase_sector(1024);
	flash_wait_status(1);

	/* sign page FLASH_CONFIG_PAGE */
	flash_write_page_through_buffer(1, FLASH_LOG_CONFIG_PAGE, 0, sizeof(FLASH_SIGNATURE), 1, FLASH_SIGNATURE);
	flash_wait_ready(1);

	/* init ring buffer */

	buf_head = buffer;
	buf_tail = buffer;
	current_page = FLASH_LOG_FIRST_PAGE;
	
	debug_printf("Logging starts at page %i\r\n", current_page);

	flash_sleep_deep();

	return current_page;
}
