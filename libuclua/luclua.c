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

#include <stdio.h>
#include <stdlib.h>

#include <luaconf.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <uclua.h>

#define	LCOOKIE_IDX		"luclua_cookie"

struct uclua_cookie {
	lua_State *L;
};

lcookie_t *
uclua_new(void)
{
	lcookie_t *lcook;
	lua_State *L;

	lcook = NULL;
	L = luaL_newstate();
	if (L == NULL)
		return (NULL);

	lcook = malloc(sizeof(*lcook));
	if (lcook == NULL)
		goto out;

	lcook->L = L;
	luaL_openlibs(L);

	*(lcookie_t **)lua_newuserdata(L, sizeof(lcook)) = lcook;
	lua_setfield(L, LUA_REGISTRYINDEX, LCOOKIE_IDX);

out:
	if (lcook == NULL)
		lua_close(L);
	return (lcook);
}

bool
uclua_parse_file(lcookie_t *lcook, FILE *f)
{
	lua_State *L;

	L = lcook->L;
	/*
	 * XXX Run the the file in a protected context.
	 */
	return (false);
}

void
uclua_free(lcookie_t *lcook)
{

	if (lcook == NULL)
		return;

	lua_close(lcook->L);
	free(lcook);
}
