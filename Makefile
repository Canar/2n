CC=gcc
PROG=2n
MINGW_CPP=x86_64-w64-mingw32-gcc
PREFIX=/usr/local/bin
SHELL=/bin/bash
FF=$(shell which ffmpeg || echo /usr/bin/does_not_exist )

$(PROG):	$(PROG).c Makefile config.h config.linux.h $(FF)
	$(CC) $(CFLAGS) -o $(PROG) $(PROG).c

$(FF):	
	@echo Warning: FFMPEG executable not found. 2n will build but not function.

config.h:	config.linux.h
	echo Defaulting to Linux config. Symlink a different config for other platforms.
	ln -s config.linux.h config.h

tcc:
	tcc -o $(PROG) 2n.c

debug:
	$(CC) -g -DDEBUG -fsanitize=address,undefined -o $(PROG) 2n.c

warn:
	$(CC) -Wall -o $(PROG) 2n.c

install:	2n
	cp $(PROG) $(PREFIX)/$(PROG)

.PHONY: install debug tcc
