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

#include "luclua_internal.h"

typedef ucl_object_t *(uclua_process_type_func)(lua_State *, int);

static uclua_process_type_func uclua_process_table;
static uclua_process_type_func uclua_process_bool;
static uclua_process_type_func uclua_process_number;
static uclua_process_type_func uclua_process_string;
static uclua_process_type_func uclua_process_table;

static uclua_process_type_func *uclua_processors[] = {
	[LUA_TBOOLEAN] = uclua_process_bool,
	[LUA_TNUMBER] = uclua_process_number,
	[LUA_TSTRING] = uclua_process_string,
	[LUA_TTABLE] = uclua_process_table,
};

ucl_object_t *
uclua_ucl(lcookie_t *lcook)
{
	ucl_object_t *obj;
	lua_State *L;

	if (!lcook->dirty)
		/* XXX May be NULL if no files consumed! */
		return (lcook->ucl);

	L = lcook->L;
	lua_getfield(L, LUA_REGISTRYINDEX, LENV_IDX);
	if ((obj = uclua_process_table(L, lua_gettop(L))) != NULL) {
		uclua_ucl_free(lcook);
		lcook->dirty = false;
		lcook->ucl = obj;
		return (lcook->ucl);
	}

	/* XXX Error */
	return (NULL);
}

void
uclua_ucl_free(lcookie_t *lcook)
{

	if (lcook->ucl == NULL)
		return;
	ucl_object_unref(lcook->ucl);
	lcook->ucl = NULL;
}

static ucl_object_t *
uclua_process_table(lua_State *L, int idx)
{
	const char *key;
	uclua_process_type_func *processor;
	ucl_object_t *obj, *val;
	int ltype;

	obj = ucl_object_typed_new(UCL_OBJECT);
	if (obj == NULL) {
		/* XXX Error */
		return (false);
	}

	lua_pushnil(L);
	while (lua_next(L, idx) != 0) {
		ltype = lua_type(L, -2);
		if (ltype != LUA_TSTRING && ltype != LUA_TNUMBER) {
			/* XXX */
			lua_pop(L, 1);
			continue;
		}
		key = luaL_tolstring(L, -2, NULL);
		ltype = lua_type(L, -2);
		if (ltype < 0 || ltype > nitems(uclua_processors)) {
			goto next;
		}

		processor = uclua_processors[ltype];
		if (processor != NULL) {
			if ((val = (*processor)(L, lua_gettop(L) - 1)) == NULL) {
				/* XXX Error */
				goto next;
			}

			if (!ucl_object_insert_key(obj, val, key, 0, true)) {
				/* XXX Error */
				goto next;
			}
		}
next:
		lua_pop(L, 2);
	}

	return (obj);
}

static ucl_object_t *
uclua_process_bool(lua_State *L, int idx)
{

	return (ucl_object_frombool(lua_toboolean(L, idx)));
}

static ucl_object_t *
uclua_process_number(lua_State *L, int idx)
{

	if (lua_isinteger(L, idx))
		return (ucl_object_fromint(lua_tointeger(L, idx)));
#if LUA_FLOAT_TYPE == LUA_FLOAT_FLOAT || LUA_FLOAT_TYPE == LUA_FLOAT_DOUBLE
	return (ucl_object_fromdouble((double)lua_tonumber(L, idx)));
#else
	/* XXX ERROR no LUA_FLOAT_LONGDOUBLE*/
	return (NULL);
#endif
}

static ucl_object_t *
uclua_process_string(lua_State *L, int idx)
{
	const char *str;
	ucl_object_t *obj;

	str = luaL_tolstring(L, idx, NULL);
	obj = ucl_object_fromstring(str);
	lua_pop(L, 1);
	return (obj);
}