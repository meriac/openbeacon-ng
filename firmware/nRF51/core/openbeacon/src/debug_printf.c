/***************************************************************
 *
 * OpenBeacon.org - debug_printf wrapper for ts_printf
 *
 * Copyright 2010-2012 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

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

#include <openbeacon.h>
#if !defined UART_DISABLE || defined ENABLE_USB_FULLFEATURED
#include <ctype.h>
#include <printf.h>
#include <stdarg.h>
#include <string.h>

static void
putc_debug (void *p, char c)
{
	(void) p;

	default_putchar (c);
}

void
debug_printf (const char *fmt, ...)
{
	va_list va;

	va_start (va, fmt);
	tfp_format (NULL, putc_debug, fmt, va);
	va_end (va);
}

char
hex_char (unsigned char hex)
{
	hex &= 0xF;
	return (hex < 0xA) ? (hex + '0') : ((hex - 0xA) + 'A');
}

void
hex_dump (const unsigned char *buf, unsigned int addr, int len)
{
	int i;
	char c;

	while(len>0)
	{
		debug_printf ("%08x:", addr);

		/* print hex bytes */
		for (i = 0; i < 16; i++)
		{
			if(i>=len)
				debug_printf ("   ");
			else
				debug_printf (" %02x", buf[i]);
		}

		debug_printf ("  |");

		/* print characters */
		for (i = 0; i < 16; i++)
		{
			c = (i<len) ? *buf++ : ' ';
			default_putchar ( ((c>=' ')&&(c<127)) ? c : '.' );
		}

		debug_printf ("|\n\r");

		/* next block? */
		if(len<=16)
			break;
		len-=16;
		addr+=16;
	}
}
#endif /* UART_DISABLE */
