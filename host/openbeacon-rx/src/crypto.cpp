/***************************************************************
 *
 * OpenBeacon.org - AES Encryption & Signing
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crypto.h"

const TAES g_default_key = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

static TCryptoEngine g_signature, g_encrypt;

TAES* aes_sign(const void* data, uint32_t length)
{
	uint8_t i, t, *src;

	/* reset signature buffer */
	memset(g_signature.out, 0xFF, sizeof(g_signature.out));

	/* sign data */
	src = (uint8_t*)data;
	while(length)
	{
		/* process data block by block */
		t = (length>=AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : length;
		length -= t;

		/* XOR previous AES output data in */
		for(i=0; i<t; i++)
			g_signature.in[i] = g_signature.out[i] ^ *src++;

		/* AES hash block, result in 'out' */
		aes(&g_signature);
	}

	/* return full AES signature */
	return &g_signature.out;
}

static void aes_process(const uint8_t *src, uint8_t *dst, uint32_t length)
{
	uint8_t t, i;

	/* encrypt data */
	while(length)
	{
		/* process data block by block */
		t = (length>=AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : length;
		length -= t;

		/* AES encrypt block, result in 'out' */
		aes(&g_encrypt);
		/* make AES output the IV of the next encryption */
		if(length)
			memcpy(g_encrypt.in, g_encrypt.out, AES_BLOCK_SIZE);

		/* XOR previous AES output data in */
		for(i=0; i<t; i++)
			*dst++ = g_encrypt.out[i] ^ *src++;
	}
}

uint8_t aes_encr(const void* in, void* out, uint32_t length, uint8_t mac_len)
{
	/* verify parameters */
	if(mac_len>AES_BLOCK_SIZE)
		return 1;
	if(length<=mac_len)
		return 2;

	/* calculate payload length */
	length -= mac_len;
	/* sign payload to create IV */
	memcpy(&g_encrypt.in, aes_sign(in, length), mac_len);
	/* pad IV with 0xFF if needed to generate IV block */
	if(mac_len<AES_BLOCK_SIZE)
		memset(&g_encrypt.in[mac_len], 0xFF, AES_BLOCK_SIZE-mac_len);
	/* copy IV to payload end */
	memcpy((uint8_t*)out + length, &g_encrypt.in, mac_len);

	/* encrypt data */
	aes_process((uint8_t*)in, (uint8_t*)out, length);
	return 0;
}

uint8_t aes_decr(const void* in, void* out, uint32_t length, uint8_t mac_len)
{
	/* verify parameters */
	if(mac_len>AES_BLOCK_SIZE)
		return 1;
	if(length<=mac_len)
		return 2;

	/* calculate payload length */
	length -= mac_len;
	/* re-create IV from end of packet */
	memcpy(&g_encrypt.in, ((uint8_t*)in) + length, mac_len);
	/* pad IV with 0xFF if needed to generate IV block */
	if(mac_len<AES_BLOCK_SIZE)
		memset(&g_encrypt.in[mac_len], 0xFF, AES_BLOCK_SIZE-mac_len);

	/* decrypt data */
	aes_process((uint8_t*)in, (uint8_t*)out, length);

	/* verify signature */
	if(memcmp(aes_sign(out, length), ((uint8_t*)in) + length, mac_len)==0)
	{
		/* reset signature in output */
		memset(((uint8_t*)out) + length, 0xFF, mac_len);
		return 0;
	}
	else
	{
		/* erase broken payload */
		memset(out, 0, length);
		return 3;
	}
}
#define AES_ROUNDS 10

static const uint8_t g_sbox[256] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
  0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
  0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
  0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
  0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
  0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
  0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
  0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
  0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
  0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
  0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
  0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
  0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
  0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
  0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
  0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
  0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static uint8_t
xtime (uint8_t x)
{
  /* FIXME: timing attack */
  return (x & 0x80) ? ((x << 1) ^ 0x1b) : (x << 1);
}

#define SubBytes(i) g_state[i] = g_sbox[g_state[i]]
#define AddRoundKey(i) g_state[i] ^= g_key[i]
#define MixColumn(i) \
	t[0] = g_state[i]; \
	t[1] = g_state[i + 1]; \
	t[2] = g_state[i + 2]; \
	t[3] = g_state[i + 3]; \
	x = t[0] ^ t[1] ^ t[2] ^ t[3]; \
	g_state[i] ^= x ^ xtime (t[0] ^ t[1]); \
	g_state[i + 1] ^= x ^ xtime (t[1] ^ t[2]); \
	g_state[i + 2] ^= x ^ xtime (t[2] ^ t[3]); \
	g_state[i + 3] ^= x ^ xtime (t[3] ^ t[0]);

static uint8_t *g_key;
static uint8_t *g_state;

static void
AddRoundKeys(void)
{
	/* unroll AddRoundKey */
	AddRoundKey(0);
	AddRoundKey(1);
	AddRoundKey(2);
	AddRoundKey(3);
	AddRoundKey(4);
	AddRoundKey(5);
	AddRoundKey(6);
	AddRoundKey(7);
	AddRoundKey(8);
	AddRoundKey(9);
	AddRoundKey(10);
	AddRoundKey(11);
	AddRoundKey(12);
	AddRoundKey(13);
	AddRoundKey(14);
	AddRoundKey(15);
}

void aes(TCryptoEngine* engine)
{
	uint8_t t[4];
	uint8_t x, rcon;
	uint8_t round;

	g_state = engine->out;
	g_key = engine->key;

	memcpy(&engine->out, &engine->in, AES_BLOCK_SIZE);

	AddRoundKeys();

	rcon = 1;
	for (round = 0; round < AES_ROUNDS; round++)
	{
		/* unroll SubBytes */
		SubBytes(0);
		SubBytes(1);
		SubBytes(2);
		SubBytes(3);
		SubBytes(4);
		SubBytes(5);
		SubBytes(6);
		SubBytes(7);
		SubBytes(8);
		SubBytes(9);
		SubBytes(10);
		SubBytes(11);
		SubBytes(12);
		SubBytes(13);
		SubBytes(14);
		SubBytes(15);

		/* row 2 */
		x = g_state[1];
		g_state[1]  = g_state[5];
		g_state[5]  = g_state[9];
		g_state[9]  = g_state[13];
		g_state[13] = x;
		/* row 3 */
		x = g_state[2];
		g_state[2]  = g_state[10];
		g_state[10] = x;
		x = g_state[6];
		g_state[6]  = g_state[14];
		g_state[14] = x;
		/* row 4 */
		x = g_state[3];
		g_state[3]  = g_state[15];
		g_state[15] = g_state[11];
		g_state[11] = g_state[7];
		g_state[7]  = x;

		if (round < (AES_ROUNDS-1))
		{
			MixColumn(0);
			MixColumn(4);
			MixColumn(8);
			MixColumn(12);
		}

		g_key[0] ^= g_sbox[g_key[13]] ^ rcon;
		g_key[1] ^= g_sbox[g_key[14]];
		g_key[2] ^= g_sbox[g_key[15]];
		g_key[3] ^= g_sbox[g_key[12]];

		g_key[4] ^= g_key[0];
		g_key[5] ^= g_key[1];
		g_key[6] ^= g_key[2];
		g_key[7] ^= g_key[3];

		g_key[8] ^= g_key[4];
		g_key[9] ^= g_key[5];
		g_key[10] ^= g_key[6];
		g_key[11] ^= g_key[7];

		g_key[12] ^= g_key[8];
		g_key[13] ^= g_key[9];
		g_key[14] ^= g_key[10];
		g_key[15] ^= g_key[11];

		AddRoundKeys();

		/* update rcon */
		rcon = xtime(rcon);
	}
}

/* initial key derivation */
void aes_key_derivation(const TAES* key)
{
	/* use base key to derieve needed keys */
	memcpy(g_encrypt.key, key, sizeof(g_encrypt.key));

	/* create site-signature key */
	memset(g_encrypt.in, AES_KEYID_SIGNATURE, sizeof(g_encrypt.in));
	aes(&g_encrypt);
	memcpy(g_signature.key, g_encrypt.out, sizeof(g_signature.key));

	/* finally, create site-encryption key */
	memset(g_encrypt.in, AES_KEYID_ENCRYPTION, sizeof(g_encrypt.in));
	aes(&g_encrypt);
	memcpy(g_encrypt.key, g_encrypt.out, sizeof(g_encrypt.key));
}

void aes_init(void)
{
	/* derieve initial key */
	aes_key_derivation(&g_default_key);
}
