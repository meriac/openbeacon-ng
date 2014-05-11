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
#include <openbeacon.h>
#include <aes.h>

const TAES g_default_key = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

static volatile uint8_t g_enc_done;
static TCryptoEngine g_signature, g_encrypt, g_auth;

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

		/* pad block if needed */
		if(t<AES_BLOCK_SIZE)
			memset(&g_signature.in[t], 0xFF, AES_BLOCK_SIZE-t);

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

void aes(TCryptoEngine* engine)
{
	g_enc_done = FALSE;
	NRF_ECB->EVENTS_ENDECB = 0;
	NRF_ECB->ECBDATAPTR = (uint32_t)engine;
	NRF_ECB->TASKS_STARTECB = 1;

	while(!g_enc_done)
		__WFI();
}

/* initial key derivation */
void aes_key_derivation(const TAES* key, uint32_t uid)
{
	/* use base key to derieve needed keys */
	memcpy(g_encrypt.key, key, sizeof(g_encrypt.key));

	/* create site-signature key */
	memset(g_encrypt.in, AES_KEYID_SIGNATURE, sizeof(g_encrypt.in));
	aes(&g_encrypt);
	memcpy(g_signature.key, g_encrypt.out, sizeof(g_signature.key));

	/* create tag-specific authentication key */
	memset(g_encrypt.in, AES_KEYID_AUTH, sizeof(g_encrypt.in));
	*((uint32_t*)&g_encrypt.in) ^= uid;
	aes(&g_encrypt);
	memcpy(g_auth.key, g_encrypt.out, sizeof(g_auth.key));

	/* finally, create site-encryption key */
	memset(g_encrypt.in, AES_KEYID_ENCRYPTION, sizeof(g_encrypt.in));
	aes(&g_encrypt);
	memcpy(g_encrypt.key, g_encrypt.out, sizeof(g_encrypt.key));
}

void ECB_IRQ_Handler(void)
{
	if(NRF_ECB->EVENTS_ENDECB)
	{
		/* acknowledge event */
		NRF_ECB->EVENTS_ENDECB = 0;
		g_enc_done = TRUE;
	}
}

void aes_init(uint32_t uid)
{
	g_enc_done = FALSE;

	NRF_ECB->TASKS_STOPECB = 1;
	NRF_ECB->INTENSET =
		ECB_INTENSET_ENDECB_Enabled   << ECB_INTENSET_ENDECB_Pos;
	NVIC_SetPriority(ECB_IRQn, IRQ_PRIORITY_AES);
	NVIC_EnableIRQ(ECB_IRQn);

	/* derieve initial key */
	aes_key_derivation(&g_default_key, uid);
}
