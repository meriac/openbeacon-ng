/***************************************************************
 *
 * OpenBeacon.org - JSON MQTT publish client
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009-2011 Milosch Meriac <meriac@bitmanufaktur.de>
 * Modified by Ciro Cattuto <ciro.cattuto@isi.it>
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
#include <mosquitto.h>

#include "crypto.h"

#define UDP_PORT 2342
#define MOSQUITTO_TCP_PORT 1883
#define BUF_SIZE 2048
#define ACC_SCALE_G (2.0 / (1 << 15))

static uint32_t g_total_crc_ok, g_total_crc_errors;
static uint32_t g_ignored_protocol, g_invalid_protocol, g_unknown_reader;
static uint32_t g_doubled_reader;
static uint8_t g_decrypted_one;
static struct mosquitto *mosq;
static char *buf = NULL;


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
send_error(double timestamp, struct sockaddr_in *reader_addr, uint16_t reader_id, char *error_msg, uint32_t error_code)
{
	int i = 0;

	i += sprintf(buf+i, "{");

	i += sprintf(buf+i, "\"reader\": {\"ip\":\"%s\",\"id\":%d,\"t\":%d},",
		inet_ntoa(reader_addr->sin_addr),
		reader_id,
		(uint32_t) timestamp);

	i += sprintf(buf+i, "\"error\": \"%s (%d)\"", error_msg, error_code);

	i += sprintf(buf+i, "}");

	mosquitto_publish(mosq, NULL, "openbeacon/invalid", i, buf, 0, false);
}


void
send_packet(double timestamp, struct sockaddr_in *reader_addr, uint16_t reader_id, const uint8_t *signature, const TBeaconNgTracker *track)
{
	int i = 0, j;
	uint32_t t;
	const TBeaconNgSighting *slot;

	i += sprintf(buf+i, "{");

	i += sprintf(buf+i, "\"reader\": {\"ip\":\"%s\",\"id\":%d,\"t\":%d},",
		inet_ntoa(reader_addr->sin_addr),
		reader_id,
		(uint32_t) timestamp);

	i += sprintf(buf+i, "\"packet\": {\"id\":\"%08X\",\"t\":%d,",
		track->uid,
		track->epoch
	);

    i += sprintf(buf+i, "\"crc\":\"");
    for (j=0; j<CONFIG_SIGNATURE_SIZE; j++)
        i += sprintf(buf+i, "%02X", signature[j]);
    i += sprintf(buf+i, "\",");

	/* show specific fields */
	switch(track->proto)
	{
		case RFBPROTO_BEACON_NG_SIGHTING:
		{
			i += sprintf(buf+i, "\"type\":\"tracker\",\"payload\":[");
			slot = track->p.sighting;
			for(t=0; t<CONFIG_SIGHTING_SLOTS; t++)
			{
				if(slot->uid)
				{
					i += sprintf(buf+i, "%s{\"id\":\"%08X\",\"tx_dbm\":%d,\"rx_dbm\":%d}",
						t ? ",":"",
						slot->uid,
						slot->tx_power,
						slot->rx_power
					);
				}
				slot++;
			}
			i += sprintf(buf+i, "]");
			break;
		}

		case RFBPROTO_BEACON_NG_STATUS:
		{
			i += sprintf(buf+i,
				"\"type\":\"status\",\"payload\":{\"rx_loss\":%1.2f,\"tx_loss\":%1.2f,\"ticks\":%i,\"voltage\":%1.1f,\"acc_x\":%1.3f,\"acc_y\":%1.3f,\"acc_z\":%1.3f,\"flash_log_free_blocks\":%d,\"boot_count\":%d,\"flags\":%d}",
				track->p.status.rx_loss / 100.0,
				track->p.status.tx_loss / 100.0,
				track->p.status.ticks,
				track->p.status.voltage / 10.0,
				track->p.status.acc_x * ACC_SCALE_G,
				track->p.status.acc_y * ACC_SCALE_G,
				track->p.status.acc_z * ACC_SCALE_G,
				track->p.status.flash_log_free_blocks,
				track->p.status.boot_count,
				track->p.status.flags
			);
			break;
		}
	}

	i += sprintf(buf+i, "}}");

	mosquitto_publish(mosq, NULL, "openbeacon/valid", i, buf, 0, false);
}


