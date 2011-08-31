/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fmap.h>

#include "input.h"

/*
 * do_prompt - Prompt a user for input, perform rudimentary sanity checking,
 * convert to specified data type, and store up to "len" bytes in result.
 * Conversions to unsigned types are done using strtoull() or similar.
 *
 * returns 0 to indicate success, <0 to indicate failure
 */
static int do_prompt(const char *prompt, enum input_types type,
              void *result, size_t len)
{
	char buf[MAXLEN];
	char *endptr;

	printf("%s: ", prompt);
	if (fgets(buf, sizeof(buf), stdin) == NULL) {
		fprintf(stderr, "unable to obtain input\n");
		return -1;
	}

	switch(type) {
	case TYPE_ULL: {
		unsigned long long int val;

		val = strtoull(buf, &endptr, 0);
		if ((buf == endptr) ||
		    ((unsigned int)(endptr - buf + 1) != strlen(buf))) {
			fprintf(stderr, "invalid input detected\n");
			return -1;
		}

		/* TODO: Add "out of range" warning? Current behavior
		 * is to silently truncate. */

		memcpy(result, &val, len);
		break;
	}
	case TYPE_STRING:
		if (strlen(buf) > len) {
			fprintf(stderr, "\"%s\" is too long\n", buf);
			return -1;
		}

		/* replace any trailing whitespace character with terminator */
		buf[strlen(buf) - 1] = '\0';
		memcpy(result, buf, strlen(buf));
		break;
	default:
		return -1;
	}

	return 0;
}

int input_interactive(const char *filename)
{
	int fd, rc = EXIT_SUCCESS;
	struct fmap *fmap = NULL;
	int total_size, i;

	fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "unable to open file \"%s\": %s\n",
				filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto exit_interactive_1;
	}

	if ((fmap = malloc(sizeof(*fmap))) == NULL) {
		rc = EXIT_FAILURE;
		goto exit_interactive_2;
	}
	memset(fmap, 0, sizeof(*fmap));

	memcpy(&(fmap->signature), FMAP_SIGNATURE, strlen(FMAP_SIGNATURE));
	fmap->ver_major	= FMAP_VER_MAJOR;
	fmap->ver_minor	= FMAP_VER_MINOR;

	if (do_prompt("binary base address", TYPE_ULL,
	              &(fmap->base), sizeof(fmap->base)) < 0) {
		rc = EXIT_FAILURE;
		goto exit_interactive_2;
	}

	if (do_prompt("binary size", TYPE_ULL,
	              &(fmap->size), sizeof(fmap->size)) < 0) {
		rc = EXIT_FAILURE;
		goto exit_interactive_2;
	}

	if (do_prompt("name of firmware image", TYPE_STRING,
	              &(fmap->name), FMAP_STRLEN) < 0) {
		rc = EXIT_FAILURE;
		goto exit_interactive_2;
	}

	if (do_prompt("number of areas", TYPE_ULL,
	              &(fmap->nareas), sizeof(fmap->nareas)) < 0) {
		rc = EXIT_FAILURE;
		goto exit_interactive_2;
	}

	/* Add space for the FMAP areas */
	total_size = sizeof(*fmap) + ((sizeof(fmap->areas[0])) * fmap->nareas);
	fmap = realloc(fmap, total_size);

	for (i = 0; i < fmap->nareas; i++) {
		char prompt[32];

		snprintf(prompt, sizeof(prompt), "area %d offset", i + 1);
		if (do_prompt(prompt, TYPE_ULL,
		              &(fmap->areas[i].offset),
			      sizeof(fmap->areas[i].offset)) < 0) {
			rc = EXIT_FAILURE;
			goto exit_interactive_3;
		}

		snprintf(prompt, sizeof(prompt), "area %d size", i + 1);
		if (do_prompt(prompt, TYPE_ULL,
		              &(fmap->areas[i].size),
			      sizeof(fmap->areas[i].size)) < 0) {
			rc = EXIT_FAILURE;
			goto exit_interactive_3;
		}

		snprintf(prompt, sizeof(prompt), "area %d name", i + 1);
		if (do_prompt(prompt, TYPE_STRING,
		              &(fmap->areas[i].name),
			      sizeof(fmap->areas[i].name)) < 0) {
			rc = EXIT_FAILURE;
			goto exit_interactive_3;
		}

		snprintf(prompt, sizeof(prompt), "area %d flags", i + 1);
		if (do_prompt(prompt, TYPE_ULL,
		              &(fmap->areas[i].flags),
			      sizeof(fmap->areas[i].flags)) < 0) {
			rc = EXIT_FAILURE;
			goto exit_interactive_3;
		}
	}

	fmap_print(fmap);
	if (write(fd, fmap, total_size) != total_size) {
		fprintf(stderr, "failed to write \"%s\": %s\n",
				filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto exit_interactive_3;
	}

exit_interactive_3:
	free(fmap);
exit_interactive_2:
	close(fd);
exit_interactive_1:
	return rc;
}
