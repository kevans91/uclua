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

#include <assert.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <luaconf.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <uclua.h>
#include "luclua_internal.h"

#define	LCOOKIE_IDX		"uclua_cookie"

typedef void lualib_modify_fn(lcookie_t *);

static lualib_modify_fn	uclua_modify_base;
static lualib_modify_fn	uclua_modify_load;

static const struct uclua_lualib {
	const luaL_Reg			 lib;
	lualib_modify_fn		*modifier;
} dflibs[] = {
	{ .lib = {"_G", luaopen_base}, .modifier = uclua_modify_base },
	{ .lib = {LUA_LOADLIBNAME, luaopen_package}, .modifier = uclua_modify_load },
	{ .lib = {LUA_COLIBNAME, luaopen_coroutine} },
	{ .lib = {LUA_TABLIBNAME, luaopen_table} },
	/* { .lib = {LUA_IOLIBNAME, luaopen_io} }, */
	/* { .lib = {LUA_OSLIBNAME, luaopen_os} }, */
	{ .lib = {LUA_STRLIBNAME, luaopen_string} },
	{ .lib = {LUA_MATHLIBNAME, luaopen_math} },
	{ .lib = {LUA_UTF8LIBNAME, luaopen_utf8} },
	/* {LUA_DBLIBNAME, luaopen_debug} }, */
#if defined(LUA_COMPAT_BITLIB)
	{ .lib = {LUA_BITLIBNAME, luaopen_bit32} },
#endif
/*	{ .lib = {"ucl", luaopen_ucl} }, */
};

struct uclua_floader {
	char	 fload_buff[BUFSIZ];
	FILE	*fload_file;
	bool	 fload_eof;
	bool	 fload_error;
};

static void uclua_init_state(lcookie_t *);
static const char *uclua_read_file(lua_State *, void *, size_t *);

lcookie_t *
uclua_new(void)
{
	lcookie_t *lcook;
	lua_State *L;

	lcook = NULL;
	L = luaL_newstate();
	if (L == NULL)
		return (NULL);

	lcook = calloc(1, sizeof(*lcook));
	if (lcook == NULL)
		goto out;

	lcook->L = L;
	lcook->dirfd = -1;
	uclua_init_state(lcook);

	*(lcookie_t **)lua_newuserdata(L, sizeof(lcook)) = lcook;
	lua_setfield(L, LUA_REGISTRYINDEX, LCOOKIE_IDX);

out:
	if (lcook == NULL)
		lua_close(L);
	return (lcook);
}

bool
uclua_set_sandbox(lcookie_t *lcook, const char *dirname)
{
	int fd;

	fd = open(dirname, O_DIRECTORY | O_SEARCH);
	if (fd == -1)
		return (false);
	if (lcook->dirfd != -1)
		close(lcook->dirfd);
	lcook->dirfd = fd;
	return (true);
}

static int
uclua_load_file(lcookie_t *lcook, FILE *f, const char *name)
{
	struct uclua_floader fload;
	lua_State *L;
	int lerr;

	L = lcook->L;
	fload.fload_file = f;
	fload.fload_eof = fload.fload_error = false;

	lerr = lua_load(L, uclua_read_file, &fload, name, NULL);
	if (lerr != LUA_OK) {
		lua_pushnil(L);
		lua_pushvalue(L, -2);
		return (2);
	} else if (fload.fload_error) {
		lua_pushnil(L);
		lua_pushstring(L, "i/o error");
		return (2);
	}

	lua_getfield(L, LUA_REGISTRYINDEX, LENV_IDX);
	lua_setupvalue(L, -2, 1);

	return (1);
}

bool
uclua_parse_file(lcookie_t *lcook, FILE *f)
{
	lua_State *L;
	int lerr;

	L = lcook->L;

	lua_settop(L, 0);
	lerr = uclua_load_file(lcook, f, "cfgfile");
	assert(lerr > 0);
	if (lua_isnil(L, 1)) {
		assert(lerr > 1);
		fprintf(stderr, "%s\n", luaL_tolstring(L, -1, NULL));
		return (false);
	}

	lerr = lua_pcall(L, 0, 0, 0);
	if (lerr != LUA_OK) {
		printf("pcall error %s\n", luaL_checkstring(L, -1));
		return (false);
	}

	lcook->dirty = true;

	return (true);
}

void
uclua_free(lcookie_t *lcook)
{

	if (lcook == NULL)
		return;

	lua_close(lcook->L);
	free(lcook);
}

