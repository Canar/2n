# 2n

*by Benjamin Cook*\
<http://baryon.it>

## Abstract

A minimalist, portable audio player written in C.\
Pipes audio from `ffmpeg` into a local raw player.\
Tested on Linux and Windows but probably ports elsewhere.\
`pacat` is default, `waveOut-write.exe` for `mingw`.

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
Default configuration plays to the `default` Pulseaudio device.

User-editable playlist is loaded at runtime and found at\
`$HOME / CFGDIR / PLAYLISTFN` per `config.h`.\
Playlist comprises null-separated filenames.\
Reload by quitting and relaunching without argument.\
Playback continues on the same line where it quit,\
at the time elapsed when quit.\
(Note that this might give unexpected results if/when the filename\
at that line number changes!)

`2n` runs on both Linux and MinGW-Windows. Windows platform requires\
build and install of `waveOut-write.exe` and is more unsupported\
than anything else.

### Configuration files

Configuration is performed by editing `config.h` before building.\
`2n` defines sample output format at compile-time via `config.h`.\
`ln -sf config.platform.h config.h` to select your configuration.

## Known Issues
Timestamp is inaccurate.

Zero error-handling. Not opposed to adding some, but I never encounter errors.

## Planned Enhancements

At some point, playlist handling will change such that the playlist\
automatically reloads when the playlist file is modified. This may\
need to be a platform-specific feature as I'm not sure I care to\
handle Windows-specific code here.

## Release Log
0.1 - Initial private release.\
0.2 - First git publication. `2022-06-25`\
0.3a - Fixed Termux platform stuff. `2022-06-29`\
0.2.1 - Added DEBUG ffmpeg -report switch `2022-11-13`
