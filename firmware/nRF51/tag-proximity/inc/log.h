/***************************************************************
 *
 * OpenBeacon.org - flash logging routines
 *
 * Copyright 2014 Ciro Cattuto <ciro.cattuto@isi.it>
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

#include <flash.h>

/* RAM ring buffer */

#define BUF_SIZE		4096
#define BUF_LEN_THRES	(BUF_SIZE / 3 * 2)


/* flash storage */

#define FLASH_LOG_CONFIG_PAGE	256
#define FLASH_LOG_FIRST_PAGE	1024
#define FLASH_LOG_LAST_PAGE		2047 /* AT45D flash sector 1 */


/* logging routines */

extern uint8_t flash_setup_logging(void);
extern uint16_t flash_log(uint16_t len, uint8_t *data);
extern void flash_log_write_trigger(void);
extern void flash_log_status(void);
extern void flash_dump(void);

#endif /*__LOG_H__*/
