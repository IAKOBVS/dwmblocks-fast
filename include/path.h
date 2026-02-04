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
	char cmd[PATH_MAX + PATH_MAX];
	/* Convert the filename into a glob. */
	if (unlikely(snprintf(cmd, sizeof(cmd), "echo '%s' | sed '%s'", filename, "s/\\(\\/sys\\/devices\\/.*\\)\\/\\([^/0-9]*\\)[0-9]\\{,1\\}\\/\\([^/]*\\)$/\\1\\/\\2[0-9]*\\/\\3/")) == -1)
		return NULL;
	DBG(fprintf(stderr, "%s:%d:%s: cmd: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, cmd));
	FILE *fp = popen(cmd, "r");
	if (unlikely(fp == NULL))
		return NULL;
	char output[PATH_MAX + NAME_MAX];
	const int fd = fileno(fp);
	if (unlikely(fd == -1)) {
		pclose(fp);
		return NULL;
	}
	ssize_t read_len = read(fd, output, sizeof(output) - 1);
	if (unlikely(pclose(fp) == -1))
		return NULL;
	if (unlikely(read_len == -1))
		return NULL;
	if (*(output + read_len - 1) == '\n')
		--read_len;
	output[read_len] = '\0';
	DBG(fprintf(stderr, "%s:%d:%s: output: %s.\n", __FILE__, __LINE__, ASSERT_FUNC, output));
	glob_t g;
	/* Expand the glob into the real file. */
	int ret = glob(output, 0, NULL, &g);
	if (ret == 0) {
		const size_t len = strlen(g.gl_pathv[0]);
		char *heap = (char *)malloc(len + 1);
		if (heap == NULL)
			return NULL;
		memcpy(heap, g.gl_pathv[0], len);
		*(heap + len) = '\0';
		globfree(&g);
		DBG(fprintf(stderr, "%s:%d:%s: heap (malloc'd): %s.\n", __FILE__, __LINE__, ASSERT_FUNC, heap));
		return heap;
	} else {
		if (unlikely(ret == GLOB_NOMATCH))
			globfree(&g);
	}
	return NULL;
}

#endif /* PATH_H */
