/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of dwmblocks-fast.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided that
 * the above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "../config.h"
#include "../blocks/procfs.h"
#include "../macros.h"
#include "../utils.h"

unsigned int b_obs_recording_pid;
unsigned int b_obs_open_pid;

char *
b_write_obs(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval, const char *proc_name, unsigned int proc_name_len, unsigned int *pid_cache, unsigned int proc_interval, const char *proc_icon_on, const char *proc_icon_off)
{
	/* Need to search /proc/[pid] for proc. */
	if (*pid_cache == 0) {
		/* Cache the pid to avoid searching for next calls. */
		*pid_cache = b_proc_exist(proc_name, proc_name_len);
		if (unlikely(*pid_cache == (unsigned int)-1))
			DIE();
		if (*pid_cache == 0) {
			/* OBS is not recording, but still on. Keep checking. */
			if (pid_cache == &b_obs_recording_pid && b_obs_open_pid)
				*interval = proc_interval;
			/* OBS is closed. Stop checking. */
			else
				*interval = 0;
		}
		dst = u_stpcpy(dst, proc_icon_off);
	} else {
		/* Construct path: /proc/[pid]/status. */
		char fname[S_LEN("/proc/") + sizeof(unsigned int) * 8 + S_LEN("/status") + 1];
		char *fnamep = fname;
		/* /proc/ */
		fnamep = u_stpcpy_len(fnamep, S_LITERAL("/proc/"));
		/* /proc/[pid] */
		fnamep = u_utoa_p(*pid_cache, fnamep);
		/* /proc/[pid]/status */
		fnamep = u_stpcpy_len(fnamep, S_LITERAL("/status"));
		(void)fnamep;
		int ret = b_proc_exist_at(proc_name, proc_name_len, fname);
		if (ret == 0) {
			*pid_cache = 0;
			*interval = 0;
			return dst;
		} else if (unlikely(ret == -1)) {
			DIE(return dst);
		}
		dst = u_stpcpy(dst, proc_icon_on);
	}
	*interval = proc_interval;
	return dst;
	(void)unused;
	(void)dst_size;
}

char *
b_write_obs_on(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_obs(dst, dst_size, unused, interval, S_LITERAL("obs"), &b_obs_open_pid, INTERVAL_OBS_ON, ICON_OBS_ON, ICON_OBS_OFF);
	(void)unused;
	(void)dst_size;
}

char *
b_write_obs_recording(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_obs(dst, dst_size, unused, interval, S_LITERAL("obs-ffmpeg-mux"), &b_obs_recording_pid, INTERVAL_OBS_RECORDING, ICON_OBS_RECORDING_ON, ICON_OBS_RECORDING_OFF);
	(void)unused;
	(void)dst_size;
}
