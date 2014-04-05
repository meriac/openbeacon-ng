/***************************************************************
 *
 * OpenBeacon.org - nRF51 Misc Helper Routines
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
#include <openbeacon.h>
#include <helper.h>

uint32_t sqrt32(uint32_t val)
{
	uint32_t res, bit, t;

	bit = 1<<30;
	while(bit>val)
		bit >>= 2;

	res = 0;
	while(bit)
	{
		t = res+bit;
		if(val>=t)
		{
			val -= t;
			res = (res>>1) + bit;
		}
		else
			res >>= 1;
		bit>>= 2;
	}
	return res;
}
