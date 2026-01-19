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

CFLAGS += $(ARCHFLAGS)
LDFLAGS += $(ALSAFLAGS) $(X11FLAGS) $(NVMLFLAGS) $(FREEBSDFLAGS) $(OPENBSDFLAGS)

PREFIX      = /usr/local
CC          = cc
CFLAGS      += -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2
SRC         = src
PROG        = dwmblocks-fast
BINDIR      = bin
SCRIPTS     = scripts/dwmblocks-fast-*
SCRIPTS_NEW = bin/dwmblocks-fast-*
HFILES      = src/*.h

all: options dwmblocks-fast

options:
	@echo dwmblocks-fast build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

dwmblocks-fast: $(SRC)/main.c $(SRC)/blocks.def.h $(SRC)/blocks.h $(SRC)/config.def.h $(SRC)/config.h $(SRC)/components.def.h $(SRC)/components.h
	mkdir -p $(BINDIR)
	$(CC) -o $(BINDIR)/dwmblocks-fast $(SRC)/main.c $(CFLAGS) $(LDFLAGS)

$(SRC)/blocks.h:
	cp $(SRC)/blocks.def.h $@

$(SRC)/config.h:
	cp $(SRC)/config.def.h $@

$(SRC)/components.h:
	cp $(SRC)/components.def.h $@

clean:
	rm -f $(SRC)/*.o $(SRC)/*/*.o $(SRC)/*.gch $(SRC)/*/*.gch $(BINDIR)/dwmblocks-fast

install: $(BINDIR)/dwmblocks-fast $(SCRIPTS_NEW)
	@./updatesig
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(BINDIR)/dwmblocks-fast $(SCRIPTS_NEW)
	cp -f $(BINDIR)/dwmblocks-fast $(SCRIPTS_NEW) $(DESTDIR)$(PREFIX)/bin

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks-fast

.PHONY: all options clean install uninstall
