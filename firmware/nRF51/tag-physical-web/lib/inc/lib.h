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

#ifndef __LIB_H__
#define __LIB_H__

#include <acc.h>
#include <flash.h>
#include <radio.h>
#include <timer.h>

/* userland entry */
extern void entry(void);

/* shortcuts to often used functions */
#define pin_set nrf_gpio_pin_set
#define pin_clear nrf_gpio_pin_clear

/* string compression for tah URLs */
typedef enum {
	PROTO_HTTP_WWW  = 0, /* http://www.  */
	PROTO_HTTPS_WWW = 1, /* https://www. */
	PROTO_HTTP      = 2, /* http://      */
	PROTO_HTTPS     = 3, /* https://     */
	PROTO_TEL       = 4, /* tel:         */
	PROTO_MAILTO    = 5, /* mailto:      */
	PROTO_GEO       = 6, /* geo:         */
	DOT_COM         = 7, /* .com         */
	DOT_ORG         = 8, /* .org         */
	DOT_EDU         = 9, /* .edu         */
	} TTagURLShortener;

#endif/*__LIB_H__*/
