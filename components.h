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

#ifndef COMPONENTS_H
#	define COMPONENTS_H 1

#	include <sys/sysinfo.h>
#	include <assert.h>
#	include <time.h>
#	include <unistd.h>
#	include <fcntl.h>
#	include <string.h>
#	include <dirent.h>
#	include <stdlib.h>
#	include <stdio.h>

#	include "lib.h"
#	include "gpu.h"
#	include "audio.h"

#	define ERR(x)              \
		do {                \
			perror(""); \
			assert(0);  \
			x;          \
		} while (0)

#	define PLAYBACK 1
#	define CAPTURE  2

#	ifdef USE_ALSA
static struct Audio {
	const char *card;
	const char *selem_name;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	long min_vol, max_vol, curr_vol;
	int init;
	int ret;
	int playback_or_capture;
	int has_mute;
} gc_speaker, gc_mic;
#	endif

#	ifdef USE_NVML
static struct Gpu {
	unsigned int deviceCount;
	nvmlDevice_t *device;
	unsigned int *temp;
	int init;
	nvmlReturn_t ret;
	/* nvmlTemperature_t *temp; */
} gc_gpu;
#	endif

static unsigned int gc_obs_recording_pid;
static unsigned int gc_obs_open_pid;

/* Execute shell script. */
static char *
write_cmd(char *dst, unsigned int dst_len, const char *cmd)
{
#	if HAVE_POPEN && HAVE_PCLOSE
	FILE *fp = popen(cmd, "r");
	if (fp == NULL)
		ERR(return NULL);
	int ret;
	unsigned int read;
	read = fread(dst, 1, dst_len, fp);
	ret = pclose(fp);
	if (ret < 0)
		ERR(return NULL);
	/* Chop newline. */
	char *nl = (char *)memchr(dst, '\n', read);
	if (nl) {
		*nl = '\0';
		dst = nl;
	} else {
		*(dst += read) = '\0';
	}
#	else
	assert("write_cmd: calling write_cmd when popen or pclose is not available!");
	(void)dst_len;
	(void)cmd;
#	endif
	return dst;
}

#	ifdef USE_NVML

static void
gpu_cleanup()
{
	free(gc_gpu.device);
	free(gc_gpu.temp);
	nvmlShutdown();
}

static void
gpu_err()
{
	fprintf(stderr, "%s\n\n", nvmlErrorString(gc_gpu.ret));
	if (gc_gpu.init)
		gpu_cleanup();
}

static void
gpu_init()
{
	gc_gpu.ret = nvmlInit();
	if (gc_gpu.ret != NVML_SUCCESS)
		ERR(nvmlErrorString(gc_gpu.ret); exit(EXIT_FAILURE));
	gc_gpu.ret = nvmlDeviceGetCount(&gc_gpu.deviceCount);
	if (gc_gpu.ret != NVML_SUCCESS)
		ERR(gpu_err());
	gc_gpu.device = (nvmlDevice_t *)malloc(gc_gpu.deviceCount * sizeof(nvmlDevice_t));
	if (gc_gpu.device == NULL)
		ERR(gpu_err());
	gc_gpu.temp = (unsigned int *)malloc(gc_gpu.deviceCount * sizeof(unsigned int));
	if (gc_gpu.temp == NULL)
		ERR(gpu_err());
	for (unsigned int i = 0; i < gc_gpu.deviceCount; ++i) {
		gc_gpu.ret = nvmlDeviceGetHandleByIndex(i, gc_gpu.device + i);
		if (gc_gpu.ret != NVML_SUCCESS)
			ERR(gpu_err());
	}
	gc_gpu.init = 1;
}
#	endif

#	ifdef USE_ALSA
static void
audio_cleanup_one(struct Audio *audio)
{
	if (audio->handle)
		snd_mixer_close(audio->handle);
	snd_mixer_selem_id_free(audio->sid);
}

static void
audio_cleanup()
{
	audio_cleanup_one(&gc_speaker);
	audio_cleanup_one(&gc_mic);
}

static void
audio_err(void)
{
	fprintf(stderr, "%s\n\n", snd_strerror(gc_speaker.ret));
	fprintf(stderr, "%s\n\n", snd_strerror(gc_mic.ret));
	audio_cleanup();
}

