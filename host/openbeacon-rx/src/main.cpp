/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009-2011 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bmMapHandleToItem.h"
#include "bmReaderPositions.h"
#include "openbeacon.h"

static bmMapHandleToItem g_map_reader, g_map_tag, g_map_proximity;

#define XXTEA_KEY_NONE 0

#ifdef  CUSTOM_ENCRYPTION_KEYS
#include "custom-encryption-keys.h"
#else /*CUSTOM_ENCRYPTION_KEYS */

#define XXTEA_KEY_25C3_BETA 1
#define XXTEA_KEY_25C3_FINAL 2
#define XXTEA_KEY_24C3 3
#define XXTEA_KEY_23C3 4
#define XXTEA_KEY_CAMP07 5
#define XXTEA_KEY_LASTHOPE 6

/* proximity tag TEA encryption keys */
static const long tea_keys[][XXTEA_BLOCK_COUNT] = {
	{0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff}							/* 0: default key */
};
#endif /*CUSTOM_ENCRYPTION_KEYS */

#define TEA_KEY_COUNT (sizeof(tea_keys)/sizeof(tea_keys[0]))

static uint32_t g_tea_key_usage[TEA_KEY_COUNT + 1];
static uint32_t g_total_crc_ok, g_total_crc_errors;
static uint32_t g_ignored_protocol, g_invalid_protocol, g_unknown_reader;
static uint32_t g_doubled_reader;
static uint8_t g_decrypted_one;

#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key[(p&3)^e]^z)) )
#define DELTA 0x9e3779b9UL

#define UDP_PORT 2342
#define STRENGTH_LEVELS_COUNT 4
#define TAGSIGHTINGFLAG_SHORT_SEQUENCE 0x01
#define TAGSIGHTINGFLAG_BUTTON_PRESS 0x02
#define TAGSIGHTING_BUTTON_TIME_SECONDS 5
#define TAG_MASS 1.0

#define PROXAGGREGATION_TIME_SLOT_SECONDS 2
#define PROXAGGREGATION_TIME_SLOTS 10
#define PROXAGGREGATION_TIME (PROXAGGREGATION_TIME_SLOT_SECONDS*PROXAGGREGATION_TIME_SLOTS)

#define MIN_AGGREGATION_SECONDS 5
#define MAX_AGGREGATION_SECONDS 16
#define RESET_TAG_POSITION_SECONDS (60*5)
#define READER_TIMEOUT_SECONDS (60*15)
#define PACKET_STATISTICS_WINDOW 10
#define PACKET_STATISTICS_READER 15
#define AGGREGATION_TIMEOUT(strength) ((uint32_t)(MIN_AGGREGATION_SECONDS+(((MAX_AGGREGATION_SECONDS-MIN_AGGREGATION_SECONDS)/(STRENGTH_LEVELS_COUNT-1))*(strength))))

#define MAX_POWER_COUNT 32

typedef struct
{
	uint32_t id, sequence;
	uint32_t tag_id;
	uint8_t flags;
	int key_id;
	uint32_t reader_id;
	double power[MAX_POWER_COUNT];
	int power_pos;
} TTagItem;

static void
diep (const char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);
	putc ('\n', stderr);
	exit (EXIT_FAILURE);
}

static u_int16_t
crc16 (const unsigned char *buffer, int size)
{
	u_int16_t crc = 0xFFFF;

	if (buffer && size)
		while (size--)
		{
			crc = (crc >> 8) | (crc << 8);
			crc ^= *buffer++;
			crc ^= ((unsigned char) crc) >> 4;
			crc ^= crc << 12;
			crc ^= (crc & 0xFF) << 5;
		}

	return crc;
}

static inline u_int16_t
icrc16 (const unsigned char *buffer, int size)
{
	return crc16 (buffer, size) ^ 0xFFFF;
}

