/* Copyright 2010, Google Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include "lib/fmap.h"

static struct option const long_options[] =
{
  {"digest", required_argument, NULL, 'd'},
  {"list", no_argument, NULL, 'l'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

void print_csum(uint8_t *digest, size_t len)
{
	char *str;
	char tmp[3];
	unsigned int i;
	int strlength = len * 2 + 1;

	str = malloc(strlength);
	memset(str, '\0', strlength);
	for (i = 0; i < len; i++) {
		snprintf(tmp, 3, "%02x", digest[i]);
		strncat(str, tmp, 3);
	}

	printf("%s\n", str);
	free(str);
}

void print_help()
{
	printf("Usage: fmap_csum [OPTION]... [FILE]\n"
	        "Print sha1sum of static regions of FMAP-compliant binary\n"
	        "Arguments:\n"
	        "\t-h, --help\t\tprint this help menu\n"
	        "\t-v, --version\t\tdisplay version\n");
}

int main(int argc, char *argv[])
{
	int fd, len, rc = EXIT_SUCCESS;
	struct stat s;
	char *filename = NULL;
	uint8_t *image;
	int argflag;
	uint8_t *digest = NULL;

	while ((argflag = getopt_long(argc, argv, "d:hlv",
	                      long_options, NULL)) > 0) {
		switch (argflag) {
		case 'v':
			printf("fmap suite version: %d.%d\n",
			       VERSION_MAJOR, VERSION_MINOR);;
			goto do_exit_1;
		case 'h':
			print_help();
			goto do_exit_1;
		default:
			print_help();
			rc = EXIT_FAILURE;
			goto do_exit_1;
		}
	}

	/* quit if filename not specified */
	if (argv[optind]) {
		filename = argv[optind];
	} else {
		print_help();
		rc = EXIT_FAILURE;
		goto do_exit_1;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "unable to open file \"%s\": %s\n",
		                filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_1;
	}
	if (fstat(fd, &s) < 0) {
		fprintf(stderr, "unable to stat file \"%s\": %s\n",
		                filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_2;
	}

	image = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (image == MAP_FAILED) {
		fprintf(stderr, "unable to map file \"%s\": %s\n",
		                filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_2;
	}

	if ((len = fmap_get_csum(image, s.st_size, &digest)) < 0) {
		fprintf(stderr, "unable to obtain checksum\n");
		rc = EXIT_FAILURE;
		goto do_exit_3;
	}

	print_csum(digest, len);

do_exit_3:
	munmap(image, s.st_size);
	free(digest);
do_exit_2:
	close(fd);
do_exit_1:
	exit(rc);
}
