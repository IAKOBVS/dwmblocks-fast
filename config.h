/* SPDX-License-Identifier: ISC */
/* Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail dot com>
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

#ifndef CONFIG_H
#	define CONFIG_H 1

#	include "macros.h"
#	include "cpu-temp-file.generated.h"

/* Use libx11. Comment to disable. */
#	define USE_X11 1

/* Monitor audio volume, requires ALSA. Comment to disable. */
#	define USE_ALSA 1

/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
#	define USE_CUDA 1
/* May not work for older versions of CUDA, in which case, comment it out. */
#	define USE_NVML_DEVICEGETTEMPERATUREV 1
#	define NVML_HEADER                    "/opt/cuda/include/nvml.h"

#	define ICON_WEBCAM_ON         "📸"
#	define ICON_WEBCAM_OFF        ""

#	define ICON_OBS_RECORDING_ON  "🔴 Recording"
#	define ICON_OBS_RECORDING_OFF ""

#	define ICON_OBS_ON            "🎥 OBS"
#	define ICON_OBS_OFF           ""

#	define INTERVAL_OBS_RECORDING 2
#	define INTERVAL_OBS_ON        2

#	define ICON_AUDIO_SPEAKER_ON  "🔉"
#	define ICON_AUDIO_SPEAKER_OFF "🔇"

#	define ICON_AUDIO_MIC_ON      "🎤"
#	define ICON_AUDIO_MIC_OFF     "🚫"

/* These will be copied to each shell script in ./scripts as shell variables. */
#	define SIG_AUDIO  1
#	define SIG_OBS    2
#	define SIG_MIC    3
#	define SIG_WEBCAM 4

/* clang-format on */

#endif /* CONFIG_H */
