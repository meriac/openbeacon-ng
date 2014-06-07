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

#define AES_KEYID_ENCRYPTION 0x01
#define AES_KEYID_SIGNATURE  0x02
#define AES_KEYID_AUTH       0x03

typedef uint8_t TAES[AES_BLOCK_SIZE];

typedef struct {
	TAES key, in, out;
} PACKED TCryptoEngine;

extern void aes_init(uint32_t uid);
extern void aes_key_derivation(const TAES* key, uint32_t uid);
extern void aes(TCryptoEngine* engine);
extern TAES* aes_sign(const void* data, uint32_t length);
extern uint8_t aes_encr(const void* in, void* out, uint32_t size, uint8_t mac_len);
extern uint8_t aes_decr(const void* in, void* out, uint32_t length, uint8_t mac_len);

#endif/*__AES_H__*/
