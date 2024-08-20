# 2n

<sup>pronounced "tune"</sup>

*by Benjamin Cook*\
<http://baryon.it>

## Abstract

A minimalist, portable audio player written in GNU C.\
Pipes audio from `ffmpeg` into a local raw player like `pacat`.\
Plays a single, user-editable playlist gaplessly.\
Remembers where it was when you quit.

## Platforms 

`2n` is tested on the following platforms.

 - Debian 12 Bookworm
 - Android 11 on Termux

Windows 11 support is planned. (See TODO.)

## Usage

Create a 2n playlist and play it:\
`$ 2n file1 file2 ...`
	
Continue playing last created playlist:\
`$ 2n`

### Keyboard Controls
Keystroke commands during playback:\
**p**: previous track\
**n**: next track\
**q**: quit 2n\
**h**: print help

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
- Handle ffmpeg errors.
- Replace fork() synchronization with select().
- Traverse directory when passed as parameter.

## Release Log
0.1 - Initial private release.\
0.2 - First git publication. `2022-06-25`\
0.2a - Fixed Termux platform stuff. `2022-06-29`\
0.2.1 - Added DEBUG ffmpeg -report switch. `2022-11-13`\
0.2.2 - DEBUG no longer closes STDERR. `2023-08-24`\
0.2.3
- `2023-09-13` 
    - Makefile improvements
- `2024-07-23`
    - modularized code
    - added getopt_long() parsing
    - improved error handling
    - solved .wav, etc. initial audio glitch
    - shuffle play
- `2024-07-24`
    - notes on select() implementation
- `2024-08-08`
    - moved various config.\*.h files to platform/
    - finalized 0.2.3\
0.2.4a1 - modularized file structure for porting `2024-08-20`\
0.3 - unreleased - poll() implementation. `2024-08-08`
 
