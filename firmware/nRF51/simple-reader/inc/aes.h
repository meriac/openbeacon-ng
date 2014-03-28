/***************************************************************
 *
 * OpenBeacon.org - nRF51 AES Hardware Encryption & Signing
 *
 * Copyright 2014 Milosch Meriac <meriac@openbeacon.de>
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
#ifndef __AES_H__
#define __AES_H__

#define AES_BLOCK_SIZE 16
#define AES_BLOCKS32 (AES_BLOCK_SIZE/4)

typedef uint8_t TAES[AES_BLOCK_SIZE];

typedef struct {
	TAES key, in, out;
} PACKED TCryptoEngine;

extern void aes_init(void);
extern void aes_encrypt(TCryptoEngine* engine);
extern TAES* aes_sign(const void* data, uint32_t length);



#endif/*__AES_H__*/
