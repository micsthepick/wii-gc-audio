#include "audio.h"
#include "ringbuffer.h"

#include <gccore.h>
#include <ogc/audio.h>
#include <string.h>

#define AUDIO_SAMPLES 512
#define AUDIO_STACK_SIZE 4096

static u8 audio_stack[AUDIO_STACK_SIZE] __attribute__((aligned(32)));

static s16 audio_buffer[AUDIO_SAMPLES * 2] __attribute__((aligned(32)));

static void audio_fill_buffer(void)
{
    /*
     * FIFO contains mono signed 16-bit samples.
     * AI wants interleaved stereo samples:
     *
     * L R L R L R ...
     */

    s16 mono[AUDIO_SAMPLES];

    fifo_read(
        mono,
        AUDIO_SAMPLES
    );

    for (int i = 0; i < AUDIO_SAMPLES; i++) {
        audio_buffer[i * 2 + 0] = mono[i];
        audio_buffer[i * 2 + 1] = mono[i];
    }

    DCFlushRange(
        audio_buffer,
        sizeof(audio_buffer)
    );
}


static void audio_callback(void)
{
    audio_fill_buffer();

    AUDIO_InitDMA(
        (u32)audio_buffer,
        sizeof(audio_buffer)
    );
}


void audio_init(void)
{
    AUDIO_Init(audio_stack);

    AUDIO_SetDSPSampleRate(
        AI_SAMPLERATE_32KHZ
    );

    memset(
        audio_buffer,
        0,
        sizeof(audio_buffer)
    );

    audio_fill_buffer();

    AUDIO_RegisterDMACallback(
        audio_callback
    );

    AUDIO_InitDMA(
        (u32)audio_buffer,
        sizeof(audio_buffer)
    );

    AUDIO_StartDMA();
}