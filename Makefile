# SPDX-License-Identifier: ISC
# Copyright 2020 torrinfail
# Copyright 2025-2026 James Tirta Halim <tirtajames45 at gmail com>
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

include config.mk

# Variables
CFLAGS = $(CFLAGS_OPTIMIZE) -fanalyzer -Wno-unknown-argument
LDFLAGS = $(LDFLAGS_OPTIMIZE) $(LDFLAGS_ALSA) $(LDFLAGS_X11) $(LDFLAGS_CUDA) $(LDFLAGS_FREEBSD) $(LDFLAGS_OPENBSD)
PREFIX = /usr/local
CC = cc
CFLAGS += -O2 -Wpedantic -pedantic -Wall -Wextra -Wuninitialized -Wshadow -Warray-bounds -Wnull-dereference -Wformat -Wunused -Wwrite-strings
SRC = .
BIN = bin
SCRIPTSBASE = dwmblocks-fast-*
PROG = dwmblocks-fast
PROG_BIN = $(BIN)/dwmblocks-fast
SCRIPTS = $(BIN)/$(SCRIPTSBASE)
INCLUDE = .
CONFIG = $(INCLUDE)/config.h
BLOCKS = $(INCLUDE)/blocks.h
CONFIG_DEF = $(INCLUDE)/config.def.h
BLOCKS_DEF = $(INCLUDE)/blocks.def.h
CPU_TEMP_GENERATED = $(INCLUDE)/cpu-temp-file.generated.h
CFGS = $(CONFIG) $(BLOCKS) $(CPU_TEMP_GENERATED)

OBJS =\
	$(SRC)/blocks/cat.o\
	$(SRC)/blocks/time.o\
	$(SRC)/blocks/ram.o\
	$(SRC)/blocks/disk.o\
	$(SRC)/blocks/obs.o\
	$(SRC)/blocks/webcam.o\
	$(SRC)/blocks/audio-alsa.o\
	$(SRC)/blocks/audio.o\
	$(SRC)/blocks/gpu-nvidia.o\
	$(SRC)/blocks/cpu.o

# Always recompile $(OBJS) if $(REQ) changed
REQ =\
	$(SRC)/blocks/temp.o\
	$(SRC)/blocks/procfs.o\
	$(SRC)/blocks/shell.o

REQ_H =\
	$(INCLUDE)/macros.h\
	$(INCLUDE)/utils.h\
	$(INCLUDE)/config.h\
	$(INCLUDE)/blocks.h\
	$(INCLUDE)/path.h

# Targets
all: options $(PROG_BIN) $(SCRIPTS)

check: $(PROG_BIN) $(SRC)/test.o
	mkdir -p $(BIN)
	$(CC) -o tests/test-run-bin $(CFLAGS) -g -fsanitize=address $(CPPFLAGS) $(SRC)/test.o $(OBJS) $(REQ) $(LDFLAGS)
	@./tests/test-run
	@rm $(SRC)/test.o

clean:
	rm -f $(PROG_BIN) $(SCRIPTS) $(OBJS) $(SRC)/*.o

install: $(PROG_BIN) $(SCRIPTS)
	strip $(PROG_BIN)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	command -v rsync >/dev/null && rsync -a -r -c $^ $(DESTDIR)$(PREFIX)/bin || cp -f $^ $(DESTDIR)$(PREFIX)/bin

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/$(PROG_BIN) $(DESTDIR)$(PREFIX)/bin/$(SCRIPTSBASE)

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
	@echo 'disable-cuda'
	@echo 'disable-x11'
	@echo 'disable-alsa'
	@echo ''
	@echo 'For example, to disable CUDA, run:'
	@echo 'make disable-cuda'

# Configuration scripts
disable-cuda: $(config) config.mk
	@# Comment out parts of the config.h and the config.mk
	@mv $(INCLUDE)/config.h $(INCLUDE)/config.h.bak
	@# Comment out USE_CUDA line
	@sed 's/\(^#.*define.*USE_CUDA.*1\)/\/* \1 *\//' $(INCLUDE)/config.h.bak > $(INCLUDE)/config.h
	@rm $(INCLUDE)/config.h.bak
	@cp config.mk config.mk.bak
	@# Comment out LDFLAGS_CUDA line
	@sed 's/^\(LDFLAGS_CUDA\)/# \1/' config.mk.bak > config.mk
	@rm config.mk.bak

disable-x11: $(config) config.mk
	@mv $(INCLUDE)/config.h $(INCLUDE)/config.h.bak
	@sed 's/\(^#.*define.*USE_X11.*1\)/\/* \1 *\//' $(INCLUDE)/config.h.bak > $(INCLUDE)/config.h
	@rm $(INCLUDE)/config.h.bak
	@cp config.mk config.mk.bak
	@sed 's/^\(LDFLAGS_X11.*\)/# \1/' config.mk.bak > config.mk
	@rm config.mk.bak

disable-alsa: $(config) config.mk
	@mv $(INCLUDE)/config.h $(INCLUDE)/config.h.bak
	@sed 's/\(^#.*define.*USE_ALSA.*1\)/\/* \1 *\//' $(INCLUDE)/config.h.bak > $(INCLUDE)/config.h
	@rm $(INCLUDE)/config.h.bak
	@cp config.mk config.mk.bak
	@sed 's/^\(LDFLAGS_ALSA.*\)/# \1/' config.mk.bak > config.mk
	@rm config.mk.bak

.c.o:
	$(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $<

$(SRC)/test.o: $(PROG_BIN)
	$(CC) -o $@ -c -DTEST=1 $(CFLAGS) $(CPPFLAGS) $(SRC)/$(PROG).c

$(PROG_BIN): $(CFGS) $(SRC)/$(PROG).o $(OBJS) $(REQ) $(REQ_H)
	mkdir -p $(BIN)
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) $(SRC)/$(PROG).o $(OBJS) $(REQ) $(LDFLAGS)

$(OBJS) $(SRC)/$(PROG).o $(SRC)/test.o: $(REQ) $(REQ_H)

$(SCRIPTS):
	@./updatesig $(BIN) scripts/$(SCRIPTSBASE)

$(CONFIG):
	cp $(CONFIG_DEF) $@

$(BLOCKS):
	cp $(BLOCKS_DEF) $@

config.mk:
	cp config.def.mk $@

$(CPU_TEMP_GENERATED):
	./getcpufile > $@
	
.PHONY: all options clean install uninstall config check
