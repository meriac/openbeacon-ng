/***************************************************************
 *
 * OpenBeacon.org - nRF51 Flash Routines
 *
 * Copyright 2013-2015 Milosch Meriac <milosch@meriac.com>
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

static const uint8_t g_flash_id[]={0x1f,0x00,0x00,0x01,0x00};
static uint8_t g_flash_size;
#define MEGABYTE(x) (x*1024UL*1024UL)

uint32_t flash_size(void)
{
	switch(g_flash_size)
	{
		case 0x25:
			return CONFIG_FLASH_PAGESIZE*4096UL;
		case 0x28:
			return CONFIG_FLASH_PAGESIZE*32768UL;
		default:
			return 0;
	}
}

static uint8_t flash_cmd(uint8_t cmd)
{
	/* send command */
	SPI_FLASH->TXD = cmd;
	while (!SPI_FLASH->EVENTS_READY);
	SPI_FLASH->EVENTS_READY = 0;

	/* read result */
	return SPI_FLASH->RXD;
}

static void flash_cmd_read(uint8_t cmd, uint32_t len, uint8_t *data)
{
	/* assert chipselect */
	nrf_gpio_pin_clear(CONFIG_FLASH_nCS);

	/* send command */
	flash_cmd(cmd);

	/* read result */
	while(len--)
		*data++ = flash_cmd(0x00);

	/* de-assert chipselect */
	nrf_gpio_pin_set(CONFIG_FLASH_nCS);
}

static void flash_cmd_seek(uint8_t cmd, uint32_t page)
{
	/* assert chipselect */
	nrf_gpio_pin_clear(CONFIG_FLASH_nCS);

	/* send read command */
	flash_cmd(cmd);

	/* calculate page & offset */
	page <<= 9;
	flash_cmd ((uint8_t)(page >> 16));
	flash_cmd ((uint8_t)(page >>  8));
	flash_cmd ((uint8_t)(page >>  0));
}

void flash_page_read(uint32_t page, uint8_t *data, uint32_t length)
{
	/* prepare read command */
	flash_cmd_seek(0x01, page);

	/* read result */
	while(length--)
		*data++ = flash_cmd(0x00);

	/* de-assert chipselect */
	nrf_gpio_pin_set(CONFIG_FLASH_nCS);
}

void flash_page_write(uint32_t page, const uint8_t *data, uint32_t length)
{
	/* prepare write command */
	flash_cmd_seek(0x02, page);

	/* write sector */
	while(length--)
		flash_cmd(*data++);

	/* de-assert chipselect */
	nrf_gpio_pin_set(CONFIG_FLASH_nCS);
}

void flash_sleep(int sleep)
{
	if(sleep)
	{
		/* send flash to deep power down */
		flash_cmd_read(0x79, 0, NULL);
	}
	else
	{
		/* wake up flash */
		timer_wait(MILLISECONDS(1));
		nrf_gpio_pin_clear(CONFIG_FLASH_nCS);
		timer_wait(MILLISECONDS(1));
		nrf_gpio_pin_set(CONFIG_FLASH_nCS);
		timer_wait(MILLISECONDS(1));
	}
}

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

	/* wake up flash */
	flash_sleep(0);

	/* check for flash */
	flash_cmd_read(0x9F, sizeof(data), data);
	/* remember flash size */
	g_flash_size = data[1];
	/* reset size for ID comparison */
	data[1] = 0;
	/* compare wildcard ID */
	if(memcmp(data, g_flash_id, sizeof(g_flash_id)))
		return 1;

	/* send flash to deep power down mode */
	flash_sleep(1);

	return 0;
}

