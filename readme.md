# Screenshot

A barebones alternative to Windows Snip & Sketch. Press Control + ~ to activate. Press escape after activation to quit. Because this program was intended to be run in the background there's no user interface outside of selecting which part of the print screen you want to snip.

## Building

A precompiled binary is provided under the release section, but if you wish to build yourself:

#### Dependencies
* SDL2 (& SDL2 image/ttf)
* Win32
* make
* C++ compiler, default is gcc

The makefile does require you to configure the library and include directories for SDL2.
