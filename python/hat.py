#!/usr/bin/python3

import redis
import RPi.GPIO as gpio
import serial
import time

ser = serial.Serial("/dev/ttyS0", 115200)

def cmd(command):
    ser.write(f"{command}\r\n".encode())
    time.sleep(0.1)
    return ser.read(ser.inWaiting()).decode()

if cmd('AT') == '':
    gpio.setmode(gpio.BCM)
    gpio.setup(4, gpio.OUT)
    time.sleep(2)
    gpio.output(4, False)
    gpio.cleanup()

ser.close()
r = redis.Redis(host='localhost', port=6379, decode_responses=True)
r.set('hat', '1')

