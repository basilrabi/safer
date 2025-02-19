#!/usr/bin/python3

import redis
import RPi.GPIO as gpio
import serial
import time

ser = serial.Serial("/dev/ttyS0", 115200)

def cmd(command):
    ser.write(f"{command}\r\n".encode())
    time.sleep(2)
    return ser.read(ser.inWaiting()).decode()

while cmd('AT') == '':
    gpio.setmode(gpio.BCM)
    gpio.setup(4, gpio.OUT)
    gpio.output(4, True)
    time.sleep(2)
    gpio.output(4, False)
    time.sleep(2)
    gpio.cleanup()

ser.close()
r = redis.Redis(host='localhost', port=6379, decode_responses=True)
r.set('hat', '1')

