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

/* lookup sine table of r/z to degrees */
static const uint8_t g_asin7deg_table[] = {
	 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7,
	 7, 8, 8, 9, 9,10,10,10,11,11,12,12,13,13,14,14,
	15,15,16,16,16,17,17,18,18,19,19,20,20,21,21,22,
	22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,
	30,31,31,32,32,33,33,34,35,35,36,36,37,37,38,38,
	39,40,40,41,41,42,43,43,44,44,45,46,46,47,48,48,
	49,50,51,51,52,53,53,54,55,56,57,57,58,59,60,61,
	62,63,64,65,66,67,68,70,71,72,74,76,78,80,83,90
};

/* accelerometer initialization array */
static const uint8_t g_acc_init[][2] = {
	{ACC_REG_CTRL_REG1,     0x00}, /* disable accelerometer */
	{ACC_REG_CTRL_REG3,     0x00}, /* disable data ready interrupt */
	{ACC_REG_CTRL_REG4,     0x80}, /* enable block update */
	{ACC_REG_CTRL_REG5,     0x00}, /* disable fifo */
	{ACC_REG_FIFO_CTRL_REG, 0x00}  /* enable bypass mode */
};
#define ACC_INIT_COUNT ((int)(sizeof(g_acc_init)/sizeof(g_acc_init[0])))

static int8_t asin7deg(int8_t t)
{
	return (t>=0) ? g_asin7deg_table[t] : -g_asin7deg_table[-t];
}

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

uint16_t acc_magnitude(uint32_t* angle)
{
	int16_t acc[3], a, alpha;

	/* briefly turn on accelerometer */
	acc_write(ACC_REG_CTRL_REG1, 0x97);
	timer_wait(MILLISECONDS(2));
	acc_read(ACC_REG_OUT_X, sizeof(acc), (uint8_t*)&acc);
	acc_write(ACC_REG_CTRL_REG1, 0x00);

	/* get acceleration vector magnitude */
	a =  sqrt32(
		((uint32_t)acc[0])*acc[0] + 
		((uint32_t)acc[1])*acc[1] +
		((uint32_t)acc[2])*acc[2]
	)/64;

	/* calculate tag angle */
	if(angle)
	{
		if(!a)
			alpha = 127;
		else
		{
			alpha = (acc[2]*2)/a;
			if(alpha>127)
				alpha=127;
			else
				if(alpha<-127)
					alpha=-127;
		}
		*angle = asin7deg(alpha);
	}
	return a;
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
