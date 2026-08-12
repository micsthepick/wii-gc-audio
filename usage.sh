#!/bin/bash
# pipe 16 bit 48KHz s16le PCM to this script
python stream-wiigc-encoder.py "${1:-128000}" | python tastm32-stream-wiigc-stereo.py