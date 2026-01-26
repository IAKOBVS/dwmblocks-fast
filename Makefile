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

CFLAGS += $(CFLAGS_OPTIMIZE)
LDFLAGS += $(LDFLAGS_OPTIMIZE) $(LDFLAGS_ALSA) $(LDFLAGS_X11) $(LDFLAGS_NVML) $(LDFLAGS_FREEBSD) $(LDFLAGS_OPENBSD)
OBJS = src/blocks/time.o src/blocks/procfs.o src/blocks/shell.o src/blocks/ram.o src/blocks/obs.o src/blocks/webcam.o src/blocks/audio-alsa.o src/blocks/audio.o src/blocks/gpu.o src/blocks/cpu.o src/main.o
PROG = bin/dwmblocks-fast

CC = cc

$(PROG): objs.mk $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)

clean:
	rm -f src/*.o src/*/*.o bin/*

objs.mk: objs-build.mk

objs-build.mk: Makefile.base config.mk src/* src/*/* include/* include/*/*
	./build-make
	$(MAKE)
src/blocks/time.o: src/blocks/time.c 
	$(CC) -o src/blocks/time.o -c src/blocks/time.c $(CFLAGS) $(CPPFLAGS)
src/blocks/procfs.o: src/blocks/procfs.c 
	$(CC) -o src/blocks/procfs.o -c src/blocks/procfs.c $(CFLAGS) $(CPPFLAGS)
src/blocks/shell.o: src/blocks/shell.c 
	$(CC) -o src/blocks/shell.o -c src/blocks/shell.c $(CFLAGS) $(CPPFLAGS)
src/blocks/ram.o: src/blocks/ram.c 
	$(CC) -o src/blocks/ram.o -c src/blocks/ram.c $(CFLAGS) $(CPPFLAGS)
src/blocks/obs.o: src/blocks/obs.c 
	$(CC) -o src/blocks/obs.o -c src/blocks/obs.c $(CFLAGS) $(CPPFLAGS)
src/blocks/webcam.o: src/blocks/webcam.c 
	$(CC) -o src/blocks/webcam.o -c src/blocks/webcam.c $(CFLAGS) $(CPPFLAGS)
src/blocks/audio-alsa.o: src/blocks/audio-alsa.c 
	$(CC) -o src/blocks/audio-alsa.o -c src/blocks/audio-alsa.c $(CFLAGS) $(CPPFLAGS)
src/blocks/audio.o: src/blocks/audio.c 
	$(CC) -o src/blocks/audio.o -c src/blocks/audio.c $(CFLAGS) $(CPPFLAGS)
src/blocks/gpu.o: src/blocks/gpu.c 
	$(CC) -o src/blocks/gpu.o -c src/blocks/gpu.c $(CFLAGS) $(CPPFLAGS)
src/blocks/cpu.o: src/blocks/cpu.c 
	$(CC) -o src/blocks/cpu.o -c src/blocks/cpu.c $(CFLAGS) $(CPPFLAGS)
src/main.o: src/main.c 
	$(CC) -o src/main.o -c src/main.c $(CFLAGS) $(CPPFLAGS)
