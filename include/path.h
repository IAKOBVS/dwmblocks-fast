/* SPDX-License-Identifier: ISC */
/* Copyright 2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of dwmblocks-fast.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef PATH_H
#	define PATH_H 1

#	include <glob.h>
#	include <string.h>
#	include <unistd.h>
#	include <assert.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <limits.h>
#	include <regex.h>

#	include "macros.h"
#	include "utils.h"

/* Update hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* to point to
 * the real file, given that the number may change between reboots.
 *
 * Return pointer to malloc'd resolved filename.
 * If file already exists, then return original filename
 *
 * Filename should start with /sys/devices/platform.
 *
 * Example filename: /sys/devices/platform/coretemp.0/hwmon/hwmon2/temp1_input
 * Example pattern: hwmon/hwmon and thermal/thermal_zone
 * Example pattern_glob: hwmon/hwmon[0-9]* and thermal/thermal_zone[0-9]* */
static char *
path_sysfs_resolve(const char *filename)
{
#	if !defined NAME_MAX || !defined PATH_MAX
	enum {
#		ifndef NAME_MAX
		NAME_MAX = 256,
#		endif
#		ifndef PATH_MAX
		PATH_MAX = 4096,
#		endif
	};
#	endif
	/* No need to continue if file exists. */
	if (access(filename, F_OK) == 0)
		return (char *)filename;
	regex_t r;
	regmatch_t rm[4];
	/* Convert the filename into a glob. */
	const char *pattern = "\\(\\/sys\\/devices.*\\)\\/\\([^/0-9]*\\)[0-9]*\\/\\([^/]*\\)$";
	DBG(fprintf(stderr, "%s:%d:%s: pattern: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, pattern));
	if (unlikely(regcomp(&r, pattern, 0)))
		return NULL;
	int match = regexec(&r, filename, 4, rm, 0);
	regfree(&r);
	if (unlikely(match != REG_NOERROR))
		return NULL;
	char glob_pattern[PATH_MAX + NAME_MAX];
	char *glob_end = glob_pattern;
	/* Construct the glob pattern. */
	glob_end = (char *)u_stpcpy_len(glob_end, filename + rm[1].rm_so, (size_t)(rm[1].rm_eo - rm[1].rm_so));
	glob_end = (char *)u_stpcpy_len(glob_end, S_LITERAL("/"));
	glob_end = (char *)u_stpcpy_len(glob_end, filename + rm[2].rm_so, (size_t)(rm[2].rm_eo - rm[2].rm_so));
	glob_end = (char *)u_stpcpy_len(glob_end, S_LITERAL("[0-9]*/"));
	glob_end = (char *)u_stpcpy_len(glob_end, filename + rm[3].rm_so, (size_t)(rm[3].rm_eo - rm[3].rm_so));
	DBG(fprintf(stderr, "%s:%d:%s: glob_pattern: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, glob_pattern));
	glob_t g;
	/* Expand the glob into the real file. */
	int ret = glob(glob_pattern, 0, NULL, &g);
	if (ret == 0) {
		const size_t len = strlen(g.gl_pathv[0]);
		char *heap = (char *)malloc(len + 1);
		if (heap == NULL)
			return NULL;
		u_stpcpy_len(heap, g.gl_pathv[0], len);
		globfree(&g);
		DBG(fprintf(stderr, "%s:%d:%s: heap (malloc'd): %s.\n", __FILE__, __LINE__, ASSERT_FUNC, heap));
		return heap;
	} else if (unlikely(ret == GLOB_NOMATCH)) {
		globfree(&g);
	} else { /* glob error */
		return NULL;
	}
	return NULL;
}

#endif /* PATH_H */
