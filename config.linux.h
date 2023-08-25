#define STR_(x)    #x
#define STR(x)     STR_(x)
#define STDIN      0

#define PKG        "2n"
#define VERMAJ     0
#define VERMIN     2
#define VERPT      2
#define VER        STR(VERMAJ) "." STR(VERMIN) "." STR(VERPT)
#define VERTXT     "✝ v" VER
#define PKGVER     PKG " " VERTXT

#define PLAYLISTFN "playlist"
#define STATEFN    "state"
#define CFGDIR     "/.local/share/" PKG "/"

#define BPS        32
#define RATE_      44100
#define CHAN_      2
#define B_OR       "le"

#define RATE       STR(RATE_)
#define CHAN       STR(CHAN_)
#define _FMT	   STR(BPS) B_OR

// waveOut requires "s" / "signed"
#define FMT        "f" _FMT
#define FRMT        "float" _FMT
#define STRM	   CHAN "ch " FRMT " @ " RATE "Hz"
#define VERTXT     "✝ v" VER
#define PKGVER     PKG " " VERTXT
