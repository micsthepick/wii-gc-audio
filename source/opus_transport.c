#include "opus_transport.h"
#include "opus_audio.h"
#include "ringbuffer.h"

#include <string.h>
#include <stdio.h>

typedef struct {
    uint16_t sequence;
    uint16_t length;
    uint8_t data[OPUS_MAX_PACKET];
} opus_packet_t;

static opus_packet_t packet_queue[OPUS_PACKET_QUEUE_SIZE];

static volatile unsigned queue_read;
static volatile unsigned queue_write;

static uint8_t rx_packet[OPUS_MAX_PACKET];

static uint16_t rx_sequence;
static uint16_t rx_length;
static unsigned rx_received;

static uint8_t rx_header[4];
static unsigned rx_header_received;

static uint16_t expected_sequence;
static int have_sequence;

static uint16_t last_sequence = 0;

static unsigned queue_count(void)
{
    return queue_write - queue_read;
}

static int queue_full(void)
{
    return queue_count() >= OPUS_PACKET_QUEUE_SIZE;
}

static int queue_empty(void)
{
    return queue_read == queue_write;
}

static void rx_reset(void)
{
    rx_header_received = 0;
    rx_length = 0;
    rx_received = 0;
}

void opus_transport_init(void)
{
    queue_read = 0;
    queue_write = 0;

    have_sequence = 0;

    rx_reset();
}
void opus_transport_stream_push(const uint8_t *data, unsigned len)
{
    while (len != 0) {
        /*
         * Collect the 4-byte packet header.
         */
        if (rx_header_received < sizeof(rx_header)) {
            rx_header[rx_header_received++] = *data++;
            len--;

            if (rx_header_received == sizeof(rx_header)) {
                rx_sequence =
                    (uint16_t)rx_header[0] |
                    ((uint16_t)rx_header[1] << 8);

                rx_length =
                    (uint16_t)rx_header[2] |
                    ((uint16_t)rx_header[3] << 8);

                rx_received = 0;

                /*
                 * Reject impossible packet sizes.
                 */
                if (rx_length == 0 ||
                    rx_length > OPUS_MAX_PACKET) {
                    rx_reset();
                }
            }

            continue;
        }

        /*
         * Copy packet payload.
         */
        unsigned remaining = rx_length - rx_received;
        unsigned n = len < remaining ? len : remaining;

        memcpy(
            rx_packet + rx_received,
            data,
            n
        );

        rx_received += n;
        data += n;
        len -= n;

        /*
         * Complete packet.
         */
        if (rx_received == rx_length) {
            if (!queue_full()) {
                unsigned index =
                    queue_write % OPUS_PACKET_QUEUE_SIZE;

                packet_queue[index].sequence = rx_sequence;
                packet_queue[index].length = rx_length;

                memcpy(
                    packet_queue[index].data,
                    rx_packet,
                    rx_length
                );

                queue_write++;
            }

            rx_reset();
        }
    }
}

void opus_transport_push(const uint8_t *data, unsigned len)
{
    if (len != OPUS_TRANSFER_SIZE)
        return;

    uint16_t sequence =
        (uint16_t)data[0] |
        ((uint16_t)data[1] << 8);

    uint16_t used =
        (uint16_t)data[2] |
        ((uint16_t)data[3] << 8);

    if (sequence - last_sequence > 1) {
        printf(
            "Transport: lost %u packets\n",
            sequence - last_sequence - 1
        );
    }
    last_sequence = sequence;

    if (used > OPUS_TRANSFER_PAYLOAD)
        return;

    opus_transport_stream_push(
        data + 4,
        used
    );
}


void opus_transport_process(void)
{
    while (!queue_empty()) {
        unsigned index =
            queue_read % OPUS_PACKET_QUEUE_SIZE;

        opus_packet_t *packet =
            &packet_queue[index];

        /*
         * Don't consume a packet until the PCM FIFO can
         * accept the complete decoded frame.
         */
        if (fifo_free() < OPUS_FRAME_SIZE)
            return;

        /*
         * Detect packet loss.
         */
        if (have_sequence) {
            uint16_t expected =
                expected_sequence + 1;

            while (expected != packet->sequence) {
                opus_audio_decode_missing();
                expected++;
            }
        }

        opus_audio_decode(
            packet->data,
            packet->length
        );

        expected_sequence = packet->sequence;
        have_sequence = 1;

        queue_read++;
    }
}
