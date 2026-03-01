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

#ifdef USE_ALSA
#	include <alsa/asoundlib.h>
#	include <alsa/asoundef.h>

#	include "../macros.h"

#	define B_AUDIO_ALSA_PLAYBACK 1
#	define B_AUDIO_ALSA_CAPTURE  2

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
	int last_vol;
	int last_muted;
} b_audio_alsa_ty;
b_audio_alsa_ty b_audio_alsa_speaker, b_audio_alsa_mic;

void
b_audio_alsa_cleanup_one(b_audio_alsa_ty *audio_alsa)
{
	if (audio_alsa->handle)
		snd_mixer_close(audio_alsa->handle);
	snd_mixer_selem_id_free(audio_alsa->sid);
}

void
b_audio_alsa_cleanup(void)
{
	b_audio_alsa_cleanup_one(&b_audio_alsa_speaker);
	b_audio_alsa_cleanup_one(&b_audio_alsa_mic);
}

void
b_audio_alsa_err(void)
{
	fprintf(stderr, "alsa error (speaker): %s\n", snd_strerror(b_audio_alsa_speaker.ret));
	fprintf(stderr, "alsa error (mic): %s\n", snd_strerror(b_audio_alsa_mic.ret));
	b_audio_alsa_cleanup();
}

void
b_audio_alsa_init_internal(b_audio_alsa_ty *audio_alsa, const char *card, const char *selem_name, int playback_or_capture)
{
	if (audio_alsa->init)
		return;
	snd_mixer_selem_id_malloc(&audio_alsa->sid);
	if (audio_alsa->sid == NULL)
		DIE();
	audio_alsa->ret = snd_mixer_open(&audio_alsa->handle, 0);
	if (unlikely(audio_alsa->ret != 0))
		DIE_DO(b_audio_alsa_err());
	audio_alsa->ret = snd_mixer_attach(audio_alsa->handle, card);
	if (unlikely(audio_alsa->ret != 0))
		DIE_DO(b_audio_alsa_err());
	audio_alsa->ret = snd_mixer_selem_register(audio_alsa->handle, NULL, NULL);
	if (unlikely(audio_alsa->ret != 0))
		DIE_DO(b_audio_alsa_err());
	audio_alsa->ret = snd_mixer_load(audio_alsa->handle);
	if (unlikely(audio_alsa->ret != 0))
		DIE_DO(b_audio_alsa_err());
	snd_mixer_selem_id_set_index(audio_alsa->sid, 0);
	snd_mixer_selem_id_set_name(audio_alsa->sid, audio_alsa->selem_name);
	audio_alsa->elem = snd_mixer_find_selem(audio_alsa->handle, audio_alsa->sid);
	if (audio_alsa->elem == NULL)
		DIE_DO(fprintf(stderr, "alsa error: %s not found\n", selem_name));
	if (playback_or_capture == B_AUDIO_ALSA_PLAYBACK)
		snd_mixer_selem_get_playback_volume_range(audio_alsa->elem, &audio_alsa->min_vol, &audio_alsa->max_vol);
	else if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_CAPTURE)
		snd_mixer_selem_get_capture_volume_range(audio_alsa->elem, &audio_alsa->min_vol, &audio_alsa->max_vol);
	else
		DIE_DO(b_audio_alsa_err());
	if (playback_or_capture == B_AUDIO_ALSA_PLAYBACK)
		audio_alsa->has_mute = snd_mixer_selem_has_playback_switch(audio_alsa->elem);
	else if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_CAPTURE)
		audio_alsa->has_mute = snd_mixer_selem_has_capture_switch(audio_alsa->elem);
	else
		DIE_DO(b_audio_alsa_err());
	audio_alsa->init = 1;
}

void
b_audio_alsa_init(b_audio_alsa_ty *audio_alsa)
{
	audio_alsa->card = "default";
	if (audio_alsa == &b_audio_alsa_speaker)  {
		audio_alsa->selem_name = "Master";
		audio_alsa->playback_or_capture = B_AUDIO_ALSA_PLAYBACK;
	} else if (audio_alsa == &b_audio_alsa_mic) {
		audio_alsa->selem_name = "Capture";
		audio_alsa->playback_or_capture = B_AUDIO_ALSA_CAPTURE;
	} else {
		DIE();
	}
	b_audio_alsa_init_internal(audio_alsa, audio_alsa->card, audio_alsa->selem_name, audio_alsa->playback_or_capture);
}

int
b_read_audio_alsa_vol(b_audio_alsa_ty *audio_alsa)
{
	if (unlikely(audio_alsa->init == 0)) {
		b_audio_alsa_init(audio_alsa);
		audio_alsa->init = 1;
	}
	audio_alsa->ret = snd_mixer_handle_events(audio_alsa->handle);
	if (audio_alsa->ret < 0)
		DIE_DO(b_audio_alsa_err());
	if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_PLAYBACK)
		audio_alsa->ret = snd_mixer_selem_get_playback_volume(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio_alsa->curr_vol);
	if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_CAPTURE)
		audio_alsa->ret = snd_mixer_selem_get_capture_volume(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &audio_alsa->curr_vol);
	if (unlikely(audio_alsa->ret != 0))
		DIE_DO(b_audio_alsa_err());
	const int percent = (int)((double)100 * ((double)(audio_alsa->curr_vol - audio_alsa->min_vol) / (double)(audio_alsa->max_vol - audio_alsa->min_vol)));
	return percent;
}

int
b_read_audio_alsa_muted(b_audio_alsa_ty *audio_alsa)
{
	if (audio_alsa->has_mute) {
		if (unlikely(audio_alsa->init == 0)) {
			b_audio_alsa_init(audio_alsa);
			audio_alsa->init = 1;
		}
		audio_alsa->ret = snd_mixer_handle_events(audio_alsa->handle);
		if (audio_alsa->ret < 0)
			DIE_DO(b_audio_alsa_err());
		int i = 1;
		if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_PLAYBACK)
			audio_alsa->ret = snd_mixer_selem_get_playback_switch(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else if (audio_alsa->playback_or_capture == B_AUDIO_ALSA_CAPTURE)
			audio_alsa->ret = snd_mixer_selem_get_capture_switch(audio_alsa->elem, SND_MIXER_SCHN_FRONT_LEFT, &i);
		else
			DIE_DO(b_audio_alsa_err());
		if (unlikely(audio_alsa->ret != 0))
			DIE_DO(b_audio_alsa_err());
		return !i;
	}
	return 0;
}

int
b_read_mic_vol(void)
{
	return b_read_audio_alsa_vol(&b_audio_alsa_mic);
}

int
b_read_speaker_vol(void)
{
	return b_read_audio_alsa_vol(&b_audio_alsa_speaker);
}

int
b_read_mic_muted(void)
{
	return b_read_audio_alsa_muted(&b_audio_alsa_mic);
}

int
b_read_speaker_muted(void)
{
	return b_read_audio_alsa_muted(&b_audio_alsa_speaker);
}

#	undef B_AUDIO_ALSA_PLAYBACK
#	undef B_AUDIO_ALSA_CAPTURE

#endif
