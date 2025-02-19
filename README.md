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
9. redis

The user must be a member of the group `gpio`:
`sudo adduser $USER gpio`

## Installing

### Hardware

#### Raspberry Pi

- Enable serial port connection in raspi-config
- Enable I2C kernel module in raspi-config

### Real-time Clock

The hardware uses an external real-time clock.
In orther to activate this, an [online guide](https://raspberrypi-guide.github.io/electronics/add-real-time-clock) was followed.
If the RTC is connected to the GPIO, the number 68 should show if probed:

```
$ sudo i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- 36 -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```

Then activate the RTC kernel module during boot by adding `rtc-ds1307` in `/etc/modules` and reboot.
Set the raspberry pi clock either manually or via NTP then copy the system time to the RTC:

```
sudo echo ds1307 0x68 > /sys/class/i2c-adapter/i2c-1/new_device
sudo hwclock -w
```

After this, reboot.

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

