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

#ifndef CONFIG_H
#	define CONFIG_H 1

/* Use libx11. Comment to disable. */
#	define USE_X11 1

/* Monitor audio volume, requires ALSA. Comment to disable. */
#	define USE_ALSA 1

/* Monitor Nvidia GPU. nvidia-settings as fallback. Comment to disable. */
#	define USE_NVIDIA 1
/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
#	define USE_NVML    1
#	define NVML_HEADER "/opt/cuda/targets/x86_64-linux/include/nvml.h"

/* Use unlocked stdio functions, since single-threaded. */
#	define USE_UNLOCKED_IO 1

/* Path to CPU temperature */
#	define CPU_TEMP_FILE "/sys/class/thermal/thermal_zone1/temp"

/* Shell scripts to execute if C functions are not available */
#	define CMD_RAM_USAGE        "free | awk '/^Mem:/ {printf(" % d % % ", 100 - ($4/$2 * 100))}'"
#	define CMD_GPU_NVIDIA_TEMP  "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits -i 0"
#	define CMD_GPU_NVIDIA_USAGE "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits -i 0"
#	define CMD_GPU_NVIDIA_VRAM  "nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader,nounits -i 0 | awk -F', ' '{ printf(" % d % %\n ", ($1/$2)*100) }'"
#	define CMD_GPU_NVIDIA_ALL   "nvidia-smi --query-gpu=temperature.gpu,utilization.gpu,memory.used,memory.total --format=csv,noheader,nounits -i 0 | awk -F', ' '{ printf(" % d % % % d % % % d % %\n ", $1, $2, ($3/$4)*100) }'"
#	define CMD_TIME             "date '+%I:%M %p'"
#	define CMD_DATE             "date '+%a, %d %b %Y'"
#	define CMD_CPU_TEMP         "head -c2 " CPU_TEMP_FILE
#	define CMD_MIC_MUTED        "[ $(ifmute) = 'false' ] && echo '🎤' || echo '🔇'"
#	define CMD_OBS_OPEN         "pgrep 'obs' > /dev/null && echo '🎥 |' || echo ''"
#	define CMD_OBS_RECORDING    "pgrep 'obs-ffmpeg-mux' > /dev/null && echo ' 🔴 |'"

#	define UNIT_USAGE "%"
#	define UNIT_TEMP  "°"

#endif /* CONFIG_H */
