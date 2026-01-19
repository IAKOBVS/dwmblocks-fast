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

#	include "components.h"

#	define SIG_AUDIO  10
#	define SIG_OBS    9
#	define SIG_MIC    8
#	define SIG_WEBCAM 6

/* sets delimeter between status commands. NULL character ('\0') means no delimeter. */
#	define DELIM    " | "
#	define DELIMLEN (S_LEN(DELIM))

struct Block {
	unsigned int interval;
	const unsigned int signal;
	const char *icon;
	char *(*func)(char *, unsigned int, const char *, unsigned int *);
	const char *command;
};

/* Modify this file to change what commands output to your statusbar, and recompile using the make command. */
static struct Block gx_blocks[] = {
	/* To use a shell script, set func to write_cmd and command to the shell script.
	 * To use a C function, set command to NULL. */
	/* Update Interval (sec)   Signal	Label	Function	Command */
	{ 0,    SIG_WEBCAM, "",   write_webcam_on,         NULL },
	/* Do not change the order of obs: write_obs_on must be before write_obs_recording */
	{ 0,    SIG_OBS,    NULL,   write_obs_on,            NULL },
	{ 0,    SIG_OBS,    "",   write_obs_recording,     NULL },
#	ifdef USE_ALSA
	{ 0,    SIG_MIC,    NULL,   write_mic_muted,         NULL },
#	endif
	{ 3600, 0,          "📅", write_date,              NULL },
	{ 2,    0,          "🧠", write_ram_usage_percent, NULL },
	{ 2,    0,          "💻", write_cpu_temp,          NULL },
#	ifdef USE_NVML
	{ 2,    0,          "🚀", write_gpu_temp,          NULL },
#	endif
#	ifdef USE_ALSA
	{ 0,    SIG_AUDIO,  "🔉", write_speaker_vol,       NULL },
#	endif
	{ 60,   0,          "⏰", write_time,              NULL },
};

#endif /* BLOCKS_H */
