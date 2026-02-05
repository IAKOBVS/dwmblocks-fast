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

#include "../../include/config.h"

#ifdef USE_CUDA
#	ifndef NVML_HEADER
#		define NVML_HEADER "/opt/cuda/include/nvml.h"
#	endif
#	include NVML_HEADER

#	include <stdlib.h>
#	include <assert.h>

#	include "../../include/macros.h"
#	include "../../include/utils.h"

typedef struct {
	unsigned int deviceCount;
	nvmlDevice_t *device;
	unsigned int *temp;
	nvmlUtilization_t *utilization;
	nvmlMemory_t *memory;
	nvmlReturn_t ret;
	int init;
} b_gpu_ty;
b_gpu_ty b_gpu;

typedef enum {
	B_GPU_MON_TEMP = 0,
	B_GPU_MON_USAGE,
	B_GPU_MON_VRAM
} b_gpus_ty;

void
b_gpu_cleanup()
{
	if (b_gpu.init)
		nvmlShutdown();
	free(b_gpu.device);
	free(b_gpu.temp);
	free(b_gpu.utilization);
	free(b_gpu.memory);
}

void
b_gpu_err()
{
	fprintf(stderr, "nvml error: %s\n\n", nvmlErrorString(b_gpu.ret));
	b_gpu_cleanup();
}

static ATTR_INLINE
nvmlReturn_t
b_gpu_nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int *temp)
{
#	if USE_NVML_DEVICEGETTEMPERATUREV
	nvmlTemperature_t tmp;
	tmp.sensorType = sensorType;
	tmp.version = nvmlTemperature_v1;
	const nvmlReturn_t ret = nvmlDeviceGetTemperatureV(device, &tmp);
	*temp = (unsigned int)tmp.temperature;
	return ret;
#	else
	return nvmlDeviceGetTemperature(device, sensorType, temp);
#	endif
}

void
b_gpu_init()
{
	b_gpu.ret = nvmlInit();
	if (unlikely(unlikely(b_gpu.ret != NVML_SUCCESS)))
		DIE_DO(nvmlErrorString(b_gpu.ret));
	b_gpu.ret = nvmlDeviceGetCount(&b_gpu.deviceCount);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	b_gpu.device = (nvmlDevice_t *)calloc(b_gpu.deviceCount, sizeof(nvmlDevice_t));
	if (b_gpu.device == NULL)
		DIE_DO(b_gpu_err());
	b_gpu.temp = (unsigned int *)malloc(b_gpu.deviceCount * sizeof(unsigned int));
	if (b_gpu.temp == NULL)
		DIE_DO(b_gpu_err());
	b_gpu.utilization = (nvmlUtilization_t *)malloc(b_gpu.deviceCount * sizeof(nvmlUtilization_t));
	if (b_gpu.utilization == NULL)
		DIE_DO(b_gpu_err());
	b_gpu.memory = (nvmlMemory_t *)malloc(b_gpu.deviceCount * sizeof(nvmlMemory_t));
	if (b_gpu.memory == NULL)
		DIE_DO(b_gpu_err());
	for (unsigned int i = 0; i < b_gpu.deviceCount; ++i) {
		b_gpu.ret = nvmlDeviceGetHandleByIndex(i, &b_gpu.device[i]);
		if (unlikely(b_gpu.ret != NVML_SUCCESS))
			DIE_DO(b_gpu_err());
	}
	b_gpu.init = 1;
}

static unsigned int
b_gpu_read_temp(nvmlDevice_t device, unsigned int *temp)
{
	b_gpu.ret = b_gpu_nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, temp);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return *temp;
}

static unsigned int
b_gpu_read_usage(nvmlDevice_t device, nvmlUtilization_t *utilization)
{
	b_gpu.ret = nvmlDeviceGetUtilizationRates(device, utilization);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return utilization->gpu;
}

static unsigned int
b_gpu_read_usage_vram(nvmlDevice_t device, nvmlMemory_t *memory)
{
	b_gpu.ret = nvmlDeviceGetMemoryInfo(device, memory);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return 100 - (unsigned int)(((long double)memory->free / (long double)memory->total) * (long double)100);
}

static ATTR_INLINE char *
b_write_gpus(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval, b_gpus_ty mon_type)
{
	if (b_gpu.init == 0)
		b_gpu_init();
	unsigned int avg = 0;
	switch (mon_type) {
	case B_GPU_MON_TEMP:
		for (unsigned int i = 0; i < b_gpu.deviceCount; ++i)
			avg += b_gpu_read_temp(b_gpu.device[i], &b_gpu.temp[i]);
		break;
	case B_GPU_MON_USAGE:
		for (unsigned int i = 0; i < b_gpu.deviceCount; ++i)
			avg += b_gpu_read_usage(b_gpu.device[i], &b_gpu.utilization[i]);
		break;
	case B_GPU_MON_VRAM:
		for (unsigned int i = 0; i < b_gpu.deviceCount; ++i)
			avg += b_gpu_read_usage_vram(b_gpu.device[i], &b_gpu.memory[i]);
		break;
	}
	if (b_gpu.deviceCount > 1)
		avg /= b_gpu.deviceCount;
	char *p = dst;
	p = u_utoa_lt3_p(avg, p);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

char *
b_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_len, unused, interval, B_GPU_MON_TEMP);
}

char *
b_write_gpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_len, unused, interval, B_GPU_MON_USAGE);
}

char *
b_write_gpu_vram(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_len, unused, interval, B_GPU_MON_VRAM);
}

#endif
