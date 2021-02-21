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
#include <sys/mman.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "luclua_internal.h"

#define	uclua_padding(depth)	((depth) * 4)

typedef struct {
	FILE *f;
	int depth;
} uclua_dump_info;

static int uclua_dump_object(const ucl_object_t *, bool, uclua_dump_info *);
static int uclua_dump_object_value(const ucl_object_t *, uclua_dump_info *);

int
uclua_dump_lua(lcookie_t *lcook, FILE *f)
{
	uclua_dump_info info;
	ucl_object_t *ucl;

	ucl = uclua_ucl(lcook);
	if (ucl == NULL)
		return (EINVAL);

	info.f = f;
	info.depth = 0;
	return (uclua_dump_object(ucl, true, &info));
}

static int __printflike(2, 3)
uclua_emit_string(uclua_dump_info *info, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	if ((ret = vfprintf(info->f, fmt, ap)) == -1)
		ret = feof(info->f) ? ENOSPC : errno;
	else
		ret = 0;
	va_end(ap);

	return (ret);
}

static int
uclua_dump_object_value(const ucl_object_t *obj, uclua_dump_info *info)
{
	const ucl_object_t *res;
	int ret;
	enum ucl_type otype;
	bool keys;

	/* Toss up an error if we didn't handle it somehow. */
	ret = EINVAL;
	otype = ucl_object_type(obj);
	switch (otype) {
	case UCL_ARRAY:
	case UCL_OBJECT:
		keys = otype == UCL_OBJECT;
		++info->depth;
		if ((ret = uclua_emit_string(info, "{\n")) != 0)
			return (ret);
		ret = uclua_dump_object(obj, keys, info);
		--info->depth;
		if (ret != 0)
			return (ret);
		ret = uclua_emit_string(info, "%*s}", uclua_padding(info->depth), "");
		break;
	case UCL_INT:
		ret = uclua_emit_string(info, "%jd", (intmax_t)ucl_object_toint(obj));
		break;
	case UCL_FLOAT:
		ret = uclua_emit_string(info, "%f", ucl_object_todouble(obj));
		break;
	case UCL_BOOLEAN:
		ret = uclua_emit_string(info, "%s", ucl_object_toboolean(obj) ? "true" : "false");
		break;
	case UCL_STRING:
		ret = uclua_emit_string(info, "[[%s]]", ucl_object_tostring(obj));
		break;
	default:
		/* UNREACHABLE */
		assert(0);
		break;
	}

	return (ret);
}

static char *
uclua_object_key(const ucl_object_t *obj)
{
	const char *okey;
	char *buf;
	size_t needed, len;

	okey = ucl_object_key(obj);
	needed = 0;
	len = strlen(okey);
	for (size_t i = 0; i < len; i++) {
		/* Escapes get escaped again, quotes get escaped. */
		switch (okey[i]) {
		case '\\':
		case '"':
			needed++;
		default:
			needed++;
		}
	}

	assert(needed >= len);
	buf = malloc(needed + 1);
	if (buf == NULL)
		return (NULL);

	buf[needed] = '\0';
	for (size_t i = 0, j = 0; i < len; i++) {
		switch (okey[i]) {
		case '\\':
		case '"':
			buf[j++] = '\\';
		default:
			buf[j++] = okey[i];
		}
	}

	return (buf);
}

static int
uclua_dump_object(const ucl_object_t *obj, bool keys, uclua_dump_info *info)
{
	ucl_object_iter_t it;
	enum ucl_type otype;
	int ret;

	ret = 0;
	it = ucl_object_iterate_new(obj);
	while (ret == 0 && (obj = ucl_object_iterate_safe(it, true)) != NULL) {
		otype = ucl_object_type(obj);
		switch (otype) {
		case UCL_OBJECT:
		case UCL_ARRAY:
		case UCL_INT:
		case UCL_FLOAT:
		case UCL_BOOLEAN:
		case UCL_STRING:
			break;
		default:
			fprintf(stderr, "Unknown ucl type %s\n", ucl_object_type_to_string(otype));
			continue;
		}

		/* Emit the key + assignment operator */
		if (keys) {
			char *uclkey;

			uclkey = uclua_object_key(obj);
			if (uclkey == NULL) {
				ret = ENOMEM;
			} else if (info->depth == 0) {
				ret = uclua_emit_string(info, "%*s%s = ", uclua_padding(info->depth), "", uclkey);
			} else {
				ret = uclua_emit_string(info, "%*s[\"%s\"] = ", uclua_padding(info->depth), "", uclkey);
			}

			free(uclkey);
		} else if (info->depth > 0) {
			ret = uclua_emit_string(info, "%*s", uclua_padding(info->depth), "");
		}

		if (ret != 0)
			break;

		if ((ret = uclua_dump_object_value(obj, info)) != 0)
			break;

		ret = uclua_emit_string(info, "%s\n", info->depth == 0 ? "" : ",");
	}

	ucl_object_iterate_free(it);
	return (ret);
}

