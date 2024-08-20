#ifndef CORE_H
#define CORE_H

#include <termios.h>

#define KBUSAGE "Keyboard Controls\n\
  h  print help\n\
  n  next track\n\
  p  previous track\n\
  r  restart current track\n\
  q  quit\n\
  Q  quit after current track (TODO)\n"

#define USAGE PKG": A minimal gapless music player.\n\
\n\
Usage: "PKG" [OPTION...] [FILE...]\n\
\n\
Options\n\
  -s  --shuffle  randomizes playback order of files\n\
  -h  --help     displays this text\n\
\n\
"PKG" takes a playlist of files as parameters.\n\
Playlist is written to \"$HOME"CFGDIR"\".\n\
Playlist session is saved on quit.\n\
Resume session by running "PKG" without file arguments.\n\
\n\
"KBUSAGE" \
\n\
PCM transport format: "STRM"\n\
\n"

#define OUTEXEC "pacat","-p","-v","--channels="CHAN,"--format="FRMT,"--rate="RATE,"--raw","--client-name="PKGVER,"--stream-name="STRM
//#define OUTEXEC "pw-cat","-v","-p","--channels="CHAN,"--format=f32","--rate="RATE,"-"
//#define OUTEXEC "ffmpeg","-hide_banner","-ac","2","-ar","44100","-f","f32le","-i","-","-f","pulse","default"

#define PERM 0775

extern struct termios termios_state; 

//enum PIDS {P_RET,P_REFRESH,P_INPUT,P_DECODE,P_OUTPUT,P_ENUM_COUNT};
//int pids[(int)P_ENUM_COUNT]={-3,-1,-2,-3,-4};

enum PIDS {P_RET,P_DECODE,P_INPUT,P_REFRESH,P_OUTPUT,P_ENUM_COUNT};
extern int pids[(int)P_ENUM_COUNT];

/* global utility functions */

#define CK(x) vck(x,__FILE__,__LINE__,NULL)
void vck(int,char*,int,char*);
void halt(int x);
void platform_halt();
void prerror(int test,char* format,...);

typedef struct state_struct {
	size_t pos;
	time_t off;
} statest;

extern statest state;
extern struct timespec start;

void write_buffer(char*fn,int c,char* v);
void save_state();
int offsec();

extern char*playlist_fn,*state_fn;

#endif //CORE_H
