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
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fmap.h>

#include "input.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * extract_value - Extract value from key="value" string
 *
 * line: string to extract value from
 *
 * returns allocated, NULL-terminated copy of value
 * returns NULL to indicate failure
 */
static char *extract_value(const char *line, size_t len) {
	char *val, *c;
	int i = 0;

	if (!line)
		return NULL;

	c = strchr(line, '"');
	c++;

	val = calloc(len, 1);
	while (*c != '"') {
		if (c == line + len) {
			fprintf(stderr,
			        "overrun detected: missing end-quote?\n");
			free(val);
			val = NULL;
			break;
		}
		val[i] = *c;
		i++;
		c++;
	}

	return val;
}

static int do_strcpy(void *dest, const char *src, size_t max_len)
{
	if (!dest || !src || (strlen(src) > max_len))
		return -1;

	strncpy(dest, src, max_len);
	return 0;
}

static int do_memcpy(void *dest, const char *src, size_t len)
{
	if (!dest || !src)
		return -1;

	memcpy(dest, src, len);
	return 0;
}

static int do_signature(void *dest, const char *src, size_t len)
{
	if (!dest || !src)
		return -1;

	memcpy(dest, FMAP_SIGNATURE, len);
	return 0;
}

/*
 * do_flags - translate strings into flag bitmap
 *
 * @dest:	destination memory address
 * @src:	null-terminated string containing ASCII representation of flags
 * @len:	size of destination storage location
 *
 * returns 0 to indicate success
 * returns <0 to indicate failure
 */
static int do_flags(void *dest, const char *src, size_t len)
{
	unsigned int i;
	uint16_t flags = 0;	/* TODO: can remove uint16_t assumption? */

	if (!dest || !src)
		return -1;

	if (strlen(src) == 0) {
		memset(dest, 0, len);
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(flag_lut) && flag_lut[i].str; i++) {
		if (strstr(src, flag_lut[i].str))
			flags |= flag_lut[i].val;
	}

	memcpy(dest, &flags, len);
	return 0;
}

/*
 * do_strtoul - do ASCII to (unsigned) integer conversion
 *
 * @dest:	destination memory address
 * @src:	null-terminated string containing ASCII representation of number
 * @len:	size of destination storage location
 *
 * returns 0 to indicate success
 * returns <0 to indicate failure
 */
static int do_strtoul(void *dest, const char *src, size_t len)
{
	void *x;

	if (!dest || !src)
		return -1;

	switch(len) {
	case 1:
		x = malloc(len);
		*(uint8_t *)x = (uint8_t)strtoul(src, NULL, 0);
		break;
	case 2:
		x = malloc(len);
		*(uint16_t *)x = (uint16_t)strtoul(src, NULL, 0);
		break;
	case 4:
		x = malloc(len);
		*(uint32_t *)x = (uint32_t)strtoull(src, NULL, 0);
		break;
	case 8:
		x = malloc(len);
		*(uint64_t *)x = (uint64_t)strtoull(src, NULL, 0);
		break;
	default:
		return -1;
	}

	memcpy(dest, x, len);
	free(x);
	return 0;
}

static int doit(const char *line,
                struct kv_attr kv_attrs[], unsigned int num_entries)
{
	int rc = 0;
	unsigned int i;

	for (i = 0; i < num_entries; i++) {
		const char *p = line;
		char *value = NULL;

		/* Ignore out partial key matches, e.g. foo= vs. foo_bar= */
		while ((p = strstr(p, kv_attrs[i].key)) != NULL) {
			if (*(p + strlen(kv_attrs[i].key)) == '=') {
				break;
			}
			p++;
		}

		if (p == NULL) {
			fprintf(stderr, "key \"%s\" not found, aborting\n",
			        kv_attrs[i].key);
			rc = -1;
			break;
		}

		/* found key, extract value from remainder of input string */
		if (!(value = extract_value(p, strlen(p)))) {
			fprintf(stderr,
			        "failed to extract value from \"%s\"\n", p);
			rc = -1;
			free(value);
			break;
		}

		if (kv_attrs[i].handler(kv_attrs[i].dest, value, kv_attrs[i].len)) {
			fprintf(stderr, "failed to process \"%s\"\n", value);
			rc = -1;
			free(value);
			break;
		}

		free(value);
	}

	return rc;
}

