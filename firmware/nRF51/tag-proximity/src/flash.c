/***************************************************************
 *
 * OpenBeacon.org - nRF51 Flash Routines
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
#include <flash.h>
#include <timer.h>


/* 1-byte AT45D commands */

#define AT45D_CMD_STATUS						0xD7
#define AT45D_CMD_IDENT							0x9F
#define AT45D_CMD_SLEEP							0xB9
#define AT45D_CMD_SLEEP_RESUME					0xAB
#define AT45D_CMD_SLEEP_DEEP					0x79

#define AT45D_CMD_READ_BUF1_LF					0xD1
#define AT45D_CMD_READ_BUF2_LF					0xD3
#define AT45D_CMD_WRITE_BUF1					0x84
#define AT45D_CMD_WRITE_BUF2					0x87

#define AT45D_CMD_ERASE_PAGE					0x81
#define AT45D_CMD_ERASE_BLOCK					0x50
#define AT45D_CMD_ERASE_SECTOR					0x7C 

#define AT45D_CMD_WRITE_PAGE_BUF1				0x88
#define AT45D_CMD_WRITE_PAGE_BUF2				0x89
#define AT45D_CMD_WRITE_PAGE_BUF1_ERASE			0x83
#define AT45D_CMD_WRITE_PAGE_BUF2_ERASE			0x86
#define AT45D_CMD_WRITE_PAGE_THROUGH_BUF1_ERASE	0x82	
#define AT45D_CMD_WRITE_PAGE_THROUGH_BUF2_ERASE	0x85
#define AT45D_CMD_WRITE_PAGE_THROUGH_BUF1		0x02
#define AT45D_CMD_REWRITE_PAGE_THROUGH_BUF1		0x58
#define AT45D_CMD_REWRITE_PAGE_THROUGH_BUF2		0x59

#define AT45D_CMD_READ_PAGE						0xD2
#define AT45D_CMD_READ_PAGE_BUF1				0x53
#define AT45D_CMD_READ_PAGE_BUF2				0x55

#define AT45D_CMD_COMPARE_PAGE_BUF1				0x60
#define AT45D_CMD_COMPARE_PAGE_BUF2				0x61

/* multi-byte AT45D commands */
static const uint8_t AT45D_CMD_RESET[] = {0xF0, 0x00, 0x00, 0x00};
static const uint8_t AT45D_CMD_CHIP_ERASE[] = {0xC7, 0x94, 0x80, 0x9A};
static const uint8_t AT45D_CMD_WP_ENABLE[] = {0x3D, 0x2A, 0x7F, 0xA9};
static const uint8_t AT45D_CMD_WP_DISABLE[] = {0x3D, 0x2A, 0x7F, 0x9A};


/* helper SPI macros */

#define SPI_CLEAR_NCS						\
do {										\
	nrf_gpio_pin_clear(CONFIG_FLASH_nCS);	\
} while (0);

#define SPI_SET_NCS							\
do {										\
	nrf_gpio_pin_set(CONFIG_FLASH_nCS);		\
} while (0);

#define SPI_SEND_BYTE(BYTE)				\
do {									\
	SPI_FLASH->TXD = (BYTE);			\
	while (!SPI_FLASH->EVENTS_READY);	\
	SPI_FLASH->EVENTS_READY = 0;		\
	SPI_FLASH->RXD;						\
} while (0);

#define SPI_RECV_BYTE(BYTE)				\
do {									\
	SPI_FLASH->TXD = 0;					\
	while (!SPI_FLASH->EVENTS_READY);	\
	SPI_FLASH->EVENTS_READY = 0;		\
	(BYTE) = SPI_FLASH->RXD;			\
} while (0);

