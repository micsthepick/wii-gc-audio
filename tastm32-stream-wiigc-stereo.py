#!/usr/bin/env python3
import serial, sys, time, os, gc
import serial_helper
import argparse_helper
import tastm32
import psutil

if os.name == 'nt':
    psutil.Process().nice(psutil.REALTIME_PRIORITY_CLASS)
else:
    psutil.Process().nice(20)

gc.disable()

parser = argparse_helper.audio_parser()
args = parser.parse_args()

DEBUG = args.debug

if args.serial is None:
    ser = tastm32.TAStm32(serial_helper.select_serial_port())
else:
    ser = tastm32.TAStm32(args.serial)

f = sys.stdin.buffer

print("--- Starting read loop")

ser.write(b'R')
time.sleep(0.1)
cmd = ser.read(2)
print(bytes(cmd))

ser.write(b'SAG\x80\x00')
time.sleep(0.1)
cmd = ser.read(2)
print(bytes(cmd))

ser.write(b'QA1')
time.sleep(0.1)

ser.ser.reset_input_buffer()

# Seed it with an arbitrary first frame.
ser.write(bytes([65, 0, 0, 0, 0, 0, 0, 0, 0]))


def read_exact(n):
    data = bytearray()

    while len(data) < n:
        chunk = f.read(n - len(data))
        if not chunk:
            return None
        data.extend(chunk)

    return data

c = b'w'

while True:
    if c == b'w':
        for _ in range(64):
            samples = read_exact(128)
            if samples is None:
                sys.exit(0)

            ser.write(b'W' + samples)

        ser.write(b'w')

    c = ser.read(1)

    if c == b'\xB0':
        print("overflow!")
