#!/usr/bin/env python3
import serial, sys, time, os, gc
import serial_helper
import argparse_helper
import tastm32
import psutil

if(os.name == 'nt'):
    psutil.Process().nice(psutil.REALTIME_PRIORITY_CLASS)
else:
    psutil.Process().nice(20)

gc.disable()

def bitswap(b):
    b = (b&0xF0) >> 4 | (b&0x0F) << 4
    b = (b&0xCC) >> 2 | (b&0x33) << 2
    b = (b&0xAA) >> 1 | (b&0x55) << 1
    return b

data = None

parser = argparse_helper.audio_parser()
args = parser.parse_args()

DEBUG = args.debug

if args.serial == None:
    dev = tastm32.TAStm32(serial_helper.select_serial_port())
else:
    dev = tastm32.TAStm32(args.serial)

# connect to device
ser = dev

# open file
f = sys.stdin.buffer #open(sys.argv[1], "rb")

latches = 0

cmd = None
inputs = None

print("--- Starting read loop")

# reset to make sure there is no leftover data
ser.write(b'R')
time.sleep(0.1)
cmd = ser.read(2)
print(bytes(cmd))

# set up the WII(GC port) correctly
ser.write(b'SAG\x80\x00')
print('out: ', b'SAG\x80\x00')

time.sleep(0.1)
cmd = ser.read(2)
print(bytes(cmd))

# bulk data mode
ser.write(b'QA1')
print('out: ', b'QA1')
time.sleep(0.1)

ser.ser.reset_input_buffer() # clear anything that might be sitting on the serial line at the moment

# seed it with an arbitrary first frame of data to get the run to be initialized
ser.write(bytes([65,0,0,0,0,0,0,0,0]))
print('out: ', bytes([65,0,0,0,0,0,0,0,0]))

while True:
    c = ser.read(1) # keep this loop as tight as possible

    if DEBUG:
        print(c)

    if c.count(b'\xB0'): # this should not ever occur based on the protocol
        print("overflow!")
        continue
    if c.count(b'a'): # we want 28 latches
        for twice in range(4): # send 4 sets of 7 latches
            inputs = f.read(8*7)

            for i in range(0, len(inputs), 8):
                data = b'A' + inputs[i:i+8]
            ser.write(data)

        ser.write(b'a') # tell the hardware that we have completed our bulk transfer
