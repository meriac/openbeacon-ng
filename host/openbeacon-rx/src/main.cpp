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
#include <pthread.h>
#include <math.h>

#include "crypto.h"
#include "bmMapHandleToItem.h"

#define TAGAGGREGATION_TIME 30
#define PROXAGGREGATION_TIME 10
#define MAX_PROXIMITY_SLOTS 16

#define ACC_SCALE_G (2.0 / (1 << 15))

typedef struct
{
	uint32_t tag_id, epoch;
	bool button, calibrated;
	double last_seen;
	uint32_t last_reader_id;
	double rx_loss, tx_loss, px_power;
} TTagItem;

typedef struct
{
	uint32_t last_seen;
	int tx_power, rx_power;
	uint32_t distance;
} TTagProximitySlot;

typedef struct
{
	uint32_t tag1, tag2;
	TTagItem *tag1p, *tag2p;
	uint32_t last_seen;
	int last_power;
	bool calibrated;
	double tag1_calrx,tag2_calrx;
	uint32_t fifo_pos;
	TTagProximitySlot fifo[MAX_PROXIMITY_SLOTS];
} TTagProximity;

static FILE* g_out;
static bool g_first;
static bool g_DoEstimation = true;
static bmMapHandleToItem g_map_reader, g_map_tag, g_map_proximity;

static uint32_t g_total_crc_ok, g_total_crc_errors;
static uint32_t g_ignored_protocol, g_invalid_protocol, g_unknown_reader;
static uint32_t g_doubled_reader;
static uint8_t g_decrypted_one;

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
process_packet(double timestamp, uint32_t reader_id, const TBeaconNgTracker &track)
{
	int i;
	double cal;
	uint32_t tag1, tag2;
	TTagItem *tag, *tag1p, *tag2p;
	TTagProximity *prox;
	TTagProximitySlot *prox_slot;
	const TBeaconNgSighting *slot;
	pthread_mutex_t *tag_mutex, *prox_mutex;

	/* find tag */
	if((tag = (TTagItem*)g_map_tag.Add(track.uid, &tag_mutex))==NULL)
		diep("can't add tag item");

	/* tag seen first time */
	if(!tag->tag_id)
	{
		tag->tag_id = track.uid;
		tag->calibrated = false;
		tag->epoch = 0;
	}

	/* ignore doubled/replayed packets */
	if(track.epoch <= tag->epoch)
	{
		pthread_mutex_unlock (tag_mutex);
		return;
	}

	/* remember reader time */
	tag->epoch = track.epoch;
	tag->last_seen = timestamp;
	tag->last_reader_id = reader_id;
	
	switch(track.proto)
	{
		case RFBPROTO_BEACON_NG_SIGHTING:
		{
			slot = track.p.sighting;
			for(i=0; i<CONFIG_SIGHTING_SLOTS; i++)
			{
				/* ignore empty slots */
				if(!slot->uid)
					continue;

				/* sort UIDs */
				if(track.uid<slot->uid)
				{
					tag1p = tag;
					tag2p = NULL;
					tag1 = track.uid;
					tag2 = slot->uid;
				}
				else
				{
					tag1p = NULL;
					tag2p = tag;
					tag1 = slot->uid;
					tag2 = track.uid;
				}

				/* look up connection */
				if ((prox = (TTagProximity *)
					g_map_proximity.Add ((((uint64_t) tag1) << 32) | tag2,
					&prox_mutex)) == NULL)
					diep ("can't add tag proximity sighting");
				/* first occurence */
				if(!prox->tag1)
				{
					prox->tag1 = tag1;
					prox->tag1p = tag1p;
					prox->tag2 = tag2;
					prox->tag2p = tag2p;
				}

				/* erase old sigthings */
				if((timestamp-prox->last_seen)>=PROXAGGREGATION_TIME)
				{
					prox->fifo_pos = 0;
					bzero(&prox->fifo, sizeof(prox->fifo));
				}
				/* remember time */
				prox->last_seen = timestamp;
				prox->last_power = slot->rx_power;

				/* remember sighting */
				prox_slot = &prox->fifo[prox->fifo_pos++];
				if(prox->fifo_pos>=MAX_PROXIMITY_SLOTS)
					prox->fifo_pos = 0;
				prox_slot->last_seen =  timestamp;
				prox_slot->rx_power = slot->rx_power;
				prox_slot->tx_power = slot->tx_power;

				/* pre-calculate calibration values */
				if(prox->calibrated)
				{
					cal = (tag->tag_id == tag1) ? prox->tag1_calrx : prox->tag2_calrx;
					/* calculate distance in mm */
					prox_slot->distance =
						(__exp10((cal - prox_slot->rx_power)/20.0)/
						(41.88*(2400+CONFIG_PROX_CHANNEL)))*1000000;
				}
				else
				{
					prox_slot->distance = 0;

					/* populate second tag pointer */
					if(!prox->tag1p)
						prox->tag1p = (TTagItem*)g_map_tag.Find(prox->tag1, NULL);
					if(!prox->tag2p)
						prox->tag2p = (TTagItem*)g_map_tag.Find(prox->tag2, NULL);

					/* wait till both sides are calibrated */
					if(	prox->tag1p && prox->tag2p &&
						prox->tag1p->calibrated &&
						prox->tag2p->calibrated)
					{
						prox->calibrated = true;
						prox->tag1_calrx =
							prox->tag1p->rx_loss +
	//						prox->tag2p->px_power +
							prox->tag2p->tx_loss;
						prox->tag2_calrx =
							prox->tag2p->rx_loss +
	//						prox->tag1p->px_power +
							prox->tag1p->tx_loss;
					}
				}

				/* release proximity object */
				pthread_mutex_unlock (prox_mutex);
				/* next slot */
				slot++;
			}
			break;
		}

		case RFBPROTO_BEACON_NG_STATUS:
		{
			tag->calibrated = true;
			tag->rx_loss  = track.p.status.rx_loss/100.0;
			tag->tx_loss  = track.p.status.tx_loss/100.0;
			break;
		}
	}

	/* release tag object */
	pthread_mutex_unlock (tag_mutex);
}

