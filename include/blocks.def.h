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

#	include "config.h"

#	include "blocks/webcam.h"
#	include "blocks/time.h"
#	include "blocks/procfs.h"
#	include "blocks/shell.h"
#	include "blocks/obs.h"
#	include "blocks/audio.h"
#	include "blocks/cpu.h"
#	include "blocks/ram.h"
#	include "blocks/gpu.h"
#	include "blocks/webcam.h"

typedef struct {
	unsigned int interval;
	const unsigned int signal;
	const char *icon;
	char *(*func)(char *, unsigned int, const char *, unsigned int *);
	const char *command;
} g_block_ty;

/* clang-format off */

/* Modify this file to change what commands output to your statusbar, and recompile using the make command. */
static ATTR_MAYBE_UNUSED g_block_ty g_blocks[] = {
	/* To use a shell script, set func to b_write_cmd and command to the shell script.
	 * To use a C function, set command to NULL.
	 *
	 * Update_interval    Signal    Icon    Function    Command */
#ifdef HAVE_RPOCFS
	{ 0,    SIG_WEBCAM, NULL, b_write_webcam_on,         NULL },
#endif

#ifdef HAVE_PROCFS
	/* ================================================================================================= */
	/* Do not change the order: b_write_obs_on must be placed before b_write_obs_recording! */
	/* ================================================================================================= */
	{ 0,    SIG_OBS,    NULL, b_write_obs_on,            NULL },
	{ 0,    SIG_OBS,    NULL, b_write_obs_recording,     NULL },
	/* ================================================================================================= */
#endif

#	ifdef USE_AUDIO
	{ 0,    SIG_MIC,    NULL, b_write_mic_vol,           NULL },
#	endif

	{ 3600, 0,          "📅", b_write_date,              NULL },

#ifdef HAVE_PROCFS
	{ 30,   0,          "🧠", b_write_ram_usage_percent, NULL },
#endif

#ifdef HAVE_PROCFS
	/* b_write_cpu_all: [temp] [usage] */
	{ 2,    0,          "💻", b_write_cpu_all,           NULL },
	/* { 2,    0,          "💻", b_write_cpu_temp,          NULL }, */
	/* { 2,    0,          "💻", b_write_cpu_usage,         NULL }, */
#endif

#	ifdef USE_NVIDIA
	/* b_write_gpu_all: [temp] [usage] [vram] */
	{ 2,    0,          "🚀", b_write_gpu_all,           NULL },
	/* { 2,    0,          "🚀", b_write_gpu_temp,          NULL }, */
	/* { 2,    0,          "🚀", b_write_gpu_usage,         NULL }, */
	/* { 2,    0,          "🚀", b_write_gpu_vram,          NULL }, */
#	endif

#	ifdef USE_AUDIO
	{ 0,    SIG_AUDIO,  NULL, b_write_speaker_vol,       NULL },
#	endif

#if defined HAVE_POPEN && defined HAVE_PCLOSE
	/* { 0,    SIG_AUDIO,  "my command:", b_write_shell,       "some_command | other_command" }, */
#endif
	{ 60,   0,          "⏰", b_write_time,              NULL },
};

/* clang-format on */

#endif /* BLOCKS_H */