static void
audio_init_one(struct Audio *audio, const char *card, const char *selem_name, int playback_or_capture)
{
	audio->card = card;
	audio->selem_name = selem_name;
	audio->playback_or_capture = playback_or_capture;
	snd_mixer_selem_id_malloc(&audio->sid);
	if (audio->sid == NULL)
		ERR();
	audio->ret = snd_mixer_open(&audio->handle, 0);
	if (audio->ret != 0)
		ERR(audio_err());
	audio->ret = snd_mixer_attach(audio->handle, audio->card);
	if (audio->ret != 0)
		ERR(audio_err());
	audio->ret = snd_mixer_selem_register(audio->handle, NULL, NULL);
	if (audio->ret != 0)
		ERR(audio_err());
	audio->ret = snd_mixer_load(audio->handle);
	if (audio->ret != 0)
		ERR(audio_err());
	snd_mixer_selem_id_set_index(audio->sid, 0);
	snd_mixer_selem_id_set_name(audio->sid, audio->selem_name);
	audio->elem = snd_mixer_find_selem(audio->handle, audio->sid);
	if (audio->elem == NULL)
		ERR(audio_err());
	if (playback_or_capture == PLAYBACK)
		snd_mixer_selem_get_playback_volume_range(audio->elem, &audio->min_vol, &audio->max_vol);
	else if (audio->playback_or_capture == CAPTURE)
		snd_mixer_selem_get_capture_volume_range(audio->elem, &audio->min_vol, &audio->max_vol);
	else
		ERR(audio_err());
	if (playback_or_capture == PLAYBACK)
		audio->has_mute = snd_mixer_selem_has_playback_switch(audio->elem);
	else if (audio->playback_or_capture == CAPTURE)
		audio->has_mute = snd_mixer_selem_has_capture_switch(audio->elem);
	else
		ERR(audio_err());
	audio->init = 1;
}

static void
audio_speaker_init(void)
{
	audio_init_one(&gc_speaker, "default", "Master", PLAYBACK);
}

static void
audio_mic_init(void)
{
	audio_init_one(&gc_mic, "default", "Capture", CAPTURE);
}

static void
audio_init()
{
	audio_speaker_init();
	audio_mic_init();
}

static int
read_audio_volume(struct Audio *audio)
{
	if (audio->init == 0)
		audio_init();
	audio->ret = snd_mixer_handle_events(audio->handle);
	if (audio->ret < 0)
		ERR(audio_err(); return -1);
	if (audio->playback_or_capture == PLAYBACK)
		audio->ret = snd_mixer_selem_get_playback_volume(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio->curr_vol);
	if (audio->playback_or_capture == CAPTURE)
		audio->ret = snd_mixer_selem_get_capture_volume(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio->curr_vol);
	if (audio->ret != 0)
		ERR(audio_err(); return -1);
	const int percent = (int)(100.0f * (audio->curr_vol - audio->min_vol) / (audio->max_vol - audio->min_vol));
	return percent;
}

