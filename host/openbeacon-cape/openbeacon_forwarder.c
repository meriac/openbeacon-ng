/*************************************************************** 
 *
 * OpenBeacon.org - OnAir protocol forwarder
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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "crc32.h"
#include "helper.h"

#define MAX_PKT_SIZE 64
#define MAX_UARTS 2
#define BUFLEN 2048
#define PORT 2342
#define IP4_SIZE 4
#define OPENBEACON_SIZE 32
#define REFRESH_SERVER_IP_TIME (60*5)
#define DUPLICATES_BACKLOG_SIZE 32

static uint8_t buffer[BUFLEN];
static uint32_t g_duplicate_backlog[DUPLICATES_BACKLOG_SIZE];
static int g_duplicate_pos;

typedef struct {
	int fd;
	int id;
	int pos;
	uint8_t escape;
	uint8_t buf[MAX_PKT_SIZE];
} TReader;

TReader g_reader[MAX_UARTS];

static void port_reader_pkt(TReader *reader)
{
	int i;
	uint8_t duplicate;
	uint32_t crc;

	/* return invalid packet size */
	if(reader->pos != (OPENBEACON_SIZE+1))
		return;

	/* calculate checkum to check for duplicates */
	crc = crc32 (&reader->buf[1], OPENBEACON_SIZE);
	/* check previous packets for duplicates */
	duplicate = 0;
	for (i = 0; i < DUPLICATES_BACKLOG_SIZE; i++)
		if (g_duplicate_backlog[i] == crc)
		{
			duplicate = 1;
			break;
		}

	/* remember unique CRC's */
	if(!duplicate)
	{
		g_duplicate_backlog[g_duplicate_pos++] = crc;
		if(g_duplicate_pos>=DUPLICATES_BACKLOG_SIZE)
			g_duplicate_pos=0;

		printf("rx_pkt[%i]=%i [%i]\n", reader->id, reader->pos, (int8_t)reader->buf[0]);
	}
}

static void port_reader(TReader *reader, uint8_t *buffer, int len)
{
	uint8_t data;

	while(len--)
	{
		data = *buffer++;

		if(reader->escape)
		{
			reader->escape = 0;

			switch(data)
			{
				case 0x00 :
					if(reader->pos < MAX_PKT_SIZE)
						reader->buf[reader->pos++] = 0xFF;
					break;

				case 0x01 :
					port_reader_pkt(reader);
					/* reset packet */
					reader->pos = 0;
					break;

				default:
					fprintf(stderr, "error: invalid encoding [%02X]\n", data);
			}
		}
		else
			if(data == 0xFF)
				reader->escape = 1;
			else
				if(reader->pos < MAX_PKT_SIZE)
					reader->buf[reader->pos++] = data;
	}
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
	cfsetspeed(&options, B1000000);
	tcsetattr(handle, TCSANOW, &options);
	tcflush(handle, TCIFLUSH);

	return handle;
}

int main( int argc, const char* argv[] )
{
	const char* host;
	struct sockaddr_in si_me, si_other, si_server;
	struct hostent *addr;
	int sock, len, port;
	uint32_t server_ip;
	socklen_t slen = sizeof (si_other);
	time_t lasttime, t;
	fd_set fds;
	int i, maxfd, res;
	struct timeval timeout;
	TReader *reader;

	if( argc < 2 )
	{
		fprintf (stderr, "usage: %s hostname [port]\n", argv[0]);
		return 1;
	}

	/* reset variables */
	g_duplicate_pos = 0;
	memset(&g_duplicate_backlog, 0, sizeof(g_duplicate_backlog));

	/* initialize descriptor list */
	FD_ZERO(&fds);
	memset(&g_reader, 0, sizeof(g_reader));
	g_reader[0].id = 1;
	g_reader[0].fd = port_open("/dev/ttyO1");
	g_reader[1].id = 2;
	g_reader[1].fd = port_open("/dev/ttyO2");
	maxfd = g_reader[1].fd+1;

	/* assign default port if needed */
	port = ( argc < 3 ) ? PORT : atoi(argv[2]);

	/* get host name */
	host = argv[1];

	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		fprintf(stderr, "error: can't create socket\n");
		return 2;
	}
	else
	{
		memset ((char *) &si_me, 0, sizeof (si_me));
		si_me.sin_family = AF_INET;
		si_me.sin_port = htons (port);
		si_me.sin_addr.s_addr = htonl (INADDR_ANY);

		if (bind (sock, (sockaddr *) & si_me, sizeof (si_me)) == -1)
		{
			fprintf (stderr, "error: can't bind to listening socket\n");
			return 3;
		}

		lasttime = 0;
		server_ip = 0;

		memset (&si_server, 0, sizeof (si_server));
		si_server.sin_family = AF_INET;
		si_server.sin_port = htons (port);

		/* loop over UART data */
		while (1)
		{
			/* update server connection */
			t = time (NULL);
			if ((t - lasttime) > REFRESH_SERVER_IP_TIME)
			{
				if ((addr = gethostbyname (host)) == NULL)
					fprintf (stderr, "error: can't resolve server name (%s)\n", host);
				else
					if ((addr->h_addrtype != AF_INET)
						|| (addr->h_length != IP4_SIZE))
						fprintf (stderr, "error: wrong address type\n");
					else
					{
						memcpy (&si_server.sin_addr, addr->h_addr, addr->h_length);

						if (server_ip != si_server.sin_addr.s_addr)
						{
							server_ip = si_server.sin_addr.s_addr;
							fprintf (stderr, "refreshed server IP to [%s]\n",
							inet_ntoa (si_server.sin_addr));
						}

						lasttime = t;
					}
			}

			/* set timeout value within input loop */
			timeout.tv_usec = 0; /* milliseconds */
			timeout.tv_sec  = 1; /* seconds */
			/* register descriptors */
			for(i=0; i<MAX_UARTS; i++)
				FD_SET(g_reader[i].fd, &fds);
			/* wait for data */
			res = select(maxfd, &fds, NULL, NULL, &timeout);
			/* retry on timeout */
			if(res == 0)
				continue;

			/* process data for all readers */
			for(i=0; i<MAX_UARTS; i++)
			{
				reader = &g_reader[i];

				if(FD_ISSET(reader->fd, &fds))
					do {
						res = read(reader->fd, buffer, BUFLEN);
						if(res>0)
							port_reader(reader, buffer, res);
					} while (res == BUFLEN);
			}
		}

		while (1)
		{
			if ((len = recvfrom (sock, &buffer, BUFLEN - IP4_SIZE, 0,
				(sockaddr *) & si_other, &slen)) == -1)
				return 4;

			if (len == OPENBEACON_SIZE)
			{
				memcpy (&buffer[len], &si_other.sin_addr.s_addr, IP4_SIZE);
				sendto (sock, &buffer, len + IP4_SIZE, 0,
					(struct sockaddr *) &si_server, sizeof (si_server));
			}
		}
	}
}
