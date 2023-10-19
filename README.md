# GUI for Equipment Operators

## Build Requirements

### Fedora 39

1. gtk3-devel
1. hiredis-devel
1. meson
1. pcre2-devel
1. systemd-devel

### Raspberrypi OS - Bookworm

1. cppcheck
1. libgtk-3-dev
1. libhiredis-dev
1. libpcre2-dev
1. libsystemd-dev
1. meson

## Installing

### Building
```
meson setup builddir
cd builddir
ninja cppcheck # optional
ninja install
```

### Running

The program can be run using the command `gui`.

## Development Notes

- C language was used since it is known to have CPU-level optimizations
- The meson build system was used to easily build and manage library dependency
- Redis was used due to utilize a fast in-memory database which lessens wear on SD card
- Systemd is used for logging
- If you are developing in Raspberry Pi OS and using git with signing enabled, you need to add `~/.gnupg/gpg-agent.conf` with the entry `pinentry-program /usr/bin/pinentry-tty`