/*
 Send the 3-byte address for 264-byte page addressing:
 15 page address bits + 9 byte offset bits.
*/
#define SPI_SEND_ADDR(PAGE_ADDR,BYTE_OFFSET)				\
do {														\
	SPI_FLASH->TXD = (uint8_t) (((PAGE_ADDR) >> 7) & 0xFF);	\
	while (!SPI_FLASH->EVENTS_READY);						\
	SPI_FLASH->EVENTS_READY = 0;							\
	SPI_FLASH->RXD;											\
															\
	SPI_FLASH->TXD =										\
		(uint8_t) (((PAGE_ADDR) & 0x7F) << 1) |				\
		(uint8_t) (((BYTE_OFFSET) & 0x100) >> 8);			\
	while (!SPI_FLASH->EVENTS_READY);						\
	SPI_FLASH->EVENTS_READY = 0;							\
	SPI_FLASH->RXD;											\
															\
	SPI_FLASH->TXD = (uint8_t) ((BYTE_OFFSET) & 0xFF);		\
	while (!SPI_FLASH->EVENTS_READY);						\
	SPI_FLASH->EVENTS_READY = 0;							\
	SPI_FLASH->RXD;											\
} while (0);



static void flash_cmd_read(uint8_t cmd, uint8_t len, uint8_t *data)
{
	SPI_CLEAR_NCS;

	/* send command */
	SPI_SEND_BYTE(cmd);

	while (len--)
		SPI_RECV_BYTE(*data++);

	SPI_SET_NCS;
}


static void flash_cmd_write(const uint8_t *cmd, uint8_t len)
{
	SPI_CLEAR_NCS;

	while (len--)
		SPI_SEND_BYTE(*cmd++);

	SPI_SET_NCS;
}


uint16_t flash_status(void)
{
	uint8_t data[2];

	flash_cmd_read(AT45D_CMD_STATUS, sizeof(data), data);

	/* status byte 1 to higher 8 bits
	   status byte 2 to lower 8 bits */
	return
		((((uint16_t) data[0]) & 0xFF) << 8) |
		(((uint16_t) data[1]) & 0xFF);
}


/* wait until chip is ready and return waiting iterations */
uint16_t flash_wait_ready(uint8_t wait_interval_ms)
{

	uint8_t status;
	uint16_t counter;

	SPI_CLEAR_NCS;

	/* send command */
	SPI_SEND_BYTE(AT45D_CMD_STATUS);

	/* clock out status bytes until ready flag is set */
	for (counter=0 ; ; counter++)
	{
		SPI_RECV_BYTE(status);
		if (status & AT45D_STATUS_READY)
			break;

		if (wait_interval_ms)
			timer_wait(wait_interval_ms * MILLISECONDS(1));
	}

	SPI_SET_NCS;

	return counter;
}


/* wait until chip is ready, then return status */
uint16_t flash_wait_status(uint8_t wait_interval_ms)
{
	uint8_t byte1, byte2;

	SPI_CLEAR_NCS;

	/* send command */
	SPI_SEND_BYTE(AT45D_CMD_STATUS);

	do {
		SPI_RECV_BYTE(byte1);
		SPI_RECV_BYTE(byte2);

		if (byte1 & AT45D_STATUS_READY)
			break;

		if (wait_interval_ms)
			timer_wait(wait_interval_ms * MILLISECONDS(1));
	} while (1);

	SPI_SET_NCS;

	/* status byte 1 to higher 8 bits
	   status byte 2 to lower 8 bits */
	return
		((((uint16_t) byte1) & 0xFF) << 8) |
		(((uint16_t) byte2) & 0xFF);
}


void flash_wakeup(void)
{
	SPI_CLEAR_NCS;
	timer_wait(MILLISECONDS(1));
	SPI_SET_NCS;
	timer_wait(MILLISECONDS(1));
}


void flash_sleep_deep(void)
{
	SPI_CLEAR_NCS;
	SPI_SEND_BYTE(AT45D_CMD_SLEEP_DEEP);
	SPI_SET_NCS;
}


void flash_sleep(void)
{
	SPI_CLEAR_NCS;
	SPI_SEND_BYTE(AT45D_CMD_SLEEP);
	SPI_SET_NCS;
}


void flash_sleep_resume(void)
{
	SPI_CLEAR_NCS;
	SPI_SEND_BYTE(AT45D_CMD_SLEEP_RESUME);
	SPI_SET_NCS;
}


void flash_reset(void)
{
	flash_cmd_write(AT45D_CMD_RESET, sizeof(AT45D_CMD_RESET));
}


void flash_set_sector_protection(uint8_t enable)
{
	if (enable)
		flash_cmd_write(AT45D_CMD_WP_ENABLE, sizeof(AT45D_CMD_WP_ENABLE));
	else
		flash_cmd_write(AT45D_CMD_WP_DISABLE, sizeof(AT45D_CMD_WP_DISABLE));
}


