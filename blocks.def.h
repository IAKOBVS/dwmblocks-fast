/* SPDX-License-Identifier: ISC */
/* Copyright 2020 torrinfail
 * Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of dwmblocks-fast, derived from dwmblocks with modifications.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

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
static struct Block g_blocks[] = {
	/* To use a shell script, set func to c_write_cmd and command to the shell script.
	 * To use a C function, set command to NULL. */
	/* Update Interval (sec)   Signal	Label	Function	Command */
	{ 0,    SIG_WEBCAM, "",   c_write_webcam_on,         NULL },
	/* Do not change the order of obs: c_write_obs_on must be before c_write_obs_recording */
	{ 0,    SIG_OBS,    NULL,   c_write_obs_on,            NULL },
	{ 0,    SIG_OBS,    "",   c_write_obs_recording,     NULL },
#	if USE_ALSA
	{ 0,    SIG_MIC,    NULL,   c_write_mic_muted,         NULL },
#	endif
	{ 3600, 0,          "📅", c_write_date,              NULL },
	{ 2,    0,          "🧠", c_write_ram_usage_percent, NULL },
	{ 2,    0,          "💻", c_write_cpu_temp,          NULL },
#	if USE_NVML
	{ 2,    0,          "🚀", c_write_gpu_temp,          NULL },
#	endif
#	if USE_ALSA
	{ 0,    SIG_AUDIO,  "🔉", c_write_speaker_vol,       NULL },
#	endif
	{ 60,   0,          "⏰", c_write_time,              NULL },
};

#endif /* BLOCKS_H */
