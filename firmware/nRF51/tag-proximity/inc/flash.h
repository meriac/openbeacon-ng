/***************************************************************
 *
 * OpenBeacon.org - nRF51 Flash Routines
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
#ifndef __FLASH_H__
#define __FLASH_H__

extern uint32_t flash_size(void);

extern uint16_t flash_status(void);
extern uint16_t flash_wait_ready(uint8_t wait_interval_ms);
extern uint16_t flash_wait_status(uint8_t wait_interval_ms);

extern void flash_reset(void);
extern void flash_wakeup(void);
extern void flash_sleep_deep(void);
extern void flash_sleep(void);
extern void flash_sleep_resume(void);

extern void flash_set_sector_protection(uint8_t enable);
extern void flash_erase_page(uint16_t page_addr);
extern void flash_erase_block(uint16_t page_addr);
extern void flash_erase_sector(uint16_t page_addr);
extern void flash_erase_chip(void);

extern void flash_read_buffer(uint8_t bufnum, uint16_t byte_offset, uint16_t len, uint8_t *data);
extern void flash_write_buffer(uint8_t bufnum, uint16_t byte_offset, uint16_t len, uint8_t *data);

extern void flash_write_buffer_to_page(uint8_t bufnum, uint16_t page_addr, uint8_t erase_page);
extern void flash_write_page_through_buffer(uint8_t bufnum, uint16_t page_addr, uint16_t byte_offset, uint16_t len, uint8_t erase_page, uint8_t *data);
extern void flash_rewrite_page_through_buffer(uint8_t bufnum, uint16_t page_addr);

extern void flash_read_page_to_buffer(uint8_t bufnum, uint16_t page_addr);
extern void flash_read_page(uint16_t page_addr, uint16_t byte_offset, uint16_t len, uint8_t *data);
extern uint8_t flash_compare_page_to_buffer(uint8_t bufnum, uint16_t page_addr);

extern uint8_t flash_init(void);

#endif/*__FLASH_H__*/
