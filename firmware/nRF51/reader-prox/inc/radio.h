/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
 *
 * Copyright 2013-2015 Milosch Meriac <meriac@openbeacon.org>
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
#ifndef __RADIO_H__
#define __RADIO_H__

#ifndef RADIO_MAX_PKT_BUFFERS
#define RADIO_MAX_PKT_BUFFERS 128
#endif/*RADIO_MAX_PKT_BUFFERS*/

#ifdef  RADIO_OLD_FORMAT
#  define NRF_TRACKER_SIZE 16
#  define NRF_TRACKER_PREFIX  0x80UL
#  define NRF_TRACKER_ADDRESS 0x40C04080UL
#else
#  define NRF_TRACKER_SIZE 32
#  define NRF_TRACKER_PREFIX 0x46UL //NRF_RADIO->PREFIX0 = 0x46D7UL
//#  define NRF_TRACKER_ADDRESS 0xEA8AF0B1UL
#  define NRF_TRACKER_ADDRESS 0xCC864569UL
#endif/*RADIO_OLD_FORMAT*/

typedef struct
{
	int8_t rssi;
	uint8_t buf[NRF_TRACKER_SIZE];
} TBeaconBuffer;

extern void radio_init(void);
extern int radio_packet_count(void);
extern BOOL radio_rx(TBeaconBuffer *buf);


#endif/*__RADIO_H__*/
