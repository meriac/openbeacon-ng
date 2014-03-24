/***************************************************************
 *
 * OpenBeacon.org - debug_printf wrapper for ts_printf
 *
 * Copyright 2010-2013 Milosch Meriac <meriac@openbeacon.de>
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

#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

#if defined UART_DISABLE && !defined ENABLE_USB_FULLFEATURED
#define debug_printf(...)
#else /*UART_DISABLE */
extern void debug_printf (const char *fmt, ...);
extern char hex_char (unsigned char hex);
extern void hex_dump (const unsigned char *buf, unsigned int addr,
					  unsigned int len);
#endif /*UART_DISABLE */

#endif/*__DEBUG_PRINTF_H__*/