static int
read_audio_muted(struct Audio *audio)
{
	if (audio->has_mute) {
		if (audio->init == 0)
			audio_speaker_init();
		audio->ret = snd_mixer_handle_events(audio->handle);
		if (audio->ret < 0)
			ERR(audio_err());
		int i = 1;
		if (audio->playback_or_capture == PLAYBACK)
			audio->ret = snd_mixer_selem_get_playback_switch(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else if (audio->playback_or_capture == CAPTURE)
			audio->ret = snd_mixer_selem_get_capture_switch(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else
			ERR(audio_err());
		if (audio->ret != 0)
			ERR(audio_err());
		return !i;
	}
	return 0;
}

static int
read_mic_volume(void)
{
	return read_audio_volume(&gc_mic);
}

static int
read_speaker_volume(void)
{
	return read_audio_volume(&gc_speaker);
}

static int
read_mic_muted(void)
{
	return read_audio_muted(&gc_mic);
}

static int
read_speaker_muted(void)
{
	return read_audio_muted(&gc_speaker);
}

#	endif

static int
read_ram_usage_percent(void)
{
	struct sysinfo info;
	if (sysinfo(&info) != 0)
		ERR(return -1);
	const int percent = 100 - (((double)info.freeram / (double)info.totalram) * 100);
	return percent;
}

static struct tm *
read_time(void)
{
	time_t t = time(NULL);
	if (t == (time_t)-1)
		return NULL;
	return localtime(&t);
}

static int
atou_lt3(const char *s, int digits)
{
	if (digits == 2)
		return (*s - '0') * 10 + (*(s + 1) - '0');
	else if (digits == 1)
		return (*s - '0');
	else /* digits == 3 */
		return (*s - '0') * 100 + (*(s + 1) - '0') * 10 + (*(s + 2) - '0') * 1;
}

static int
read_cpu_temp(void)
{
	/* Milidegrees = degrees * 1000 */
	char temp[S_LEN("100") + S_LEN("1000") + 1] = { 0 };
	int fd = open(CPU_TEMP_FILE, O_RDONLY);
	if (fd == -1)
		ERR(return -1);
	int read_sz = read(fd, temp, S_LEN(temp));
	if (read_sz < 0)
		ERR(close(fd); return -1);
	read_sz -= (int)S_LEN("1000");
	int ret = close(fd);
	if (ret == -1)
		ERR(return -1);
	return atou_lt3(temp, read_sz);
}

static char *
utoa_p(unsigned int number, char *buf)
{
	char *start = buf;
	do
		*buf++ = number % 10 + '0';
	while ((number /= 10) != 0);
	char *end = buf;
	*buf-- = '\0';
	int c;
	for (; start < buf;) {
		c = *start;
		*start++ = *buf;
		*buf-- = c;
	}
	return (char *)end;
}

/* FIXME: broken */
static char *
utoa1_p(unsigned int number, char *buf)
{
	*buf = number + '0';
	*(buf + 1) = '\0';
	return buf + 1;
}

/* FIXME: broken */
static char *
utoa2_p(unsigned int number, char *buf)
{
	if (number < 10) {
		*buf = number + '0';
		*(buf + 1) = '\0';
		return buf + 1;
	} else { /* number < 100 */
		*(buf + 0) = number / 10 % 10 + '0';
		*(buf + 1) = number % 10 + '0';
		*(buf + 2) = '\0';
		return buf + 2;
	}
}

/* FIXME: broken */
static char *
utoa3_p(unsigned int number, char *buf)
{
	if (number < 10) {
		*buf = number + '0';
		*(buf + 1) = '\0';
		return buf + 1;
	} else if (number < 100) {
		*(buf + 0) = number / 10 % 10 + '0';
		*(buf + 1) = number % 10 + '0';
		*(buf + 2) = '\0';
		return buf + 2;
	} else {
		*(buf + 0) = number / 100 % 10 + '0';
		*(buf + 1) = number / 10 % 10 + '0';
		*(buf + 2) = number % 10 + '0';
		*(buf + 3) = '\0';
		return buf + 3;
	}
}

#	define PROC   "/proc/"
#	define STATUS "/status"
#	define PAGESZ 4096

#	define STATUS_NAME_START "Name:\t"

static int
read_process_exists_at(const char *process_name, unsigned int process_name_len, const char *pid_status_path)
{
	int fd = open(pid_status_path, O_RDONLY);
	if (fd == -1) {
		if (errno == ENOMEM)
			ERR(return 0);
		return 0;
	}
	char buf[PAGESZ];
	/* Read /proc/[pid]/status */
	ssize_t read_sz = read(fd, buf, PAGESZ);
	if (read_sz < 0)
		ERR(close(fd); return 0);
	int ret;
	ret = close(fd);
	if (ret == -1)
		ERR(return 0);
	buf[read_sz] = '\0';
	/* Find "Name: [process_name]\n" */
	const char *name_field = xstrstr_len(buf, (unsigned int)read_sz, STATUS_NAME_START, S_LEN(STATUS_NAME_START));
	if (name_field == NULL)
		return 0;
	/* bufp = "[process_name]\n" */
	name_field += S_LEN(STATUS_NAME_START);
	read_sz -= (name_field - buf);
	const char *name_field_e = (const char *)memchr(name_field, '\n', (unsigned int)read_sz);
	if (name_field_e == NULL)
		ERR(return 0);
	/* Check if matches wanted process. */
	if (process_name_len == (unsigned int)(name_field_e - name_field) && !memcmp(process_name, name_field, process_name_len))
		return 1;
	return 0;
}

static unsigned int
read_process_exists(const char *process_name, unsigned int process_name_len)
{
	char fname[S_LEN(PROC) + sizeof(unsigned int) * 8 + S_LEN(STATUS) + 1] = PROC;
	char *fnamep = fname + S_LEN(PROC);
	/* Open /proc/ */
	DIR *dp = opendir(PROC);
	if (dp == NULL)
		ERR(return 0);
	struct dirent *ep;
	while ((ep = readdir(dp))) {
		/* Enter /proc/[pid] */
		if (*(ep->d_name) == '.' || !xisdigit(*(ep->d_name)))
			continue;
		char *fname_e = fnamep;
		fname_e = xstpcpy(fname_e, ep->d_name);
		fname_e = xstpcpy_len(fname_e, S_LITERAL(STATUS));
		if (read_process_exists_at(process_name, process_name_len, fname))
			return (unsigned int)atoi(ep->d_name);
	}
	if (closedir(dp) == -1)
		ERR();
	return 0;
}

static char *
write_ram_usage_percent(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
#	ifdef __linux__
	int usage = read_ram_usage_percent();
	if (usage < 0)
		ERR(return dst);
	char *p = dst;
	p = utoa_p((unsigned int)usage, p);
	*p = '%';
	*(p + 1) = '\0';
	return p + 1;
	(void)dst_len;
	(void)unused;
	(void)interval;
#	else
	if (write_cmd(dst, dst_len, CMD_RAM_USAGE) != NULL)
		ERR(return dst);
#	endif
}

static char *
write_cpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	int usage = read_cpu_temp();
	if (usage < 0)
		ERR(return NULL);
	char *p = utoa_p((unsigned int)usage, dst);
	p = xstpcpy_len(p, S_LITERAL("°"));
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static char *
write_time(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	struct tm *tm = read_time();
	if (tm == NULL)
		ERR(return NULL);
	char *p = dst;
	/* Write hour */
	if (tm->tm_hour < 10)
		*p++ = '0';
	p = utoa_p((unsigned int)tm->tm_hour, p);
	*p++ = ':';
	if (tm->tm_min < 10)
		*p++ = '0';
	/* Write minutes */
	p = utoa_p((unsigned int)tm->tm_min, p);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static char *
write_date(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	struct tm *tm = read_time();
	if (tm == NULL)
		ERR(return NULL);
	char *p = dst;
	/* Write day */
	switch (tm->tm_wday) {
	case 0: p = xstpcpy_len(p, S_LITERAL("Sun, ")); break;
	case 1: p = xstpcpy_len(p, S_LITERAL("Mon, ")); break;
	case 2: p = xstpcpy_len(p, S_LITERAL("Tue, ")); break;
	case 3: p = xstpcpy_len(p, S_LITERAL("Wed, ")); break;
	case 4: p = xstpcpy_len(p, S_LITERAL("Thu, ")); break;
	case 5: p = xstpcpy_len(p, S_LITERAL("Thu, ")); break;
	case 6: p = xstpcpy_len(p, S_LITERAL("Fri, ")); break;
	}
	p = utoa_p((unsigned int)tm->tm_mday, p);
	*p++ = ' ';
	/* Write month */
	switch (tm->tm_mon) {
	case 0: p = xstpcpy_len(p, S_LITERAL("Jan ")); break;
	case 1: p = xstpcpy_len(p, S_LITERAL("Feb ")); break;
	case 2: p = xstpcpy_len(p, S_LITERAL("Mar ")); break;
	case 3: p = xstpcpy_len(p, S_LITERAL("Apr ")); break;
	case 4: p = xstpcpy_len(p, S_LITERAL("May ")); break;
	case 5: p = xstpcpy_len(p, S_LITERAL("Jun ")); break;
	case 6: p = xstpcpy_len(p, S_LITERAL("Jul ")); break;
	case 7: p = xstpcpy_len(p, S_LITERAL("Agu ")); break;
	case 8: p = xstpcpy_len(p, S_LITERAL("Sep ")); break;
	case 9: p = xstpcpy_len(p, S_LITERAL("Oct ")); break;
	case 10: p = xstpcpy_len(p, S_LITERAL("Nov ")); break;
	case 11: p = xstpcpy_len(p, S_LITERAL("Dec ")); break;
	}
	/* Write year */
	p = utoa_p((unsigned int)tm->tm_year + 1900, p);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	ifdef USE_NVML

static char *
write_gpu_temp(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	if (gc_gpu.init == 0)
		gpu_init();
	char *p = dst;
	unsigned int avg;
	if (gc_gpu.deviceCount == 1) {
		gc_gpu.ret = nvmlDeviceGetTemperature(gc_gpu.device[0], NVML_TEMPERATURE_GPU, (unsigned int *)gc_gpu.temp);
		/* FIXME: does not work. */
		/* gc_gpu.ret = nvmlDeviceGetTemperatureV(gc_gpu.device[i], gc_gpu.temp + i); */
		if (gc_gpu.ret != NVML_SUCCESS)
			ERR(gpu_err());
		avg = gc_gpu.temp[0];
	} else if (gc_gpu.deviceCount > 0) {
		avg = 0;
		for (unsigned int i = 0; i < gc_gpu.deviceCount; ++i) {
			gc_gpu.ret = nvmlDeviceGetTemperature(gc_gpu.device[i], NVML_TEMPERATURE_GPU, (unsigned int *)gc_gpu.temp + i);
			/* FIXME: does not work. */
			/* gc_gpu.ret = nvmlDeviceGetTemperatureV(gc_gpu.device[i], gc_gpu.temp + i); */
			if (gc_gpu.ret != NVML_SUCCESS)
				ERR(gpu_err());
			avg += *(gc_gpu.temp + i);
		}
		avg /= gc_gpu.deviceCount;
	} else {
		return p;
	}
	p = utoa_p(avg, p);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	endif

static char *
write_speaker_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	p = utoa_p((unsigned int)read_speaker_volume(), dst);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static char *
write_mic_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	int vol = read_speaker_volume();
	if (vol == -1)
		ERR(return dst);
	p = utoa_p((unsigned int)vol, dst);
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	define MIC_UNMUTED "🎤"
#	define MIC_MUTED   "🔇"

static char *
write_mic_muted(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	if (read_mic_muted()) {
		p = xstpcpy_len(dst, S_LITERAL(MIC_MUTED));
	} else {
		int vol = read_mic_volume();
		if (vol == -1)
			ERR();
		p = xstpcpy_len(p, S_LITERAL(MIC_UNMUTED));
		*p++ = ' ';
		p = utoa_p((unsigned int)vol, p);
	}
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#	define OBS_OPEN_ICON       "🎥 OBS"
#	define OBS_OPEN_INTERVAL   4
#	define OBS_RECORD_ICON     "🔴 Recording"
#	define OBS_RECORD_INTERVAL 2

static char *
write_obs(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval, const char *process_name, unsigned int process_name_len, unsigned int process_interval, const char *process_icon, unsigned int *pid_cache)
{
	/* Need to search /proc/[pid] for process. */
	if (*pid_cache == 0) {
		/* Cache the pid to avoid searching for next calls. */
		*pid_cache = read_process_exists(process_name, process_name_len);
		if (*pid_cache == 0) {
			/* OBS is not recording, but still on. Keep checking. */
			if (pid_cache == &gc_obs_recording_pid && gc_obs_open_pid)
				*interval = process_interval;
			/* OBS is closed. Stop checking. */
			else
				*interval = 0;
			return dst;
		}
	} else {
		/* Construct path: /proc/[pid]/status. */
		char fname[S_LEN(PROC) + sizeof(unsigned int) * 8 + S_LEN(STATUS) + 1];
		char *fnamep = fname;
		/* /proc/ */
		fnamep = xstpcpy_len(fnamep, S_LITERAL(PROC));
		/* /proc/[pid] */
		fnamep = utoa_p(*pid_cache, fnamep);
		/* /proc/[pid]/status */
		fnamep = xstpcpy_len(fnamep, S_LITERAL(STATUS));
		(void)fnamep;
		if (!read_process_exists_at(process_name, process_name_len, fname)) {
			*pid_cache = 0;
			*interval = 0;
			return dst;
		}
	}
	dst = xstpcpy(dst, process_icon);
	*interval = process_interval;
	return dst;
	(void)unused;
	(void)dst_len;
}

static char *
write_obs_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return write_obs(dst, dst_len, unused, interval, S_LITERAL("obs"), OBS_OPEN_INTERVAL, OBS_OPEN_ICON, &gc_obs_open_pid);
	(void)unused;
	(void)dst_len;
}

static char *
write_obs_recording(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	return write_obs(dst, dst_len, unused, interval, S_LITERAL("obs-ffmpeg-mux"), OBS_RECORD_INTERVAL, OBS_RECORD_ICON, &gc_obs_recording_pid);
	(void)unused;
	(void)dst_len;
}

static char *
write_webcam_on(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	int fd = open("/proc/modules", O_RDONLY);
	if (fd == -1)
		ERR();
	char buf[4096];
	ssize_t readsz;
	readsz = read(fd, buf, sizeof(buf));
	if (close(fd) == -1)
		ERR();
	if (readsz < 0)
		ERR();
	/* Check if webcam is running. */
	if (xstrstr_len(buf, sizeof(buf), S_LITERAL("uvcvideo")))
		dst = xstpcpy_len(dst, S_LITERAL("📸"));
	return dst;
}

#endif /* COMPONENTS_H */