static int
parse_packet (double timestamp, struct sockaddr_in *reader_addr, const void *data, int len)
{
	const TBeaconLogSighting *pkt;
	TBeaconNgTracker track;
	uint32_t t;

	if(len<(int)sizeof(TBeaconLogSighting))
		return len;

	pkt = (const TBeaconLogSighting*)data;
	if(pkt->hdr.protocol != BEACONLOG_SIGHTING)
	{
		send_error(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), "Invalid protocol", pkt->hdr.protocol);
		//fprintf(stderr, " Invalid protocol [0x%02X]\n\r", pkt->hdr.protocol);
		return len;
	}

	t = ntohs(pkt->hdr.size);
	if(ntohs(t != sizeof(TBeaconLogSighting)))
	{
		send_error(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), "Invalid packet size", t);
		//fprintf(stderr, " Invalid packet size (%u)\n\r", t);
		return len;
	}
	
	if(ntohs(pkt->hdr.icrc16) != icrc16(&pkt->hdr.protocol, (sizeof(TBeaconLogSighting)-sizeof(pkt->hdr.icrc16))))
	{
		send_error(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), "Invalid packet CRC", 0);
		//fprintf(stderr, " Invalid packet CRC\n\r");
		return len;
	}

	/* decrypt valid packet */
	if((t = aes_decr(&pkt->log, &track, sizeof(track), CONFIG_SIGNATURE_SIZE))!=0)
	{
		send_error(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), "Packet decryption error", t);
		//fprintf(stderr, " Failed decrypting packet with error [%i]\n\r", t);
		return len;
	}

	/* ignore unknown packets */
	if(!((track.proto == RFBPROTO_BEACON_NG_SIGHTING)||
		(track.proto == RFBPROTO_BEACON_NG_STATUS)))
	{
		send_error(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), "Unkown packet protocol", track.proto);
		//fprintf(stderr, " Unknown protocol [%i]\n\r", track.proto);
		return len;
	}

	/* send packet */
	send_packet(timestamp, reader_addr, ntohs(pkt->hdr.reader_id), ((uint8_t *) &pkt->log) + sizeof(track) - CONFIG_SIGNATURE_SIZE, &track);

	return sizeof(TBeaconLogSighting);
}


static int
listen_packets (void)
{
	int sock, size, res;
	//uint32_t reader_addr;
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

		if (bind (sock, (struct sockaddr *) & si_me, sizeof (si_me)) == -1)
			diep ("bind");

		while (1)
		{
			if ((size = recvfrom (sock, &buffer, sizeof (buffer), 0,
								  (struct sockaddr *) & si_other, &slen)) == -1)
				diep ("recvfrom()");

			/* orderly shutdown */
			if (!size)
				break;

			pkt = buffer;
			//reader_addr = ntohl (si_other.sin_addr.s_addr);

			timestamp = microtime ();
			while ((res =
					parse_packet (timestamp, &si_other, pkt, size)) > 0)
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
	int ret;

	/* initialize statistics */
	g_unknown_reader = 0;
	g_decrypted_one = 0;
	g_total_crc_ok = g_total_crc_errors = 0;
	g_doubled_reader = 0;
	g_ignored_protocol = g_invalid_protocol = 0;

	/* initialize encryption */
	aes_init();

	/* initialize send buffer */
	buf = (char *) calloc(BUF_SIZE, sizeof(char));
	if (buf == NULL)
		return -1;

	/* initialize Mosquitto library */
	mosquitto_lib_init();
	mosq = mosquitto_new("openbeacon-mqtt", true, NULL);
	mosquitto_connect(mosq, (argc > 1) ? argv[1] : "127.0.0.1", MOSQUITTO_TCP_PORT, 600);
	mosquitto_loop_start(mosq);

	/* main loop */
	ret = listen_packets();

	mosquitto_disconnect(mosq);
	mosquitto_loop_stop(mosq, false);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return ret;
}

