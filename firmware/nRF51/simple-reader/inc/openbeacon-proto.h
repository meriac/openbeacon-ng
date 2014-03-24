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

#ifndef CONFIG_NAVIGATION_CHANNEL
#define CONFIG_NAVIGATION_CHANNEL CONFIG_TRACKER_CHANNEL
#endif /*CONFIG_NAVIGATION_CHANNEL */

#define BEACONLOG_SIGHTING 0x01

#define XXTEA_BLOCK_COUNT 4
#define MAX_POWER_LEVELS 4

#define RFBPROTO_BEACONTRACKER_OLD      16
#define RFBPROTO_READER_ANNOUNCE        22
#define RFBPROTO_BEACONTRACKER_OLD2     23
#define RFBPROTO_BEACONTRACKER          24
#define RFBPROTO_BEACONTRACKER_STRANGE  25
#define RFBPROTO_BEACONTRACKER_EXT      26
#define RFBPROTO_FORWARD                27
#define RFBPROTO_FORWARD_MOVING         28
#define RFBPROTO_PROXTRACKER            42
#define RFBPROTO_PROXREPORT             69
#define RFBPROTO_PROXREPORT_EXT         70

#define FWDTAG_SLOTS 4
#define FWDTAG_COUNT_BITS 5
#define FWDTAG_COUNT_MASK ((1<<FWDTAG_COUNT_BITS)-1)
#define FWDTAG_STRENGTH_BITS 3
#define FWDTAG_STRENGTH_MASK ((1<<FWDTAG_STRENGTH_BITS)-1)
#define FWDTAG_STRENGTH_COUNT (FWDTAG_STRENGTH_MASK+1)
#define FWDTAG_SLOT(power,count) (((power&FWDTAG_STRENGTH_MASK)<<FWDTAG_COUNT_BITS)|(count&FWDTAG_COUNT_MASK))
#define FWDTAG_SLOT_POWER (slot) ((slot>>FWDTAG_COUNT_BITS)&FWDTAG_STRENGTH_MASK)
#define FWDTAG_SLOT_COUNT (slot) (count&FWDTAG_COUNT_MASK)

#define PROX_MAX 4
#define PROX_TAG_ID_BITS 12
#define PROX_TAG_COUNT_BITS 2
#define PROX_TAG_STRENGTH_BITS 2
#define PROX_TAG_ID_MASK ((1<<PROX_TAG_ID_BITS)-1)
#define PROX_TAG_COUNT_MASK ((1<<PROX_TAG_COUNT_BITS)-1)
#define PROX_TAG_STRENGTH_MASK ((1<<PROX_TAG_STRENGTH_BITS)-1)

#define PROX_MAX 4

#define RFBFLAGS_ACK			0x01
#define RFBFLAGS_SENSOR			0x02
#define RFBFLAGS_INFECTED		0x04
#define RFBFLAGS_MOVING			0x08
#define RFBFLAGS_SENSOR2		0x10
#define RFBFLAGS_OVERFLOW		0x40
#define RFBFLAGS_FIXED_TAG		0x80

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

#define LOGFLAG_PROXIMITY               0x10
#define LOGFLAG_BUTTON                  0x20

typedef struct
{
	uint8_t proto, proto2;
	uint8_t flags, strength;
	uint32_t seq;
	uint32_t oid;
	uint16_t reserved;
	uint16_t crc;
} PACKED TBeaconTrackerOld;

typedef struct
{
	uint8_t strength;
	uint16_t oid_last_seen;
	uint16_t powerup_count;
	uint8_t reserved;
	uint32_t seq;
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
	uint16_t oid_prox[PROX_MAX];
	uint16_t seq;
} PACKED TBeaconProx;

typedef struct
{
	uint8_t opcode, res;
	uint32_t data[2];
} PACKED TBeaconReaderCommand;

typedef struct
{
	uint8_t opcode, strength;
	uint32_t uptime, ip;
} PACKED TBeaconReaderAnnounce;

typedef struct
{
	uint16_t oid;
	uint8_t slot[FWDTAG_SLOTS];
	uint32_t seq;
} PACKED TBeaconForward;

typedef union
{
	TBeaconProx prox;
	TBeaconTracker tracker;
	TBeaconTrackerExt trackerExt;
	TBeaconReaderCommand reader_command;
	TBeaconReaderAnnounce reader_announce;
	TBeaconForward forward;
} PACKED TBeaconPayload;

typedef struct
{
	uint8_t proto;
	uint16_t oid;
	uint8_t flags;

	TBeaconPayload p;

	uint16_t crc;
} PACKED TBeaconWrapper;

typedef union
{
	uint8_t proto;
	TBeaconWrapper pkt;
	TBeaconTrackerOld old;
	uint32_t block[XXTEA_BLOCK_COUNT];
	uint8_t byte[XXTEA_BLOCK_COUNT * 4];
} PACKED TBeaconEnvelope;

typedef struct
{
	uint32_t timestamp;
	uint32_t ip;
	TBeaconEnvelope env;
} PACKED TBeaconEnvelopeLog;

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
	TBeaconEnvelope log;
} PACKED TBeaconLogSighting;

#endif/*__OPENBEACON_PROTO_H__*/
