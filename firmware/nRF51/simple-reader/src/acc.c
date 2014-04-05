/***************************************************************
 *
 * OpenBeacon.org - nRF51 3D Accelerometer Routines
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
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
#include <acc.h>
#include <timer.h>

void acc_write(uint8_t cmd, uint8_t data)
{
	nrf_gpio_pin_clear(CONFIG_ACC_nCS);

	/* send command */
	SPI_ACC->TXD = cmd;
	while (!SPI_ACC->EVENTS_READY);
	SPI_ACC->EVENTS_READY = 0;
	/* dummy read */
	SPI_ACC->RXD;

	/* send command */
	SPI_ACC->TXD = data;
	while (!SPI_ACC->EVENTS_READY);
	SPI_ACC->EVENTS_READY = 0;
	/* dummy read */
	SPI_ACC->RXD;

	nrf_gpio_pin_set(CONFIG_ACC_nCS);
}

void acc_read(uint8_t cmd, uint8_t len, uint8_t *data)
{
	nrf_gpio_pin_clear(CONFIG_ACC_nCS);

	/* send command */
	SPI_ACC->TXD = 0xC0|cmd;
	while (!SPI_ACC->EVENTS_READY);
	SPI_ACC->EVENTS_READY = 0;
	/* dummy read */
	SPI_ACC->RXD;

	while(len--)
	{
		/* send dummy zero */
		SPI_ACC->TXD = 0;
		while (!SPI_ACC->EVENTS_READY);
		SPI_ACC->EVENTS_READY = 0;

		*data++ = SPI_ACC->RXD;
	}

	nrf_gpio_pin_set(CONFIG_ACC_nCS);
}

uint8_t acc_init(void)
{
	uint8_t data;

	/* initialize GPIO */
	nrf_gpio_cfg_input(CONFIG_ACC_INT1, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(CONFIG_ACC_MISO, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_output(CONFIG_ACC_MOSI);
	nrf_gpio_cfg_output(CONFIG_ACC_SCK);
	nrf_gpio_cfg_output(CONFIG_ACC_nCS);

	/* mark inactive by default */
	nrf_gpio_pin_set(CONFIG_ACC_nCS);

	/* configure peripheral */
	SPI_ACC->PSELMISO = CONFIG_ACC_MISO;
	SPI_ACC->PSELMOSI = CONFIG_ACC_MOSI;
	SPI_ACC->PSELSCK = CONFIG_ACC_SCK;

	/* configure accelerometer for 8MHz */
	SPI_ACC->FREQUENCY = SPI_FREQUENCY_FREQUENCY_M8;
	SPI_ACC->CONFIG =
		(SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos) |\
		(SPI_CONFIG_CPHA_Trailing << SPI_CONFIG_CPHA_Pos) |\
		(SPI_CONFIG_CPOL_ActiveLow << SPI_CONFIG_CPOL_Pos);

	/* reset events */
	SPI_ACC->EVENTS_READY = 0U;

	/* enable SPI accelerometer peripheral */
	SPI_ACC->ENABLE =
		(SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);

	/* check accelerometer read */
	acc_read(ACC_REG_WHO_AM_I, sizeof(data), &data);
	if(data!=0x33)
		return 1;

	/* disable accelerometer */
	acc_write(ACC_REG_CTRL_REG1, 0x00);
	/* disable data ready interrupt */
	acc_write(ACC_REG_CTRL_REG3, 0x00);
	/* disable fifo */
	acc_write(ACC_REG_CTRL_REG5, 0x00);
	/* enable bypass mode */
	acc_write(ACC_REG_FIFO_CTRL_REG, 0x00);

	return 0;
}

