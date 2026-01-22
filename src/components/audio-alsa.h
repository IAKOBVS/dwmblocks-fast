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

#ifndef C_AUDIO_ALSA_H
#	define C_AUDIO_ALSA_H 1

#	include "../config.h"

#	ifdef USE_ALSA
#	include <alsa/asoundlib.h>
#	include <alsa/asoundef.h>

#	include "../macros.h"
#	include "../utils.h"

#		define C_AUDIO_UNMUTED "🔉"
#		define C_AUDIO_MUTED   "🔇"
#		define C_MIC_UNMUTED   "🎤"
#		define C_MIC_MUTED     "🚫"
#		define C_PLAYBACK      1
#		define C_CAPTURE       2

typedef struct {
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
} c_audio_alsa_ty;
static c_audio_alsa_ty c_audio_alsa_speaker, c_audio_alsa_mic;

static void
c_audio_alsa_cleanup_one(c_audio_alsa_ty *audio_alsa)
{
	if (audio_alsa->handle)
		snd_mixer_close(audio_alsa->handle);
	snd_mixer_selem_id_free(audio_alsa->sid);
}

static void
c_audio_alsa_cleanup()
{
	c_audio_alsa_cleanup_one(&c_audio_alsa_speaker);
	c_audio_alsa_cleanup_one(&c_audio_alsa_mic);
}

static void
c_audio_alsa_err(void)
{
	fprintf(stderr, "%s\n\n", snd_strerror(c_audio_alsa_speaker.ret));
	fprintf(stderr, "%s\n\n", snd_strerror(c_audio_alsa_mic.ret));
	c_audio_alsa_cleanup();
}

static void
c_audio_alsa_init_one(c_audio_alsa_ty *audio_alsa, const char *card, const char *selem_name, int playback_or_capture)
{
	audio_alsa->card = card;
	audio_alsa->selem_name = selem_name;
	audio_alsa->playback_or_capture = playback_or_capture;
	snd_mixer_selem_id_malloc(&audio_alsa->sid);
	if (audio_alsa->sid == NULL)
		ERR();
	audio_alsa->ret = snd_mixer_open(&audio_alsa->handle, 0);
	if (audio_alsa->ret != 0)
		ERR(c_audio_alsa_err());
	audio_alsa->ret = snd_mixer_attach(audio_alsa->handle, audio_alsa->card);
	if (audio_alsa->ret != 0)
		ERR(c_audio_alsa_err());
	audio_alsa->ret = snd_mixer_selem_register(audio_alsa->handle, NULL, NULL);
	if (audio_alsa->ret != 0)
		ERR(c_audio_alsa_err());
	audio_alsa->ret = snd_mixer_load(audio_alsa->handle);
	if (audio_alsa->ret != 0)
		ERR(c_audio_alsa_err());
	snd_mixer_selem_id_set_index(audio_alsa->sid, 0);
	snd_mixer_selem_id_set_name(audio_alsa->sid, audio_alsa->selem_name);
	audio_alsa->elem = snd_mixer_find_selem(audio_alsa->handle, audio_alsa->sid);
	if (audio_alsa->elem == NULL)
		ERR(c_audio_alsa_err());
	if (playback_or_capture == C_PLAYBACK)
		snd_mixer_selem_get_playback_volume_range(audio_alsa->elem, &audio_alsa->min_vol, &audio_alsa->max_vol);
	else if (audio_alsa->playback_or_capture == C_CAPTURE)
		snd_mixer_selem_get_capture_volume_range(audio_alsa->elem, &audio_alsa->min_vol, &audio_alsa->max_vol);
	else
		ERR(c_audio_alsa_err());
	if (playback_or_capture == C_PLAYBACK)
		audio_alsa->has_mute = snd_mixer_selem_has_playback_switch(audio_alsa->elem);
	else if (audio_alsa->playback_or_capture == C_CAPTURE)
		audio_alsa->has_mute = snd_mixer_selem_has_capture_switch(audio_alsa->elem);
	else
		ERR(c_audio_alsa_err());
	audio_alsa->init = 1;
}

static void
c_audio_alsa_speaker_init(void)
{
	c_audio_alsa_init_one(&c_audio_alsa_speaker, "default", "Master", C_PLAYBACK);
}

static void
c_audio_alsa_mic_init(void)
{
	c_audio_alsa_init_one(&c_audio_alsa_mic, "default", "Capture", C_CAPTURE);
}

static void
c_audio_alsa_init()
{
	c_audio_alsa_speaker_init();
	c_audio_alsa_mic_init();
}

static int
c_read_audio_alsa_vol(c_audio_alsa_ty *audio_alsa)
{
	if (audio_alsa->init == 0)
		c_audio_alsa_init();
	audio_alsa->ret = snd_mixer_handle_events(audio_alsa->handle);
	if (audio_alsa->ret < 0)
		ERR(c_audio_alsa_err(); return -1);
	if (audio_alsa->playback_or_capture == C_PLAYBACK)
		audio_alsa->ret = snd_mixer_selem_get_playback_volume(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio_alsa->curr_vol);
	if (audio_alsa->playback_or_capture == C_CAPTURE)
		audio_alsa->ret = snd_mixer_selem_get_capture_volume(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio_alsa->curr_vol);
	if (audio_alsa->ret != 0)
		ERR(c_audio_alsa_err(); return -1);
	const int percent = (int)(100.0f * (audio_alsa->curr_vol - audio_alsa->min_vol) / (audio_alsa->max_vol - audio_alsa->min_vol));
	return percent;
}

static int
c_read_audio_alsa_muted(c_audio_alsa_ty *audio_alsa)
{
	if (audio_alsa->has_mute) {
		if (audio_alsa->init == 0)
			c_audio_alsa_speaker_init();
		audio_alsa->ret = snd_mixer_handle_events(audio_alsa->handle);
		if (audio_alsa->ret < 0)
			ERR(c_audio_alsa_err());
		int i = 1;
		if (audio_alsa->playback_or_capture == C_PLAYBACK)
			audio_alsa->ret = snd_mixer_selem_get_playback_switch(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else if (audio_alsa->playback_or_capture == C_CAPTURE)
			audio_alsa->ret = snd_mixer_selem_get_capture_switch(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else
			ERR(c_audio_alsa_err());
		if (audio_alsa->ret != 0)
			ERR(c_audio_alsa_err());
		return !i;
	}
	return 0;
}

static int
c_read_mic_vol(void)
{
	return c_read_audio_alsa_vol(&c_audio_alsa_mic);
}

static int
c_read_speaker_vol(void)
{
	return c_read_audio_alsa_vol(&c_audio_alsa_speaker);
}

static int
c_read_mic_muted(void)
{
	return c_read_audio_alsa_muted(&c_audio_alsa_mic);
}

static int
c_read_speaker_muted(void)
{
	return c_read_audio_alsa_muted(&c_audio_alsa_speaker);
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
	p = utoa_p((unsigned int)c_read_speaker_vol(), p);
	p = xstpcpy_len(p, S_LITERAL(UNIT_USAGE));
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
		int vol = c_read_mic_vol();
		if (vol == -1)
			ERR();
		p = xstpcpy_len(p, S_LITERAL(C_MIC_UNMUTED));
		*p++ = ' ';
		p = utoa_p((unsigned int)vol, p);
		p = xstpcpy_len(p, S_LITERAL(UNIT_USAGE));
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

#endif /* C_AUDIO_ALSA_H */