static int parse_header(const char *line, struct fmap *fmap) {
	struct kv_attr kv_attrs[] = {
		{ .key = "fmap_signature",
		  .dest = &fmap->signature,
		  .len = strlen(FMAP_SIGNATURE),
		  .handler = do_signature,
		},
		{ .key = "fmap_ver_major",
		  .dest = &fmap->ver_major,
		  .len = sizeof(fmap->ver_major),
		  .handler = do_strtoul,
		},
		{ .key = "fmap_ver_minor",
		  .dest = &fmap->ver_minor,
		  .len = sizeof(fmap->ver_minor),
		  .handler = do_strtoul,
		},
		{ .key = "fmap_base",
		  .dest = &fmap->base,
		  .len = sizeof(fmap->base),
		  .handler = do_strtoul,
		},
		{ .key = "fmap_size",
		  .dest = &fmap->size,
		  .len = sizeof(fmap->size),
		  .handler = do_strtoul,
		},
		{ .key = "fmap_name",
		  .dest = &fmap->name,
		  .len = FMAP_STRLEN,	/* treated as a maximum length */
		  .handler = do_strcpy,
		},
		{ .key = "fmap_nareas",
		  .dest = &fmap->nareas,
		  .len = sizeof(fmap->nareas),
		  .handler = do_strtoul,
		},
	};

	return doit(line, kv_attrs, ARRAY_SIZE(kv_attrs));
}

static int parse_area(const char *line, struct fmap_area *area) {
	struct kv_attr kv_attrs[] = {
		{ .key = "area_offset",
		  .dest = &area->offset,
		  .len = sizeof(area->offset),
		  .handler = do_strtoul,
		},
		{ .key = "area_size",
		  .dest = &area->size,
		  .len = sizeof(area->size),
		  .handler = do_strtoul,
		},
		{ .key = "area_name",
		  .dest = &area->name,
		  .len = FMAP_STRLEN,	/* treated as a maximum length */
		  .handler = do_strcpy,
		},
		{ .key = "area_flags",
		  .dest = &area->flags,
		  .len = sizeof(area->flags),
		  .handler = do_flags,
		},
	};

	return doit(line, kv_attrs, ARRAY_SIZE(kv_attrs));
}

int input_kv_pair(const char *infile, const char *outfile)
{
	char line[LINE_MAX];
	FILE *fp_in, *fp_out;
	struct fmap *fmap;
	int area;
	int rc = EXIT_SUCCESS;
	size_t total_size = 0;

	fp_in = fopen(infile, "r");
	if (!fp_in) {
		fprintf(stderr, "cannot open file \"%s\" (%s)\n",
		        infile, strerror(errno));
		rc = EXIT_FAILURE;
		goto input_kv_pair_exit_1;
	}

	fp_out = fopen(outfile, "w");
	if (!fp_out) {
		fprintf(stderr, "cannot open file \"%s\" (%s)\n",
		        infile, strerror(errno));
		rc = EXIT_FAILURE;
		goto input_kv_pair_exit_1;
	}

	fmap = malloc(sizeof(*fmap));
	if (!fmap)
		goto input_kv_pair_exit_2;

	/* assume first line contains fmap header */
	memset(line, 0, sizeof(line));
	if (!fgets(line, sizeof(line), fp_in) || parse_header(line, fmap)) {
		fprintf(stderr, "failed to parse header\n");
		rc = EXIT_FAILURE;
		goto input_kv_pair_exit_2;
	}

	total_size = sizeof(*fmap) + (sizeof(fmap->areas[0]) * fmap->nareas);
	fmap = realloc(fmap, total_size);
	memset(line, 0, sizeof(line));
	area = 0;
	while (fgets(line, sizeof(line), fp_in)) {
		if (parse_area(line, &fmap->areas[area])) {
			rc = EXIT_FAILURE;
			goto input_kv_pair_exit_2;
		}
		memset(line, 0, sizeof(line));
		area++;
	}

	if (fwrite(fmap, 1, total_size, fp_out) != total_size) {
		fprintf(stderr, "failed to write fmap binary\n");
		rc = EXIT_FAILURE;
	}

input_kv_pair_exit_2:
	free(fmap);
	fclose(fp_in);
	fclose(fp_out);
input_kv_pair_exit_1:
	return rc;
}

/*
 * LCOV_EXCL_START
 * Unit testing stuff done here so we do not need to expose static functions.
 */

