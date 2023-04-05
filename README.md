# GUI for Equipment Operators

## Build Requirements

### Fedora 38

1. hiredis-devel
1. gtk3-devel
1. systemd-devel
1. meson

## Building

```
meson setup builddir
cd builddir
ninja
cp ../theme.css ~/theme.css
./main
```

## Development Notes

- C language was used since it is known to have CPU-level optimizations
- The meson build system was used to easily build and manage library dependency
- Redis was used due to utilize a fast in-memory database which lessens wear on SD card
- Systemd is used for logging
