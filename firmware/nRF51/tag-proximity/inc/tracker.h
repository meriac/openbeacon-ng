/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Radio Routines
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
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
#ifndef __TRACKER_H__
#define __TRACKER_H__

extern const void* tracker_px(uint16_t listen_wait_ms);
extern const void* tracker_tx(uint16_t listen_wait_ms, uint16_t tx_delay_us);
extern void tracker_receive(uint32_t uid, int8_t tx_power, int8_t rx_power);
extern void tracker_init(uint32_t uid);
extern uint8_t tracker_loop(void);
extern void tracker_second_tick(void);

#endif/*__TRACKER_H__*/