CC=gcc
PROG=2n
MINGW_CPP=x86_64-w64-mingw32-gcc
WAVEOUT_CC=waveout.0.c
PREFIX=/usr/local/bin
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
	$(CC) -g -o $(PROG) 2n.c

waveout.exe:	waveout/$(WAVEOUT_CC)
	$(MINGW_CPP) -o waveout.exe waveout/$(WAVEOUT_CC) -lwinmm

waveout:	waveout.exe

waveOut-ls.exe:	waveOut-ls.c
	$(MINGW_CPP) -o waveOut-ls.exe waveOut-ls.c -lwinmm

waveOut-write.exe:	waveOut-write.c
	$(MINGW_CPP) -o waveOut-write.exe waveOut-write.c -Wall -lwinmm

install:	2n
	cp $(PROG) $(PREFIX)/$(PROG)

.PHONY: install debug tcc waveout
