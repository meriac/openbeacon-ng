/***************************************************************
 *
 * OpenBeacon.org - nRF51 Timer Routines
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
#ifndef __TIMER_H__
#define __TIMER_H__

#define LF_FREQUENCY 32768UL

#define SECONDS(x) ((uint32_t)((LF_FREQUENCY*x)+0.5))
#define MILLISECONDS(x) ((uint32_t)(((LF_FREQUENCY*x)/1000.0)+0.5))

extern void timer_init(void);
extern void timer_wait(uint32_t ticks);

#endif/*__TIMER_H__*/
