#!/usr/bin/python3

import redis
import struct
import smbus
import sys
import time
import RPi.GPIO as GPIO

I2C_ADDR = 0x36

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(6, GPIO.IN)
GPIO.setup(13, GPIO.OUT)

r = redis.Redis(host='localhost', port=6379, decode_responses=True)

def readVoltage(bus):
    address = I2C_ADDR
    read = bus.read_word_data(address, 2)
    swapped = struct.unpack("<H", struct.pack(">H", read))[0]
    voltage = swapped * 1.25 /1000/16
    return voltage

def readCapacity(bus):
    address = I2C_ADDR
    read = bus.read_word_data(address, 4)
    swapped = struct.unpack("<H", struct.pack(">H", read))[0]
    capacity = swapped/256
    if capacity > 100:
        capacity = 100
    return int(capacity)

bus = smbus.SMBus(1)

while True:
    r.set('voltage', f'{readVoltage(bus):.2f}')
    r.set('battery', f'{readCapacity(bus)}')
    if GPIO.input(6):
        r.set('pre_shutdown', '1')
        t = time.localtime()
        current_time = time.strftime('%Y-%m-%d-%H:%M:%S', t)
        r.set('pre_shutdown_time', current_time)
    else:
        r.set('pre_shutdown', '0')
    time.sleep(2)

