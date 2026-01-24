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

#ifndef BLOCKS_H
#	define BLOCKS_H 1

/* blocks/audio.h */
char *
c_write_speaker_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_mic_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/gpu.h */
char *
c_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_gpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_gpu_vram(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_gpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/cpu.h */
char *
c_write_cpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_cpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_cpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/obs.h */
char *
c_write_obs_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_obs_recording(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/ram.h */
char *
c_write_ram_usage_percent(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/time.h */
char *
c_write_time(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
c_write_date(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

/* blocks/shell.h */
char *
c_write_shell(char *dst, unsigned int dst_len, const char *cmd, unsigned int *interval);

/* blocks/webcam.h */
char *
c_write_webcam_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

#endif /* BLOCKS_H */
