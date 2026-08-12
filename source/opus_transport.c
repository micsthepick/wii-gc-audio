#include "opus_transport.h"
#include "opus_audio.h"
#include "ringbuffer.h"
#include "gcsi.h"

#include <string.h>
#include <stdio.h>
#include <ogc/irq.h>

typedef struct {
    uint8_t sequence;
    uint16_t length;
    uint8_t data[OPUS_MAX_PACKET];
} opus_packet_t;

static opus_packet_t packet_queue[OPUS_PACKET_QUEUE_SIZE];
static opus_packet_t decode_packet;

static volatile unsigned queue_read;
static volatile unsigned queue_write;

static uint8_t rx_packet[OPUS_MAX_PACKET];

static uint8_t rx_sequence;
static uint16_t rx_length;
static unsigned rx_received;

static uint8_t expected_transfer_sequence;
static int have_transfer_sequence;
static volatile int new_stream_pending;
static uint8_t new_stream_sequence;
static int new_stream_latched;


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

    expected_transfer_sequence = 0;
    have_transfer_sequence = 0;
    new_stream_pending = 0;
    new_stream_sequence = 0;
    new_stream_latched = 0;
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
        uint8_t transfer_header = data[0];
        uint8_t transfer_sequence =
            transfer_header & OPUS_TRANSFER_SEQUENCE_MASK;
        unsigned offset = OPUS_TRANSFER_HEADER;

        if ((transfer_header & OPUS_TRANSFER_NEW_STREAM) &&
            new_stream_latched &&
            transfer_sequence == new_stream_sequence) {
            /* SI can return the first transfer repeatedly. */
            data += OPUS_TRANSFER_SIZE;
            len -= OPUS_TRANSFER_SIZE;
            continue;
        }

        if (transfer_header & OPUS_TRANSFER_NEW_STREAM) {
            rx_reset();
            queue_read = queue_write;
            new_stream_pending = 1;
            have_transfer_sequence = 0;
            new_stream_sequence = transfer_sequence;
            new_stream_latched = 1;
        } else {
            new_stream_latched = 0;
        }

        if (have_transfer_sequence) {
            uint8_t delta =
                (uint8_t)(
                    transfer_sequence -
                    expected_transfer_sequence
                ) & OPUS_TRANSFER_SEQUENCE_MASK;

            if (delta >= 64) {
                /* Duplicate or stale transfer. Keep reassembly intact. */
                data += OPUS_TRANSFER_SIZE;
                len -= OPUS_TRANSFER_SIZE;
                continue;
            }

            if (delta != 0)
                rx_reset();
        }

        expected_transfer_sequence =
            (uint8_t)(transfer_sequence + 1) &
            OPUS_TRANSFER_SEQUENCE_MASK;
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
    if (new_stream_pending) {
        new_stream_pending = 0;
        opus_audio_reset();
    }

    while (!queue_empty()) {
        /*
         * A valid Opus packet can contain up to 120 ms of audio.
         */
        if (fifo_free() < OPUS_MAX_FRAME_SIZE)
            return;

        /*
         * SI receives in interrupt context. Pop into decoder-owned storage
         * so a NEW_STREAM event cannot recycle the queue slot while Opus is
         * reading it.
         */
        u32 level = IRQ_Disable();

        if (queue_empty()) {
            IRQ_Restore(level);
            break;
        }

        unsigned index = queue_read % OPUS_PACKET_QUEUE_SIZE;
        opus_packet_t *packet = &packet_queue[index];

        decode_packet.sequence = packet->sequence;
        decode_packet.length = packet->length;
        memcpy(decode_packet.data, packet->data, packet->length);
        queue_read++;

        IRQ_Restore(level);

        opus_audio_decode(
            decode_packet.data,
            decode_packet.length
        );
    }

    si_maybe_resume();
}
