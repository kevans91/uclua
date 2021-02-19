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

#include <luaconf.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <uclua.h>
#include "luclua_internal.h"

#define	LCOOKIE_IDX		"uclua_cookie"

static const luaL_Reg dflibs[] = {
	/* {LUA_LOADLIBNAME, luaopen_package}, */
	{LUA_COLIBNAME, luaopen_coroutine},
	{LUA_TABLIBNAME, luaopen_table},
	/* {LUA_IOLIBNAME, luaopen_io}, */
	/* {LUA_OSLIBNAME, luaopen_os}, */
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_UTF8LIBNAME, luaopen_utf8},
	/* {LUA_DBLIBNAME, luaopen_debug}, */
#if defined(LUA_COMPAT_BITLIB)
	{LUA_BITLIBNAME, luaopen_bit32},
#endif
/*	{"ucl", luaopen_ucl}, */
};

static void uclua_init_state(lcookie_t *);

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
	uclua_init_state(lcook);

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

static void
uclua_init_state(lcookie_t *lcook)
{
	const luaL_Reg *lib;
	lua_State *L;

	L = lcook->L;
	luaL_requiref(L, "_G", luaopen_base, 1);
	lua_pushnil(L);
	lua_setfield(L, -2, "dofile");

	lua_pushnil(L);
	lua_setfield(L, -2, "loadfile");
	lua_pop(L, 1);

	for (size_t i = 0; i < nitems(dflibs); ++i) {
		lib = &dflibs[i];
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);
	}
}
