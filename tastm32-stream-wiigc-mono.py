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


def pack_pcm4(samples):
    return bytes((
        (samples[1] << 4) | samples[0],
        samples[2],
        (samples[4] << 4) | samples[3],
        (samples[6] << 4) | samples[5],
        (samples[8] << 4) | samples[7],
        (samples[10] << 4) | samples[9],
        (samples[12] << 4) | samples[11],
        (samples[14] << 4) | samples[13],
    ))


while True:
    c = ser.read(1)

    if c == b'\xB0':
        print("overflow!")
        continue

    if c == b'a':
        for _ in range(28):
            samples = read_exact(15)

            if samples is None:
                sys.exit(0)

            # Input is 0000xxxx, so retain only the PCM4 nibble.
            samples = [x & 0x0f for x in samples]

            ser.write(b'A' + pack_pcm4(samples))

        ser.write(b'a')