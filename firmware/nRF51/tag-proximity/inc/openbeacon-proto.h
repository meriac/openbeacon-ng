/****************************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright 2013-2015 Milosch Meriac <milosch@meriac.com>
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

#define BEACONLOG_SIGHTING 0x01

#define CONFIG_TRACKER_CHANNEL 81
#define CONFIG_PROX_CHANNEL 76

#define CONFIG_SIGNATURE_SIZE 5
#define CONFIG_SIGHTING_SLOTS 3

#define RFBPROTO_BEACON_NG_SIGHTING 33
#define RFBPROTO_BEACON_NG_FSTATUS  35
#define RFBPROTO_BEACON_NG_SSTATUS  36
#define RFBPROTO_BEACON_NG_PROX     37

typedef struct
{
	uint16_t flags;
	uint8_t voltage;
	uint8_t boot_count;
	uint32_t epoch;
	uint8_t pading[5];
	int8_t acc[3];
	uint16_t flash_log_blocks_filled;
} PACKED TBeaconNgStatusFast;

typedef struct
{
	uint16_t flags;
	int16_t px_power;
	int16_t tx_power;
	int16_t tx_loss;
	int16_t rx_loss;
	uint32_t git_hash_32;
	uint16_t firmware_version;
	uint16_t flash_log_blocks_total;
} PACKED TBeaconNgStatusSlow;

typedef struct
{
	uint32_t uid;
	int8_t tx_power;
	int8_t rx_power;
} PACKED TBeaconNgSighting;

typedef union
{
	TBeaconNgStatusFast fstatus;
	TBeaconNgStatusSlow sstatus;
	TBeaconNgSighting sighting[CONFIG_SIGHTING_SLOTS];
	uint8_t raw[18];
} PACKED TBeaconNgPayload;

typedef struct
{
	uint32_t uid;
	uint16_t counter;
	uint16_t listen_wait_ms;
	TBeaconNgPayload p;
	uint8_t proto;
 	uint8_t signature[CONFIG_SIGNATURE_SIZE];
} PACKED TBeaconNgTracker;

typedef struct
{
	uint32_t uid;
	uint32_t epoch;
	uint16_t listen_wait_ms;
	int8_t tx_power;
	uint8_t signature[CONFIG_SIGNATURE_SIZE];
} PACKED TBeaconNgProx;

typedef struct
{
	uint16_t icrc16;
	uint8_t protocol;
	uint8_t interface;
	uint16_t reader_id;
	uint16_t size;
} PACKED TBeaconNetworkHdr;

/* BEACONLOG_SIGHTING */
typedef struct
{
	TBeaconNetworkHdr hdr;
	uint32_t sequence;
	uint32_t timestamp;
	TBeaconNgTracker log;
} PACKED TBeaconLogSighting;

#endif/*__OPENBEACON_PROTO_H__*/
