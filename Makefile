CC=gcc
PROG=2n
MINGW_CPP=x86_64-w64-mingw32-gcc
WAVEOUT_CC=waveout.0.c
PREFIX=/usr/local/bin

$(PROG):	2n.c Makefile
	$(CC) $(CFLAGS) -o $(PROG) 2n.c

tcc:
	tcc -o $(PROG) 2n.c

debug:
	$(CC) -g -o $(PROG) 2n.c

waveout.exe: waveout/$(WAVEOUT_CC)
	$(MINGW_CPP) -o waveout.exe waveout/$(WAVEOUT_CC) -lwinmm

waveout: waveout.exe

waveOut-ls.exe: waveOut-ls.c
	$(MINGW_CPP) -o waveOut-ls.exe waveOut-ls.c -lwinmm

waveOut-write.exe: waveOut-write.c
	$(MINGW_CPP) -o waveOut-write.exe waveOut-write.c -Wall -lwinmm

install:
	cp $(PROG) $(PREFIX)/$(PROG)

.PHONY: install debug tcc waveout
