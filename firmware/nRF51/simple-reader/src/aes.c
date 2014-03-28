/***************************************************************
 *
 * OpenBeacon.org - nRF51 AES Hardware Encryption & Signing
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
#include <aes.h>

static TCryptoEngine g_signature, g_encrypt;

TAES* aes_sign(const void* data, uint32_t length)
{
	uint8_t t, *src, *in, *out;

	/* reset signature buffer */
	memset(g_signature.out, 0xFF, sizeof(g_signature.out));

	/* reset input buffer if needed */
	if(length<AES_BLOCK_SIZE)
		memset(&g_signature.in[length], 0, AES_BLOCK_SIZE-length);

	/* sign data */
	src = (uint8_t*)data;
	while(length)
	{
		in  = g_signature.in;
		out = g_signature.out;

		/* full block left */
		if(length>=AES_BLOCK_SIZE)
		{
			/* XOR'ing out-signature with data */
			t = AES_BLOCKS32;
			while(t--)
			{
				*((uint32_t*)in) = *((uint32_t*)out) ^ *((uint32_t*)src);
				in+=4;
				out+=4;
				src+=4;
			}
			/* decrement remaining size */
			length -= AES_BLOCK_SIZE;
		}
		else
			/* XOR remainder into in-buffer */
			while(length)
			{
				length--;
				(*in++) = (*out++) ^ (*src++);
			}

		/* AES hash block, result in 'out' */
		aes_encrypt(&g_signature);
	}

	/* invert result */
	out = g_signature.out;
	t = AES_BLOCKS32;
	while(t--)
	{
		*((uint32_t*)out) ^= 0xFFFFFFFFUL;
		out+=4;
	}

	/* return full AES signature */
	return &g_signature.out;
}

uint8_t aes_enc(const void* in, void* out, uint32_t size, uint8_t mac_len)
{
	uint8_t *key;
	uint32_t t,*bin,*bout,*bkey;

	if(mac_len>AES_BLOCK_SIZE)
		return 1;
	if(size<=mac_len)
		return 2;

	/* reset IV buffer */
	memset(&g_encrypt.out, 0xFF, sizeof(g_encrypt.out));

	/* calculate payload length */
	size -= mac_len;
	/* sign payload */
	memcpy(&g_encrypt.in, aes_sign(in, size), mac_len);
	/* pad with zeros if needed to generate IV block */
	if(mac_len<AES_BLOCK_SIZE)
		memset(&g_encrypt.in[mac_len], 0, AES_BLOCK_SIZE-mac_len);

	/* copy IV/HMAC to end of the packet */
	memcpy(((uint8_t*)out)+size, &g_encrypt.in, mac_len);

	/* create AES keystream based on IV */
	while(size)
	{
		bin  = (uint32_t*)&g_encrypt.in;
		bout = (uint32_t*)&g_encrypt.out;

		/* XOR results of previous block into new input */
		t = AES_BLOCKS32;
		while(t--)
			*bin++ ^= *bout++;
		/* create keystream block */
		aes_encrypt(&g_encrypt);

		/* encrypt full bock using the generated keystream */
		if(size>=AES_BLOCK_SIZE)
		{
			bin  = (uint32_t*)in;
			bout = (uint32_t*)out;
			bkey = (uint32_t*)g_encrypt.out;
			t = AES_BLOCKS32;
			while(t--)
				(*bout++) = (*bin++) ^ (*bkey++);

			/* next block */
			in  = (uint8_t*)in  + AES_BLOCK_SIZE;
			out = (uint8_t*)out + AES_BLOCK_SIZE;
		}
		else
		{
			key = g_encrypt.out;
			while(size)
			{
				size--;
				*((uint8_t*)out) = *((uint8_t*)in) ^ *key++;

				/* next byte */
				in  = ((uint8_t*)in)  + 1;
				out = ((uint8_t*)out) + 1;
			}
		}
	}
	return 0;
}

void aes_encrypt(TCryptoEngine* engine)
{
	NRF_ECB->ECBDATAPTR = (uint32_t)engine;
	NRF_ECB->TASKS_STARTECB = 1;

	while(!NRF_ECB->EVENTS_ENDECB)
		__WFE();
	NRF_ECB->EVENTS_ENDECB = 0;
}

void aes_init(void)
{
	NRF_ECB->TASKS_STOPECB = 1;
	NRF_ECB->INTENSET =
		(ECB_INTENSET_ENDECB_Enabled << ECB_INTENSET_ENDECB_Pos);
}
