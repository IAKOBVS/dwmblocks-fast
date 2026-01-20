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

#ifndef C_GPU_H
#	define C_GPU_H 1

#	include "../config.h"

#	if USE_NVML
#		include <stdlib.h>
#		include <assert.h>

#		include "gpu-lib.h"
#		include "../macros.h"
#		include "../utils.h"

static struct Gpu {
	unsigned int deviceCount;
	nvmlDevice_t *device;
	unsigned int *temp;
	int init;
	nvmlReturn_t ret;
	/* nvmlTemperature_t *temp; */
} c_gpu;

static void
c_gpu_cleanup()
{
	free(c_gpu.device);
	free(c_gpu.temp);
	nvmlShutdown();
}

static void
c_gpu_err()
{
	fprintf(stderr, "%s\n\n", nvmlErrorString(c_gpu.ret));
	if (c_gpu.init)
		c_gpu_cleanup();
}

static void
c_gpu_init()
{
	c_gpu.ret = nvmlInit();
	if (c_gpu.ret != NVML_SUCCESS)
		ERR(nvmlErrorString(c_gpu.ret); exit(EXIT_FAILURE));
	c_gpu.ret = nvmlDeviceGetCount(&c_gpu.deviceCount);
	if (c_gpu.ret != NVML_SUCCESS)
		ERR(c_gpu_err());
	c_gpu.device = (nvmlDevice_t *)malloc(c_gpu.deviceCount * sizeof(nvmlDevice_t));
	if (c_gpu.device == NULL)
		ERR(c_gpu_err());
	c_gpu.temp = (unsigned int *)malloc(c_gpu.deviceCount * sizeof(unsigned int));
	if (c_gpu.temp == NULL)
		ERR(c_gpu_err());
	for (unsigned int i = 0; i < c_gpu.deviceCount; ++i) {
		c_gpu.ret = nvmlDeviceGetHandleByIndex(i, c_gpu.device + i);
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
	}
	c_gpu.init = 1;
}

static char *
c_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	if (c_gpu.init == 0)
		c_gpu_init();
	char *p = dst;
	unsigned int avg;
	if (c_gpu.deviceCount == 1) {
		c_gpu.ret = nvmlDeviceGetTemperature(c_gpu.device[0], NVML_TEMPERATURE_GPU, (unsigned int *)c_gpu.temp);
		/* FIXME: does not work. */
		/* c_gpu.ret = nvmlDeviceGetTemperatureV(c_gpu.device[i], c_gpu.temp + i); */
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		avg = c_gpu.temp[0];
	} else if (c_gpu.deviceCount > 0) {
		avg = 0;
		for (unsigned int i = 0; i < c_gpu.deviceCount; ++i) {
			c_gpu.ret = nvmlDeviceGetTemperature(c_gpu.device[i], NVML_TEMPERATURE_GPU, (unsigned int *)c_gpu.temp + i);
			/* FIXME: does not work. */
			/* c_gpu.ret = nvmlDeviceGetTemperatureV(c_gpu.device[i], c_gpu.temp + i); */
			if (c_gpu.ret != NVML_SUCCESS)
				ERR(c_gpu_err());
			avg += *(c_gpu.temp + i);
		}
		avg /= c_gpu.deviceCount;
	} else {
		return p;
	}
	p = utoa_p(avg, p);
	p = xstpcpy_len(p, S_LITERAL("°"));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	elif USE_NVIDIA

#		include "shell.h"
static char *
c_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_shell(dst, dst_len, CMD_GPU_NVIDIA_TEMP);
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	endif

#endif /* C_GPU_H */
