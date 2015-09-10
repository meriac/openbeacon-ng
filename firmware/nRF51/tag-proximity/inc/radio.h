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

/* set tracker packet TX power */
#define TX_POWER 4
#define TX_POWER_VALUE RADIO_TXPOWER_TXPOWER_Pos4dBm

/* set proximity packet TX power */
#define PX_POWER -20
#define PX_POWER_VALUE RADIO_TXPOWER_TXPOWER_Neg20dBm

#define RXTX_BASELOSS -4.0
#define BALUN_INSERT_LOSS -2.25
#define BALUN_RETURN_LOSS -10.0
#define ANTENNA_GAIN 0.5
#define RX_LOSS ((RXTX_BASELOSS/2.0)+BALUN_RETURN_LOSS+ANTENNA_GAIN)
#define TX_LOSS ((RXTX_BASELOSS/2.0)+BALUN_INSERT_LOSS+ANTENNA_GAIN)

extern void radio_init(uint32_t uid);

#endif/*__RADIO_H__*/
