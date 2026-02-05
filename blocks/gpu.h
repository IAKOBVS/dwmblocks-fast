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

#ifndef B_GPU_H
#	define B_GPU_H 1

#	include "../config.h"

#	ifdef USE_CUDA

/* ../src/blocks/gpu.c */

char *
b_write_gpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
b_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
b_write_gpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);
char *
b_write_gpu_vram(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval);

#	endif /* USE_CUDA */

#endif /* B_GPU_H */
