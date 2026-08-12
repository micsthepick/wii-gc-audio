#include "opus_transport.h"
#include "opus_audio.h"
#include "ringbuffer.h"
#include "gcsi.h"

#include <string.h>
#include <stdio.h>

#define OPUS_MAX_PACKET_GAP 20

typedef struct {
    uint8_t sequence;
    uint16_t length;
    uint8_t data[OPUS_MAX_PACKET];
} opus_packet_t;

static opus_packet_t packet_queue[OPUS_PACKET_QUEUE_SIZE];

static volatile unsigned queue_read;
static volatile unsigned queue_write;

static uint8_t rx_packet[OPUS_MAX_PACKET];

static uint8_t rx_sequence;
static uint16_t rx_length;
static unsigned rx_received;

static uint8_t expected_sequence;
static int have_sequence;

static uint8_t expected_transfer_sequence;
static int have_transfer_sequence;


int queue_empty(void)
{
    return queue_read == queue_write;
}


int queue_full(void)
{
    return (queue_write - queue_read) >= OPUS_PACKET_QUEUE_SIZE;
}


int opus_transport_needs_backpressure(void)
{
    return (queue_write - queue_read) >= OPUS_PACKET_QUEUE_HIGH_WATER;
}


static void rx_reset(void)
{
    rx_length = 0;
    rx_received = 0;
}


void opus_transport_init(void)
{
    queue_read = 0;
    queue_write = 0;

    rx_sequence = 0;
    rx_length = 0;
    rx_received = 0;

    expected_sequence = 0;
    have_sequence = 0;

    expected_transfer_sequence = 0;
    have_transfer_sequence = 0;
}


static void queue_rx_packet(void)
{
    if (queue_full()) {
        printf("Transport: packet queue full\n");
    } else {
        unsigned index = queue_write % OPUS_PACKET_QUEUE_SIZE;
        opus_packet_t *packet = &packet_queue[index];

        packet->sequence = rx_sequence;
        packet->length = rx_length;
        memcpy(packet->data, rx_packet, rx_length);
        queue_write++;
    }

    rx_reset();
}


void opus_transport_stream_push(
    const uint8_t *data,
    unsigned len
)
{
    while (len >= OPUS_TRANSFER_SIZE) {
        uint8_t transfer_sequence = data[0];
        unsigned offset = OPUS_TRANSFER_HEADER;

        if (have_transfer_sequence) {
            uint8_t delta =
                (uint8_t)(
                    transfer_sequence -
                    expected_transfer_sequence
                );

            if (delta >= 128) {
                /* Duplicate or stale transfer. Keep reassembly intact. */
                data += OPUS_TRANSFER_SIZE;
                len -= OPUS_TRANSFER_SIZE;
                continue;
            }

            if (delta != 0)
                rx_reset();
        }

        expected_transfer_sequence = (uint8_t)(transfer_sequence + 1);
        have_transfer_sequence = 1;

        while (offset + OPUS_RECORD_HEADER <= OPUS_TRANSFER_SIZE) {
            uint8_t sequence = data[offset];
            uint16_t packet_len =
                (uint16_t)data[offset + 1] |
                ((uint16_t)data[offset + 2] << 8);

            offset += OPUS_RECORD_HEADER;

            if (packet_len == 0 || packet_len > OPUS_MAX_PACKET) {
                rx_reset();
                break;
            }

            if (rx_received == 0) {
                rx_sequence = sequence;
                rx_length = packet_len;
            } else if (sequence != rx_sequence || packet_len != rx_length) {
                rx_sequence = sequence;
                rx_length = packet_len;
                rx_received = 0;
            }

            unsigned remaining = rx_length - rx_received;
            unsigned available = OPUS_TRANSFER_SIZE - offset;
            unsigned copy = remaining < available ? remaining : available;

            memcpy(rx_packet + rx_received, data + offset, copy);
            rx_received += copy;
            offset += copy;

            if (rx_received != rx_length)
                break;

            queue_rx_packet();
        }

        data += OPUS_TRANSFER_SIZE;
        len -= OPUS_TRANSFER_SIZE;
    }
}


void opus_transport_push(
    const uint8_t *data,
    unsigned len
)
{
    if (len != OPUS_TRANSFER_SIZE)
        return;

    opus_transport_stream_push(data, len);
}


void opus_transport_process(void)
{
    while (!queue_empty()) {

        unsigned index =
            queue_read % OPUS_PACKET_QUEUE_SIZE;

        opus_packet_t *packet =
            &packet_queue[index];

        /*
         * We need space for at least one decoded Opus frame.
         */
        if (fifo_free() < OPUS_FRAME_SIZE)
            return;

        /*
         * Detect missing Opus packets.
         */
        if (have_sequence) {

            uint8_t delta =
                (uint8_t)(
                    packet->sequence -
                    expected_sequence
                );

            if (delta >= 128) {
                /* Stale or duplicate packet, not a forward loss. */
                queue_read++;
                continue;
            }

            if (delta != 0) {

                if (delta <= OPUS_MAX_PACKET_GAP) {
                    unsigned required_frames = (unsigned)delta + 1;

                    if (fifo_free() < required_frames * OPUS_FRAME_SIZE)
                        return;

                    while (delta--) {
                        opus_audio_decode_missing();
                    }

                } else {
                    have_sequence = 0;
                }
            }
        }

        opus_audio_decode(
            packet->data,
            packet->length
        );

        expected_sequence =
            (uint8_t)(packet->sequence + 1);

        have_sequence = 1;

        queue_read++;
    }

    si_maybe_resume();
}
