/***************************************************************
 *
 * OpenBeacon.org - nRF51 2.4GHz Name Routines
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
#include <db.h>

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

typedef struct {
	int size;
	const char* list;
	const char* postfix;
} TList;

static const TList g_prefix[] = {
	{ sizeof(g_prefix_cal)-1,    g_prefix_cal ,   "cal" },
	{ sizeof(g_prefix_matic)-1,  g_prefix_matic , "matic" },
	{ sizeof(g_prefix_ferous)-1, g_prefix_ferous, "ferous" },
	{ sizeof(g_prefix_metric)-1, g_prefix_metric, "metric" },
	{ sizeof(g_prefix_nated)-1,  g_prefix_nated,  "nated" }, 
	{ sizeof(g_prefix_stic)-1,   g_prefix_stic,   "stic" },
	{ sizeof(g_prefix_opic)-1,   g_prefix_opic,   "opic" },
	{ sizeof(g_prefix_ected)-1,  g_prefix_ected,  "ected" }
};

static const TList g_postfix[] = {
	{ sizeof(g_prefix_meter)-1,  g_prefix_meter,  "meter" },
	{ sizeof(g_prefix_graph)-1,  g_prefix_graph,  "graph" },
	{ sizeof(g_prefix_scope)-1,  g_prefix_scope,  "scope" },
};

static int name_pick(int pos, int len, char* res, const TList *list, int count, uint32_t seed)
{
	char c;
	const char* p;
	const TList *words;

	/* pick a list from a hash of the random seed */
	c = (uint8_t)(
		(seed >> 0 ) ^
		(seed >> 8 ) ^
		(seed >> 16) ^
		(seed >> 24)
	);
	words = &list[c % count];

	/* pick a word */
	p = &words->list[seed % words->size];
	/* find beginning of word */
	while((c = *p)!=0)
		p--;
	p++;
	/* copy the whole word into the output buffer */
	while((pos<(len-1)) && ((c = *p++)!=0))
		res[pos++] = c;

	/* copy postfix into the output buffer */
	p = words->postfix;
	while((pos<(len-1)) && ((c = *p++)!=0))
		res[pos++] = c;

	/* add termination */
	res[pos] = 0;

	/* return string length */
	return  pos;
}

int name_prefix(int pos, int len, char* res, uint32_t seed)
{
	return name_pick(pos, len, res, g_prefix, ARRAY_COUNT(g_prefix), seed);
}

int name_postfix(int pos, int len, char* res, uint32_t seed)
{
	return name_pick(pos, len, res, g_postfix, ARRAY_COUNT(g_postfix), seed);
}

int name(int len, char* res, uint32_t seed)
{
	char buffer[80];
	int pos1, pos2, count;

	pos1 = name_prefix(0, sizeof(buffer), buffer, (seed >> 16));
	buffer[pos1++] = ' ';

	pos2 = name_postfix(pos1, sizeof(buffer), buffer, (uint16_t)seed);
	buffer[pos2] = 0;

	if(pos2<=len)
	{
		memcpy(res, buffer, pos2);
		count = pos2;
	}
	else
	{
		count = pos2-pos1;
		if(count>len)
			count = len;

		memcpy(res, &buffer[pos1], count);
	}

	return count;
}
