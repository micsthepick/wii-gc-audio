#!/usr/bin/env python3

import struct
import sys

import opuslib


SAMPLE_RATE = 48000
CHANNELS = 2
FRAME_SIZE = 960

PCM_BYTES_PER_FRAME = FRAME_SIZE * CHANNELS * 2

TRANSFER_SIZE = 128
HEADER_SIZE = 4
PAYLOAD_SIZE = TRANSFER_SIZE - HEADER_SIZE

OPUS_MAX_PACKET = 1500


def read_exact(stream, size):
    data = bytearray()

    while len(data) < size:
        chunk = stream.read(size - len(data))

        if not chunk:
            return None

        data.extend(chunk)

    return bytes(data)


def main():
    encoder = opuslib.Encoder(
        SAMPLE_RATE,
        CHANNELS,
        opuslib.APPLICATION_AUDIO,
    )

    encoder.bitrate = int(sys.argv[1]) if len(sys.argv) > 1 else 64000

    seq = 0

    while True:
        pcm = read_exact(
            sys.stdin.buffer,
            PCM_BYTES_PER_FRAME,
        )

        if pcm is None:
            break

        packet = encoder.encode(
            pcm,
            FRAME_SIZE,
        )

        packet_len = len(packet)

        if not packet_len <= OPUS_MAX_PACKET:
            raise RuntimeError(
                f"invalid Opus packet size: {packet_len}"
            )

        offset = 0

        while offset < packet_len:
            chunk = packet[
                offset:
                offset + PAYLOAD_SIZE
            ]

            offset += len(chunk)

            # Pad final transfer.
            chunk = chunk.ljust(
                PAYLOAD_SIZE,
                b'\x00',
            )

            transfer = struct.pack(
                '<HH',
                seq,
                packet_len,
            ) + chunk

            assert len(transfer) == TRANSFER_SIZE

            sys.stdout.buffer.write(transfer)
            sys.stdout.buffer.flush()

        seq = (seq + 1) & 0xffff


if __name__ == "__main__":
    main()