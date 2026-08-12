#include "opus_audio.h"
#include "ringbuffer.h"

#include <opus/opus.h>

volatile u32 opus_errors = 0;
volatile s32 opus_last_error = 0;
volatile s32 opus_last_packet_samples = 0;
volatile u32 opus_last_packet_length = 0;
volatile u32 opus_count = 0;

static OpusDecoder *decoder;

static int16_t pcm[OPUS_MAX_FRAME_SIZE * OPUS_CHANNELS]
    __attribute__((aligned(32)));

int opus_audio_init(void)
{
    int err;

    decoder = opus_decoder_create(
        OPUS_SAMPLE_RATE,
        OPUS_CHANNELS,
        &err
    );

    return err;
}

void opus_audio_reset(void)
{
    if (decoder != NULL)
        opus_decoder_ctl(decoder, OPUS_RESET_STATE);
}

int opus_audio_decode(const uint8_t *packet, size_t len)
{
    int samples;

    samples = opus_decode(
        decoder,
        packet,
        len,
        pcm,
        OPUS_MAX_FRAME_SIZE,
        0
    );

    if (samples < 0) {
        opus_errors++;
        opus_last_error = samples;
        opus_last_packet_samples = opus_packet_get_nb_samples(
            packet,
            len,
            OPUS_SAMPLE_RATE
        );
        opus_last_packet_length = len;
        return samples;
    }

    if (samples > OPUS_FRAME_SIZE) {
        opus_errors++;
        opus_last_error = samples;
    }

    opus_count++;

    fifo_write(pcm, samples);

    return samples;
}

int opus_audio_decode_missing(void)
{
    int samples;

    if (fifo_free() < OPUS_FRAME_SIZE)
        return 0;

    samples = opus_decode(
        decoder,
        NULL,
        0,
        pcm,
        OPUS_FRAME_SIZE,
        0
    );

    if (samples < 0)
        return samples;

    return fifo_write(pcm, samples);
}
