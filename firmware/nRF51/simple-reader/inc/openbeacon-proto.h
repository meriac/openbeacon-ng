/****************************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
 *
 ****************************************************************************

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


#ifndef __OPENBEACON_PROTO_H__
#define __OPENBEACON_PROTO_H__

#define CONFIG_TRACKER_CHANNEL 81
#define CONFIG_PROX_CHANNEL 76
#define CONFIG_SIGNATURE_SIZE 6

#define BEACON_SIGHTING_SLOTS 3

#define RFBPROTO_BEACON_NG_TRACKER  30
#define RFBPROTO_BEACON_NG_SIGHTING 31
#define RFBPROTO_BEACON_NG_STATUS   32
#define RFBPROTO_BEACON_NG_PROX     33

typedef struct
{
	int8_t tx_power;
	int8_t rx_power;
	uint32_t uid;
} PACKED TBeaconNgSighting;

typedef union
{
	TBeaconNgSighting sighting[BEACON_SIGHTING_SLOTS];
	uint8_t raw[16];
} PACKED TBeaconNgPayload;

typedef struct
{
	uint8_t proto;
	int8_t tx_power;
	uint32_t uid;
	TBeaconNgPayload p;
	uint32_t seq;
 	uint8_t signature[CONFIG_SIGNATURE_SIZE];
} PACKED TBeaconNgTracker;

typedef struct
{
} PACKED TBeaconNgProxAnnounce;

typedef union
{
	uint8_t raw[8];
} PACKED TBeaconNgProxPayload;

typedef struct
{
	uint8_t proto;
	int8_t tx_power;
	TBeaconNgProxPayload p;
	uint8_t signature[CONFIG_SIGNATURE_SIZE];
} PACKED TBeaconNgProx;

#endif/*__OPENBEACON_PROTO_H__*/
