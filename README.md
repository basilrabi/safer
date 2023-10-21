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

### Software

- C language was used since it is known to have CPU-level optimizations
- The meson build system was used to easily build and manage library dependency
- Redis was used due to utilize a fast in-memory database which lessens wear on SD card
- Systemd is used for logging
- If you are developing in Raspberry Pi OS and using git with signing enabled, you need to add `~/.gnupg/gpg-agent.conf` with the entry `pinentry-program /usr/bin/pinentry-tty`

### Hardware

#### Raspberry Pi

- Enable serial port connection in raspi-config
- Enable I2C kernel module in raspi-config

#### Waveshare GNSS/GSM Hat

To activate the module the following python script can be run:

```
import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(4, GPIO.OUT)
time.sleep(2)
GPIO.output(4, False)

```
