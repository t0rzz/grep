# Grep for Windows

A Windows port of the UNIX grep utility, implemented in C with PCRE2 regex support.

## Features

- Pattern matching with PCRE2 regex engines
- Options: -i, -v, -n, -c, -l, -h, -H, -r, -e, -f, -E, -F, -G, -P, -w, -x
- Extended regex (-E)
- Perl-compatible regex (-P)
- Fixed strings (-F)
- Basic regex (-G)
- Word regex (-w)
- Line regex (-x)
- Case insensitive search (-i)
- Invert match (-v)
- Line numbers (-n)
- Count matches (-c)
- Files with matches (-l)
- Recursive directory search (-r)
- Binary file detection
- Multiple patterns (-e, -f)

## Building

Requires MinGW GCC and MSYS2 PCRE2.

1. Install MSYS2 and PCRE2: pacman -S mingw-w64-x86_64-pcre2
2. Compile with: gcc -Wall -O2 -o grep_win.exe grep_win.c -I C:\msys64\mingw64\include -L C:\msys64\mingw64\lib -lpcre2-8

## Usage

See help: grep_win.exe --help

## Note

This implementation aims for full UNIX grep compatibility with PCRE2 for regex support.