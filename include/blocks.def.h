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
#	include "blocks/temp.h"
#	include "blocks/cat.h"

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
	/* To use a shell script, set func to b_write_shell and command to the shell script.
	 * To use a C function, set command to NULL.
	 *
	 * Update_interval    Signal    Icon    Function    Command/File */

	/* Webcam */
#	ifdef HAVE_PROCFS
	{ 0,    SIG_WEBCAM, NULL, b_write_webcam_on,         NULL },
#	endif

	/* Obs */
#	ifdef HAVE_PROCFS
	/* ================================================================================================= */
	/* Do not change the order: b_write_obs_on must be placed before b_write_obs_recording! */
	/* ================================================================================================= */
	{ 0,    SIG_OBS,    NULL, b_write_obs_on,            NULL },
	{ 0,    SIG_OBS,    NULL, b_write_obs_recording,     NULL },
	/* ================================================================================================= */
#	endif

	/* Audio volume (mic) */
#	if defined USE_ALSA
	{ 0,    SIG_MIC,    NULL, b_write_mic_vol,           NULL },
#	endif

	/* Date */
	{ 3600, 0,          "📅", b_write_date,              NULL },

	/* Ram */
#	ifdef HAVE_SYSINFO
	{ 30,   0,          "🧠", b_write_ram_usage_percent, NULL },
#	endif

	/* Read a file */
	/* { 2,   0,          "my_file:", b_write_cat, "/home/james/.xinitrc" }, */

	/* Temp file */
#	ifdef HAVE_PROCFS
	/* If using sysfs, make sure that the path starts with /sys/devices/platform, not /sys/class. Pass the file
	 * use the realpath command, to get the real path. */
	/* { 2,    0,          "my_temp: ", b_write_temp,"/path/to/temp" }, */
#	endif

	/* CPU temp, usage */
#	ifdef HAVE_PROCFS
	/* format: [temp] [usage] */
#ifdef HAVE_SYSFS
	{ 2,    0,          "💻", b_write_cpu_all,           TEMP_FILE_CPU },
	/* { 2,    0,          "💻", b_write_cpu_temp,          TEMP_FILE_CPU }, */
#endif
	/* { 2,    0,          "💻", b_write_cpu_usage,         NULL }, */
#	endif

	/* GPU temp, usage */
#	if defined USE_CUDA
	/* format: [temp] [usage] [vram] */
	{ 2,    0,          "🚀", b_write_gpu_all,           NULL },
	/* { 2,    0,          "🚀", b_write_gpu_temp,          NULL }, */
	/* { 2,    0,          "🚀", b_write_gpu_usage,         NULL }, */
	/* { 2,    0,          "🚀", b_write_gpu_vram,          NULL }, */
#	endif

	/* Audio volume (speaker) */
#	if defined USE_ALSA
	{ 0,    SIG_AUDIO,  NULL, b_write_speaker_vol,       NULL },
#	endif

	/* Shell script or command */
#	if defined HAVE_POPEN && defined HAVE_PCLOSE && defined HAVE_FILENO
	/* { 0,    SIG_AUDIO,  "my command:", b_write_shell,       "some_command | other_command" }, */
#	endif

	/* Time */
	{ 59,   0,          "⏰", b_write_time,              NULL },
};

/* clang-format on */

#endif /* BLOCKS_H */
