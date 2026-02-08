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
#	include "../blocks/audio-alsa.h"
#	include "../utils.h"
#	include "../config.h"

char *
b_write_speaker_vol(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	char *p = dst;
	if (!b_read_speaker_muted()) {
		p = u_stpcpy_len(p, S_LITERAL(ICON_AUDIO_SPEAKER_ON));
	} else {
		p = u_stpcpy_len(p, S_LITERAL(ICON_AUDIO_SPEAKER_OFF));
	}
	*p++ = ' ';
	p = u_utoa_le3_p((unsigned int)b_read_speaker_vol(), p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

char *
b_write_mic_vol(char *dst, unsigned int dst_size, const char *unused, unsigned int *interval)
{
	char *p = dst;
	if (!b_read_mic_muted()) {
		p = u_stpcpy_len(p, S_LITERAL(ICON_AUDIO_MIC_ON));
	} else {
		if (S_LEN(ICON_AUDIO_MIC_OFF) == 0)
			return dst;
		p = u_stpcpy_len(p, S_LITERAL(ICON_AUDIO_MIC_OFF));
	}
	int vol = b_read_mic_vol();
	if (unlikely(vol == -1))
		DIE(return dst);
	*p++ = ' ';
	p = u_utoa_le3_p((unsigned int)vol, p);
	return p;
	(void)dst_size;
	(void)unused;
	(void)interval;
}

#endif
