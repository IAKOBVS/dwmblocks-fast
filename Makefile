# SPDX-License-Identifier: ISC */
# Copyright 2020 torrinfail
# This file is part of dwmblocks-fast, derived from dwmblocks with
# modifications.
# 
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted, provided that
# the above copyright notice and this permission notice appear in all
# copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

PREFIX  := /usr/local
CC      := cc
CFLAGS  := -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2

# TODO: automate disabling flags

# Compile only for this architecture (comment to disable)
CFLAGS += -march=native

# X11 (comment to disable)
LDFLAGS += -lX11

# Alsa (comment to disable)
LDFLAGS += -lasound

# NVML (comment to disable)
NVMLLIB = /opt/cuda/lib64
LDFLAGS += -L$(NVMLLIB) -lnvidia-ml

# # FreeBSD (uncomment)
#LDFLAGS += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# LDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

all: options dwmblocks-fast

options:
	@echo dwmblocks-fast build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

dwmblocks-fast: dwmblocks-fast.c blocks.def.h blocks.h
	${CC} -o dwmblocks-fast dwmblocks-fast.c ${CFLAGS} ${LDFLAGS}

blocks.h:
	cp blocks.def.h $@

clean:
	rm -f *.o *.gch dwmblocks-fast

install: dwmblocks-fast
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwmblocks-fast ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwmblocks-fast

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwmblocks-fast

.PHONY: all options clean install uninstall
