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

#ifndef MACROS_H
#	define MACROS_H 1

#	include <stdio.h>
#	include <assert.h>

#	define ERR(x)                          \
		do {                            \
			perror("errno error:"); \
			assert(0);              \
			x;                      \
		} while (0)

#	define ERR_DO(x)                       \
		do {                            \
			perror("errno error:"); \
			x;                      \
			assert(0);              \
		} while (0)

#	ifndef ATTR_INLINE
#		ifdef __inline
#			define ATTR_INLINE __inline
#		elif (defined __cplusplus || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L))
#			define ATTR_INLINE inline
#		else
#			define ATTR_INLINE
#		endif
#	endif

#	ifdef __glibc_has_attribute
#		define HAS_ATTRIBUTE(attr) __glibc_has_attribute(attr)
#	elif defined __has_attribute
#		define HAS_ATTRIBUTE(attr) __has_attribute(attr)
#	else
#		define HAS_ATTRIBUTE(attr) 0
#	endif /* has_attribute */

#	ifdef __attribute_maybe_unused__
#		define ATTR_MAYBE_UNUSED __attribute_maybe_unused__
#	elif HAS_ATTRIBUTE(__unused__)
#		define ATTR_MAYBE_UNUSED __attribute__((__unused__))
#	else
#		define ATTR_MAYBE_UNUSED
#	endif

#	define MAX(x, y)    (((x) > (y)) ? (x) : (y))
#	define MIN(x, y)    (((x) < (y)) ? (x) : (y))
#	define S_LEN(s)     (sizeof(s) - 1)
#	define S_LITERAL(s) s, S_LEN(s)

#	ifdef __GLIBC_PREREQ
#		define XGLIBC_PREREQ(maj, min) __GLIBC_PREREQ(maj, min)
#	elif defined __GLIBC__
#		define XGLIBC_PREREQ(maj, min) ((__GLIBC__ << 16) + __GLIBC_MINOR__ >= ((maj) << 16) + (min))
#	endif

#	if (defined __GLIBC__ && (__GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ <= 19) && defined _BSD_SOURCE || defined _SVID_SOURCE) \
	|| (defined _POSIX_C_SOURCE && (_POSIX_C_SOURCE - 0) >= 2)
#		define HAVE_POPEN  1
#		define HAVE_PCLOSE 1
#	endif

#	if XGLIBC_PREREQ(2, 10) && (_POSIX_C_SOURCE - 0) >= 200809L \
	|| defined _GNU_SOURCE
#		define HAVE_STPCPY 1
#	endif

#	ifdef _GNU_SOURCE
#		define HAVE_MEMMEM            1
#		define HAVE_MEMRCHR           1
#		define HAVE_STRCHRNUL         1
#		define HAVE_FGETS_UNLOCKED    1
#		define HAVE_FPUTS_UNLOCKED    1
#		define HAVE_GETWC_UNLOCKED    1
#		define HAVE_GETWCHAR_UNLOCKED 1
#		define HAVE_FGETWC_UNLOCKED   1
#		define HAVE_FPUTWC_UNLOCKED   1
#		define HAVE_PUTWCHAR_UNLOCKED 1
#		define HAVE_FGETWS_UNLOCKED   1
#		define HAVE_FPUTWS_UNLOCKED   1
#		define HAVE_WMEMPCPY          1
#		define HAVE_MEMPCPY           1
#		define HAVE_STRCASESTR        1
#	endif

#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FREAD_UNLOCKED
#		define io_fread(ptr, size, n, stream) fread_unlocked(ptr, size, n, stream)
#	else
#		define io_fread(ptr, size, n, stream) fread(ptr, size, n, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETC_UNLOCKED
#		define io_fgetc(stream) fgetc_unlocked(stream)
#	else
#		define io_fgetc(stream) fgetc(stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETWS_UNLOCKED
#		define io_fgetws(ws, n, stream) fgetws_unlocked(ws, n, stream)
#	else
#		define io_fgetws(ws, n, stream) fgetws(ws, n, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETC_UNLOCKED
#		define getc(stream) getc_unlocked(stream)
#	else
#		define getc(stream) getc(stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETCHAR_UNLOCKED
#		define getchar() getchar_unlocked()
#	else
#		define getchar() getchar()
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETS_UNLOCKED
#		define io_fgets(s, n, stream) fgets_unlocked(s, n, stream)
#	else
#		define io_fgets(s, n, stream) fgets(s, n, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETWC_UNLOCKED
#		define getwc(stream) getwc_unlocked(stream)
#	else
#		define getwc(stream) getwc(stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETWCHAR_UNLOCKED
#		define getwchar() getwchar_unlocked()
#	else
#		define getwchar() getwchar()
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETWC_UNLOCKED
#		define io_fgetwc(stream) fgetwc_unlocked(stream)
#	else
#		define io_fgetwc(stream) fgetwc(stream)
#	endif

#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTWC_UNLOCKED
#		define io_fputwc(wc, stream) fputwc_unlocked(wc, stream)
#	else
#		define io_fputwc(wc, stream) fputwc(wc, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTWCHAR_UNLOCKED
#		define putwchar(wc) putwchar_unlocked(wc)
#	else
#		define putwchar(wc) putwchar(wc)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTS_UNLOCKED
#		define io_fputs(s, stream) fputs_unlocked(s, stream)
#	else
#		define io_fputs(s, stream) fputs(s, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTWS_UNLOCKED
#		define io_fputws(ws, stream) fputws_unlocked(ws, stream)
#	else
#		define io_fputws(ws, stream) fputws(ws, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTC_UNLOCKED
#		define putc(c, stream) putc_unlocked(c, stream)
#	else
#		define putc(c, stream) putc(c, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTCHAR_UNLOCKED
#		define putchar(c) putchar_unlocked(c)
#	else
#		define putchar(c) putchar(c)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FWRITE_UNLOCKED
#		define io_fwrite(ptr, size, n, stream) fwrite_unlocked(ptr, size, n, stream)
#	else
#		define io_fwrite(ptr, size, n, stream) fwrite(ptr, size, n, stream)
#	endif
#	if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTC_UNLOCKED
#		define io_fputc(c, stream) fputc_unlocked(c, stream)
#	else
#		define io_fputc(c, stream) fputc(c, stream)
#	endif

#	if USE_UNLOCKED_IO && HAVE_CLEARERR_UNLOCKED
#		define clearerr(stream) clearerr_unlocked(stream)
#	else
#		define clearerr(stream) clearerr(stream)
#	endif
#	if USE_UNLOCKED_IO && HAVE_FEOF_UNLOCKED
#		define io_feof(stream) feof_unlocked(stream)
#	else
#		define io_feof(stream) feof(stream)
#	endif
#	if USE_UNLOCKED_IO && HAVE_FERROR_UNLOCKED
#		define io_ferror(stream) ferror_unlocked(stream)
#	else
#		define io_ferror(stream) ferror(stream)
#	endif
#	if USE_UNLOCKED_IO && HAVE_FILENO_UNLOCKED
#		define io_fileno(stream) fileno_unlocked(stream)
#	else
#		define io_fileno(stream) fileno(stream)
#	endif
#	if USE_UNLOCKED_IO && HAVE_FFLUSH_UNLOCKED
#		define io_fflush(stream) fflush_unlocked(stream)
#	else
#		define io_fflush(stream) fflush(stream)
#	endif

#endif /* MACROS_H */
