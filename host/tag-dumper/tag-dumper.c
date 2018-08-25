/***************************************************************
 *
 * OpenBeacon.org - Decode Tags Serial Port Dumps
 *
 * Copyright 2016 Milosch Meriac <milosch@meriac.com>
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define TAG_UART_BAUD_RATE 921600
#define BUFFER_SIZE 1024
#define PACKED __attribute__((packed))

#include "inc/openbeacon-proto.h"
#include "heatshrink_decoder.h"
#include "crc32.h"
#include "helper.h"

bool g_decoding;
heatshrink_decoder g_hsd;
uint32_t g_tag_id, g_page_count, g_group;
size_t g_out_pos;
TBeaconProxSighting g_sighting;

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

static void port_process_tag(void)
{
	fprintf(stdout,
		"{ \"tag_me\": \"0x%08X\", \"tag_them\": \"0x%08X\", \"time_local_s\": %8u, \"time_remote_s\": %8u, \"rssi\": %3i, \"angle\": %3i, \"group\": %u }\r\n",
		g_tag_id,
		g_sighting.tag_id,
		g_sighting.epoch_local,
		g_sighting.epoch_remote,
		g_sighting.power,
		g_sighting.angle,
		g_group);
}

static bool port_rx(uint8_t last_type, const uint8_t* buffer, int size)
{
	uint32_t crc, length;
	HSD_sink_res sres;
	HSD_poll_res pres;
	size_t in_pos, read, written;
	TBeaconProxSightingPage *page;

	switch(last_type)
	{
		case 0x01:
			if(size==sizeof(uint32_t))
			{
				g_tag_id = *((uint32_t*)buffer);
				fprintf(stderr, "Received Tag ID=0x%08X\n", g_tag_id);
			}
			break;

		case 0x02:
			if(size!=sizeof(*page))
				fprintf(stderr, "Received invalid page size of %i bytes\n", size);
			else
			{
				page = (TBeaconProxSightingPage*)buffer;

				/* abort decoding on log interruptions */
				if(page->length == 0xFFFF)
				{
					heatshrink_decoder_reset(&g_hsd);
					g_out_pos = 0;
					g_page_count++;
					g_decoding = false;
					break;
				}

				crc = crc32(page, sizeof(*page)-sizeof(page->crc32));
				length = page->length & BEACON_PROXSIGHTING_LENGTH_MASK;
				if((crc!=page->crc32) || (length>sizeof(page->buffer)))
				{
					fprintf(stderr, "Invalid page at page number %05i of size %i bytes (len=%i)\n",
						g_page_count, size, page->length);
					g_decoding = false;
				}
				else
				{
					/* restart decoding if new log is found */
					if(page->length & BEACON_PROXSIGHTING_PAGE_MARKER)
					{
						fprintf(stderr, "Found new log group[%u] at page %u\n",
							g_group, g_page_count);
						heatshrink_decoder_reset(&g_hsd);
						g_out_pos = 0;
						g_decoding = true;
						g_group++;
					}

					in_pos = 0;
					do {
						if((sres = heatshrink_decoder_sink(
							&g_hsd, &page->buffer[in_pos], sizeof(page->buffer)-in_pos, &read))<0)
						{
							fprintf(stderr, "ERROR: Invalid decoder result (%i)\n", sres);
							g_decoding = false;
							return false;
						}
						in_pos += read;

						do {
							written = 0;
							pres = heatshrink_decoder_poll(
								&g_hsd,
								((uint8_t*)&g_sighting) + g_out_pos,
								sizeof(g_sighting)-g_out_pos,
								&written);

							if(pres>=0)
							{
								g_out_pos+=written;

								if(g_out_pos==sizeof(g_sighting))
								{
									port_process_tag();
									g_out_pos = 0;
								}
							}
							else
							{
								fprintf(stderr, "ERROR: poll error %i\n", pres);
								g_decoding = false;
								return false;
							}
						} while(pres == HSDR_POLL_MORE);
					} while(in_pos<sizeof(page->buffer));
				}

				/* always increment page count independent of CRC */
				g_page_count++;
			}
			break;

		case 0x03:
			fprintf(stdout, "Terminated reception ot tag data.\n");
			g_tag_id = 0;
			g_decoding = false;
			break;
	}

	return true;
}

int main( int argc, const char* argv[] )
{
	int fd, i, pos;
	fd_set fds;
	int maxfd, res;
	bool escaped, done, overflow;
	uint8_t buffer_in[BUFFER_SIZE], buffer_out[BUFFER_SIZE], data, last_type;
	struct timeval timeout;

	if( argc < 2 )
	{
		fprintf (stderr, "usage: %s /dev/ttyUSB0\n", argv[0]);
		return 1;
	}

	/* initialize descriptor list */
	FD_ZERO(&fds);
	fd = port_open(argv[1]);
	maxfd = fd+1;


	/* initialize decompression library */
	heatshrink_decoder_reset(&g_hsd);

	/* loop over UART data */
	pos = 0;
	last_type = 0;
	escaped = done = overflow = false;
	while (!done)
	{
		/* set timeout value within input loop */
		timeout.tv_usec = 0; /* milliseconds */
		timeout.tv_sec  = 1; /* seconds */
		/* register descriptors */
		FD_SET(fd, &fds);
		/* wait for data, retry on timeout */
		if(select(maxfd, &fds, NULL, NULL, &timeout) == 0)
		{
			fprintf(stderr, ".");
			continue;
		}
		if(FD_ISSET(fd, &fds))
		{
			do {
				/* quit if end of file is reached */
				if((res = read(fd, buffer_in, BUFFER_SIZE))<=0)
					goto done;

				/* iterate through receive buffer */
				for(i=0; i<res; i++)
				{
					data = buffer_in[i];

					if(escaped)
					{
						escaped = false;

						switch(data)
						{
							/* deal with escaped 0xFF */
							case 0x00:
								if(pos<BUFFER_SIZE)
									buffer_out[pos++] = 0xFF;
								else
									overflow = true;
								break;

							/* transmission start */
							case 0x01:
								g_page_count = 0;
								last_type = data;
								pos = 0;
								break;

							/* frame start/end */
							case 0x02:
							case 0x03:
								if(overflow)
								{
									overflow = false;
									fprintf (stderr, "ERROR: overflow error, ignoring packet\n");
								}
								else
									if(pos && last_type)
									{
										if(!port_rx(last_type, buffer_out, pos))
											break;
									}
								last_type = data;
								pos = 0;
								fflush(stdout);
								break;

							default:
								fprintf (stderr, "ERROR: invalid character sequence (0x%02X)!\n", data);
								last_type = 0;
						}
					}
					else
					{
						if(data==0xFF)
							escaped = true;
						else
							if(pos<BUFFER_SIZE)
								buffer_out[pos++] = data;
							else
								overflow = true;
					}
				}
			} while (res == BUFFER_SIZE);
		}
	}

done:
	close(fd);
	return 0;
}
