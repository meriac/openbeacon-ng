/***************************************************************
 *
 * OpenBeacon.org - CRC16 routine
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
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
#include "crc16.h"

uint16_t
crc16_continue (uint16_t crc, const uint8_t * buffer, uint32_t size)
{
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

uint16_t
crc16 (const uint8_t * buffer, uint32_t size)
{
	return crc16_continue(0xFFFF, buffer, size);
}

uint16_t
icrc16 (const uint8_t * buffer, uint32_t size)
{
	return crc16 (buffer, size) ^ 0xFFFF;
}
