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
# FREEBSDLDFLAGS += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# OPENBSDLDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

################################################################################
# Variables
################################################################################

CFLAGS += $(CFLAGS_OPTIMIZE)
LDFLAGS += $(LDFLAGS_OPTIMIZE) $(LDFLAGS_ALSA) $(LDFLAGS_X11) $(LDFLAGS_NVML) $(FREEBSDLDFLAGS) $(OPENBSDLDFLAGS)
PREFIX = /usr/local
CC = cc
CFLAGS += -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2
SRC = src
BIN = bin
HFILES = src/*.h
SCRIPTSBASE = dwmblocks-fast-*
PROG = $(BIN)/dwmblocks-fast
SCRIPTS = $(BIN)/$(SCRIPTSBASE)
CFGS = $(SRC)/config.h $(SRC)/components.h

OBJS =\
	./src/components/time.o\
	./src/components/ram.o\
	./src/components/obs.o\
	./src/components/webcam.o\
	./src/components/audio-alsa.o\
	./src/components/audio.o\
	./src/components/gpu.o\
	./src/components/cpu.o\
	./src/main.o

# Always recompile $(OBJS) if $(REQ) changed
REQ =\
	src/components/procfs.o\
	src/components/shell.o\
	src/macros.h\
	src/utils.h\
	src/config.h\
	src/components.h\

################################################################################
# Targets
################################################################################

all: options $(PROG) $(SCRIPTS)

.c.o:
	$(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $<

$(PROG): $(CFGS) $(OBJS) $(REQ)
	mkdir -p $(BIN)
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) $(OBJS) $(REQ) $(LDFLAGS)

$(OBJS): $(REQ)

$(SCRIPTS):
	@./updatesig $(BIN) scripts/$(SCRIPTSBASE)

$(SRC)/config.h:
	cp $(SRC)/config.def.h $@

$(SRC)/components.h:
	cp $(SRC)/components.def.h $@

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
	@echo 'disable-x11'
	@echo 'disable-alsa'
	@echo ''
	@echo 'For example, to disable NVML, run:'
	@echo 'make disable-nvml'


disable-nvml: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_NVML.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(NVML.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-nvidia: $(config) $(disable-nvml)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_NVIDIA.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(NVIDIA.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-x11: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_X11.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(X11.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-alsa: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_ALSA.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(ALSA.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-audio: $(config) $(disable-alsa)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_AUDIO.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@cp Makefile Makefile.bak
	@sed 's/^\(AUDIO.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

clean:
	rm -f $(PROG) $(SCRIPTS) $(OBJS)

install: $(PROG) $(SCRIPTS)
	strip $(PROG)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	command -v rsync >/dev/null && rsync -a -r -c $^ $(DESTDIR)$(PREFIX)/bin || cp -f $^ $(DESTDIR)$(PREFIX)/bin

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks-fast $(DESTDIR)$(PREFIX)/bin/$(SCRIPTSBASE)

.PHONY: all options clean install uninstall config
