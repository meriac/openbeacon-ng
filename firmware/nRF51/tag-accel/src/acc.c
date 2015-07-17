/***************************************************************
 *
 * OpenBeacon.org - nRF51 3D Accelerometer Routines
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
#include <acc.h>
#include <timer.h>

/* accelerometer initialization array */
static const uint8_t g_acc_init[][2] = {
	{ACC_REG_CTRL_REG1,     0x9F}, /* enabled accelerometer @ 5kHz/low power */
	{ACC_REG_CTRL_REG2,     0x00}, /* disable filters */
	{ACC_REG_CTRL_REG3,     0x06}, /* enable watermark and overflow interrupt */
	{ACC_REG_CTRL_REG4,     0x80}, /* enable block update, high resolution */
	{ACC_REG_CTRL_REG5,     0x40}, /* enable fifo */
	{ACC_REG_FIFO_CTRL_REG, 0x88}  /* stream mode , watermark @16 */
};
#define ACC_INIT_COUNT ((int)(sizeof(g_acc_init)/sizeof(g_acc_init[0])))

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

int acc_magnitude(void)
{
	int16_t acc[3];

	/* read accelerometer */
	acc_read(ACC_REG_OUT_X, sizeof(acc), (uint8_t*)&acc);

	/* get acceleration vector magnitude */
	return  sqrt32(
		((int)acc[0])*acc[0] +
		((int)acc[1])*acc[1] +
		((int)acc[2])*acc[2]
	)/64;
}

uint8_t acc_init(void)
{
	int i;
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

	/* initialize accelerometer */
	for(i=0; i<ACC_INIT_COUNT; i++)
		acc_write(g_acc_init[i][0], g_acc_init[i][1]);

	return 0;
}
