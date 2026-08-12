#include "opus_transport.h"
#include "opus_audio.h"
#include "ringbuffer.h"

#include <string.h>
#include <stdio.h>

#define OPUS_MAX_PACKET_GAP 20

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

static uint16_t expected_sequence;
static int have_sequence;


int queue_empty(void)
{
    return queue_read == queue_write;
}


int queue_full(void)
{
    return (queue_write - queue_read) >= OPUS_PACKET_QUEUE_SIZE;
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
}


void opus_transport_stream_push(
    const uint8_t *data,
    unsigned len
)
{
    while (len >= OPUS_TRANSFER_SIZE) {

        uint16_t sequence =
            (uint16_t)data[0] |
            ((uint16_t)data[1] << 8);

        uint16_t packet_len =
            (uint16_t)data[2] |
            ((uint16_t)data[3] << 8);

        const uint8_t *payload = data + 4;

        if (packet_len == 0 || packet_len > OPUS_MAX_PACKET) {
            data += OPUS_TRANSFER_SIZE;
            len -= OPUS_TRANSFER_SIZE;
            continue;
        }

        if (rx_received == 0 ||
            sequence != rx_sequence) {

            if (rx_received != 0) {
                printf(
                    "Transport: incomplete packet %u lost\n",
                    rx_sequence
                );
            }

            rx_sequence = sequence;
            rx_length = packet_len;
            rx_received = 0;
        }

        /*
         * Copy only bytes belonging to the Opus packet.
         *
         * Any bytes remaining in the 124-byte payload are
         * padding and are ignored.
         */
        unsigned remaining =
            rx_length - rx_received;

        unsigned copy =
            remaining < OPUS_TRANSFER_PAYLOAD
                ? remaining
                : OPUS_TRANSFER_PAYLOAD;

        memcpy(
            rx_packet + rx_received,
            payload,
            copy
        );

        rx_received += copy;

        /*
         * Complete Opus packet.
         */
        if (rx_received == rx_length) {

            if (queue_full()) {

                printf(
                    "Transport: packet queue full\n"
                );

            } else {

                unsigned index =
                    queue_write % OPUS_PACKET_QUEUE_SIZE;

                opus_packet_t *packet =
                    &packet_queue[index];

                packet->sequence = rx_sequence;
                packet->length = rx_length;

                memcpy(
                    packet->data,
                    rx_packet,
                    rx_length
                );

                queue_write++;
            }

            rx_reset();
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

            uint16_t gap =
                (uint16_t)(
                    packet->sequence -
                    expected_sequence
                );

            if (gap != 0) {

                if (gap <= OPUS_MAX_PACKET_GAP) {

                    printf(
                        "Transport: lost %u packet%s\n",
                        gap,
                        gap == 1 ? "" : "s"
                    );

                    while (gap--) {
                        if (fifo_free() < OPUS_FRAME_SIZE)
                            return;

                        opus_audio_decode_missing();
                    }

                } else {
                    have_sequence = 0;
                    printf(
                        "Transport: skipped large gap to %u\n",
                        packet->sequence
                    );
                }
            }
        }

        opus_audio_decode(
            packet->data,
            packet->length
        );

        expected_sequence =
            (uint16_t)(packet->sequence + 1);

        have_sequence = 1;

        queue_read++;
    }
}