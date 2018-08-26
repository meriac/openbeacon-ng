/*************************************************************** 
 *
 * OpenBeacon.org - Serial Port Tag Sniffer
 *
 * See the following website for already decoded Sputnik data: 
 * http://people.openpcd.org/meri/openbeacon/sputnik/data/24c3/ 
 *
 * Copyright 2015 Milosch Meriac <meriac@bitmanufaktur.de> 
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define PACKED __attribute__((packed))
#include "openbeacon-proto.h"
#include "crc32.h"
#include "crypto.h"

#define TAG_UART_BAUD_RATE 921600

typedef struct
{
	int8_t rssi;
	TBeaconNgTracker pkt;
} PACKED TBeaconBuffer;

static void port_rx(const TBeaconNgTracker* pkt_encrypted, int rssi)
{
	int proto, i;
	int status;
	TBeaconNgTracker pkt;
	time_t tloc;

	/* decrypt valid packet */
	if((status = aes_decr(pkt_encrypted, &pkt, sizeof(pkt), CONFIG_SIGNATURE_SIZE))!=0)
	{
		fprintf(stderr, "error: failed decrypting packet with error [%i]\n", status);
		return;
	}

	/* white-list supported protocols */
	proto = pkt.proto & RFBPROTO_PROTO_MASK;
	if(!((proto == 30) || (proto == 31)))
	{
		fprintf(stderr, "error: uknnown protocol %i\n", proto);
		return;
	}

	time(&tloc);
	fprintf(stdout,
		"{ \"uid\":\"0x%08X\", \"time_local_s\":%8u, \"time_remote_s\":%8u, \"rssi\":%3i, \"angle\":%3i, \"voltage\":%3.1f, \"tx_power\":%i",
		pkt.uid,
		(int)tloc,
		pkt.epoch,
		rssi,
		pkt.angle,
		pkt.voltage / 10.0,
		pkt.tx_power);

	/* optionally decode button */
	if(pkt.proto & RFBPROTO_PROTO_BUTTON)
		fprintf(stdout,", \"button\": 1");

	switch(proto)
	{
		case 30:
			if(pkt.p.sighting[0].uid)
			{
				fprintf(stdout,", \"sighting\": ");
				for(i=0; i<CONFIG_SIGHTING_SLOTS; i++)
					if(pkt.p.sighting[i].uid)
						fprintf(stdout, "%c{\"uid\":\"0x%08X\",\"rssi\":%i}",
							i ? ',':'[',
							pkt.p.sighting[i].uid,
							pkt.p.sighting[i].rx_power
						);
				fprintf(stdout,"]");
			}
			break;
	}
	fprintf(stdout, "}\n\r");
	fflush(stdout);
}

static int port_open(const char *device)
{
	int handle;
	struct termios options;

	/* open serial port */
	if((handle = open( device, O_RDONLY | O_NOCTTY | O_NDELAY)) == -1)
	{
		fprintf(stderr, "error: failed to open serial port (%s)\n", device);
		exit(10);
	}

	/* apply serial port settings */
	tcgetattr(handle, &options);
	cfmakeraw(&options);
	cfsetspeed(&options, TAG_UART_BAUD_RATE);

	if(tcsetattr(handle, TCSANOW, &options))
	{
		fprintf(stderr, "error: failed to set baud %i rate for '%s'\n", TAG_UART_BAUD_RATE, device);
		exit(11);		
	}

	tcflush(handle, TCIFLUSH);

	return handle;
}

int main( int argc, const char* argv[] )
{
	int prev;
	uint8_t *p, data, buffer[64];
	int fd, res, i;
	size_t len;
	TBeaconBuffer rx;

	/* configure default AES key */
	aes_init();

	if( argc < 2 )
	{
		fprintf (stderr, "usage: %s /dev/tty.usbserial-AK0535TL\n", argv[0]);
		return 1;
	}

	/* initialize descriptor list */
	fd = port_open(argv[1]);
	printf("fd=%i\n", fd);

	/* loop over UART data */
	len = prev = 0;
	p = (uint8_t*)&rx;
	while (1)
	{
		res = read(fd, buffer, sizeof(buffer));
		for(i=0; i<res; i++)
		{
			data = buffer[i];

			if(data==0xFF)
				prev = 1;
			else
			{
				if(!prev)
				{
					if(len<sizeof(rx))
						*p++ = data;
					len++;
				}
				else
				{
					prev = 0;
					if(!data)
					{
						if(len<sizeof(rx))
							*p++ = 0xFF;
						len++;
					}
					else
					{
						if(data!=0x01)
							fprintf(stderr, "error: invalid state 0x%02X\n", data);
						else
						{
							if(len==sizeof(rx))
								port_rx(&rx.pkt, rx.rssi);
							else
								fprintf(stderr, "error: invalid RX lenght (%i)\n", (int)len);
						}
						p = (uint8_t*)&rx;
						len = 0;
					}
				}
			}
		}
	}
}