void flash_erase_page(uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	SPI_SEND_BYTE(AT45D_CMD_ERASE_PAGE);

	/* send page address + dummy byte offset */
	SPI_SEND_ADDR(page_addr,0);

	SPI_SET_NCS;
}


void flash_erase_block(uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	SPI_SEND_BYTE(AT45D_CMD_ERASE_BLOCK);

	/* send high 12 bits of page address + dummy byte offset */
	SPI_SEND_ADDR(page_addr & 0x7FF8,0);

	SPI_SET_NCS;
}


void flash_erase_sector(uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	SPI_SEND_BYTE(AT45D_CMD_ERASE_SECTOR);

	/* are we erasing a sector above 0 ? */
	if (page_addr & 0x7C00) /* check the 5-bit sector number */
		{
		/* send high 5 bits of page address + dummy byte offset */
		SPI_SEND_ADDR(page_addr & 0x7C00,0)
	} else { /* sector 0a or 0b */
		/* send high 12 bits of page address + dummy byte offset */
		SPI_SEND_ADDR(page_addr & 0x7FF0,0);
	}

	SPI_SET_NCS;
}


void flash_erase_chip(void)
{
	flash_cmd_write(AT45D_CMD_CHIP_ERASE, sizeof(AT45D_CMD_CHIP_ERASE));
}


void flash_read_buffer(uint8_t bufnum, uint16_t byte_offset, uint16_t len, uint8_t *data)
{
	uint16_t i;

	SPI_CLEAR_NCS;

	if (bufnum == 1)
	{
		SPI_SEND_BYTE(AT45D_CMD_READ_BUF1_LF);
	} else if (bufnum == 2)
	{
		SPI_SEND_BYTE(AT45D_CMD_READ_BUF2_LF);
	} else
		return;

	SPI_SEND_ADDR(0,byte_offset);

	for (i=0; i<len; i++)
		SPI_RECV_BYTE(*data++);

	SPI_SET_NCS;
}


void flash_write_buffer(uint8_t bufnum, uint16_t byte_offset, uint16_t len, uint8_t *data)
{
	uint16_t i;

	SPI_CLEAR_NCS;

	if (bufnum == 1)
	{
		SPI_SEND_BYTE(AT45D_CMD_WRITE_BUF1);
	} else if (bufnum == 2)
	{
		SPI_SEND_BYTE(AT45D_CMD_WRITE_BUF2);
	} else
		return;

	SPI_SEND_ADDR(0,byte_offset);

	for (i=0; i<len; i++)
		SPI_SEND_BYTE(*data++);

	SPI_SET_NCS;
}


void flash_write_buffer_to_page(uint8_t bufnum, uint16_t page_addr, uint8_t erase_page)
{
	SPI_CLEAR_NCS;

	if (erase_page) {
		if (bufnum == 1)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_BUF1_ERASE);
		} else if (bufnum == 2)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_BUF2_ERASE);
		} else
			return;
	} else {
		if (bufnum == 1)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_BUF1);
		} else if (bufnum == 2)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_BUF2);
		} else
			return;
	}

	SPI_SEND_ADDR(page_addr,0);

	SPI_SET_NCS;
}


void flash_read_page_to_buffer(uint8_t bufnum, uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	if (bufnum == 1)
	{
		SPI_SEND_BYTE(AT45D_CMD_READ_PAGE_BUF1);
	} else if (bufnum == 2)
	{
		SPI_SEND_BYTE(AT45D_CMD_READ_PAGE_BUF2);
	} else
		return;

	SPI_SEND_ADDR(page_addr,0);

	SPI_SET_NCS;
}


uint8_t flash_compare_page_to_buffer(uint8_t bufnum, uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	if (bufnum == 1)
	{
		SPI_SEND_BYTE(AT45D_CMD_COMPARE_PAGE_BUF1);
	} else if (bufnum == 2)
	{
		SPI_SEND_BYTE(AT45D_CMD_COMPARE_PAGE_BUF2);
	} else
		return -1;

	SPI_SEND_ADDR(page_addr,0);

	SPI_SET_NCS;

	if (flash_wait_status(1) & AT45D_STATUS_COMP)
		/* page/buffer content mismatch */
		return 1;
	else
		return 0;
}


