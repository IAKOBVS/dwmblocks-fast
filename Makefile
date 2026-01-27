# SPDX-License-Identifier: ISC
# Copyright 2020 torrinfail
# Copyright 2026 James Tirta Halim <tirtajames45 at gmail com>
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
.POSIX:

################################################################################
# User configuration
################################################################################

# Compile only for this architecture (comment to disable)
CFLAGS_OPTIMIZE += -march=native

# Link-time optimizations (comment to disable)
LDFLAGS_OPTIMIZE += -flto

# X11 (comment to disable)
LDFLAGS_X11 += -lX11

# Alsa (comment to disable)
LDFLAGS_ALSA += -lasound

# NVML (comment to disable)
LIB_NVML = /opt/cuda/lib64
LDFLAGS_NVML += -L$(LIB_NVML) -lnvidia-ml

# # FreeBSD (uncomment)
# LDFLAGS_FREEBSD += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# LDFLAGS_OPENBSD += -L/usr/X11R6/lib -I/usr/X11R6/include

################################################################################
# Variables
################################################################################

CFLAGS += $(CFLAGS_OPTIMIZE)
LDFLAGS += $(LDFLAGS_OPTIMIZE) $(LDFLAGS_ALSA) $(LDFLAGS_X11) $(LDFLAGS_NVML) $(LDFLAGS_FREEBSD) $(LDFLAGS_OPENBSD)
PREFIX = /usr/local
CC = cc
CFLAGS += -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2
SRC = src
BIN = bin
HFILES = src/*.h
SCRIPTSBASE = dwmblocks-fast-*
PROG = $(BIN)/dwmblocks-fast
SCRIPTS = $(BIN)/$(SCRIPTSBASE)
CFGS = include/config.h include/blocks.h

OBJS =\
	./src/blocks/time.o\
	./src/blocks/ram.o\
	./src/blocks/obs.o\
	./src/blocks/webcam.o\
	./src/blocks/audio-alsa.o\
	./src/blocks/audio.o\
	./src/blocks/gpu.o\
	./src/blocks/cpu.o

# Always recompile $(OBJS) if $(REQ) changed
REQ =\
	src/blocks/procfs.o\
	src/blocks/shell.o\
	include/macros.h\
	include/utils.h\
	include/config.h\
	include/blocks.h

################################################################################
# Targets
################################################################################

all: options $(PROG) $(SCRIPTS)

check: $(PROG) src/test.o
	mkdir -p $(BIN)
	$(CC) -o tests/dwmblocks-fast-test $(CFLAGS) $(CPPFLAGS) src/test.o $(OBJS) $(REQ) $(LDFLAGS)
	@./tests/test-run
	@rm src/test.o

clean:
	rm -f $(PROG) $(SCRIPTS) $(OBJS) src/*.o

install: $(PROG) $(SCRIPTS)
	strip $(PROG)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	command -v rsync >/dev/null && rsync -a -r -c $^ $(DESTDIR)$(PREFIX)/bin || cp -f $^ $(DESTDIR)$(PREFIX)/bin

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks-fast $(DESTDIR)$(PREFIX)/bin/$(SCRIPTSBASE)

options:
	@echo dwmblocks-fast build options:
	@echo "CPPFLAGS = $(CPPFLAGS)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

config: $(CFGS)
	@echo 'Automated configuration:'
	@echo 'Usage: make [OPTION]...'
	@echo ''
	@echo 'disable-nvidia'
	@echo 'disable-nvml'
	@echo 'disable-cuda (equal to disable-nvml)'
	@echo 'disable-x11'
	@echo 'disable-alsa'
	@echo ''
	@echo 'For example, to disable NVML, run:'
	@echo 'make disable-nvml'

################################################################################
# Configuration scripts
################################################################################

# Comment out parts of the config.h and the Makefile
disable-nvml: $(config)
	@mv include/config.h include/config.h.bak
	# Comment out USE_NVML line
	@sed 's/\(^#.*define.*USE_NVML.*1\)/\/*\1*\//' include/config.h.bak > include/config.h
	@rm include/config.h.bak
	@cp Makefile Makefile.bak
	# Comment out LDFLAGS_NVML line
	@sed 's/^\(LDFLAGS_NVML.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-cuda: disable-nvml

disable-nvidia: $(config) $(disable-nvml)
	@mv include/config.h include/config.h.bak
	@sed 's/\(^#.*define.*USE_NVIDIA.*1\)/\/*\1*\//' include/config.h.bak > include/config.h
	@rm include/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(LDFLAGS_NVIDIA.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-x11: $(config)
	@mv include/config.h include/config.h.bak
	@sed 's/\(^#.*define.*USE_X11.*1\)/\/*\1*\//' include/config.h.bak > include/config.h
	@rm include/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(LDFLAGS_X11.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-alsa: $(config)
	@mv include/config.h include/config.h.bak
	@sed 's/\(^#.*define.*USE_ALSA.*1\)/\/*\1*\//' include/config.h.bak > include/config.h
	@rm include/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(LDFLAGS_ALSA.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-audio: $(config) $(disable-alsa)
	@mv include/config.h include/config.h.bak
	@sed 's/\(^#.*define.*USE_AUDIO.*1\)/\/*\1*\//' include/config.h.bak > include/config.h
	@rm include/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(LDFLAGS_AUDIO.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

################################################################################

.c.o:
	$(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $<

src/test.o: $(PROG)
	$(CC) -o $@ -c -DTEST=1 $(CFLAGS) $(CPPFLAGS) src/main.c

$(PROG): $(CFGS) src/main.o $(OBJS) $(REQ)
	mkdir -p $(BIN)
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) src/main.o $(OBJS) $(REQ) $(LDFLAGS)

$(OBJS) src/main.o src/test.o: $(REQ)

$(SCRIPTS):
	@./updatesig $(BIN) scripts/$(SCRIPTSBASE)

include/config.h:
	cp include/config.def.h $@

include/blocks.h:
	cp include/blocks.def.h $@

.PHONY: all options clean install uninstall config check
