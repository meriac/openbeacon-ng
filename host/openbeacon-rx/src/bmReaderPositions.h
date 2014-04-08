/***************************************************************
 *
 * OpenBeacon.org - Reader Position Settings
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation; version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef BMREADERPOSITIONS_H_
#define BMREADERPOSITIONS_H_

typedef struct
{
	uint32_t id, room, floor, group;
	double x, y;
} TReaderItem;

#define IPv4(a,b,c,d) ( ((uint32_t)a)<<24 | ((uint32_t)b)<<16 | ((uint32_t)c)<<8 | ((uint32_t)d)<<0 )

static const TReaderItem g_ReaderList[] = {

	// Mikes Lab: group 8
	{1020, 1, 1, 8, 855, 522},
	{1021, 1, 1, 8, 610, 522},
	{1023, 1, 1, 8, 788, 766},


	// BruCON  2011: group 4
	// Lounge Level 1
	{0x65, 1, 1, 4, 151, 452},
	{0x66, 1, 1, 4, 839, 506},
	{0x6F, 1, 1, 4, 775, 608},
	{0x73, 1, 1, 4, 461, 236},
	{0xC4, 1, 1, 4, 681, 232},
	{0xC5, 1, 1, 4, 314, 522},
	{116, 1, 1, 4, 410, 657},
	{108, 1, 1, 4, 1006, 280},


	// HTW: group 1
	{IPv4 (10, 1, 254, 100), 707, 7, 1, 15.5, 5.5},
	{IPv4 (10, 1, 254, 103), 707, 7, 1, 0.5, 0.5},
	{IPv4 (10, 1, 254, 104), 707, 7, 1, 0.5, 5.5},
	{IPv4 (10, 1, 254, 105), 707, 7, 1, 8.0, 3.0},
	{IPv4 (10, 1, 254, 106), 707, 7, 1, 15.5, 0.5},


	// BCC: 24C3 Hackcenter - Level A
	{IPv4 (10, 254, 3, 3), 101, 1, 2, 414, 434},
	{IPv4 (10, 254, 3, 11), 101, 1, 2, 188, 539},
	{IPv4 (10, 254, 3, 10), 101, 1, 2, 387, 681},
	{IPv4 (10, 254, 4, 6), 101, 1, 2, 742, 500},

	// BCC: 24C3 Lounge - Level A
	{IPv4 (10, 254, 4, 13), 102, 1, 2, 497, 922},
	{IPv4 (10, 254, 4, 17), 102, 1, 2, 633, 832},

	// BCC: 24C3 Hardware Lab - Level A
	{IPv4 (10, 254, 2, 7), 103, 1, 2, 370, 814},

	// BCC: 24C3 Workshop - Level A
	{IPv4 (10, 254, 3, 21), 104, 1, 2, 212, 782},

	// BCC: 24C3 Angel Heaven - Level A
	{IPv4 (10, 254, 5, 2), 105, 1, 2, 763, 195},

	// BCC: 24C3 Foebud - Level B
	{IPv4 (10, 254, 5, 12), 201, 2, 2, 252, 387},

	// BCC: 24C3 Helpdesk - Level B
	{IPv4 (10, 254, 5, 17), 202, 2, 2, 252, 640},

	// BCC: 24C3 Entrance - Level B
	{IPv4 (10, 254, 6, 2), 203, 2, 2, 252, 798},

	// BCC: 24C3 Checkroom - Level B
	{IPv4 (10, 254, 1, 7), 204, 2, 2, 664, 896},

	// BCC: 24C3 Stairs Speakers - Level B
	{IPv4 (10, 254, 1, 1), 205, 2, 2, 734, 831},

	// BCC: 24C3 Stairs CERT - Level B
	{IPv4 (10, 254, 4, 21), 206, 2, 2, 734, 342},

	// BCC: 24C3 Saal 2 - Level B
	{IPv4 (10, 254, 3, 5), 207, 2, 2, 314, 78},
	{IPv4 (10, 254, 3, 15), 207, 2, 2, 92, 201},

	// BCC: 24C3 Saal 3 - Level B
	{IPv4 (10, 254, 2, 5), 208, 2, 2, 549, 78},
	{IPv4 (10, 254, 2, 23), 208, 2, 2, 800, 78},
	{IPv4 (10, 254, 2, 150), 208, 2, 2, 662, 214},

	// BCC: 24C3 Canteen - Level B
	{IPv4 (10, 254, 2, 12), 209, 2, 2, 412, 590},
	{IPv4 (10, 254, 2, 6), 209, 2, 2, 578, 590},
	{IPv4 (10, 254, 3, 1), 209, 2, 2, 573, 430},
	{IPv4 (10, 254, 2, 15), 209, 2, 2, 480, 734},

	// BCC: 24C3 Saal 1 - Level C
	{IPv4 (10, 254, 6, 16), 301, 3, 2, 528, 520},
	{IPv4 (10, 254, 6, 22), 301, 3, 2, 528, 650},
	{IPv4 (10, 254, 5, 11), 301, 3, 2, 722, 432},
	{IPv4 (10, 254, 6, 21), 301, 3, 2, 722, 742},
	{IPv4 (10, 254, 0, 100), 301, 3, 2, 722, 586},

	// BCC: 24C3 Debian - Level C
	{IPv4 (10, 254, 8, 1), 302, 3, 2, 426, 330},

	// BCC: 24C3 Stairs Press - Level C
	{IPv4 (10, 254, 8, 5), 303, 3, 2, 669, 310},

	// BCC: 24C3 Chaoswelle CAcert - Level C
	{IPv4 (10, 254, 8, 14), 304, 3, 2, 585, 249},

	// BCC: 24C3 Wikipedia - Level C
	{IPv4 (10, 254, 7, 11), 305, 3, 2, 432, 838},
	{IPv4 (10, 254, 9, 13), 305, 3, 2, 465, 910},

	// BCC: 24C3 POC Helpdesk VOIP - Level C
	{IPv4 (10, 254, 7, 17), 306, 3, 2, 292, 668},

	// BCC: 24C3 NOC Helpdesk - Level C
	{IPv4 (10, 254, 7, 19), 307, 3, 2, 286, 492},

	// BCC: 24C3 Stairs Hinterzimmer - Level C
	{IPv4 (10, 254, 9, 19), 308, 3, 2, 676, 858},

	// BCC: 25C3 Hackcenter - Level A
	{IPv4 (10, 254, 0, 111), 101, 1, 2, 433, 682},
	{IPv4 (10, 254, 0, 115), 101, 1, 2, 743, 512},
	{IPv4 (10, 254, 0, 116), 101, 1, 2, 253, 501},

	// BCC: 25C3 Lounge - Level A
	{IPv4 (10, 254, 0, 117), 102, 1, 2, 467, 924},

	// BCC: 25C3 Hardware Lab - Level A
	{IPv4 (10, 254, 0, 118), 103, 1, 2, 372, 783},

	// BCC: 25C3 Workshop - Level A
	{IPv4 (10, 254, 0, 119), 104, 1, 2, 263, 814},

	// BCC: 25C3 Foebud - Level B
	{IPv4 (10, 254, 0, 103), 201, 2, 2, 243, 386},
	{IPv4 (10, 254, 0, 113), 201, 2, 2, 418, 244},

	// BCC: 25C3 Helpdesk - Level B
	{IPv4 (10, 254, 0, 110), 202, 2, 2, 243, 641},

	// BCC: 25C3 Entrance - Level B
	{IPv4 (10, 254, 0, 102), 203, 2, 2, 380, 916},

	// BCC: 25C3 Checkroom - Level B
	{IPv4 (10, 254, 0, 112), 204, 2, 2, 602, 871},

	// BCC: 25C3 Stairs Speakers - Level B
	{IPv4 (10, 254, 0, 108), 205, 2, 2, 749, 816},

	// BCC: 25C3 Stairs CERT - Level B
	{IPv4 (10, 254, 0, 109), 206, 2, 2, 730, 344},

	// BCC: 25C3 Saal 2 - Level B
	{IPv4 (10, 254, 0, 135), 207, 2, 2, 538, 72},
	{IPv4 (10, 254, 0, 136), 207, 2, 2, 798, 72},
	{IPv4 (10, 254, 0, 137), 207, 2, 2, 701, 218},

	// BCC: 25C3 Saal 3 - Level B
	{IPv4 (10, 254, 0, 131), 208, 2, 2, 478, 210},
	{IPv4 (10, 254, 0, 133), 208, 2, 2, 256, 72},
	{IPv4 (10, 254, 0, 134), 208, 2, 2, 85, 200},

	// BCC: 25C3 Canteen - Level B
	{IPv4 (10, 254, 0, 105), 209, 2, 2, 105, 596},
	{IPv4 (10, 254, 0, 107), 209, 2, 2, 659, 589},
	{IPv4 (10, 254, 0, 114), 209, 2, 2, 563, 425},
	{IPv4 (10, 254, 0, 138), 209, 2, 2, 527, 682},

	// BCC: 25C3 Saal 1 - Level C
	{IPv4 (10, 254, 0, 101), 301, 3, 2, 491, 588},
	{IPv4 (10, 254, 0, 125), 301, 3, 2, 728, 745},
	{IPv4 (10, 254, 0, 126), 301, 3, 2, 728, 432},
	{IPv4 (10, 254, 0, 127), 301, 3, 2, 398, 721},
	{IPv4 (10, 254, 0, 128), 301, 3, 2, 402, 448},

	// BCC: 25C3 Debian - Level C
	{IPv4 (10, 254, 0, 123), 302, 3, 2, 417, 344},

	// BCC: 25C3 Stairs Press - Level C
	{IPv4 (10, 254, 0, 124), 303, 3, 2, 675, 318},

	// BCC: 25C3 Chaoswelle CAcert - Level C
	{IPv4 (10, 254, 0, 132), 304, 3, 2, 245, 295},

	// BCC: 25C3 Wikipedia - Level C
	{IPv4 (10, 254, 0, 122), 305, 3, 2, 408, 823},
	{IPv4 (10, 254, 0, 121), 305, 3, 2, 587, 916},

	// BCC: 25C3 NOC Helpdesk - Level C
	{IPv4 (10, 254, 0, 120), 307, 3, 2, 293, 529},

	// BCC: 25C3 Stairs Hinterzimmer - Level C
	{IPv4 (10, 254, 0, 129), 308, 3, 2, 743, 817},


	// FH Wildau: group 10
	{IPv4 (10, 110, 0, 214), 101, 1, 10, 685, 290},
	{IPv4 (10, 110, 0, 213), 101, 1, 10, 685, 150},
	{IPv4 (10, 110, 0, 212), 102, 1, 10, 225, 13},
	{IPv4 (10, 110, 0, 211), 102, 1, 10, 505, 13},
	{IPv4 (10, 110, 0, 210), 102, 1, 10, 475, 13},
	{IPv4 (10, 110, 0, 209), 102, 1, 10, 385, 13},
	{IPv4 (10, 110, 0, 208), 103, 1, 10, 230, 172},
	{IPv4 (10, 110, 0, 236), 103, 1, 10, 230, 390},
	{IPv4 (10, 110, 0, 241), 101, 1, 10, 620, 290},
	{IPv4 (10, 110, 0, 221), 201, 2, 10, 600, 410},
	{IPv4 (10, 110, 0, 220), 201, 2, 10, 365, 410},
	{IPv4 (10, 110, 0, 219), 203, 2, 10, 140, 405},
	{IPv4 (10, 110, 0, 218), 203, 2, 10, 140, 100},
	{IPv4 (10, 110, 0, 217), 202, 2, 10, 365, 150},
	{IPv4 (10, 110, 0, 216), 202, 2, 10, 600, 150},
	{IPv4 (10, 110, 0, 215), 204, 2, 10, 730, 485},
	{IPv4 (10, 110, 0, 230), 304, 3, 10, 620, 410},
	{IPv4 (10, 110, 0, 229), 304, 3, 10, 390, 410},
	{IPv4 (10, 110, 0, 228), 303, 3, 10, 111, 410},
	{IPv4 (10, 110, 0, 227), 303, 3, 10, 110, 410},
	{IPv4 (10, 110, 0, 226), 302, 3, 10, 390, 145},
	{IPv4 (10, 110, 0, 225), 302, 3, 10, 620, 145},
	{IPv4 (10, 110, 0, 223), 301, 3, 10, 750, 233},
	{IPv4 (10, 110, 0, 222), 303, 3, 10, 20, 230},
};

#define READER_COUNT (sizeof(g_ReaderList)/sizeof(g_ReaderList[0]))

#endif
