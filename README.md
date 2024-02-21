# GUI for Equipment Operators

## Build Requirements (Raspberrypi OS - Bookworm)

1. cppcheck
2. libgtk-3-dev
3. libhiredis-dev
4. libpcre2-dev
5. libsystemd-dev
6. meson
7. python3-hiredis
8. python3-redis

The user must be a member of the group `gpio`:
`sudo adduser $USER gpio`

## Installing

### x708 UPS

To install the x708 UPS HAT, run x708/install.sh as root first.

### Building
```
meson setup builddir
cd builddir
ninja cppcheck # optional
ninja install
systemctl --user daemon-reload
systemctl --user enable --now bat.service
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

#### Waveshare GNSS/GSM HAT

##### AT Commands

- `AT` - Checks if HAT status is `OK`
- `AT+CPIN?` - Check if the SIM card is `READY`

###### Clock

- `AT+CCLK?` - Gets the present time.
- `AT+CCLK="23/10/24,21:18:30+32"` - Sets the clock to `2023-10-24 21:18:30+8`

###### GNSS

- `AT+CGNSPWR?` - Checks the power status of GNSS

###### GSM

`AT+CMGF?` - Gets whether SMS is read/sent as PDU (0) or text (1)
`AT+CMGF=1` - Sets SMS reading/sending as text
`AT+CMGL="REC UNREAD"` - Reads all unread SMS

### Sample Scripts

```python3
import serial
import time
ser = serial.Serial("/dev/ttyS0", 115200)

def cmd(command):
    ser.write(f"{command}\r\n".encode())
    time.sleep(0.1)
    return ser.read(ser.inWaiting()).decode()

cmd('AT+CCLK?')
cmd('AT+CCLK="23/11/03,13:50:30+32"')

ser.close()
```

