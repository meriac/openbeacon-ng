/***************************************************************
 *
 * OpenBeacon.org - log tag sigtings to external flash
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

#ifndef __LOG_H__
#define __LOG_H__

extern uint8_t log_init(uint32_t tag_id);
extern void log_dump(void);
extern void log_sighting(uint32_t epoch_local, uint32_t epoch_remote,
	uint32_t tag_id, uint8_t power, int8_t angle);
extern void log_process(void);

#endif/*__LOG_H__*/
