#!/usr/bin/env python3

import struct
import sys

import opuslib


SAMPLE_RATE = 48000
CHANNELS = 2
FRAME_SIZE = 960                  # 20 ms

PCM_BYTES_PER_FRAME = (
    FRAME_SIZE * CHANNELS * 2
)

TRANSFER_SIZE = 128
TRANSFER_HEADER_SIZE = 4
TRANSFER_PAYLOAD_SIZE = (
    TRANSFER_SIZE - TRANSFER_HEADER_SIZE
)

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
    encoder.vbr = False

    tx_buffer = bytearray()

    opus_sequence = 0
    transfer_sequence = 0

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

        if not 0 < len(packet) <= OPUS_MAX_PACKET:
            raise RuntimeError(
                f"invalid Opus packet size: {len(packet)}"
            )

        # Opus packet:
        #   u16 sequence
        #   u16 length
        #   payload
        tx_buffer.extend(
            struct.pack(
                "<HH",
                opus_sequence,
                len(packet),
            )
        )
        tx_buffer.extend(packet)

        opus_sequence = (
            opus_sequence + 1
        ) & 0xffff

        # Emit complete 128-byte transfers.
        while len(tx_buffer) >= TRANSFER_PAYLOAD_SIZE:
            payload = tx_buffer[:TRANSFER_PAYLOAD_SIZE]
            del tx_buffer[:TRANSFER_PAYLOAD_SIZE]

            transfer = (
                struct.pack(
                    "<HH",
                    transfer_sequence,
                    TRANSFER_PAYLOAD_SIZE,
                )
                + payload
            )

            sys.stdout.buffer.write(transfer)
            sys.stdout.buffer.flush()

            transfer_sequence = (
                transfer_sequence + 1
            ) & 0xffff


if __name__ == "__main__":
    main()