static inline void
xxtea_decode (u_int32_t * v, u_int32_t length, const long *tea_key)
{
	u_int32_t z, y = v[0], sum = 0, e, p, q;

	if (!v || !tea_key)
		return;

	q = 6 + 52 / length;
	sum = q * DELTA;
	while (sum)
	{
		e = (sum >> 2) & 3;
		for (p = length - 1; p > 0; p--)
			z = v[p - 1], y = v[p] -= MX;

		z = v[length - 1];
		y = v[0] -= MX;
		sum -= DELTA;
	}
}

static inline void
shuffle_tx_byteorder (u_int32_t * v, u_int32_t len)
{
	while (len--)
	{
		*v = htonl (*v);
		v++;
	}
}

static inline double
microtime_calc (struct timeval *tv)
{
	return tv->tv_sec + (tv->tv_usec / 1000000.0);
}

static inline double
microtime (void)
{
	struct timeval tv;

	if (!gettimeofday (&tv, NULL))
		return microtime_calc (&tv);
	else
		return 0;
}

static void
hex_dump (const void *data, unsigned int addr, unsigned int len)
{
	unsigned int start, i, j;
	const unsigned char *buf;
	char c;

	buf = (const unsigned char *) data;
	start = addr & ~0xf;

	for (j = 0; j < len; j += 16)
	{
		fprintf (stderr, "%08x:", start + j);

		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
				fprintf (stderr, " %02x", buf[start + i + j]);
			else
				fprintf (stderr, "   ");
		}
		fprintf (stderr, "  |");
		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
			{
				c = buf[start + i + j];
				if (c >= ' ' && c < 127)
					fprintf (stderr, "%c", c);
				else
					fprintf (stderr, ".");
			}
			else
				fprintf (stderr, " ");
		}
		fprintf (stderr, "|\n\r");
	}
}

static int
parse_packet (double timestamp, uint32_t reader_id, const void *data, int len)
{
	int res, j, key_id;
	int tag_strength, tag_count;
	uint32_t tag_id, tag_flags, tag_sequence;
	TBeaconEnvelope env;
	TBeaconLogSighting *log;
	TTagItem *item;
	double delta, time, *pt;

	key_id = -1;
	res = 0;

	/* check for new log file format */
	if ((len >= (int) sizeof (TBeaconLogSighting))
		&& ((len % sizeof (TBeaconLogSighting)) == 0))
	{
		log = (TBeaconLogSighting *) data;
		if ((log->hdr.protocol == BEACONLOG_SIGHTING) &&
			(ntohs (log->hdr.size) == sizeof (TBeaconLogSighting)) &&
			(icrc16
			 ((uint8_t *) & log->hdr.protocol,
			  sizeof (*log) - sizeof (log->hdr.icrc16)) ==
			 ntohs (log->hdr.icrc16)))
		{
			reader_id = ntohs (log->hdr.reader_id);
			data = &log->log;
			len = sizeof (log->log);
			res = sizeof (TBeaconLogSighting);
		}
	}

	/* else if old log file format */
	if (!res)
	{
		if (len < (int) sizeof (env))
			return 0;
		else
			len = res = sizeof (env);
	}

	for (j = 0; j < (int) TEA_KEY_COUNT; j++)
	{
		memcpy (&env, data, len);

		/* decode packet */
		shuffle_tx_byteorder (env.block, XXTEA_BLOCK_COUNT);
		xxtea_decode (env.block, XXTEA_BLOCK_COUNT, tea_keys[j]);
		shuffle_tx_byteorder (env.block, XXTEA_BLOCK_COUNT);

		/* verify CRC */
		if (crc16 (env.byte,
				   sizeof (env.pkt) - sizeof (env.pkt.crc)) ==
			ntohs (env.pkt.crc))
		{
			key_id = j + 1;
			break;
		}
	}

	/* ignore broken packets */
	if (key_id < 0)
		return 0;

	switch (env.pkt.proto)
	{
		case RFBPROTO_BEACONTRACKER_EXT:
			tag_id = ntohs (env.pkt.oid);
			tag_sequence = ntohl (env.pkt.p.trackerExt.seq);
			tag_strength = env.pkt.p.trackerExt.strength;
			if (tag_strength >= STRENGTH_LEVELS_COUNT)
				tag_strength = STRENGTH_LEVELS_COUNT - 1;
			tag_flags = (env.pkt.flags & RFBFLAGS_SENSOR) ?
				TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
			break;

		case RFBPROTO_BEACONTRACKER:
			{
				tag_id = ntohs (env.pkt.oid);
				tag_sequence = ntohl (env.pkt.p.tracker.seq);
				tag_strength = env.pkt.p.tracker.strength;
				if (tag_strength >= STRENGTH_LEVELS_COUNT)
					tag_strength = STRENGTH_LEVELS_COUNT - 1;
				tag_flags = (env.pkt.flags & RFBFLAGS_SENSOR) ?
					TAGSIGHTINGFLAG_BUTTON_PRESS : 0;
			}
			break;

		default:
			tag_strength = -1;
			tag_sequence = 0;
			tag_id = 0;
			tag_flags = 0;

			fprintf(stderr, "\nInvalid protocol 0x%02X (%u):\n",
				env.pkt.proto,
				env.pkt.proto);
			hex_dump(&env.pkt,0,sizeof(env.pkt));
	}

	/* only count tag strengths on lowest level zero */
	if ((tag_strength == 0)
		&& (item =
			(TTagItem *)
			g_map_tag.Add ((((uint64_t) reader_id) << 32) |
							  tag_id, NULL)) != NULL)
	{
		/* on first occurence */
		if(!item->tag_id)
		{
			item->tag_id = tag_id;
			item->reader_id = reader_id;
			item->sequence = tag_sequence;
			item->flags = tag_flags;
		}

		time = microtime();

		/* store current timestamp in fifo buffer */
		item->power[item->power_pos++] = time;
		if(item->power_pos>=MAX_POWER_COUNT)
			item->power_pos = 0;

		/* count all tags within a 0.5 second window */
		tag_count = 0;
		pt = item->power;
		for( j = 0; j<MAX_POWER_COUNT; j++)
		{
			delta = time - *pt++;
			if( delta<0.5 )
				tag_count++;
		}
		printf("{\"tag\":%04u, \"reader\":%04u, \"power\":%02u},\n", tag_id, reader_id, tag_count);
	}
	return 0;
}

