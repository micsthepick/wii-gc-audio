#!/usr/bin/env python3
import sys

def triangle(phase):
    phase &= 0xff
    if phase < 128:
        return phase * 2
    return 255 - (phase - 128) * 2

phase = 0
pitches = [1, 4]
pitch_index = 0
blocks_at_pitch = 0

while True:
    pitch = pitches[pitch_index]

    block = bytearray()
    for _ in range(56):
        block.append(triangle(phase))
        phase += pitch

    sys.stdout.buffer.write(block)
    sys.stdout.buffer.flush()

    blocks_at_pitch += 1
    if blocks_at_pitch >= 100:
        blocks_at_pitch = 0
        pitch_index ^= 1