void flash_read_page(uint16_t page_addr, uint16_t byte_offset, uint16_t len, uint8_t *data)
{
	uint8_t i;

	SPI_CLEAR_NCS;

	SPI_SEND_BYTE(AT45D_CMD_READ_PAGE);

	SPI_SEND_ADDR(page_addr,byte_offset);

	for (i=0; i<4; i++)
		SPI_SEND_BYTE(0);

	while (len--)
		SPI_RECV_BYTE(*data++);
	
	SPI_SET_NCS;
}


void flash_write_page_through_buffer(uint8_t bufnum, uint16_t page_addr, uint16_t byte_offset, uint16_t len, uint8_t erase_page, uint8_t *data)
{
	SPI_CLEAR_NCS;

	if (erase_page) {
		if (bufnum == 1)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_THROUGH_BUF1_ERASE);
		} else if (bufnum == 2)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_THROUGH_BUF2_ERASE);
		} else
			return;
	} else {
		if (bufnum == 1)
		{
			SPI_SEND_BYTE(AT45D_CMD_WRITE_PAGE_THROUGH_BUF1);
		} else
			return;
	}

	SPI_SEND_ADDR(page_addr,byte_offset);

	while (len--)
		SPI_SEND_BYTE(*data++);

	SPI_SET_NCS;
}


void flash_rewrite_page_through_buffer(uint8_t bufnum, uint16_t page_addr)
{
	SPI_CLEAR_NCS;

	if (bufnum == 1)
	{
		SPI_SEND_BYTE(AT45D_CMD_REWRITE_PAGE_THROUGH_BUF1);
	} else if (bufnum == 2)
	{
		SPI_SEND_BYTE(AT45D_CMD_REWRITE_PAGE_THROUGH_BUF2);
	} else
		return;

	SPI_SEND_ADDR(page_addr,0);

	SPI_SET_NCS;
}


static uint16_t flash_num_pages;

uint16_t flash_get_num_pages(void)
{
	return flash_num_pages;
}


static const uint8_t g_flash_id[] = {0x1f, 0x00, 0x00, 0x01, 0x00};

uint8_t flash_init(void)
{
	uint8_t data[5];

	/* initialize GPIO */
	nrf_gpio_cfg_input(CONFIG_FLASH_MISO, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_output(CONFIG_FLASH_MOSI);
	nrf_gpio_cfg_output(CONFIG_FLASH_SCK);
	nrf_gpio_cfg_output(CONFIG_FLASH_nRESET);
	nrf_gpio_cfg_output(CONFIG_FLASH_nCS);

	/* mark inactive by default */
	nrf_gpio_pin_set(CONFIG_FLASH_nRESET);
	nrf_gpio_pin_set(CONFIG_FLASH_nCS);

	/* configure peripheral */
	SPI_FLASH->PSELMISO = CONFIG_FLASH_MISO;
	SPI_FLASH->PSELMOSI = CONFIG_FLASH_MOSI;
	SPI_FLASH->PSELSCK = CONFIG_FLASH_SCK;

	/* configure flash for 8MHz */
	SPI_FLASH->FREQUENCY = SPI_FREQUENCY_FREQUENCY_M8;
	SPI_FLASH->CONFIG =
		(SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos) |\
		(SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |\
		(SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos);

	/* reset events */
	SPI_FLASH->EVENTS_READY = 0U;

	/* enable SPI flash peripheral */
	SPI_FLASH->ENABLE =
		(SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);

	/* wake up & reset flash chip */
	flash_wakeup();
	flash_reset();

	/* check for flash */
	flash_cmd_read(AT45D_CMD_IDENT, sizeof(data), data);

	/* set number of flash pages */
	switch (data[1])
	{
		case 0x25:	/* 8 Mbit chip */
			flash_num_pages = 1 << 12;
			break;
		case 0x28:	/* 64 Mbit chip */
			flash_num_pages = 1 << 15;
			break;
		default:
			return 1;
	}

	/* reset size for ID comparison */
	data[1] = 0;
	/* compare wildcard ID */
	if(memcmp(data, g_flash_id, sizeof(g_flash_id)))
		return 1;

	/* send flash to deep power down */
	flash_sleep_deep();

	return 0;
}

