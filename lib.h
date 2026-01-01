#ifndef LIB_H
#define LIB_H 1

#include <string.h>

#include "config.h"

#define S_LEN(s) (sizeof(s) - 1)
#define S_LITERAL(s) s, S_LEN(s)

#ifdef __GLIBC_PREREQ
#	define XGLIBC_PREREQ(maj, min) __GLIBC_PREREQ(maj, min)
#elif defined __GLIBC__
#	define XGLIBC_PREREQ(maj, min) ((__GLIBC__ << 16) + __GLIBC_MINOR__ >= ((maj) << 16) + (min))
#endif

#if XGLIBC_PREREQ(2, 10) && (_POSIX_C_SOURCE - 0) >= 200809L \
|| defined _GNU_SOURCE
#	define XHAVE_STPCPY 1
#endif

#ifdef _GNU_SOURCE
#	define HAVE_MEMMEM            1
#	define HAVE_MEMRCHR           1
#	define HAVE_STRCHRNUL         1
#	define HAVE_FGETS_UNLOCKED    1
#	define HAVE_FPUTS_UNLOCKED    1
#	define HAVE_GETWC_UNLOCKED    1
#	define HAVE_GETWCHAR_UNLOCKED 1
#	define HAVE_FGETWC_UNLOCKED   1
#	define HAVE_FPUTWC_UNLOCKED   1
#	define HAVE_PUTWCHAR_UNLOCKED 1
#	define HAVE_FGETWS_UNLOCKED   1
#	define HAVE_FPUTWS_UNLOCKED   1
#	define HAVE_WMEMPCPY          1
#	define HAVE_MEMPCPY           1
#	define HAVE_STRCASESTR        1
#endif

#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FREAD_UNLOCKED
#	define fread(ptr, size, n, stream) fread_unlocked(ptr, size, n, stream)
#else
#	define fread(ptr, size, n, stream) fread(ptr, size, n, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETC_UNLOCKED
#	define fgetc(stream) fgetc_unlocked(stream)
#else
#	define fgetc(stream) fgetc(stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETWS_UNLOCKED
#	define fgetws(ws, n, stream) fgetws_unlocked(ws, n, stream)
#else
#	define fgetws(ws, n, stream) fgetws(ws, n, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETC_UNLOCKED
#	define getc(stream) getc_unlocked(stream)
#else
#	define getc(stream) getc(stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETCHAR_UNLOCKED
#	define getchar() getchar_unlocked()
#else
#	define getchar() getchar()
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETS_UNLOCKED
#	define fgets(s, n, stream) fgets_unlocked(s, n, stream)
#else
#	define fgets(s, n, stream) fgets(s, n, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETWC_UNLOCKED
#	define getwc(stream) getwc_unlocked(stream)
#else
#	define getwc(stream) getwc(stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_GETWCHAR_UNLOCKED
#	define getwchar() getwchar_unlocked()
#else
#	define getwchar() getwchar()
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_READ) && HAVE_FGETWC_UNLOCKED
#	define fgetwc(stream) fgetwc_unlocked(stream)
#else
#	define fgetwc(stream) fgetwc(stream)
#endif

#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTWC_UNLOCKED
#	define fputwc(wc, stream) fputwc_unlocked(wc, stream)
#else
#	define fputwc(wc, stream) fputwc(wc, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTWCHAR_UNLOCKED
#	define putwchar(wc) putwchar_unlocked(wc)
#else
#	define putwchar(wc) putwchar(wc)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTS_UNLOCKED
#	define fputs(s, stream) fputs_unlocked(s, stream)
#else
#	define fputs(s, stream) fputs(s, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTWS_UNLOCKED
#	define fputws(ws, stream) fputws_unlocked(ws, stream)
#else
#	define fputws(ws, stream) fputws(ws, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTC_UNLOCKED
#	define putc(c, stream) putc_unlocked(c, stream)
#else
#	define putc(c, stream) putc(c, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_PUTCHAR_UNLOCKED
#	define putchar(c) putchar_unlocked(c)
#else
#	define putchar(c) putchar(c)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FWRITE_UNLOCKED
#	define fwrite(ptr, size, n, stream) fwrite_unlocked(ptr, size, n, stream)
#else
#	define fwrite(ptr, size, n, stream) fwrite(ptr, size, n, stream)
#endif
#if (USE_UNLOCKED_IO || USE_UNLOCKED_IO_WRITE) && HAVE_FPUTC_UNLOCKED
#	define fputc(c, stream) fputc_unlocked(c, stream)
#else
#	define fputc(c, stream) fputc(c, stream)
#endif

#if USE_UNLOCKED_IO && HAVE_CLEARERR_UNLOCKED
#	define clearerr(stream) clearerr_unlocked(stream)
#else
#	define clearerr(stream) clearerr(stream)
#endif
#if USE_UNLOCKED_IO && HAVE_FEOF_UNLOCKED
#	define feof(stream) feof_unlocked(stream)
#else
#	define feof(stream) feof(stream)
#endif
#if USE_UNLOCKED_IO && HAVE_FERROR_UNLOCKED
#	define ferror(stream) ferror_unlocked(stream)
#else
#	define ferror(stream) ferror(stream)
#endif
#if USE_UNLOCKED_IO && HAVE_FILENO_UNLOCKED
#	define fileno(stream) fileno_unlocked(stream)
#else
#	define fileno(stream) fileno(stream)
#endif
#if USE_UNLOCKED_IO && HAVE_FFLUSH_UNLOCKED
#	define fflush(stream) fflush_unlocked(stream)
#else
#	define fflush(stream) fflush(stream)
#endif

static int
xisdigit(int c)
{
	return ((unsigned)c - '0' < 10);
}

static char *
xstpcpy_len(char *dst, const char *src, size_t n)
{
#ifdef XHAVE_MEMPCPY 
	dst = (char *)mempcpy(dst, src, n);
	*dst = '\0';
	return dst;
#else
	dst = (char *)memcpy(dst, src, n) + n;
	*dst = '\0';
	return dst;
#endif
}

static char *
xstpcpy(char *dst, const char *src)
{
#ifdef XHAVE_STPCPY
	return stpcpy(dst, src);
#else
	size_t n = strlen(src);
	dst = (char *)memcpy(dst, src, n) + n;
	*dst = '\0';
	return dst;
#endif
}

static char *
xstrstr_len(const char *hs, size_t hs_len, const char *ne, size_t ne_len)
{
#ifdef XHAVE_STPCPY
	return (char *)memmem(hs, hs_len, ne, ne_len);
#else
	return strstr((char *)hs, (char *)ne);
	(void)hs_len;
	(void)ne_len;
#endif
}

#endif /* LIB_H */
