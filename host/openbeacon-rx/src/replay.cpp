/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009-2015 Milosch Meriac <milosch@meriac.com>
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
#include <pcap.h>

#include "main.h"
#include "crypto.h"
#include "helper.h"
#include "replay.h"

void
parse_pcap (const char *file, bool realtime)
{
	int len, res;
	pcap_t *h;
	char error[PCAP_ERRBUF_SIZE];
	pcap_pkthdr header;
	const ip *ip_hdr;
	const udphdr *udp_hdr;
	const uint8_t *packet;
	uint32_t reader_id, timestamp_old, timestamp;

	timestamp_old = 0;

	if ((h = pcap_open_offline (file, error)) == NULL)
		diep("Failed to open '%s'");
	/* iterate over all IPv4 UDP packets */
	while ((packet = (const uint8_t *) pcap_next (h, &header)) != NULL)
	{
		/* check for Ethernet protocol */
		if ((((uint32_t) (packet[12]) << 8) | packet[13]) == 0x0800)
		{
			/* skip Ethernet header */
			packet += 14;
			ip_hdr = (const ip *) packet;

			/* if IPv4 UDP protocol */
			if ((ip_hdr->ip_v == 0x4) && (ip_hdr->ip_p == 17))
			{
				len = 4 * ip_hdr->ip_hl;
				packet += len;
				udp_hdr = (const udphdr *) packet;

				/* get UDP packet payload size */
#ifdef __APPLE__
				len = udp_hdr->uh_ulen;
#else
				len = udp_hdr->len;
#endif
				len = ntohs (len) - sizeof (udphdr);
				packet += sizeof (udphdr);

				/* run estimation every second */
				timestamp = microtime_calc (&header.ts);
				if ((timestamp - timestamp_old) >= 1)
				{
					timestamp_old = timestamp;
					thread_estimation_step (stdout, timestamp, realtime);
				}

				/* process all packets in this packet */
				reader_id = ntohl (ip_hdr->ip_src.s_addr);

				/* iterate over all packets */
				while ((res = parse_packet (timestamp, reader_id, packet, len)) > 0)
				{
					len -= res;
					packet += res;
				}
			}
		}
	}
}
