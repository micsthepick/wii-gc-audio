# wii gc audio streamer thingy

Requires:
1. soft modded wii (original edition with GC ports)
2. TAStm32 with my custom firmware
3. Cable for connecting TAStm32 to wii (one for P1 is sufficient)
4. Streaming pc (and pc cable)

## setup:
* install python and the requirments found here (apart from pyserial the only current requirement is opuslib)
* compile the fork of TAStm32 found at https://github.com/micsthepick/TAStm32, and load the firmware on your TAStm32.
* compile this libogc project (make sure that you have the powerpc build of opus library from devkitpro)
* upload with wiiflow or copy wii-gc-audio.dol to SD:/apps/streamer/boot.dol and load from homebrew channel
* connect the P1 TAStm32 port to P1 port of wii
* copy all the python scripts and the usage.sh script to the python scripts dir included with TAStm32, or otherwise copy serial_helper to this directory
* pipe your favourite stereo 16 bit 48KHz s16le PCM stream to usage.sh
