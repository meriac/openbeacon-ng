/***************************************************************
 *
 * OpenBeacon.org - helper functions
 *
 * Copyright 2016 Milosch Meriac <milosch@meriac.com>
 *
 ***************************************************************

 This file is part of the OpenBeacon.org active RFID tracking project

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
#include <stdio.h>
#include <helper.h>

void
hex_dump (const unsigned char *buf, unsigned int len)
{
	unsigned int start, i, j;
	char c;

	start = 0;

	for (j = 0; j < len; j += 16)
	{
		printf ("%08x:", start + j);

		for (i = 0; i < 16; i++)
		{
			if ((start + i + j) < len)
				printf (" %02x", buf[start + i + j]);
			else
				printf ("   ");
		}
		printf ("  |");
		for (i = 0; i < 16; i++)
		{
			if ((start + i + j) < len)
			{
				c = buf[start + i + j];
				if (c >= ' ' && c < 127)
					printf ("%c", c);
				else
					printf (".");
			}
			else
				printf (" ");
		}
		printf ("|\n\r");
	}
}
