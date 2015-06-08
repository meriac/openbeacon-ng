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
#ifndef __ACC_H__
#define __ACC_H__

/* LIS3DH register definition */

#define ACC_REG_STATUS_REG_AUX    0x07

#define ACC_REG_OUT_ADC1          0x08
#define ACC_REG_OUT_ADC1_L        0x08
#define ACC_REG_OUT_ADC1_H        0x09

#define ACC_REG_OUT_ADC2          0x0A
#define ACC_REG_OUT_ADC2_L        0x0A
#define ACC_REG_OUT_ADC2_H        0x0B

#define ACC_REG_OUT_ADC3          0x0C
#define ACC_REG_OUT_ADC3_L        0x0C
#define ACC_REG_OUT_ADC3_H        0x0D

#define ACC_REG_INT_COUNTER_REG   0x0E
#define ACC_REG_WHO_AM_I          0x0F
#define ACC_REG_TEMP_CFG_REG      0x1F
#define ACC_REG_CTRL_REG1         0x20
#define ACC_REG_CTRL_REG2         0x21
#define ACC_REG_CTRL_REG3         0x22
#define ACC_REG_CTRL_REG4         0x23
#define ACC_REG_CTRL_REG5         0x24
#define ACC_REG_CTRL_REG6         0x25
#define ACC_REG_REFERENCE         0x26
#define ACC_REG_STATUS_REG2       0x27

#define ACC_REG_OUT_X             0x28
#define ACC_REG_OUT_X_L           0x28
#define ACC_REG_OUT_X_H           0x29

#define ACC_REG_OUT_Y             0x2A
#define ACC_REG_OUT_Y_L           0x2A
#define ACC_REG_OUT_Y_H           0x2B

#define ACC_REG_OUT_Z             0x2C
#define ACC_REG_OUT_Z_L           0x2C
#define ACC_REG_OUT_Z_H           0x2D

#define ACC_REG_FIFO_CTRL_REG     0x2E
#define ACC_REG_FIFO_SRC_REG      0x2F
#define ACC_REG_INT1_CFG          0x30

#if CONFIG_ACCEL_MOTION
extern uint8_t moving;
#endif

#if CONFIG_ACCEL_SLEEP
extern uint8_t sleep;
#endif

extern uint8_t acc_init(void);
extern void acc_write(uint8_t cmd, uint8_t data);
extern void acc_read(uint8_t cmd, uint8_t len, uint8_t *data);
extern void acc_sample(void);
extern int16_t acc_get(uint8_t axis);

#endif/*__ACC_H__*/
