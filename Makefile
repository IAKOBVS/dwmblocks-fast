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
SRC     := src
MAIN    := main
PROG    := dwmblocks-fast
BINDIR  := bin
SCRIPTS := scripts/dwmblocks-fast-*

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

all: options ${PROG}

options:
	@echo ${PROG} build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

${PROG}: ${SRC}/${MAIN}.c ${SRC}/blocks.def.h ${SRC}/blocks.h ${SRC}/config.def.h ${SRC}/config.h ${SRC}/components.def.h ${SRC}/components.h
	${CC} -o ${BINDIR}/${PROG} ${SRC}/${MAIN}.c ${CFLAGS} ${LDFLAGS}
	./updatesig

blocks.h:
	cp ${SRC}/blocks.def.h $@

config.h:
	cp ${SRC}/config.def.h $@

components.h:
	cp ${SRC}/components.def.h $@

clean:
	rm -f ${SRC}/*.o ${SRC}/*.gch ${PROG}

SCRIPTS_OUT := ./bin/dwmblocks-fast-*
SCRIPTS_BASE := dwmblocks-fast-*

install: ${BINDIR}/${PROG} ${SCRIPTS_OUT}
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BINDIR}/${PROG} ${SCRIPTS_OUT} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${PROG} ${DESTDIR}${PREFIX}/bin/${SCRIPTS_BASE}

uninstall: 
	rm -f ${DESTDIR}${PREFIX}/bin/${PROG}

.PHONY: all options clean install uninstall
