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

#include "crypto.h"
#include "bmMapHandleToItem.h"

static bmMapHandleToItem g_map_reader, g_map_tag, g_map_proximity;

static uint32_t g_total_crc_ok, g_total_crc_errors;
static uint32_t g_ignored_protocol, g_invalid_protocol, g_unknown_reader;
static uint32_t g_doubled_reader;
static uint8_t g_decrypted_one;

#define UDP_PORT 2342
#define STRENGTH_LEVELS_COUNT 4

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
	uint32_t t;
	const TBeaconLogSighting *pkt;
	TBeaconNgTracker track;

	if(len<(int)sizeof(TBeaconLogSighting))
		return len;

	fprintf(stderr, "RX[0x%08X]: ", reader_id);

	pkt = (const TBeaconLogSighting*)data;
	if(pkt->hdr.protocol != BEACONLOG_SIGHTING)
	{
		fprintf(stderr, " Invalid protocol [0x%02X]\n\r", pkt->hdr.protocol);
		return len;
	}

	t = ntohs(pkt->hdr.size);
	if(ntohs(t != sizeof(TBeaconLogSighting)))
	{
		fprintf(stderr, " Invalid packet size (%u)\n\r", t);
		return len;
	}
	
	if(ntohs(pkt->hdr.icrc16) != icrc16(&pkt->hdr.protocol, (sizeof(TBeaconLogSighting)-sizeof(pkt->hdr.icrc16))))
	{
		fprintf(stderr, " Invalid packet CRC\n\r");
		return len;
	}

	/* decrypt valid packet */
	if((t = aes_decr(&pkt->log, &track, sizeof(track), CONFIG_SIGNATURE_SIZE))!=0)
	{
		fprintf(stderr, " Failed decrypting packet with error [%i]\n\r", t);
		return len;
	}

	fprintf(stderr, "\n\r");
	hex_dump(&track, 0, sizeof(track)-CONFIG_SIGNATURE_SIZE);

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

	g_map_tag.SetItemSize (sizeof (TTagItem));

	/* initialize encryption */
	aes_init();

	return listen_packets ();
}
