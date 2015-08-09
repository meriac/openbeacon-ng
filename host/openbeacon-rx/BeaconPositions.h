/***************************************************************
 *
 * OpenBeacon.org - Reader Position Settings
 *
 * Copyright 2015 Milosch Meriac <milosch@meriac.com>
 *
 ***************************************************************/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation; version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __READERPOSITIONS_H__
#define __READERPOSITIONS_H__

typedef struct
{
	uint32_t id, room, floor, group;
	double pX, pY;
} TBeaconItem;

static const TBeaconItem g_BeaconList[] = {
	{0x898C666C, 1, 1, 1,   24, 1000},
	{0x8B61F634, 1, 1, 1,   68,  700},
	{0x3300C73C, 1, 1, 1, 1000,  700}
};

#define BEACON_COUNT ((int)(sizeof(g_BeaconList)/sizeof(g_BeaconList[0])))

#endif/*__READERPOSITIONS_H__*/
