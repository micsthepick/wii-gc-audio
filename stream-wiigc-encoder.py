#!/usr/bin/env python3

import struct
import sys

import opuslib


SAMPLE_RATE = 48000
CHANNELS = 2
FRAME_SIZE = 960

PCM_BYTES_PER_FRAME = FRAME_SIZE * CHANNELS * 2

TRANSFER_SIZE = 128
TRANSFER_HEADER_SIZE = 1
RECORD_HEADER_SIZE = 3

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

    transfer_seq = 0
    opus_seq = 0
    transfer = bytearray()

    def emit_transfer(pad=False):
        nonlocal transfer_seq, transfer

        payload_size = TRANSFER_SIZE - TRANSFER_HEADER_SIZE
        if pad:
            transfer.extend(b'\x00' * (payload_size - len(transfer)))

        if len(transfer) != payload_size:
            return

        sys.stdout.buffer.write(bytes([transfer_seq]) + transfer)
        sys.stdout.buffer.flush()
        transfer_seq = (transfer_seq + 1) & 0xff
        transfer = bytearray()

    while True:
        pcm = read_exact(
            sys.stdin.buffer,
            PCM_BYTES_PER_FRAME,
        )

        if pcm is None:
            if transfer:
                emit_transfer(pad=True)
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
            payload_size = TRANSFER_SIZE - TRANSFER_HEADER_SIZE
            space = payload_size - len(transfer)

            # A record needs its header and at least one data byte.
            if space < RECORD_HEADER_SIZE + 1:
                emit_transfer(pad=True)
                space = payload_size

            transfer.extend(struct.pack('<BH', opus_seq, packet_len))
            space -= RECORD_HEADER_SIZE

            chunk_len = min(packet_len - offset, space)
            transfer.extend(packet[offset:offset + chunk_len])
            offset += chunk_len

            emit_transfer()

        opus_seq = (opus_seq + 1) & 0xff


if __name__ == "__main__":
    main()
