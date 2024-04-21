# 2n

*by Benjamin Cook*\
<http://baryon.it>

## Abstract

A minimalist, portable audio player written in C.\
Pipes audio from `ffmpeg` into a local raw player.\
Tested on Linux and Windows but probably ports elsewhere.\
`pacat` is default, `waveOut-write.exe` for Windows-MinGW.

Plays a single, user-editable playlist gaplessly.\
Remembers where it was when you quit.

## Usage

Create a 2n playlist and play it:\
`        2n file1 file2 ...`
	
Continue playing last created playlist:\
`        2n`

Keystroke commands during playback:\
**p**: previous track\
**n**: next track\
**q**: quit 2n

## Features and Configuration

`2n` supports every input and output format that `ffmpeg` does.\
Default configuration plays to the `default` Pulseaudio device.\
Pulseaudio devices are handled through the `pacat` executable.

User-editable playlist is loaded at runtime and found at\
`$HOME / CFGDIR / PLAYLISTFN` per `config.h`.\
Playlist comprises null-separated filenames.\
Reload by quitting and relaunching without argument.\
Playback continues on the same line where it quit,\
at the time elapsed when quit.\
(Note that this might give unexpected results if/when the filename\
at that line number changes!)

<!-- ======================================================================= -->

### Windows-MinGW

`waveOut-write.exe` is like `pacat` for Windows-MinGW.\
It worked when I tested. Haven't bothered to hook it up right yet.\
Some DirectSound experimentation in the `ds*` files.

### Configuration files

Configuration is performed by editing `config.h` before building.\
`2n` defines sample output format at compile-time via `config.h`.\
`ln -sf config.platform.h config.h` to select your configuration.

## Known Issues

- Timestamp is inaccurate.
- Zero error-handling. If decoder errors, track is skipped.
- Decoder process can underrun and cause an audible skip.
- Output process terminates too early. (Short files won't play.)

## Planned Enhancements

- Reload playlist when modified.
- Proper Windows-MinGW build code.

## Release Log
0.1 - Initial private release.\
0.2 - First git publication. `2022-06-25`\
0.3a - Fixed Termux platform stuff. `2022-06-29`\
0.2.1 - Added DEBUG ffmpeg -report switch `2022-11-13`\
0.2.2 - DEBUG no longer closes STDERR `2023-08-24`\
HEAD - Makefile improvements `2023-09-13`
