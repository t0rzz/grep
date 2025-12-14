# GNU Grep 3.11 for Windows

A complete Windows port of GNU grep 3.11, implemented in C with static PCRE2 regex support for full portability.

## Features

- Full GNU grep 3.11 compatibility
- PCRE2 regex engines (Perl, Extended, Basic)
- Fixed string matching
- Word and line boundary matching
- Case insensitive search
- Invert match
- Line numbers and byte offsets
- Count matches
- Files with/without matches
- Recursive directory search
- Binary file handling
- Multiple patterns support
- Context lines (before/after)
- Color output
- Include/exclude patterns
- Null-separated output

## Building

### Prerequisites
- MSYS2 with MinGW GCC
- PCRE2 development libraries

### Static Build (Recommended)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-pcre2
gcc -o grep.exe grep_win.c /mingw64/lib/libpcre2-8.a
```

This produces a single, portable `grep.exe` with no external dependencies.

### Dynamic Build
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-pcre2
gcc -o grep.exe grep_win.c -lpcre2-8
```

Requires `libpcre2-8-0.dll` to be distributed alongside.

## Installation

Download the latest release from [GitHub Releases](https://github.com/t0rzz/grep/releases) - it's a single portable executable.

## Usage

```bash
grep [OPTIONS] PATTERN [FILE...]
grep --help
```

## Compatibility

- Fully compatible with GNU grep 3.11 behavior
- Handles Windows path separators and line endings correctly
- Supports all standard grep options and features

## Development

Built with automated CI/CD via GitHub Actions. Contributions welcome!

## License

GPLv3+ (same as GNU grep)