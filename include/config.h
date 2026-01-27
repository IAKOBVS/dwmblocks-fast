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

#	define ICON_WEBCAM_ON       "📸"
#	define ICON_OBS_RECORDING   "🔴 Recording"
#	define ICON_OBS_OPEN        "🎥 OBS"
#	define ICON_SPEAKER_UNMUTED "🔉"
#	define ICON_SPEAKER_MUTED   "🔇"
#	define ICON_MIC_UNMUTED     "🎤"
#	define ICON_MIC_MUTED       "🚫"

#	define INTERVAL_OBS_RECORDING 2
#	define INTERVAL_OBS_OPEN      2

#	define UNIT_USAGE "%"
#	define UNIT_TEMP  "°"

/* These will be copied to each shell script in ./scripts as shell variables. */
#	define SIG_AUDIO  1
#	define SIG_OBS    2
#	define SIG_MIC    3
#	define SIG_WEBCAM 4

/* sets delimeter between status commands. NULL character ('\0') means no delimeter. */
#	define DELIM    " | "
#	define DELIMLEN (S_LEN(DELIM))

/* clang-format on */

#endif /* CONFIG_H */
