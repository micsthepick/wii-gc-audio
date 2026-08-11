#include "audio.h"
#include "ringbuffer.h"

#include <gccore.h>
#include <ogc/audio.h>
#include <string.h>

u32 underruns = 0;

#define AUDIO_SAMPLES 7680

static s16 audio_buffer[AUDIO_SAMPLES * 2]
    __attribute__((aligned(32)));

static void audio_fill_buffer(void)
{
    /*
     * Anything not supplied by the FIFO is silence.
     */
    memset(audio_buffer, 0, sizeof(audio_buffer));

    unsigned got = fifo_read(audio_buffer, AUDIO_SAMPLES);

    if (got < AUDIO_SAMPLES) {
        underruns++;
    }

    DCFlushRange(audio_buffer, sizeof(audio_buffer));
}

void audio_init(void)
{
    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);

    memset(audio_buffer, 0, sizeof(audio_buffer));
    DCFlushRange(audio_buffer, sizeof(audio_buffer));

    AUDIO_RegisterDMACallback(audio_fill_buffer);
    AUDIO_InitDMA((u32)audio_buffer, sizeof(audio_buffer));
    AUDIO_StartDMA();
}