static int extract_value_test() {
	int rc = 0, len = 512;
	char kv[len];
	char *ret;

	if (extract_value(NULL, 0)) {
		printf("FAILURE: extract_value_test NULL case not "
		       "handled properly\n");
		rc |= 1;
	}

	/* normal case */
	sprintf(kv, "foo=\"bar\"");
	ret = extract_value(kv, len);
	if (strcmp(ret, "bar")) {
		printf("FAILURE: \"%s\" != \"%s\"\n", ret, "bar");
		rc |= 1;
	}
	free(ret);

	/* missing end quote */
	memset(kv, 0, sizeof(kv));
	sprintf(kv, "foo=\"bar");
	ret = extract_value(kv, len);
	if (ret) {
		printf("FAILURE: missing end-quote not caught\n");
		rc |= 1;
	}

	return rc;
}

static int trivial_helper_function_tests() {
	int rc = 0, len = 512;
	int tmp;
	char dest[len], src[len];

	/*
	 * do_strcpy
	 */
	sprintf(src, "hello world");
	do_strcpy(dest, src, len);
	if (strncmp(dest, src, strlen(src))) {
		printf("FAILURE: do_strcpy failed\n");
		rc |= 1;
	}

	if ((do_strcpy(NULL, src, strlen(src)) == 0) ||
	    (do_strcpy(dest, NULL, strlen(src)) == 0)) {
		printf("FAILURE: do_strcpy NULL case not handled properly\n");
		rc |= 1;
	}

	/* length of src exceeds max length */
	tmp = do_strcpy(dest, src, strlen(src) - 1);
	if (!tmp) {
		printf("FAILURE: do_strcpy failed to catch overrun\n");
		rc |= 1;
	}

	/*
	 * do_memcpy
	 */
	if ((do_memcpy(NULL, src, strlen(src)) == 0) ||
	    (do_memcpy(dest, NULL, strlen(src)) == 0)) {
		printf("FAILURE: do_memcpy NULL case not handled properly\n");
		rc |= 1;
	}

	sprintf(src, "hello world");
	do_memcpy(dest, src, len);
	if (memcmp(dest, src, strlen(src))) {
		printf("FAILURE: do_memcpy failed\n");
		rc |= 1;
	}

	/*
	 * do_signature
	 */
	if ((do_signature(NULL, src, strlen(src)) == 0) ||
	    (do_signature(dest, NULL, strlen(src)) == 0)) {
		printf("FAILURE: do_signature NULL case not handled properly\n");
		rc |= 1;
	}

	sprintf(src, "hello world");
	do_signature(dest, src, strlen(FMAP_SIGNATURE));
	if (strncmp(dest, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE))) {
		printf("FAILURE: do_signature failed\n");
		rc |= 1;
	}

	return rc;
}

static int do_strtoul_test() {
	int rc = 0;
	uint8_t dest_8;
	uint16_t dest_16;
	uint32_t dest_32;
	uint64_t dest_64;
	char src[32];

	sprintf(src, "0xff");
	do_strtoul(&dest_8, src, 1);
	if (dest_8 != 0xff) {
		printf("FAILURE: do_strtoul failed to convert a uint8_t\n");
		rc |= 1;
	}

	sprintf(src, "0xffff");
	do_strtoul(&dest_16, src, 2);
	if (dest_16 != 0xffff) {
		printf("FAILURE: do_strtoul failed to convert a uint16_t\n");
		rc |= 1;
	}

	sprintf(src, "0xffffffff");
	do_strtoul(&dest_32, src, 4);
	if (dest_32 != 0xffffffff) {
		printf("FAILURE: do_strtoul failed to convert a uint32_t\n");
		rc |= 1;
	}

	sprintf(src, "0xffffffffffffffff");
	do_strtoul(&dest_64, src, 8);
	if (dest_64 != 0xffffffffffffffffULL) {
		printf("FAILURE: do_strtoul failed to convert a uint64_t\n");
		rc |= 1;
	}

	/* invalid length */
	if (!do_strtoul(&dest_8, src, 3)) {
		printf("FAILURE: do_strtoul returned 0 on bad length\n");
		rc |= 1;
	}

	sprintf(src, "0xff");
	if ((do_strtoul(NULL, src, 1) == 0) ||
	    (do_strtoul(&dest_8, NULL, strlen(src)) == 0)) {
		printf("FAILURE: do_strtoul NULL case not handled properly\n");
		rc |= 1;
	}

	return rc;
}

