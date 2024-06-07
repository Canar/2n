CC=gcc
PROG=2n
MINGW_CPP=x86_64-w64-mingw32-gcc
WAVEOUT_CC=waveout.0.c
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

ds: DSound-write.exe

DSound-write.exe: DSound-write.c
	$(MINGW_CPP) -o DSound-write.exe DSound-write.c -ldsound

ds-test: tmp.raw DSound-write.exe
	cat tmp.raw | wine DSound-write.exe

tmp.raw:
	ffmpeg -loglevel -8 -i /media/gondolin/audio/seedbox/2303/*Tunes\ 2*/$(shell printf '%02d' $$(( $$RANDOM % 17 + 1 )))*.flac -ac 2 -ar 44100 -f s16le tmp.raw

ds-backup: DSound-write.c.bak

DSound-write.c.bak:
	cp DSound-write.c DSound-write.c.bak

clean-ds:
	rm DSound-write.exe tmp.raw

.PHONY: install debug tcc waveout DSound-write.c.bak
