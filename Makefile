PREFIX  := /usr/local
CC      := cc
CFLAGS  := -pedantic -Wall -Wextra -Wno-deprecated-declarations -O2 -march=native

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
