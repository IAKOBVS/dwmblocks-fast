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
#	include <errno.h>

#	ifdef __ASSERT_FUNCTION
#		define ASSERT_FUNC __ASSERT_FUNCTION
#	else
#		define ASSERT_FUNC __func__
#	endif

#	ifdef DEBUG
#		define DBG(x) x
#	else
#		define DBG(x)
#	endif

#	define DIE(x)                                  \
		do {                                    \
			if (errno)                      \
				perror("errno error:"); \
			assert(0);                      \
			x;                              \
		} while (0)

#	define DIE_DO(x)                               \
		do {                                    \
			x;                              \
			if (errno)                      \
				perror("errno error:"); \
			assert(0);                      \
		} while (0)

#	ifdef __glibc_has_builtin
#		define HAS_BUILTIN(name) __glibc_has_builtin(name)
#	elif defined __has_builtin
#		define HAS_BUILTIN(name) __has_builtin(name)
#	else
#		define HAS_BUILTIN(name) 0
#	endif /* has_builtin */

#	if defined __glibc_unlikely && defined __glibc_likely
#		define likely(x)   __glibc_likely(x)
#		define unlikely(x) __glibc_unlikely(x)
#	elif ((defined __GNUC__ && (__GNUC__ >= 3)) || defined __clang__) && HAS_BUILTIN(__builtin_expect)
#		define likely(x)   __builtin_expect((x), 1)
#		define unlikely(x) __builtin_expect((x), 0)
#	else
#		define likely(x)   (x)
#		define unlikely(x) (x)
#	endif /* unlikely */

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
#	define LEN(a)       (sizeof(a) / sizeof((a)[0]))

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

#	ifdef _POSIX_C_SOURCE
#		define HAVE_FILENO 1
#	endif

#	if XGLIBC_PREREQ(2, 10) && (_POSIX_C_SOURCE - 0) >= 200809L \
	|| defined _GNU_SOURCE
#		define HAVE_STPCPY 1
#	endif

#	ifdef _GNU_SOURCE
#		define HAVE_MEMMEM     1
#		define HAVE_MEMRCHR    1
#		define HAVE_STRCHRNUL  1
#		define HAVE_WMEMPCPY   1
#		define HAVE_MEMPCPY    1
#		define HAVE_STRCASESTR 1
#	endif

/* Unlocked-stdio wrappers removed: none were reachable (the
 * USE_UNLOCKED_IO* gates were never defined) and several referenced
 * nonexistent functions. Reintroduce deliberately if ever needed. */
#	define io_fileno(stream) fileno(stream)

#	ifdef __linux__
#		define HAVE_PROCFS   1
#		define HAVE_SYSINFO  1
#		define HAVE_SYSFS    1
#		include <linux/version.h>
#		if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#			define HAVE_PROCFS_PID_COMM 1
#		endif
#		if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
#			define HAVE_POWERCAP 1
#		endif
#	endif

#endif /* MACROS_H */
