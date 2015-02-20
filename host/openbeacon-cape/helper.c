#include <stdio.h>
#include <helper.h>

void
hex_dump (const unsigned char *buf, unsigned int len)
{
	unsigned int start, i, j;
	char c;

	start = 0;

	for (j = 0; j < len; j += 16)
	{
		printf ("%08x:", start + j);

		for (i = 0; i < 16; i++)
		{
			if ((start + i + j) < len)
				printf (" %02x", buf[start + i + j]);
			else
				printf ("   ");
		}
		printf ("  |");
		for (i = 0; i < 16; i++)
		{
			if ((start + i + j) < len)
			{
				c = buf[start + i + j];
				if (c >= ' ' && c < 127)
					printf ("%c", c);
				else
					printf (".");
			}
			else
				printf (" ");
		}
		printf ("|\n\r");
	}
}