static int
listen_packets (void)
{
	int sock, size, res;
	uint32_t reader_id;
	double timestamp;
	uint8_t buffer[1500], *pkt;
	struct sockaddr_in si_me, si_other;
	socklen_t slen = sizeof (si_other);

	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep ("socket");
	else
	{
		memset ((char *) &si_me, 0, sizeof (si_me));
		si_me.sin_family = AF_INET;
		si_me.sin_port = htons (UDP_PORT);
		si_me.sin_addr.s_addr = htonl (INADDR_ANY);

		if (bind (sock, (sockaddr *) & si_me, sizeof (si_me)) == -1)
			diep ("bind");

		while (1)
		{
			if ((size = recvfrom (sock, &buffer, sizeof (buffer), 0,
								  (sockaddr *) & si_other, &slen)) == -1)
				diep ("recvfrom()");

			/* orderly shutdown */
			if (!size)
				break;

			pkt = buffer;
			reader_id = ntohl (si_other.sin_addr.s_addr);

			timestamp = microtime ();
			while ((res =
					parse_packet (timestamp, reader_id, pkt, size)) > 0)
			{
				size -= res;
				pkt += res;
			}
		}
	}

	return 0;
}

int
main (int argc, char **argv)
{
	/* initialize statistics */
	g_unknown_reader = 0;
	g_decrypted_one = 0;
	g_total_crc_ok = g_total_crc_errors = 0;
	g_doubled_reader = 0;
	g_ignored_protocol = g_invalid_protocol = 0;
	memset (&g_tea_key_usage, 0, sizeof (g_tea_key_usage));

	g_map_tag.SetItemSize (sizeof (TTagItem));

	return listen_packets ();
}
