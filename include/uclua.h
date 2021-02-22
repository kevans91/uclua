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

#ifndef _INCL_UCLUA_H
#define	_INCL_UCLUA_H

#include <stdbool.h>
#include <stdio.h>

#include <ucl.h>

struct uclua_cookie;
typedef struct uclua_cookie lcookie_t;

typedef enum uclua_dump_type {
	UCLUAD_JSON = 0,
	UCLUAD_UCL,
	UCLUAD_YAML,
	UCLUAD_LUA,
} uclua_dump_type;

typedef enum uclua_error {
	UCLUE_OK = 0,
	/* Sandbox-related errors */
	UCLUE_SANDBOX_NOTDIR,	/* Specified sandbox is not a directory. */
	UCLUE_SANDBOX_NOENT,	/* Sandbox directory does not exist. */
	UCLUE_SANDBOX_ACCES,	/* Sandbox access denied. */
	UCLUE_SANDBOX_FAILURE,	/* Failed to open sandbox directory. */
	/* File loading */
	UCLUE_LUA_ERROR,		/* Lua error encountered. (XXX) */
	UCLUE_IO_ERROR,			/* I/O failure. */
	/* Lua -> UCL errors */
	UCLUE_NOTYPE,			/* No equivalent UCL type. */
	UCLUE_BADKEYTYPE,		/* Bad key type. */
	UCLUE_BADCONV,			/* Bad value conversion. */
	UCLUE_MUTATE,			/* Error while mutating UCL object. */
	UCLUE_NOMEM,			/* No memory. */
	/* Dump errors */
	UCLUE_DUMP_EMITFAIL,	/* Failed to emit requested type. */
	UCLUE_DUMP_NOSPC,		/* No space left in requested file. */
	UCLUE_DUMP_WRITEFAIL,	/* Generic failure to write to requested file. */
} uclua_error;

lcookie_t *uclua_new(void);
bool uclua_set_sandbox(lcookie_t *, const char *);
bool uclua_parse_file(lcookie_t *, FILE *);
ucl_object_t *uclua_ucl(lcookie_t *);
int uclua_dump(lcookie_t *, uclua_dump_type, FILE *);
void uclua_reset(lcookie_t *);
void uclua_free(lcookie_t *);

uclua_error uclua_get_error(lcookie_t *);
const char *uclua_error_string(uclua_error);

#endif	/* _INCL_UCLUA_H */
