/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker filter
 *
 * accepts stdout from position tracker on stdin and constantly
 * updates two JSON files reflecting the current state of the
 * tracker. One of the files ig gzip compressed.
 *
 * The basic idea is to point this filter to a RAM backed tmpfs
 * mount which is served by an Apache webserver et al.
 *
 * Copyright 2009-2011 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

#define LOG "FILTER_SINGULARSIGHTING "

static FILE *g_ftextlog;
static gzFile g_fgzlog;

static char *g_file_gztarget_tmp, *g_file_gztarget;
static char *g_file_target_tmp, *g_file_target;
static const char g_gz_suffix[] = ".gz";
static const char g_tmp_suffix[] = ".tmp";

#define GZ_SUFFIX_SIZE (sizeof(g_gz_suffix)-1)
#define TMP_SUFFIX_SIZE (sizeof(g_tmp_suffix)-1)

static void
start_new_fileset (void)
{
	if (g_fgzlog)
	{
		gzclose (g_fgzlog);
		if (rename (g_file_gztarget_tmp, g_file_gztarget))
			fprintf (stderr, LOG "failed to rename file '%s'->'%s'\n",
					 g_file_gztarget_tmp, g_file_gztarget);
	}
	if ((g_fgzlog = gzopen (g_file_gztarget_tmp, "w9")) == NULL)
		fprintf (stderr, LOG "failed to open temporary gzip file '%s'\n",
				 g_file_gztarget_tmp);

	if (g_ftextlog)
	{
		fclose (g_ftextlog);
		if (rename (g_file_target_tmp, g_file_target))
			fprintf (stderr, LOG "failed to rename file '%s'->'%s'\n",
					 g_file_target_tmp, g_file_target);
	}
	if ((g_ftextlog = fopen (g_file_target_tmp, "w")) == NULL)
		fprintf (stderr, LOG "failed to open temporary target file '%s'\n",
				 g_file_target_tmp);
}

int
main (int argc, char *argv[])
{
	int data, len, size, res;
	uint8_t c[3];

	g_ftextlog = NULL;
	g_fgzlog = NULL;

	if (argc != 2)
	{
		fprintf (stderr, LOG " usage: %s some_path/somefile_prefix\n",
				 argv[0]);
		return -1;
	}

	/* use first parameter as target prefix */
	g_file_target = argv[1];

	/* add one byte for zero termination */
	len = strlen (g_file_target) + 1;

	size = len + TMP_SUFFIX_SIZE;
	g_file_target_tmp = (char *) malloc (size);
	if (g_file_target_tmp)
		snprintf (g_file_target_tmp, size, "%s%s", g_file_target,
				  g_tmp_suffix);

	size = len + GZ_SUFFIX_SIZE;
	g_file_gztarget = (char *) malloc (size);
	if (g_file_gztarget)
		snprintf (g_file_gztarget, size, "%s%s", g_file_target, g_gz_suffix);

	size = len + TMP_SUFFIX_SIZE + GZ_SUFFIX_SIZE;
	g_file_gztarget_tmp = (char *) malloc (size);
	if (g_file_gztarget_tmp)
		snprintf (g_file_gztarget_tmp, size, "%s%s%s", g_file_target,
				  g_tmp_suffix, g_gz_suffix);

	if (!(g_file_target_tmp && g_file_gztarget && g_file_gztarget_tmp))
	{
		fprintf (stderr, LOG " out of memory error\n");
		res = -2;
	}
	else
	{
		c[1] = c[2] = 0;
		start_new_fileset ();
		while ((data = getchar ()) != EOF)
		{
			/* convert to 8 bits */
			c[0] = (uint8_t) data;

			/* echo everything to maintain pipe chain */
			putchar (c[0]);

			/* find start of new object */
			if (c[2] == '\n' && c[1] == '}' && c[0] == ',')
				start_new_fileset ();
			else
			{
				/* echo to zip file */
				if (g_fgzlog)
					gzwrite (g_fgzlog, c, 1);
				/* echo to text file */
				if (g_ftextlog)
					fwrite (c, 1, 1, g_ftextlog);
			}

			/* remember last two characters */
			c[2] = c[1];
			c[1] = c[0];
		}
		/* close files */
		gzclose (g_fgzlog);
		fclose (g_ftextlog);
		/* return OK */
		res = 0;
	}

	/* free memory at the end */
	free (g_file_target_tmp);
	free (g_file_gztarget);
	free (g_file_gztarget_tmp);

	return res;
}
