# IFF ILBM picture viewer for Linux/X

- Original author: R. Eberle
- Forked and extended by neoman/titan

Simple picture viewer for Linux/X, displays 2- to 256-color
IFF ILBM pictures in a window on your X screen. v0.6: Most features are supported except animations.

![Bildschirmfoto vom 2024-07-19 08-18-08](https://github.com/user-attachments/assets/767a7cc0-f455-4f8c-83cc-425d88fb812f)

## Usage

    xiffview [options] filename

Default mode/action (= no options given) is to display image.

### Options change program operation:

- `-v` Show program version information, and exit. (Overrides any other action.)

- `-c cmd` Command mode, no image display. Execute given command (cmd) string (see below), and exit.

- `-o filename` For commands that create file output: sets filename to "filename". If no option -o is given, uses default filenames as set by command (see below).

In default mode, hit key 'q', 'ESC', or close window to quit program.
See below for more keyboard control.
In command mode, program terminates when done.

### Default mode functions:

Default mode is GUI-/Image-display, which means no -c (command mode, see
    below) has been found on the command line.

#### Mouse control:

Move pointer over image to display pixel coordinates.

Move pointer over color palette cells to display color information
on the status bar.

#### Keyboard control:

- `ESC` or `q`      Quit - exit program
- `h`             Help - display program and usage information
- `s`             Image scaling - will cycle through 1x, 2x, 3x, 4x pixel size
- `F1` to `F8`      Bitplane toggle - toggles display/usage of bitplane 0 to 7
	              off/on (depending on image depth)
- `F9`            Bitplane enable all - enables display/usage of all bitplanes


### Command mode (option -c):

When `-c` is found on the command line, the next argument is expected to be a
command string for xiffview. No spaces are allowed in command strings. For
command that create files, output filename may be set with option -o (see above).

#### Commands:

`to-sprite:x,y,h,bm0,bm1,p1,p2,p3`

    x         image x-coordinate of sprite, must be multiple of 8
    y         image y-coordinate of sprite
    h         height (number of lines) of sprite
    bm0       image bitplane nr. for 1st sprite bitplane (plane 0)
    bm1       image bitplane nr. for 2nd sprite bitplane (plane 1)
    p1,p2,p3  image pen/palette indices to grab sprite colors 1-3 from

    Will generate C code for an Amiga sprite, taken from image
    bitplanes and color palette as indicated, and write it to file,
    using output file name as given by option -o, or default file name
    "sprite.c". (Sprite width is always 16 pixels.)

    Example:
    xiffview -c to-sprite:0,0,16,3,2,16,17,18 my_image.iff

---

Binary included is compiled for x86_64 CPUs.
A 32-bit "true-color" X display might be required.

There are a couple of bugs/issues - if your picture isn't
displayed perfectly it might be xiffview's fault.

Source code is included, and should compile with little to
no modifications on your Linux system.

Folder contrib/ contains some additional files, e.g. a test
pattern image, and a generic makefile for the source code.

---

#### v0.6
- added support for interlaced, HAM and EHB formats
- added support for masking by a mask plane
- 12-bit, interlaced and flicker-fixed display mode
- fixing width detection

#### v0.5
- added scaling, status bar, pixel coords, color information,  bitplane toggle, help, and ESC key
- ...and fixed a bad bug in sprite code generation ( sorry x-D )

#### v0.4
- added command mode, can generate C code for a sprite
- displays color palette
- new folder contrib/ with additional files (thanks to S. Haubenthal for Makefile contribution!)

#### v0.3
- fixed a bug in IFF decompression

#### v0.2
- first public release

---

xiffview is distributed under the terms of
AWeb Public License Version 1.0
A copy of the license is included (apl.txt).

Use at your own risk, no responsibility is taken.

---

visit http://amigaalive.de and http://arosalive.de

***support Amiga software development - click an ad!***

    :-)
