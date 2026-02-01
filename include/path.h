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

#	include "macros.h"

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
	if (access(filename, F_OK) == 0)
		return (char *)filename;
	char platform[NAME_MAX];
	char monitor_dir[NAME_MAX];
	char monitor_subdir[NAME_MAX];
	char tail[PATH_MAX];
	/* %[^/]: match non slash. */
	if (unlikely(sscanf(filename, "/sys/devices/platform/%[^/]/%[^/]/%[^/0-9]%*[0-9]/%s", platform, monitor_dir, monitor_subdir, tail) < 0))
		return NULL;
	char glob_pattern[PATH_MAX];
	const char pat[] = "[0-9]*";
	/* Construct the glob pattern. */
	int len = snprintf(glob_pattern, sizeof(glob_pattern), "/sys/devices/platform/%s/%s/%s%s/%s", platform, monitor_dir, monitor_subdir, pat, tail);
	if (unlikely(len < 0))
		return NULL;
	DBG(fprintf(stderr, "%s:%d:%s: platform: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, platform));
	DBG(fprintf(stderr, "%s:%d:%s: monitor_dir: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, monitor_dir));
	DBG(fprintf(stderr, "%s:%d:%s: monitor_subdir: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, monitor_subdir));
	DBG(fprintf(stderr, "%s:%d:%s: tail: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, tail));
	DBG(fprintf(stderr, "%s:%d:%s: glob_pattern: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, glob_pattern));
	glob_t g;
	int ret = glob(glob_pattern, 0, NULL, &g);
	/* Match */
	if (ret == 0) {
		if (access(g.gl_pathv[0], F_OK) == -1) {
			globfree(&g);
			return NULL;
		}
		len += strlen(g.gl_pathv[0] + len - S_LEN(pat));
		char *tmp = (char *)malloc((size_t)len + 1);
		if (unlikely(tmp == NULL)) {
			globfree(&g);
			return NULL;
		}
		memcpy(tmp, g.gl_pathv[0], (size_t)len);
		*(tmp + len) = '\0';
		DBG(fprintf(stderr, "%s:%d:%s: tmp (malloc'd): %s.\n", __FILE__, __LINE__, ASSERT_FUNC, tmp));
		globfree(&g);
		return tmp;
	}
	if (ret == GLOB_NOMATCH)
		globfree(&g);
	else
		return NULL;
	return NULL;
}

#endif /* PATH_H */
