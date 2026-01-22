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

#	ifdef USE_NVML
#		ifndef NVML_HEADER
#			define NVML_HEADER "/opt/cuda/targets/x86_64-linux/include/nvml.h"
#		endif
#		include NVML_HEADER

#		include <stdlib.h>
#		include <assert.h>

#		include "../macros.h"
#		include "../utils.h"

typedef struct {
	int init;
	unsigned int deviceCount;
	nvmlDevice_t *device;
	unsigned int *temp;
	nvmlReturn_t ret;
	nvmlUtilization_t *utilization;
	/* nvmlTemperature_t *temp; */
} c_gpu_ty;
static c_gpu_ty c_gpu;

typedef enum {
	C_GPU_MON_TEMP = 0,
	C_GPU_MON_USAGE,
	C_GPU_MON_VRAM
} c_gpu_monitor_ty;

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
	c_gpu.utilization = (nvmlUtilization_t *)malloc(c_gpu.deviceCount * sizeof(nvmlUtilization_t));
	if (c_gpu.utilization == NULL)
		ERR(c_gpu_err());
	for (unsigned int i = 0; i < c_gpu.deviceCount; ++i) {
		c_gpu.ret = nvmlDeviceGetHandleByIndex(i, c_gpu.device + i);
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
	}
	c_gpu.init = 1;
}

static unsigned int
c_gpu_monitor(c_gpu_monitor_ty mon_type, unsigned int i)
{
	switch (mon_type) {
	case C_GPU_MON_TEMP:
		c_gpu.ret = nvmlDeviceGetTemperature(c_gpu.device[i], NVML_TEMPERATURE_GPU, c_gpu.temp + i);
		/* FIXME: does not work. */
		/* c_gpu.ret = nvmlDeviceGetTemperatureV(c_gpu.device[i], c_gpu.temp + i); */
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		return c_gpu.temp[i];
		break;
	case C_GPU_MON_USAGE:
		c_gpu.ret = nvmlDeviceGetUtilizationRates(c_gpu.device[i], c_gpu.utilization + i);
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		return c_gpu.utilization[i].gpu;
		break;
	case C_GPU_MON_VRAM:
		c_gpu.ret = nvmlDeviceGetUtilizationRates(c_gpu.device[i], c_gpu.utilization + i);
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		return c_gpu.utilization[i].memory;
		break;
	default:
		ERR();
		break;
	}
}

static unsigned int
c_gpu_monitor_devices(c_gpu_monitor_ty mon_type)
{
	unsigned int avg = 0;
	for (unsigned int i = 0; i < c_gpu.deviceCount; ++i)
		avg += c_gpu_monitor(mon_type, i);
	if (c_gpu.deviceCount > 0)
		avg /= c_gpu.deviceCount;
	return avg;
}

static char *
c_write_gpu_monitor(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval, c_gpu_monitor_ty mon_type)
{
	if (c_gpu.init == 0)
		c_gpu_init();
	unsigned int avg = c_gpu_monitor_devices(mon_type);
	char *p = dst;
	p = utoa_p(avg, p);
	switch (mon_type) {
	case C_GPU_MON_TEMP:
		p = xstpcpy_len(p, S_LITERAL(UNIT_TEMP));
		break;
	case C_GPU_MON_USAGE:
	case C_GPU_MON_VRAM:
		p = xstpcpy_len(p, S_LITERAL(UNIT_USAGE));
		break;
	}
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static void
c_gpu_monitor_devices_all(unsigned int *avg_temp, unsigned int *avg_usage, unsigned int *avg_vram)
{
	*avg_temp = 0;
	*avg_usage = 0;
	*avg_vram = 0;
	for (unsigned int i = 0; i < c_gpu.deviceCount; ++i) {
		c_gpu.ret = nvmlDeviceGetTemperature(c_gpu.device[i], NVML_TEMPERATURE_GPU, c_gpu.temp + i);
		/* FIXME: does not work. */
		/* c_gpu.ret = nvmlDeviceGetTemperatureV(c_gpu.device[i], c_gpu.temp + i); */
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		*avg_temp += c_gpu.temp[i];
		c_gpu.ret = nvmlDeviceGetUtilizationRates(c_gpu.device[i], c_gpu.utilization + i);
		if (c_gpu.ret != NVML_SUCCESS)
			ERR(c_gpu_err());
		*avg_usage += c_gpu.utilization[i].gpu;
		*avg_vram += c_gpu.utilization[i].memory;
	}
	if (c_gpu.deviceCount > 0) {
		*avg_temp /= c_gpu.deviceCount;
		*avg_usage /= c_gpu.deviceCount;
		*avg_vram /= c_gpu.deviceCount;
	}
}

static char *
c_write_gpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	if (c_gpu.init == 0)
		c_gpu_init();
	unsigned int avg_temp, avg_usage, avg_vram;
	c_gpu_monitor_devices_all(&avg_temp, &avg_usage, &avg_vram);
	char *p = dst;
	p = utoa_p(avg_temp, p);
	p = xstpcpy_len(p, S_LITERAL(UNIT_TEMP));
	*p++ = ' ';
	p = utoa_p(avg_usage, p);
	p = xstpcpy_len(p, S_LITERAL(UNIT_USAGE));
	*p++ = ' ';
	p = utoa_p(avg_vram, p);
	p = xstpcpy_len(p, S_LITERAL(UNIT_USAGE));
	*p = '\0';
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static char *
c_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_gpu_monitor(dst, dst_len, unused, interval, C_GPU_MON_TEMP);
}

static char *
c_write_gpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_gpu_monitor(dst, dst_len, unused, interval, C_GPU_MON_USAGE);
}

static char *
c_write_gpu_vram(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_gpu_monitor(dst, dst_len, unused, interval, C_GPU_MON_VRAM);
}

#	elif defined USE_NVIDIA

#		include "shell.h"
static char *
c_write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_shell(dst, dst_len, CMD_GPU_NVIDIA_TEMP, interval);
	(void)unused;
	(void)interval;
}

static char *
c_write_gpu_usage(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_shell(dst, dst_len, CMD_GPU_NVIDIA_USAGE, interval);
	(void)unused;
	(void)interval;
}

static char *
c_write_gpu_vram(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_shell(dst, dst_len, CMD_GPU_NVIDIA_VRAM, interval);
	(void)unused;
	(void)interval;
}

static char *
c_write_gpu_all(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return c_write_shell(dst, dst_len, CMD_GPU_NVIDIA_ALL, interval);
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	endif

#endif /* C_GPU_H */
