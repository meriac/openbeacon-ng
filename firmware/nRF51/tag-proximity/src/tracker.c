/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Reader Communication Handling
 *
 * Copyright 2015 Milosch Meriac <milosch@meriac.com>
 *
 ***************************************************************

 This file is part of the OpenBeacon.org active RFID firmware

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

#include <openbeacon.h>
#include <tracker.h>
#include <openbeacon-proto.h>

static TBeaconNgTracker g_pkt_tracker ALIGN4;

const void* tracker_transmit(int tx_delay_ticks)
{
	(void)tx_delay_ticks;
	return &g_pkt_tracker;
}

void tracker_receive(uint32_t uid, int tx_power, int rx_power)
{
	(void)uid;
	(void)tx_power;
	(void)rx_power;
}

void tracker_init(uint32_t uid)
{
	memset(&g_pkt_tracker, 0, sizeof(g_pkt_tracker));
	g_pkt_tracker.uid = uid;
}