static int do_flags_test() {
	int rc = 0;
	unsigned int i;
	uint16_t dest, tmp;
	char src[512];

	if ((do_flags(NULL, src, strlen(src)) == 0) ||
	    (do_flags(&dest, NULL, strlen(src)) == 0)) {
		printf("FAILURE: do_flags NULL case not handled properly\n");
		rc |= 1;
	}

	/* convert each flag individually */
	for (i = 0; i < ARRAY_SIZE(flag_lut) && flag_lut[i].str; i++) {
		do_flags(&dest, flag_lut[i].str, strlen(flag_lut[i].str));
		if (dest != str2val(flag_lut[i].str, flag_lut)) {
			printf("FAILURE: do_flags failed to convert %s\n",
			       flag_lut[i].str);
			rc |= 1;
		};
	}

	/* place all flags in a single string */
	memset(src, 0, sizeof(src));
	tmp = 0;
	for (i = 0; i < ARRAY_SIZE(flag_lut) && flag_lut[i].str; i++) {
		/* prepend a comma before adding this entry */
		if (i != 0 && flag_lut[i - 1].str)
			strcat(src, ",");

		strcat(src, flag_lut[i].str);
		tmp |= flag_lut[i].val;

	}

	do_flags(&dest, src, strlen(src));
	if (dest != tmp) {
		printf("FAILURE: do_flags %04x != %04x, src: %s\n",
		       dest, tmp, src);
		rc |= 1;
	}

	if (do_flags(&dest, "", sizeof(dest))) {
		printf("FAILURE: do_flags failed to catch zero-length src\n");
		rc |= 1;
	}

	return rc;
}

static int dummy_handler(void *dest, const char *src, size_t max_len)
{
	return -1;
}

static int doit_test() {
	int rc = 0;
	char line[LINE_MAX], dest[LINE_MAX];
	struct fmap fmap;
	struct fmap_area fmap_area;
	struct kv_attr attr[] = {
		{ .key = "foo",
		  .dest = &dest,
		  .len = LINE_MAX,
		  .handler = do_strcpy,
		},
	};

	/* parse a valid header line */
	sprintf(line, "fmap_signature=\"0x5f5f50414d465f5f\" "
	              "fmap_ver_major=\"1\" fmap_ver_minor=\"0\" "
		      "fmap_base=\"0x00000000ffe00000\" fmap_size=\"0x200000\" "
		      "fmap_name=\"System BIOS\" fmap_nareas=\"4\"");
	rc |= parse_header(line, &fmap);

	/* parse a valid fmap area line */
	sprintf(line, "area_offset=\"0x00040000\" area_size=\"0x00180000\" "
	              "area_name=\"FV_MAIN\" area_flags=\"static,compressed\"");
	rc |= parse_area(line, &fmap_area);

	/* partially matched key should be ignored */
	sprintf(line, "foo_bar=\"foobar\" foo=\"bar\"");
	if (doit(line, attr, 1) || strcmp(attr[0].dest, "bar")) {
		printf("FAILURE: doit failed to ignore partial key match "
		       "(expected \"bar\", got \"%s\")\n",(char *)attr[0].dest);
		rc |= 1;
	}

	/* nonexitent key */
	sprintf(line, "nonexistent=\"value\"");
	if (doit(line, attr, 1) == 0) {
		printf("FAILURE: doit returned false positive\n");
		rc |= 1;
	}

	/* bad value (missing end quote) */
	sprintf(line, "foo=\"bar");
	if (doit(line, attr, 1) == 0) {
		printf("FAILURE: doit did not catch bad key\n");
		rc |= 1;
	}

	/* handler failure */
	sprintf(line, "foo=\"bar\"");
	attr[0].handler = dummy_handler;
	if (doit(line, attr, 1) == 0) {
		printf("FAILURE: handler should have failed\n");
		rc |= 1;
	}

	return rc;
}

int input_kv_pair_test()
{
	int rc = EXIT_SUCCESS;

	rc |= extract_value_test();
	rc |= trivial_helper_function_tests();
	rc |= do_strtoul_test();
	rc |= do_flags_test();
	rc |= doit_test();

	if (rc)
		printf("FAILED\n");
	return rc;
}
/* LCOV_EXCL_STOP */
