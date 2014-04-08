/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol specification and definition
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define CONFIG_TRACKER_CHANNEL 81
#define CONFIG_PROX_CHANNEL 76

#define BEACONLOG_SIGHTING 0x01

#define XXTEA_BLOCK_COUNT 4

#define RFBPROTO_BEACONTRACKER_OLD      16
#define RFBPROTO_READER_ANNOUNCE        22
#define RFBPROTO_BEACONTRACKER_OLD2     23
#define RFBPROTO_BEACONTRACKER          24
#define RFBPROTO_BEACONTRACKER_STRANGE  25
#define RFBPROTO_BEACONTRACKER_EXT      26
#define RFBPROTO_PROXTRACKER            42
#define RFBPROTO_PROXREPORT             69
#define RFBPROTO_PROXREPORT_EXT         70

#define PROX_MAX 4
#define PROX_TAG_ID_BITS 12
#define PROX_TAG_COUNT_BITS 2
#define PROX_TAG_STRENGTH_BITS 2
#define PROX_TAG_ID_MASK ((1<<PROX_TAG_ID_BITS)-1)
#define PROX_TAG_COUNT_MASK ((1<<PROX_TAG_COUNT_BITS)-1)
#define PROX_TAG_STRENGTH_MASK ((1<<PROX_TAG_STRENGTH_BITS)-1)

#define RFBFLAGS_ACK			0x01
#define RFBFLAGS_SENSOR			0x02
#define RFBFLAGS_INFECTED		0x04

/* RFBPROTO_READER_COMMAND related opcodes */
#define READER_CMD_NOP			0x00
#define READER_CMD_RESET		0x01
#define READER_CMD_RESET_CONFIG		0x02
#define READER_CMD_RESET_FACTORY	0x03
#define READER_CMD_RESET_WIFI		0x04
#define READER_CMD_SET_OID		0x05
/* RFBPROTO_READER_COMMAND related results */
#define READ_RES__OK			0x00
#define READ_RES__DENIED		0x01
#define READ_RES__UNKNOWN_CMD		0xFF

#define PACKED  __attribute__((__packed__))

typedef struct
{
	u_int8_t proto, proto2;
	u_int8_t flags, strength;
	u_int32_t seq;
	u_int32_t oid;
	u_int16_t reserved;
	u_int16_t crc;
} PACKED TBeaconTrackerOld;

typedef struct
{
	u_int8_t strength;
	u_int16_t oid_last_seen;
	u_int16_t powerup_count;
	u_int8_t reserved;
	u_int32_t seq;
} PACKED TBeaconTracker;

typedef struct
{
	uint8_t strength;
	uint16_t oid_last_seen;
	uint16_t time;
	uint8_t battery;
	uint32_t seq;
} PACKED TBeaconTrackerExt;

typedef struct
{
	u_int16_t oid_prox[PROX_MAX];
	u_int16_t seq;
} PACKED TBeaconProx;

typedef struct
{
	u_int8_t opcode, res;
	u_int32_t data[2];
} PACKED TBeaconReaderCommand;

typedef struct
{
	u_int8_t opcode, strength;
	u_int32_t uptime, ip;
} PACKED TBeaconReaderAnnounce;

typedef union
{
	TBeaconProx prox;
	TBeaconTracker tracker;
	TBeaconTrackerExt trackerExt;
	TBeaconReaderCommand reader_command;
	TBeaconReaderAnnounce reader_announce;
} PACKED TBeaconPayload;

typedef struct
{
	u_int8_t proto;
	u_int16_t oid;
	u_int8_t flags;

	TBeaconPayload p;

	u_int16_t crc;
} PACKED TBeaconWrapper;

typedef union
{
	u_int8_t proto;
	TBeaconWrapper pkt;
	TBeaconTrackerOld old;
	u_int32_t block[XXTEA_BLOCK_COUNT];
	u_int8_t byte[XXTEA_BLOCK_COUNT * 4];
} PACKED TBeaconEnvelope;

typedef struct
{
	u_int32_t timestamp;
	u_int32_t ip;
	TBeaconEnvelope env;
} PACKED TBeaconEnvelopeLog;

typedef struct
{
	u_int16_t icrc16;
	u_int8_t protocol;
	u_int8_t interface;
	u_int16_t reader_id;
	u_int16_t size;
} PACKED TBeaconNetworkHdr;

/* BEACONLOG_SIGHTING */
typedef struct
{
	TBeaconNetworkHdr hdr;
	u_int32_t sequence;
	u_int32_t timestamp;
	TBeaconEnvelope log;
} PACKED TBeaconLogSighting;

#endif/*__OPENBEACON_H__*/
