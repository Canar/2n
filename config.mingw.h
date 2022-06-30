#include "config.linux.h"

#define BPS        16 // required
#define RATE_      44100 // required ?

#define RATE       STR(RATE_)
#define _FMT	   STR(BPS) B_OR

#define FMT        "s" _FMT
#define FRMT       "signed" _FMT
#define STRM	   CHAN "ch " FRMT " @ " RATE "Hz"
