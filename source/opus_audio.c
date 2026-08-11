#include "opus_audio.h"
#include "ringbuffer.h"

#include <opus.h>

volatile u32 opus_errors = 0;
volatile u32 opus_count = 0;

static OpusDecoder *decoder;

static int16_t pcm[OPUS_FRAME_SIZE * OPUS_CHANNELS]
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

int opus_audio_decode(const uint8_t *packet, size_t len)
{
    int samples;

    samples = opus_decode(
        decoder,
        packet,
        len,
        pcm,
        OPUS_FRAME_SIZE,
        0
    );

    if (samples < 0) {
        opus_errors++;
        return samples;
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