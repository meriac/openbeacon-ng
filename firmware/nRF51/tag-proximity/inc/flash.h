/***************************************************************
 *
 * OpenBeacon.org - nRF51 Flash Routines
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
#ifndef __FLASH_H__
#define __FLASH_H__

#ifndef CONFIG_FLASH_PAGESIZE
#define CONFIG_FLASH_PAGESIZE 264
#endif/*CONFIG_FLASH_PAGESIZE*/

extern uint8_t flash_init(void);
extern uint32_t flash_size(void);
extern void flash_sleep(int sleep);
extern void flash_erase(void);
extern uint8_t flash_status(void);
extern void flash_page_read(uint32_t page, uint8_t *data, uint32_t length);
extern void flash_page_write(uint32_t page, const uint8_t *data, uint32_t length);

#endif/*__FLASH_H__*/
