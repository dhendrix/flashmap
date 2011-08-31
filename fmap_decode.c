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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "lib/fmap.h"

int main(int argc, char *argv[])
{
	int fd;
	int rc = EXIT_SUCCESS;
	struct stat s;
	char *filename;
	uint8_t *blob;
	off_t fmap_offset;

	if (argc != 2) {
		printf("usage: %s <filename>\n", argv[0]);
		rc = EXIT_FAILURE;
		goto do_exit_1;
	}
	filename = argv[1];

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("unable to open file \"%s\": %s\n",
		       filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_2;
	}
	if (fstat(fd, &s) < 0) {
		printf("unable to stat file \"%s\": %s\n",
		       filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_2;
	}

	blob = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (blob == MAP_FAILED) {
		printf("unable to mmap file %s: %s\n",
		       filename, strerror(errno));
		rc = EXIT_FAILURE;
		goto do_exit_2;
	}

	fmap_offset = fmap_find(blob, s.st_size);
	if (fmap_offset < 0) {
		rc = EXIT_FAILURE;
		goto do_exit_3;
	} else {
		fmap_print((struct fmap *)(blob + fmap_offset));
	}

do_exit_3:
	munmap(blob, s.st_size);
do_exit_2:
	close(fd);
do_exit_1:
	return rc;
}
