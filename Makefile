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
ARCHFLAGS += -march=native

# X11 (comment to disable)
X11FLAGS += -lX11

# Alsa (comment to disable)
ALSAFLAGS += -lasound

# NVML (comment to disable)
NVMLLIB = /opt/cuda/lib64
NVMLFLAGS += -L$(NVMLLIB) -lnvidia-ml

# # FreeBSD (uncomment)
# FREEBSDFLAGS += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# OPENBSDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

################################################################################
# Variables
################################################################################
CFLAGS += $(ARCHFLAGS)
LDFLAGS += $(ALSAFLAGS) $(X11FLAGS) $(NVMLFLAGS) $(FREEBSDFLAGS) $(OPENBSDFLAGS)
PREFIX = /usr/local
CC = cc
CFLAGS += -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2
SRC = src
PROG = dwmblocks-fast
BIN = bin
HFILES = src/*.h
SCRIPTSBASE = dwmblocks-fast-*
PROG = $(BIN)/dwmblocks-fast
SCRIPTS = $(BIN)/$(SCRIPTSBASE)
CFGS = $(SRC)/blocks.h $(SRC)/config.h $(SRC)/components.h
################################################################################

all: options $(PROG) $(SCRIPTS)

options:
	@echo dwmblocks-fast build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

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

disable-nvidia: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_NVML.*1\)/\/*\1*\//; s/\(#.*define.*USE_NVIDIA.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@mv Makefile Makefile.bak
	@sed 's/^\(NVML.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-nvml: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_NVML.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@sed 's/^\(NVML.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-x11: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_X11.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@sed 's/^\(X11.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

disable-alsa: $(config)
	@mv src/config.h src/config.h.bak
	@sed 's/\(^#.*define.*USE_ALSA.*1\)/\/*\1*\//' src/config.h.bak > src/config.h
	@rm src/config.h.bak
	@sed 's/^\(ALSA.*\)/# \1/' Makefile.bak > Makefile
	@rm Makefile.bak

$(PROG): $(SRC)/main.c $(CFGS)
	mkdir -p $(BIN)
	$(CC) -o $@ $(SRC)/main.c $(CFLAGS) $(LDFLAGS)

$(SRC)/blocks.h:
	cp $(SRC)/blocks.def.h $@

$(SRC)/config.h:
	cp $(SRC)/config.def.h $@

$(SRC)/components.h:
	cp $(SRC)/components.def.h $@

$(SCRIPTS):
	@./updatesig $(BIN) scripts/$(SCRIPTSBASE)

clean:
	rm -f $(PROG) $(SCRIPTS)

install: $(PROG) $(SCRIPTS)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $^ $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/src/dwmblocks-fast
	cp -rf * $(DESTDIR)$(PREFIX)/src/dwmblocks-fast

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks-fast $(DESTDIR)$(PREFIX)/bin/$(SCRIPTSBASE)

.PHONY: all options clean install uninstall
