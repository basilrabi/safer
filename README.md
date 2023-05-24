# GUI for Equipment Operators

## Build Requirements

### Fedora 38

1. gtk3-devel
1. hiredis-devel
1. meson
1. pcre2-devel
1. systemd-devel

## Installing

```
meson setup builddir
cd builddir
meson install
```

The program can be run using the command `gui`.

## Development Notes

- C language was used since it is known to have CPU-level optimizations
- The meson build system was used to easily build and manage library dependency
- Redis was used due to utilize a fast in-memory database which lessens wear on SD card
- Systemd is used for logging
