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

#ifndef C_AUDIO_H
#	define C_AUDIO_H 1

#	include "../macros.h"
#	include "../utils.h"
#	include "../config.h"
#	include "audio-lib.h"

#	if USE_ALSA

#		define C_AUDIO_UNMUTED "🔉"
#		define C_AUDIO_MUTED   "🔇"
#		define C_MIC_UNMUTED   "🎤"
#		define C_MIC_MUTED     "🚫"
#		define C_PLAYBACK      1
#		define C_CAPTURE       2

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
} c_audio_speaker, c_audio_mic;

static void
c_audio_cleanup_one(struct Audio *audio)
{
	if (audio->handle)
		snd_mixer_close(audio->handle);
	snd_mixer_selem_id_free(audio->sid);
}

static void
c_audio_cleanup()
{
	c_audio_cleanup_one(&c_audio_speaker);
	c_audio_cleanup_one(&c_audio_mic);
}

static void
c_audio_err(void)
{
	fprintf(stderr, "%s\n\n", snd_strerror(c_audio_speaker.ret));
	fprintf(stderr, "%s\n\n", snd_strerror(c_audio_mic.ret));
	c_audio_cleanup();
}

static void
c_audio_init_one(struct Audio *audio, const char *card, const char *selem_name, int playback_or_capture)
{
	audio->card = card;
	audio->selem_name = selem_name;
	audio->playback_or_capture = playback_or_capture;
	snd_mixer_selem_id_malloc(&audio->sid);
	if (audio->sid == NULL)
		ERR();
	audio->ret = snd_mixer_open(&audio->handle, 0);
	if (audio->ret != 0)
		ERR(c_audio_err());
	audio->ret = snd_mixer_attach(audio->handle, audio->card);
	if (audio->ret != 0)
		ERR(c_audio_err());
	audio->ret = snd_mixer_selem_register(audio->handle, NULL, NULL);
	if (audio->ret != 0)
		ERR(c_audio_err());
	audio->ret = snd_mixer_load(audio->handle);
	if (audio->ret != 0)
		ERR(c_audio_err());
	snd_mixer_selem_id_set_index(audio->sid, 0);
	snd_mixer_selem_id_set_name(audio->sid, audio->selem_name);
	audio->elem = snd_mixer_find_selem(audio->handle, audio->sid);
	if (audio->elem == NULL)
		ERR(c_audio_err());
	if (playback_or_capture == C_PLAYBACK)
		snd_mixer_selem_get_playback_volume_range(audio->elem, &audio->min_vol, &audio->max_vol);
	else if (audio->playback_or_capture == C_CAPTURE)
		snd_mixer_selem_get_capture_volume_range(audio->elem, &audio->min_vol, &audio->max_vol);
	else
		ERR(c_audio_err());
	if (playback_or_capture == C_PLAYBACK)
		audio->has_mute = snd_mixer_selem_has_playback_switch(audio->elem);
	else if (audio->playback_or_capture == C_CAPTURE)
		audio->has_mute = snd_mixer_selem_has_capture_switch(audio->elem);
	else
		ERR(c_audio_err());
	audio->init = 1;
}

static void
c_audio_speaker_init(void)
{
	c_audio_init_one(&c_audio_speaker, "default", "Master", C_PLAYBACK);
}

static void
c_audio_mic_init(void)
{
	c_audio_init_one(&c_audio_mic, "default", "Capture", C_CAPTURE);
}

static void
c_audio_init()
{
	c_audio_speaker_init();
	c_audio_mic_init();
}

static int
c_read_audio_volume(struct Audio *audio)
{
	if (audio->init == 0)
		c_audio_init();
	audio->ret = snd_mixer_handle_events(audio->handle);
	if (audio->ret < 0)
		ERR(c_audio_err(); return -1);
	if (audio->playback_or_capture == C_PLAYBACK)
		audio->ret = snd_mixer_selem_get_playback_volume(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio->curr_vol);
	if (audio->playback_or_capture == C_CAPTURE)
		audio->ret = snd_mixer_selem_get_capture_volume(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio->curr_vol);
	if (audio->ret != 0)
		ERR(c_audio_err(); return -1);
	const int percent = (int)(100.0f * (audio->curr_vol - audio->min_vol) / (audio->max_vol - audio->min_vol));
	return percent;
}

static int
c_read_audio_muted(struct Audio *audio)
{
	if (audio->has_mute) {
		if (audio->init == 0)
			c_audio_speaker_init();
		audio->ret = snd_mixer_handle_events(audio->handle);
		if (audio->ret < 0)
			ERR(c_audio_err());
		int i = 1;
		if (audio->playback_or_capture == C_PLAYBACK)
			audio->ret = snd_mixer_selem_get_playback_switch(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else if (audio->playback_or_capture == C_CAPTURE)
			audio->ret = snd_mixer_selem_get_capture_switch(audio->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else
			ERR(c_audio_err());
		if (audio->ret != 0)
			ERR(c_audio_err());
		return !i;
	}
	return 0;
}

static int
c_read_mic_volume(void)
{
	return c_read_audio_volume(&c_audio_mic);
}

static int
c_read_speaker_volume(void)
{
	return c_read_audio_volume(&c_audio_speaker);
}

static int
c_read_mic_muted(void)
{
	return c_read_audio_muted(&c_audio_mic);
}

static int
c_read_speaker_muted(void)
{
	return c_read_audio_muted(&c_audio_speaker);
}

static char *
c_write_speaker_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	if (!c_read_speaker_muted())
		p = xstpcpy_len(p, S_LITERAL(C_AUDIO_UNMUTED));
	else
		p = xstpcpy_len(p, S_LITERAL(C_AUDIO_MUTED));
	*p++ = ' ';
	p = utoa_p((unsigned int)c_read_speaker_volume(), p);
	*p++ = '%';
	*p = '\0';
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

static char *
c_write_mic_vol(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
	char *p = dst;
	if (c_read_mic_muted()) {
		p = xstpcpy_len(p, S_LITERAL(C_MIC_MUTED));
	} else {
		int vol = c_read_mic_volume();
		if (vol == -1)
			ERR();
		p = xstpcpy_len(p, S_LITERAL(C_MIC_UNMUTED));
		*p++ = ' ';
		p = utoa_p((unsigned int)vol, p);
		*p++ = '%';
		*p = '\0';
	}
	return p;
	(void)dst_len;
	(void)unused;
	(void)interval;
}

#		undef C_MIC_UNMUTED
#		undef C_MIC_MUTED
#		undef C_PLAYBACK
#		undef C_CAPTURE

#	endif

#endif /* C_AUDIO_H */
