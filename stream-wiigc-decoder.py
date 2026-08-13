#!/usr/bin/env python3

"""Decode the transfer stream produced by stream-wiigc-encoder.py.

Input is a sequence of fixed 128-byte transfers on stdin. Decoded signed
16-bit little-endian interleaved PCM is written to stdout.
"""

import argparse
import struct
import sys

import opuslib


TRANSFER_SIZE = 128
TRANSFER_HEADER_SIZE = 1
RECORD_HEADER_SIZE = 3
OPUS_MAX_PACKET = 1500
TRANSFER_NEW_STREAM = 0x80
TRANSFER_SEQUENCE_MASK = 0x7f


def read_exact(stream, size):
    data = bytearray()

    while len(data) < size:
        chunk = stream.read(size - len(data))
        if not chunk:
            return None if not data else bytes(data)
        data.extend(chunk)

    return bytes(data)


def transfer_seq_distance(old, new):
    """Number of transfer sequence values between old and new, modulo 128."""
    return ((new - old) & TRANSFER_SEQUENCE_MASK) - 1


def opus_seq_distance(old, new):
    """Number of Opus sequence values between old and new, modulo 256."""
    return ((new - old) & 0xff) - 1


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Decode 128-byte framed Opus transfers to raw signed 16-bit "
            "little-endian PCM"
        )
    )
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--channels", type=int, choices=(1, 2), default=2)
    parser.add_argument(
        "--frame-size",
        type=int,
        default=960,
        help="samples per channel in each Opus frame (default: 960 / 20 ms)",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="do not report sequence gaps and final statistics",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    source = sys.stdin.buffer
    sink = sys.stdout.buffer

    decoder = opuslib.Decoder(args.sample_rate, args.channels)

    last_transfer_seq = None
    new_stream_seq = None
    last_decoded_opus_seq = None

    assembling_seq = None
    assembling_len = 0
    assembling_data = bytearray()

    # After a transfer loss, the first record may be the middle of a packet.
    # Its header has no fragment offset, so ignore that Opus sequence until a
    # different sequence establishes a definite packet boundary.
    reject_seq = None

    transfers = 0
    transfer_losses = 0
    packets = 0
    concealed = 0
    discarded = 0

    def report(message):
        if not args.quiet:
            print(message, file=sys.stderr)

    def reset_assembly():
        nonlocal assembling_seq, assembling_len, assembling_data
        assembling_seq = None
        assembling_len = 0
        assembling_data = bytearray()

    def conceal(count):
        nonlocal concealed

        # Do not manufacture an unbounded amount of audio after corrupt input.
        if count > 20:
            report(f"Opus sequence jump of {count}; limiting PLC to 20 frames")
            count = 20

        for _ in range(count):
            sink.write(decoder.decode(b"", args.frame_size, False))
            concealed += 1

    def decode_packet(opus_seq, packet):
        nonlocal last_decoded_opus_seq, packets

        if last_decoded_opus_seq is not None:
            missing = opus_seq_distance(last_decoded_opus_seq, opus_seq)
            if missing < 0:
                # Duplicate of the most recently decoded packet.
                return
            if missing:
                report(f"missing {missing} Opus packet(s) before {opus_seq}")
                conceal(missing)

        try:
            pcm = decoder.decode(bytes(packet), args.frame_size, False)
        except opuslib.OpusError as error:
            report(f"discarding invalid Opus packet {opus_seq}: {error}")
            return

        sink.write(pcm)
        last_decoded_opus_seq = opus_seq
        packets += 1

    while True:
        transfer = read_exact(source, TRANSFER_SIZE)

        if transfer is None:
            break
        if len(transfer) != TRANSFER_SIZE:
            report(f"discarding truncated final transfer ({len(transfer)} bytes)")
            break

        transfers += 1
        transfer_header = transfer[0]
        transfer_seq = transfer_header & TRANSFER_SEQUENCE_MASK
        payload = transfer[TRANSFER_HEADER_SIZE:]

        if transfer_header & TRANSFER_NEW_STREAM:
            if new_stream_seq == transfer_seq:
                continue

            decoder = opuslib.Decoder(args.sample_rate, args.channels)
            reset_assembly()
            last_transfer_seq = None
            last_decoded_opus_seq = None
            reject_seq = None
            new_stream_seq = transfer_seq
        else:
            new_stream_seq = None

        if last_transfer_seq is not None:
            missing = transfer_seq_distance(last_transfer_seq, transfer_seq)

            if missing < 0 or missing >= 63:
                report(f"discarding duplicate/stale transfer {transfer_seq}")
                continue

            if missing:
                transfer_losses += missing
                report(
                    f"missing {missing} transfer(s) before transfer {transfer_seq}"
                )
                reset_assembly()

                # We cannot know the fragment offset at the start of this
                # transfer. Skip its first Opus sequence conservatively.
                if len(payload) >= RECORD_HEADER_SIZE:
                    reject_seq = payload[0]

        last_transfer_seq = transfer_seq
        position = 0

        while position + RECORD_HEADER_SIZE <= len(payload):
            opus_seq, packet_len = struct.unpack_from("<BH", payload, position)

            # Zero-filled remainder produced by emit_transfer(pad=True).
            if packet_len == 0:
                break

            if packet_len > OPUS_MAX_PACKET:
                report(
                    f"invalid packet length {packet_len} in transfer "
                    f"{transfer_seq}; discarding remainder"
                )
                reset_assembly()
                discarded += 1
                break

            position += RECORD_HEADER_SIZE

            if reject_seq is not None:
                if opus_seq == reject_seq:
                    # With no fragment offset, no later record boundary in this
                    # transfer can be found safely.
                    break
                reject_seq = None

            if assembling_seq is None:
                assembling_seq = opus_seq
                assembling_len = packet_len
            elif opus_seq != assembling_seq or packet_len != assembling_len:
                report(
                    f"unexpected record {opus_seq}/{packet_len} while assembling "
                    f"{assembling_seq}/{assembling_len}; resynchronising"
                )
                reset_assembly()
                assembling_seq = opus_seq
                assembling_len = packet_len
                discarded += 1

            needed = assembling_len - len(assembling_data)
            available = len(payload) - position
            take = min(needed, available)

            assembling_data.extend(payload[position:position + take])
            position += take

            if len(assembling_data) == assembling_len:
                complete_seq = assembling_seq
                complete_packet = assembling_data
                reset_assembly()
                decode_packet(complete_seq, complete_packet)

                # If fewer than four bytes remain, the encoder padded them.
                if len(payload) - position < RECORD_HEADER_SIZE + 1:
                    break
            else:
                # The fragment consumed the remainder of this transfer.
                break

    sink.flush()
    report(
        f"decoded {packets} packet(s) from {transfers} transfer(s); "
        f"lost {transfer_losses} transfer(s), concealed {concealed} frame(s), "
        f"discarded {discarded} packet(s)"
    )


if __name__ == "__main__":
    main()
