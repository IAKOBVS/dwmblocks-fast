PROG = $(BIN)/dwmblocks-fast
CC = cc
CFGS = include/config.h include/blocks.h
BIN = bin
SRC = src
TEST = tests

$(PROG): objs.mk $(OBJS) $(CFGS)
	$(CC) -o $(SRC)/main.o -c $(SRC)/main.c $(CFLAGS) $(CPPFLAGS)
	$(CC) -o $@ $(SRC)/main.c $(OBJS) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)

check: $(PROG) $(TEST)/test-run.o
	@./$(TEST)/test-run-bin

clean:
	rm -f $(PROG) $(SCRIPTS) $(OBJS) $(SRC)/main.o

install: $(PROG) $(SCRIPTS)
	strip $(PROG)
	chmod 755 $^
	mkdir -p $(DESTDIR)$(PREFIX)/$(BIN)
	command -v rsync >/dev/null && rsync -a -r -c $^ $(DESTDIR)$(PREFIX)/$(BIN) || cp -f $^ $(DESTDIR)$(PREFIX)/$(BIN)

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/$(BIN)/dwmblocks-fast $(DESTDIR)$(PREFIX)/$(BIN)/$(SCRIPTSBASE)

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

$(TEST)/test-run.o: $(OBJS)
	$(CC) -o $(TEST)/test-run-bin -DTEST=1 $(SRC)/main.c $(OBJS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

$(SCRIPTS):
	@./updatesig $(BIN) scripts/$(SCRIPTSBASE)

include/config.h:
	cp include/config.def.h $@

include/blocks.h:
	cp include/blocks.def.h $@

objs.mk: objs-build.mk

objs-build.mk: Makefile config.mk $(SRC)/* $(SRC)/*/* include/* include/*/*
	@./build-make && $(MAKE)

.PHONY: all options clean install uninstall config check
