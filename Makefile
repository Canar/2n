CC=gcc
PROG=2n
MINGW_CPP=x86_64-w64-mingw32-gcc
PREFIX=/usr/local/bin
SHELL=/bin/bash
FF=$(shell which ffmpeg || echo /usr/bin/does_not_exist )
MODULES=2n core fork
OBJECTS=$(addsuffix .o,$(MODULES))

$(PROG):	$(OBJECTS) Makefile config.h platform/config.linux.h $(FF)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJECTS)

clean:
	rm $(OBJECTS)

%.o : %.c
	$(CC) $(CFLAGS) $< -c

$(FF):	
	@echo Warning: FFMPEG executable not found. 2n will build but not function.

config.h:	platform/config.linux.h
	echo Defaulting to Linux config. Symlink a different config for other platforms.
	ln -s platform/config.linux.h config.h

#tcc:
#	tcc -o $(PROG) 2n.c

debug: $(OBJECTS)
	$(CC) -Wall -g -DDEBUG -fsanitize=address,undefined -o $(PROG) $(OBJECTS)

warn: $(OBJECTS)
	$(CC) -Wall -o $(PROG) $(OBJECTS)

install:	2n
	cp $(PROG) $(PREFIX)/$(PROG)

.PHONY: install debug warn tcc clean
