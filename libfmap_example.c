/*
 * Copyright 2011, Google Inc.
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
 *
 * libfmap_example: Simple example for creating a flashmap.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fmap.h>

/* stuff that should be passed in by higher-level logic, ie firmware build
 * system, parsed from a config file, etc. */
#define FMAP_BASE	0xfc000000
#define FMAP_NAME	"x86_BIOS"
#define FMAP_SIZE	(4096 * 1024)

int main(int argc, char *argv[])
{
	int fd, rc = EXIT_FAILURE;
	struct fmap *fmap = NULL;
	size_t total_size;
	const char *filename;

	if (argc != 2) {
		printf("%s: Create fmap binary using example code\n", argv[0]);
		printf("Usage: %s <filename>\n", argv[0]);
		goto exit_1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "unable to open file \"%s\": %s\n",
				filename, strerror(errno));
		goto exit_1;
	}

	fmap = fmap_create(FMAP_BASE, FMAP_SIZE, (uint8_t *)FMAP_NAME);
	if (!fmap) {
		fprintf(stderr, "unable to initialize fmap\n");
		goto exit_2;
	}

	if (fmap_append_area(&fmap, FMAP_BASE, 0x100,
	                     (const uint8_t *)"area_1",
	                     FMAP_AREA_STATIC) < 0)
		goto exit_3;
	if (fmap_append_area(&fmap, FMAP_BASE + 0x100, 0x100,
	                     (const uint8_t *)"area_2",
	                     FMAP_AREA_COMPRESSED) < 0)
		goto exit_3;
	if (fmap_append_area(&fmap, FMAP_BASE + 0x200, 0x100,
	                     (const uint8_t *)"area_3",
	                     FMAP_AREA_RO) < 0)
		goto exit_3;
	if (fmap_append_area(&fmap, FMAP_BASE + 0x300, 0x100,
	                     (const uint8_t *)"area_4",
	                     FMAP_AREA_COMPRESSED | FMAP_AREA_RO) <0)
		goto exit_3;

	fmap_print(fmap);
	total_size = fmap_size(fmap);
	if (write(fd, fmap, total_size) != total_size) {
		fprintf(stderr, "failed to write \"%s\": %s\n",
				filename, strerror(errno));
		goto exit_3;
	}

	rc = EXIT_SUCCESS;
exit_3:
	fmap_destroy(fmap);
exit_2:
	close(fd);
exit_1:
	return rc;
}
