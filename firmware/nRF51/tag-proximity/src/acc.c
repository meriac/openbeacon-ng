/***************************************************************
 *
 * OpenBeacon.org - nRF51 3D Accelerometer Routines
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
#include <openbeacon-proto.h>
#include <main.h>
#include <acc.h>
#include <timer.h>

/* accelerometer initialization array */
static const uint8_t g_acc_init[][2] = {
	{ACC_REG_CTRL_REG1,     0x00}, /* disable accelerometer */
	{ACC_REG_CTRL_REG3,     0x00}, /* disable data ready interrupt */
	{ACC_REG_CTRL_REG4,     0x80}, /* enable block update */
	{ACC_REG_CTRL_REG5,     0x00}, /* disable fifo */
	{ACC_REG_FIFO_CTRL_REG, 0x00}  /* enable bypass mode */
};
#define ACC_INIT_COUNT ((int)(sizeof(g_acc_init)/sizeof(g_acc_init[0])))

/* most recent acceleration measurement */
static int16_t acc[3];


#if CONFIG_ACCEL_MOTION
#define ACC_BUFFER_LEN   10
#define ACC_DELTA_THRES  100000L

static int16_t acc_buffer[ACC_BUFFER_LEN][3];
static int32_t acc_avg[3];
static uint8_t acc_buffer_index;
static uint8_t acc_buffer_full;

uint8_t moving = 1;
#endif /* CONFIG_ACCEL_MOTION */


#if CONFIG_ACCEL_SLEEP
#define ACC_SLEEP_THRES  300

uint8_t sleep = 0;
static uint16_t sleep_counter;
#endif /* CONFIG_ACCEL_SLEEP */



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


#if CONFIG_ACCEL_MOTION
static void acc_process_sample(void)
{
	int16_t acc_delta;
    uint32_t acc_delta_norm = 0;
    uint8_t i, j;

	if (acc_buffer_full)
	{
		acc_delta_norm = 0;

		for (i=0; i<3; i++)
		{
			acc_avg[i] = 0;
			for (j=0; j<ACC_BUFFER_LEN; j++)
				acc_avg[i] += acc_buffer[j][i];
			acc_avg[i] /= ACC_BUFFER_LEN;

			acc_delta = acc[i] - acc_avg[i];
			acc_delta_norm += acc_delta * acc_delta;
		}

		if (acc_delta_norm < ACC_DELTA_THRES)
 		{
			moving = 0;
#if CONFIG_ACCEL_SLEEP
			if (!sleep && ++sleep_counter > ACC_SLEEP_THRES)
				sleep = 1;
#endif /* CONFIG_ACCEL_SLEEP */
		} else {
			moving = 1;
			acc_buffer_index = 0;
			acc_buffer_full = 0;
#if CONFIG_ACCEL_SLEEP
			if (sleep)
				status_flags |= FLAG_WOKEUP;
			sleep = 0;
			sleep_counter = 0;
#endif /* CONFIG_ACCEL_SLEEP */
		} 
	}

	for (i=0; i<3; i++)
		acc_buffer[acc_buffer_index][i] = acc[i];

	acc_buffer_index++;
	if (acc_buffer_index == ACC_BUFFER_LEN)
		acc_buffer_index = 0;

	if (!acc_buffer_full && !acc_buffer_index)
		acc_buffer_full = 1;
}
#endif /* CONFIG_ACCEL_MOTION */


void acc_sample(void)
{
	/* briefly turn on accelerometer */
	acc_write(ACC_REG_CTRL_REG1, 0x97);
	timer_wait(MILLISECONDS(2));
	acc_read(ACC_REG_OUT_X, sizeof(acc), (uint8_t*)&acc);
	acc_write(ACC_REG_CTRL_REG1, 0x00);

#if CONFIG_ACCEL_MOTION
	acc_process_sample();
#endif
}


inline int16_t acc_get(uint8_t axis)
{
	return acc[axis];
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

#if CONFIG_ACCEL_MOTION
	acc_buffer_index = 0;
	acc_buffer_full = 0;
	moving = 1;
#endif /* CONFIG_ACCEL_MOTION */

#if CONFIG_ACCEL_SLEEP
	sleep = 0;
	sleep_counter = 0;
#endif /* CONFIG_ACCEL_SLEEP */

	/* make first measurement */
	acc_sample();

	return 0;
}