void
print_packet(FILE *out, uint32_t reader_id, const TBeaconNgTracker &track)
{
	uint32_t t;
	const TBeaconNgSighting *slot;

	/* show common fields */
	fprintf(out, "{\"id\"=\"0x%08X\",\"t\"=%i,",
		track.uid,
		track.epoch
	);

	/* show specific fields */
	switch(track.proto)
	{
		case RFBPROTO_BEACON_NG_SIGHTING:
		{
			fprintf(out, "\"sighting\"=[");
			slot = track.p.sighting;
			for(t=0; t<CONFIG_SIGHTING_SLOTS; t++)
			{
				if(slot->uid)
				{
					fprintf(out, "%s{\"id\"=\"0x%08X\",\"TX dBm\"=%03i,\"RX dBm\"=%03i}",
						t ? ",":"",
						slot->uid,
						slot->tx_power,
						slot->rx_power
					);
				}
				slot++;
			}
			fprintf(out, "]");
			break;
		}

		case RFBPROTO_BEACON_NG_STATUS:
		{
			fprintf(out,
				"\"status\"={\"rx_loss\"=%1.2f,\"tx_loss\"=%1.2f,\"ticks\"=%06i,\"voltage\"=%1.1f,\"acc_x\"=%1.3f,\"acc_y\"=%1.3f,\"acc_z\"=%1.3f,\"flash_log_free_blocks\"=%d,\"boot_count\"=%0d,\"flags\"=%04X}",
				track.p.status.rx_loss / 100.0,
				track.p.status.tx_loss / 100.0,
				track.p.status.ticks,
				track.p.status.voltage / 10.0,
				track.p.status.acc_x * ACC_SCALE_G,
				track.p.status.acc_y * ACC_SCALE_G,
				track.p.status.acc_z * ACC_SCALE_G,
				track.p.status.flash_log_free_blocks,
				track.p.status.boot_count,
				track.p.status.flags
			);
			break;
		}
	}

	fprintf(out, "}\n\r");
}

