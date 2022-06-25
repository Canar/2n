#define PKG        "2n"
#define VERMAJ     0
#define VERMIN     2

// waveOut requires 16
#define BPS        16
#define RATE_      44100
#define CHAN_      2
#define B_OR       "le"

#define PLAYLISTFN "playlist"
#define STATEFN    "state"

#define STR_(x)    #x
#define STR(x)     STR_(x)
#define CFGDIR     "/.local/share/" PKG "/"
#define VER        STR(VERMAJ) "." STR(VERMIN)

#define RATE       STR(RATE_)
#define CHAN       STR(CHAN_)
#define _FMT	   STR(BPS) B_OR

// waveOut requires "s" / "signed"
#define FMT        "s" _FMT
#define FRMT        "signed" _FMT
#define STRM	   CHAN "ch " FRMT " @ " RATE "Hz"
#define VERTXT     "✝ v" VER
#define PKGVER     PKG " " VERTXT

#define STDIN      0
