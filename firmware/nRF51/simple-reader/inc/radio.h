/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
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
#ifndef __RADIO_H__
#define __RADIO_H__

#include <openbeacon-proto.h>

#define NRF_MAC_SIZE 5UL
#define NRF_PKT_SIZE ((uint32_t)sizeof(g_Beacon))

#define AES_KEY_SIZE 16

extern TBeaconEnvelope g_Beacon;

extern void radio_init(void);

#endif/*__RADIO_H__*/
