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

#ifndef __LOG_H__
#define __LOG_H__

#include <flash.h>

/* RAM ring buffer */
#define BUF_SIZE				4096
#define BUF_LEN_THRES			(BUF_SIZE / 3 * 2)

/* compression */
#define	FLASH_LOG_COMPRESSION	1
#define COMPRESS_CHUNK_SIZE		256
#define BLOCK_SPACE_MIN			(COMPRESS_CHUNK_SIZE + 32)

/* flash storage */
#define BLOCK_PAGES				8
#define FLASH_LOG_CONFIG_PAGE	0
#define FLASH_LOG_FIRST_BLOCK	1
#define LOG_WRAPAROUND			0 /* wrap when flash is full? */
#define BLOCK_SIGNATURE			0x0BEBAC00


/* 24-byte block envelope */
typedef struct {
	uint32_t signature;
	uint32_t crc;
	uint8_t	log_version;
	uint8_t compressed;
	uint8_t	transferred;
	uint8_t	reserved;
	uint32_t uid;
	uint32_t epoch;
	uint16_t seq;
	uint16_t len;
} PACKED TLogBlockEnvelope;

/* 8 flash pages (8 * 264 = 2122 bytes): 24-byte envelope + 2088 bytes of log data */
#define LOG_BLOCK_DATA_SIZE (AT45D_PAGE_SIZE * BLOCK_PAGES - sizeof(TLogBlockEnvelope))

typedef struct
{
	TLogBlockEnvelope env;
	uint8_t data[LOG_BLOCK_DATA_SIZE];

	/* paranoid: padding in case heatshrink_encoder_poll() overflows the output buffer */ 
	uint8_t safety[COMPRESS_CHUNK_SIZE];
} PACKED TLogBlock;


/* logging routines */

extern uint8_t flash_setup_logging(uint32_t uid);
extern uint16_t flash_log(uint16_t len, uint8_t *data);
extern void flash_log_write_trigger(void);
extern void flash_log_flush(void);
extern void flash_log_status(void);
extern void flash_log_dump(void);
extern uint8_t flash_log_running(void);
extern uint16_t flash_log_free_blocks(void);

#endif /*__LOG_H__*/
