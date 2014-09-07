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
#include <math.h>

#include "bmMapHandleToItem.h"
#include "crypto.h"

#define MAX_POWER_COUNT 32

typedef struct
{
	uint32_t id, sequence;
	uint32_t tag_id;
	uint32_t reader_id;
	double power[MAX_POWER_COUNT];
	int power_pos;
} TTagItem;

typedef struct
{
	TBeaconNetworkHdr hdr;
	uint32_t sequence;
	uint32_t timestamp;
	TBeaconNgMarker log;
} PACKED TBeaconLogMarker;

static bmMapHandleToItem g_map_tag;

#define UDP_PORT 2342
#define STRENGTH_LEVELS_COUNT 4

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

void
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

static void
process_packet(double timestamp, uint32_t reader_id, const TBeaconNgMarker &marker)
{
	int j;
	uint32_t tag_id, tag_count;
	TTagItem* item;
	double time, *pt, delta;

	tag_id = ntohl(marker.uid);

	/* only count tag strengths on lowest level zero */
	if ((item =
			(TTagItem *)
			g_map_tag.Add ((((uint64_t) reader_id) << 32) |
							  tag_id, NULL)) != NULL)
	{
		/* on first occurence */
		if(!item->tag_id)
		{
			item->tag_id = tag_id;
			item->reader_id = reader_id;
			item->sequence = ntohl(marker.counter);
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
		fprintf(stdout,"{\"tag\":%u, \"reader\":%u, \"power\":%u}\n", tag_id, reader_id, tag_count);
		fflush(stdout);
	}
}

static int
parse_packet (double timestamp, uint32_t reader_id, const void *data, int len)
{
	uint32_t t;
	const TBeaconLogMarker *pkt;
	TBeaconNgMarker marker;

	if(len<(int)sizeof(TBeaconLogSighting))
		return len;

	pkt = (const TBeaconLogMarker*)data;
	if(pkt->hdr.protocol != BEACONLOG_SIGHTING)
	{
		fprintf(stderr, " Invalid protocol [0x%02X]\n\r", pkt->hdr.protocol);
		return len;
	}

	t = ntohs(pkt->hdr.size);
	if(ntohs(t != sizeof(*pkt)))
	{
		fprintf(stderr, " Invalid packet size (%u)\n\r", t);
		return len;
	}

	if(ntohs(pkt->hdr.icrc16) != icrc16(&pkt->hdr.protocol, (sizeof(*pkt)-sizeof(pkt->hdr.icrc16))))
	{
		fprintf(stderr, " Invalid packet CRC\n\r");
		return len;
	}

	/* decrypt valid packet */
	if((t = aes_decr(&pkt->log, &marker, sizeof(marker), CONFIG_SIGNATURE_SIZE))!=0)
	{
		fprintf(stderr, " Failed decrypting packet with error [%i]\n\r", t);
		return len;
	}

	/* ignore unknown packets */
	if(marker.proto != RFBPROTO_BEACON_NG_MARKER)
	{
		fprintf(stderr, " Unknown protocol [%i]\n\r", marker.proto);
		return len;
	}

	/* show & process latest packet */
	process_packet(timestamp, ntohs(pkt->hdr.reader_id), marker);
	return sizeof(TBeaconLogSighting);
}

static int
listen_packets (FILE* out)
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
	g_map_tag.SetItemSize (sizeof (TTagItem));

	/* initialize encryption */
	aes_init();

	return listen_packets (stdout);
}
