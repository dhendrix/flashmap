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

#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/fmap.h"
#include "lib/input.h"

static void print_usage()
{
	printf("Usage: fmap_encode [option] [arguments]\n"
	       "Options:\n"
	       "\t-h | --help                        print this help text\n"
	       "\t-i | --interactive <output>        interactive setup\n"
	       "\t-k | --kv <input> <output>         generate binary from kv-pairs\n"
//	       "\t-x | --xml <input> <output>        generate binary from xml\n"
	       "\n"
	);
}

int main(int argc, char *argv[])
{
	int rc = EXIT_SUCCESS;
	int opt, option_index = 0;
	static const char optstring[] = "hi:k:";
	static const struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"interactive", required_argument, NULL, 'i'},
		{"kv", required_argument, NULL, 'k'},
		{NULL, 0, NULL, 0}
	};
	char *input = NULL, *output = NULL;

	opt = getopt_long(argc, argv, optstring, long_options, &option_index);
	switch(opt) {
	case 'h':
		print_usage();
		break;
	case 'i':
		output = strdup(optarg);
		rc = input_interactive(output);
		break;
	case 'k':
		if (!optarg || !argv[optind]) {
			printf("Error: missing argument\n");
			print_usage();
			rc = EXIT_FAILURE;
			break;
		}
		input = strdup(optarg);
		output = strdup(argv[optind]);
		rc = input_kv_pair(input, output);
		if (optind != (argc - 1))
			optind++;
		break;
	default:
		print_usage();	/* FIXME: implement this */
		break;
	}

	if (input)
		free(input);
	if (output)
		free(output);
	return rc;
}
