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
	char *(*func)(char *, unsigned int, const char *, unsigned int *);
	const char *command;
	const char *pad_left;
	const char *pad_right;
} g_block_ty;

#	define TEMP_FILE_SSD "/sys/devices/pci0000:00/0000:00:1b.0/0000:02:00.0/nvme/nvme0/hwmon0/temp1_input"

/* clang-format off */

	/* Modify this file to change what commands output to your statusbar, and recompile using the make command. */
static ATTR_MAYBE_UNUSED g_block_ty g_blocks[] = {
	/* To use a shell script, set func to b_write_shell and command to the shell script.
	 * To use a C function, set command to NULL.
	 *
	 * Update_interval    Signal    Icon    Function    Command/File */

	/* Webcam */
#	ifdef HAVE_PROCFS
	{ .func = b_write_webcam_on,         .command = NULL,          .pad_left = "",    .pad_right = " | ",  .interval = 0,    .signal = SIG_WEBCAM },
#	endif

	/* Obs */
#	ifdef HAVE_PROCFS
	/****************************************************************************************/
	/* Do not change the order: b_write_obs_on must be placed before b_write_obs_recording! */
	/****************************************************************************************/
	{ .func = b_write_obs_on,            .command = NULL,          .pad_left = "",    .pad_right = " | ",  .interval = 0,    .signal = SIG_OBS    },
	{ .func = b_write_obs_recording,     .command = NULL,          .pad_left = "",    .pad_right = " | ",  .interval = 0,    .signal = SIG_OBS    },
	/****************************************************************************************/
#	endif

	/* Audio volume (mic) */
#	if defined USE_ALSA
	{ .func = b_write_mic_vol,           .command = NULL,          .pad_left = "",    .pad_right = "% | ", .interval = 0,    .signal = SIG_MIC    },
#	endif

	/* Date */
	{ .func = b_write_date,              .command = NULL,          .pad_left = "📅 ", .pad_right = " | ",  .interval = 3600, .signal = 0          },

	/* Ram */
#	ifdef HAVE_SYSINFO
	{ .func = b_write_ram_usage_percent, .command = NULL,          .pad_left = "🧠 ", .pad_right = "% | ", .interval = 30,   .signal = 0          },
#	endif

	/* Read a file */
	/* { .func = b_write_cat, .command = "/home/james/.xinitrc ", .pad_left = "my_file:", .pad_right = " | ", .interval = 2, .signal = 0 }, */

	/* Temp file */
#	ifdef HAVE_SYSFS
	/* If using sysfs, make sure that the path starts with /sys/devices/platform, not /sys/class. */
	{ .func = b_write_temp,              .command = TEMP_FILE_SSD, .pad_left = "💾 ", .pad_right = "° | ", .interval = 4,    .signal = 0          },
	/* { .func = b_write_temp, .command = "/path/to/temp ", .pad_left = "my_temp: ", .pad_right = "° | ", .interval = 2, .signal = 0 }, */
#	endif

	/* CPU temp, usage */
#	ifdef HAVE_PROCFS
	/* format: [temp] [usage] */
#		ifdef HAVE_SYSFS
	{ .func = b_write_cpu_temp,          .command = TEMP_FILE_CPU, .pad_left = "💻 ", .pad_right = "° ",   .interval = 2,    .signal = 0          },
#		endif
	{ .func = b_write_cpu_usage,         .command = NULL,          .pad_left = "",    .pad_right = "% | ", .interval = 2,    .signal = 0          },
#	endif

	/* GPU temp, usage */
#	if defined USE_CUDA
	/* format: [temp] [usage] [vram] */
	{ .func = b_write_gpu_temp,          .command = NULL,          .pad_left = "🚀 ", .pad_right = "° ",   .interval = 2,    .signal = 0          },
	{ .func = b_write_gpu_usage,         .command = NULL,          .pad_left = "",    .pad_right = "% ",   .interval = 2,    .signal = 0          },
	{ .func = b_write_gpu_vram,          .command = NULL,          .pad_left = "",    .pad_right = "% | ", .interval = 2,    .signal = 0          },
#	endif

	/* Audio volume (speaker) */
#	if defined USE_ALSA
	{ .func = b_write_speaker_vol,       .command = NULL,          .pad_left = "",    .pad_right = "% | ", .interval = 0,    .signal = SIG_AUDIO  },
#	endif

	/* Shell script or command */
#	if defined HAVE_POPEN && defined HAVE_PCLOSE && defined HAVE_FILENO
	/* { .func = b_write_shell, .command = "some_command | other_command ", .pad_left = "my command:", .pad_right = " | ", .interval = 0, .signal = SIG_AUDIO }, */
#	endif

	/* Time */
	{ .func = b_write_time,              .command = NULL,          .pad_left = "⏰ ", .pad_right = "",  .interval = 59,   .signal = 0          },
};

/* clang-format on */

#endif /* BLOCKS_H */
