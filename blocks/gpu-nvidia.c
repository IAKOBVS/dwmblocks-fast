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

#include "../config.h"

#ifdef USE_CUDA
#	ifndef NVML_HEADER
#		define NVML_HEADER "/opt/cuda/include/nvml.h"
#	endif
#	include NVML_HEADER

#	include <stdlib.h>
#	include <assert.h>

#	include "../macros.h"
#	include "../utils.h"

typedef struct {
	nvmlDevice_t dev;
	unsigned int temp;
	unsigned int power;
	nvmlUtilization_t utilization;
	nvmlMemory_t memory;
} b_gpu_mon_ty;

typedef struct {
	unsigned int deviceCount;
	b_gpu_mon_ty *buf;
	nvmlReturn_t ret;
	int init;
} b_gpu_ty;
b_gpu_ty b_gpu;

typedef enum {
	B_GPU_MON_TEMP = 0,
	B_GPU_MON_USAGE,
	B_GPU_MON_VRAM,
	B_GPU_MON_POWER_USAGE
} b_gpus_ty;

void
b_gpu_cleanup(void)
{
	if (b_gpu.init)
		nvmlShutdown();
	free(b_gpu.buf);
}

void
b_gpu_err(void)
{
	fprintf(stderr, "nvml error: %s\n", nvmlErrorString(b_gpu.ret));
	b_gpu_cleanup();
}

static ATTR_INLINE
nvmlReturn_t
b_gpu_nvmlDeviceGetTemperature(nvmlDevice_t dev, nvmlTemperatureSensors_t sensorType, unsigned int *temp)
{
#	if USE_NVML_DEVICEGETTEMPERATUREV
	nvmlTemperature_t tmp;
	tmp.sensorType = sensorType;
	tmp.version = nvmlTemperature_v1;
	const nvmlReturn_t ret = nvmlDeviceGetTemperatureV(dev, &tmp);
	*temp = (unsigned int)tmp.temperature;
	return ret;
#	else
	return nvmlDeviceGetTemperature(buf.dev, sensorType, temp);
#	endif
}

void
b_gpu_init(void)
{
	b_gpu.ret = nvmlInit();
	if (unlikely(unlikely(b_gpu.ret != NVML_SUCCESS)))
		DIE_DO(nvmlErrorString(b_gpu.ret));
	b_gpu.ret = nvmlDeviceGetCount(&b_gpu.deviceCount);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	b_gpu.buf = (b_gpu_mon_ty *)calloc(b_gpu.deviceCount, sizeof(*b_gpu.buf));
	if (b_gpu.buf == NULL)
		DIE_DO(b_gpu_err());
	for (unsigned int i = 0; i < b_gpu.deviceCount; ++i) {
		b_gpu.ret = nvmlDeviceGetHandleByIndex(i, &b_gpu.buf[i].dev);
		if (unlikely(b_gpu.ret != NVML_SUCCESS))
			DIE_DO(b_gpu_err());
	}
	b_gpu.init = 1;
}

static unsigned int
b_gpu_read_temp(nvmlDevice_t dev, unsigned int *temp)
{
	b_gpu.ret = b_gpu_nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, temp);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return *temp;
}

static unsigned int
b_gpu_read_usage(nvmlDevice_t dev, nvmlUtilization_t *utilization)
{
	b_gpu.ret = nvmlDeviceGetUtilizationRates(dev, utilization);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return utilization->gpu;
}

static unsigned int
b_gpu_read_usage_vram(nvmlDevice_t dev, nvmlMemory_t *memory)
{
	b_gpu.ret = nvmlDeviceGetMemoryInfo(dev, memory);
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return 100 - (unsigned int)(((long double)memory->free / (long double)memory->total) * (long double)100);
}

static unsigned int
b_gpu_read_usage_power(nvmlDevice_t dev, unsigned int *power)
{
	nvmlFieldValue_t values = { .fieldId = NVML_FI_DEV_POWER_INSTANT };
	b_gpu.ret = nvmlDeviceGetFieldValues(dev, 1, &values);
	/* Convert from miliwatt to watt. */
	*power = values.value.uiVal / (double)1000;
	if (unlikely(b_gpu.ret != NVML_SUCCESS))
		DIE_DO(b_gpu_err());
	return *power;
}

static ATTR_INLINE char *
b_write_gpus(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval, b_gpus_ty mon_type)
{
	if (b_gpu.init == 0)
		b_gpu_init();
	unsigned int avg = 0;
	for (unsigned int i = 0; i < b_gpu.deviceCount; ++i)
		switch (mon_type) {
		case B_GPU_MON_TEMP:
			avg += b_gpu_read_temp(b_gpu.buf[i].dev, &b_gpu.buf[i].temp);
			break;
		case B_GPU_MON_USAGE:
			avg += b_gpu_read_usage(b_gpu.buf[i].dev, &b_gpu.buf[i].utilization);
			break;
		case B_GPU_MON_VRAM:
			avg += b_gpu_read_usage_vram(b_gpu.buf[i].dev, &b_gpu.buf[i].memory);
			break;
		case B_GPU_MON_POWER_USAGE:
			avg += b_gpu_read_usage_power(b_gpu.buf[i].dev, &b_gpu.buf[i].power);
			break;
		}
	if (b_gpu.deviceCount > 1)
		avg /= b_gpu.deviceCount;
	char *p = dst;
	p = u_utoa_p(avg, p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_gpu_temp(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_size, unused, interval, B_GPU_MON_TEMP);
}

char *
b_write_gpu_usage(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_size, unused, interval, B_GPU_MON_USAGE);
}

char *
b_write_gpu_usage_vram(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_size, unused, interval, B_GPU_MON_VRAM);
}

char *
b_write_gpu_usage_power(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	return b_write_gpus(dst, dst_size, unused, interval, B_GPU_MON_POWER_USAGE);
}

#endif
