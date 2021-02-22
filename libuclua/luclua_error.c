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

#include <uclua.h>
#include "luclua_internal.h"

static const char *error2str[] = {
	[UCLUE_OK]				= "No error",

	/* Sandbox errors. */
	[UCLUE_SANDBOX_NOTDIR]	= "Specified sandbox is not a directory",
	[UCLUE_SANDBOX_NOENT]	= "Specified sandbox does not exist",
	[UCLUE_SANDBOX_ACCES]	= "Sandbox access denied",
	[UCLUE_SANDBOX_FAILURE]	= "Failed to open sandbox directory",

	/* File loading errors. */
	[UCLUE_LUA_ERROR]	= "Lua error encountered",
	[UCLUE_IO_ERROR]	= "I/O error",

	/* Lua -> UCL errors. */
	[UCLUE_NOTYPE]		= "No equivalent type",
	[UCLUE_BADKEYTYPE]	= "Bad key type",
	[UCLUE_BADCONV]		= "Bad value conversion",
	[UCLUE_MUTATE]		= "Error while mutating UCL object/array",
	[UCLUE_NOMEM]		= "Out of memory",

	/* Dump errors. */
	[UCLUE_DUMP_EMITFAIL]	= "Failed to emit requested type",
	[UCLUE_DUMP_NOSPC]		= "No space left in requested file",
	[UCLUE_DUMP_WRITEFAIL]	= "Generic failure to write to requested file",
};

uclua_error
uclua_get_error(lcookie_t *lcook)
{

	return (lcook->error);
}

const char *
uclua_error_string(uclua_error error)
{

	if (error >= nitems(error2str))
		return ("Unknown error");

	return (error2str[error]);
}
