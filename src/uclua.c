/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Kyle Evans <kevans@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>

#include <uclua.h>

enum {
	JSON_OPT = CHAR_MAX + 1,
	UCL_OPT,
	YAML_OPT,
};

const char *optstr = "o:";

struct option longopts[] = {
	{ "json",	no_argument,	NULL,	JSON_OPT },
	{ "ucl",	no_argument,	NULL,	UCL_OPT },
	{ "yaml",	no_argument,	NULL,	YAML_OPT },
	{ "output",	required_argument,	NULL,	'o' },
};

static int
usage(void)
{

	fprintf(stderr, "Usage: %s [--json | --ucl | --yaml] [-o output] [file...] \n", getprogname());
	return (1);
}

static int
parse_one(lcookie_t *lcook, const char *name)
{
	FILE *cfg;
	int ret;

	if (strcmp(name, "-") == 0)
		cfg = stdin;
	else
		cfg = fopen(name, "r");
	if (cfg == NULL) {
		printf("Failed to open file '%s'\n", name);
		return (1);
	}

	ret = 0;
	if (!uclua_parse_file(lcook, cfg)) {
		ret = 1;
		fprintf(stderr, "Failed to parse from %s\n", cfg == stdin ? "stdin" : name);
	}

	if (cfg != stdin)
		fclose(cfg);
	return (ret);
}

int
main(int argc, char *argv[])
{
	lcookie_t *lcook;
	FILE *cfg, *outf;
	const char *outfile;
	char *cwd;
	int ch, ret;
	enum uclua_dump_type udump;

	udump = UCLUAD_UCL;
	outfile = NULL;
	while ((ch = getopt_long(argc, argv, optstr, longopts, NULL)) != -1) {
		switch (ch) {
		case JSON_OPT:
			udump = UCLUAD_JSON;
			break;
		case UCL_OPT:
			udump = UCLUAD_UCL;
			break;
		case YAML_OPT:
			udump = UCLUAD_YAML;
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0 && isatty(STDIN_FILENO)) {
		fprintf(stderr, "interactive conversion not supported\n");
		return (usage());
	}

	if (outfile == NULL || strcmp(outfile, "-") == 0) {
		outf = stdout;
	} else {
		outf = fopen(outfile, "w");
		if (outf == NULL) {
			fprintf(stderr, "could not open '%s' for output\n", outfile);
			return (usage());
		}
	}

	lcook = uclua_new();
	if (lcook == NULL) {
		fprintf(stderr, "out of memory\n");
		ret = 1;
		goto out;
	}

	cwd = getcwd(NULL, 0);
	if (cwd != NULL) {
		uclua_set_sandbox(lcook, cwd);
		free(cwd);
	}

	if (argc == 0) {
		ret = parse_one(lcook, "-");
	} else {
		ret = 0;
		for (int i = 0; i < argc; i++) {
			if ((ret = parse_one(lcook, argv[i])) != 0)
				break;
		}
	}

	if (ret == 0 && uclua_dump(lcook, udump, outf) != 0) {
		fprintf(stderr, "Failed to dump!\n");
		ret = 1;
	}
out:
	if (outf != stdout)
		fclose(outf);
	if (lcook != NULL)
		uclua_free(lcook);
	return (ret);
}