static int
parse_packet (double timestamp, uint32_t reader_id, const void *data, int len)
{
	uint32_t t;
	const TBeaconLogSighting *pkt;
	TBeaconNgTracker track;

	if(len<(int)sizeof(TBeaconLogSighting))
		return len;

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

	/* ignore unknown packets */
	if(!((track.proto == RFBPROTO_BEACON_NG_SIGHTING)||
		(track.proto == RFBPROTO_BEACON_NG_STATUS)))
	{
		fprintf(stderr, " Unknown protocol [%i]\n\r", track.proto);
		return len;
	}

	/* show & process latest packet */
	print_packet(stdout, reader_id, track);
	process_packet(timestamp, reader_id, track);
	return sizeof(TBeaconLogSighting);
}

static inline void
thread_iterate_tag (void *Context, double timestamp, bool realtime)
{
	int delta;
	TTagItem *tag = (TTagItem*)Context;

	/* ignore empty slots */
	if(!tag->last_seen)
		return;

	/* calculate delta time since last sighting - expired ? */
	delta = timestamp - tag->last_seen;
	if (delta >= TAGAGGREGATION_TIME)
		return;

	if(g_first)
		fprintf(g_out,"  \"tag\":[");
	fprintf(g_out,"%s\n    {\"id\":%u,\"hex\":\"0x%08X\",\"age\":%i}",
		g_first ? "":",",
		tag->tag_id,
		tag->tag_id,
		delta
	);

	g_first = false;
}

static inline void
thread_iterate_prox (void *Context, double timestamp, bool realtime)
{
	int i, count, power, delta;
	uint32_t dist, dist_count;
	TTagProximitySlot *slot;
	TTagProximity *prox = (TTagProximity*)Context;

	/* ignore empty slots */
	if(!prox->last_seen)
		return;

	/* calculate delta time since last sighting - expired ? */
	delta = timestamp - prox->last_seen;
	if (delta >= PROXAGGREGATION_TIME)
	{
		prox->last_seen = prox->fifo_pos = 0;
		bzero(&prox->fifo, sizeof(prox->fifo));
		return;
	}

	dist = dist_count = 0;
	count = power = 0;
	slot = prox->fifo;
	for(i=0; i<MAX_PROXIMITY_SLOTS; i++, slot++)
		if(slot->last_seen && ((timestamp - slot->last_seen) <= PROXAGGREGATION_TIME))
		{
			count++;
			power+=slot->rx_power;
			if(slot->distance)
			{
				dist+=slot->distance;
				dist_count++;
			}
		}
	/* ignore empty sets */
	if(!count)
		return;

	if(g_first)
		fprintf(g_out,"\n  ],\n  \"edge\":[");
	fprintf(g_out,"%s\n    {\"tag\":[%u,%u],\"age\":%i,\"power\":%1.1f",
		g_first ? "":",",
		prox->tag1,
		prox->tag2,
		delta,
		((double)power)/count
	);

	if(dist_count)
		fprintf(g_out,",\"dist\":%1.1f", (dist/dist_count)/1000.0);

	fprintf(g_out,"}");
	g_first = false;
}

static void
thread_estimation_step (FILE *out, double timestamp, bool realtime)
{
	static uint32_t sequence = 0;

	if (realtime)
		usleep (200 * 1000);

	/* tracking dump state in JSON format */
	fprintf (out, "{\n  \"id\":%u,\n"
			"  \"api\":{\"name\":\"" PROGRAM_NAME "\",\"ver\":\""
			PROGRAM_VERSION "\"},\n" "  \"time\":%u,\n",
			sequence++, (uint32_t) timestamp);

	g_out = out;

	/* display all tags */
	g_first = true;
	g_map_tag.IterateLocked (&thread_iterate_tag, timestamp, realtime);

	/* display all edges */
	g_first = true;
	g_map_proximity.IterateLocked (&thread_iterate_prox, timestamp, realtime);

	fprintf (out, "\n  ]\n},");

	/* propagate object on stdout */
	fflush (out);
}

static void *
thread_estimation (void *context)
{
	while (g_DoEstimation)
		thread_estimation_step ((FILE*)context, microtime (), true);
	return NULL;
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
	pthread_t thread_handle;

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

		pthread_create (&thread_handle, NULL, &thread_estimation, out);

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
	g_map_proximity.SetItemSize (sizeof (TTagProximity));

	/* initialize encryption */
	aes_init();

	return listen_packets (stdout);
}
