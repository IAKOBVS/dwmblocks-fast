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
path_sysfs_resolve(const char *filename, const char *pattern, const char *pattern_glob)
{
	if (access(filename, F_OK) == 0)
		return (char *)filename;
	const char *platform_prefix = "/sys/devices/platform/";
	/* "hwmon/hwmon" */
	if (strstr(filename, platform_prefix) == filename
	    && strstr(filename, pattern)) {
		char path[PATH_MAX];
		char cwd_orig[PATH_MAX];
		getcwd(cwd_orig, sizeof(cwd_orig));
		if (unlikely(chdir(platform_prefix) == -1))
			DIE();
		getcwd(path, sizeof(path));
		const char *p = filename + strlen(platform_prefix);
		char platform[256];
		char *platform_e = platform;
		while (*p != '/' && *p != '\0')
			*platform_e++ = *p++;
		*platform_e = '\0';
		if (unlikely(chdir(platform) == -1))
			DIE();
		glob_t g_dir;
		/* "hwmon/hwmon[0-9]*" */
		int ret = glob(pattern_glob, 0, NULL, &g_dir);
		/* Match */
		if (ret == 0) {
			const char *label = strrchr(filename, '/');
			if (label) {
				++label;
				for (unsigned int i = 0; i < g_dir.gl_pathc; ++i) {
					const char *dir = g_dir.gl_pathv[i];
					if (chdir(dir) == -1)
						DIE();
					globfree(&g_dir);
					if (access(label, F_OK) == -1)
						DIE();
					char *heap = (char *)malloc(PATH_MAX);
					if (heap == NULL)
						DIE();
					if (realpath(label, heap) != heap)
						DIE();
					if (unlikely(chdir(cwd_orig)) == -1)
						DIE();
					return heap;
				}
			} else {
				DIE();
			}
		} else {
			if (ret == GLOB_NOMATCH)
				globfree(&g_dir);
			DIE();
		}
	}
	return NULL;
}

#endif /* PATH_H */
