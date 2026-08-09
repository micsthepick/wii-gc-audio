#include "audio.h"
#include "ringbuffer.h"

#include <gccore.h>
#include <ogc/audio.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_SAMPLES 512

static s16 audio_buffer[AUDIO_SAMPLES * 2]
    __attribute__((aligned(32)));

static void audio_fill_buffer(void)
{
    s16 mono[AUDIO_SAMPLES];

    int samples_available = fifo_read(
        mono,
        AUDIO_SAMPLES
    );

    /*
     * Fill the entire DMA buffer.
     *
     * If there aren't enough samples available, pad
     * the remainder with silence.
     */
    for (int i = 0; i < AUDIO_SAMPLES; i++)
    {
        s16 sample = 0;

        if (i < samples_available)
            sample = mono[i];

        audio_buffer[i * 2 + 0] = sample;
        audio_buffer[i * 2 + 1] = sample;
    }

    DCFlushRange(audio_buffer, sizeof(audio_buffer));
    __sync_synchronize();
}

static void audio_callback(void)
{
    audio_fill_buffer();
}

void audio_init(void)
{
    AUDIO_Init(NULL);

    AUDIO_SetDSPSampleRate(
        AI_SAMPLERATE_32KHZ
    );

    memset(
        audio_buffer,
        0,
        sizeof(audio_buffer)
    );

    DCFlushRange(
        audio_buffer,
        sizeof(audio_buffer)
    );

    AUDIO_RegisterDMACallback(
        audio_callback
    );

    AUDIO_InitDMA(
        (u32)audio_buffer,
        sizeof(audio_buffer)
    );

    AUDIO_StartDMA();
}