/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
 * Modified by Ciro Cattuto <ciro.cattuto@isi.it>
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
#ifndef __MAIN_H__
#define __MAIN_H__

extern uint8_t hibernate;
extern uint16_t status_flags;
extern uint8_t boot_count;
extern uint32_t reset_reason;

#if CONFIG_ACCEL_SLEEP
extern uint8_t sleep;
#endif

extern void blink(uint8_t times);
extern void blink_fast(uint8_t times);
extern uint16_t blink_wait_release(void);

#endif/*__MAIN_H__*/