void
uclua_reset(lcookie_t *lcook)
{
	lua_State *L;

	L = lcook->L;

	/* Our _ENV */
	lua_newtable(L);

	/*
	 * Metatable; __index of the new environment defaults to _G.  We leave
	 * __newindex alone so that new values get stashed directly in the initially
	 * empty environment, and we'll collect them later.
	 */
	lua_newtable(L);
	lua_pushglobaltable(L);
	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, LENV_IDX);

	lcook->dirty = false;
	uclua_ucl_free(lcook);
}

/*
 * Clobber dofile and loadfile completely to reject loading arbitrary lua
 * chunks.  The intention is to funnel all such requests through 'require'
 * instead, which accepts just a name and imposes visibility restrictions.
 */
static void
uclua_modify_base(lcookie_t *lcook)
{
	lua_State *L;

	L = lcook->L;
	lua_pushnil(L);
	lua_setfield(L, -2, "dofile");

	lua_pushnil(L);
	lua_setfield(L, -2, "loadfile");
}

static int
uclua_searcher_preload(lua_State *L)
{
	const char *name;

	name = luaL_checkstring(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
	if (lua_getfield(L, -1, name) == LUA_TNIL) {
		lua_pushfstring(L, " not preloaded\n");
		return (1);
	}

	/* Return it. */
	return (1);
}

static int
uclua_searcher_dirfd(lua_State *L)
{
	const char *name;
	char *lname;
	lcookie_t *lcook;
	FILE *f;
	int fd, lerr;

	name = luaL_checkstring(L, 1);
	assert(lua_islightuserdata(L, lua_upvalueindex(2)));

	lcook = lua_touserdata(L, lua_upvalueindex(2));
	if (lcook->dirfd == -1) {
		lua_pushfstring(L, "\tno sandbox, attempted to load '%s'", name);
		return (1);
	}

	fd = openat(lcook->dirfd, name, O_RDONLY | O_BENEATH);
	if (fd == -1) {
		lname = NULL;
		if (asprintf(&lname, "%s.lua", name) == -1) {
			lua_pushfstring(L, "\tout of memory trying to load '%s'", name);
			return (1);
		}

		fd = openat(lcook->dirfd, lname, O_RDONLY | O_BENEATH);
		free(lname);
	}

	if (fd == -1) {
		lua_pushfstring(L, "\tnot found in sandbox: '%s'", name);
		return (1);
	}

	f = fdopen(fd, "r");
	if (f == NULL) {
		close(fd);
		lua_pushfstring(L, "\tfailed to open '%s' for reading", name);
		return (1);
	}

	lerr = uclua_load_file(lcook, f, name);
	fclose(f);
	assert(lerr > 0);
	if (lua_isnil(L, -lerr)) {
		/* Failed to load. */
		assert(lerr > 1);
		return (1);
	}

	/* Nothing extra to pass. */
	lua_insert(L, -lerr);
	return (1);
}

/*
 * Use a custom searcher only.  We're removing their ability to pull in
 * C modules by default, and limiting visibility to descendants of the current
 * directory.
 */
static void
uclua_modify_load(lcookie_t *lcook)
{
	lua_State *L;

	L = lcook->L;
	lua_newtable(L);

	/* Preloads are OK. */
	lua_pushvalue(L, -2);
	lua_pushcclosure(L, uclua_searcher_preload, 1);
	lua_rawseti(L, -2, 1);

	lua_pushvalue(L, -2);
	lua_pushlightuserdata(L, lcook);
	lua_pushcclosure(L, uclua_searcher_dirfd, 2);
	lua_rawseti(L, -2, 2);

	lua_setfield(L, -2, "searchers");
}

static void
uclua_init_state(lcookie_t *lcook)
{
	const struct uclua_lualib	*libinfo;
	const luaL_Reg *lib;
	lua_State *L;

	L = lcook->L;
	for (size_t i = 0; i < nitems(dflibs); ++i) {
		libinfo = &dflibs[i];
		lib = &libinfo->lib;
		luaL_requiref(L, lib->name, lib->func, 1);
		if (libinfo->modifier != NULL)
			(*libinfo->modifier)(lcook);
		lua_pop(L, 1);
	}

	uclua_reset(lcook);
}

static const char *
uclua_read_file(lua_State *L, void *data, size_t *size)
{
	struct uclua_floader *fload;
	size_t nb;

	fload = data;
	*size = 0;
	if (fload->fload_eof)
		return (NULL);

	nb = fread(fload->fload_buff, 1, sizeof(fload->fload_buff),
	    fload->fload_file);

	if (nb < sizeof(fload->fload_buff)) {
		if (ferror(fload->fload_file)) {
			fload->fload_error = true;
			return (NULL);
		}

		fload->fload_eof = true;
	}

	*size = nb;
	return (fload->fload_buff);
}
