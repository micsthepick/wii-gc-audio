#include "audio.h"
#include "ringbuffer.h"

#include <gccore.h>
#include <ogc/audio.h>
#include <string.h>

u32 underruns = 0;

#define AUDIO_SAMPLES 7680
#define AUDIO_BUFFER_COUNT 2

static s16 audio_buffers[AUDIO_BUFFER_COUNT][AUDIO_SAMPLES * 2]
    __attribute__((aligned(32)));

static unsigned next_audio_buffer;

static void audio_prepare_buffer(unsigned index)
{
    s16 *buffer = audio_buffers[index];

    /*
     * Anything not supplied by the FIFO is silence.
     */
    memset(buffer, 0, sizeof(audio_buffers[index]));

    unsigned got = fifo_read(buffer, AUDIO_SAMPLES);

    if (got < AUDIO_SAMPLES) {
        underruns++;
    }

    DCFlushRange(buffer, sizeof(audio_buffers[index]));
}

static void audio_dma_callback(void)
{
    unsigned index = next_audio_buffer;

    audio_prepare_buffer(index);
    AUDIO_InitDMA(
        (u32)audio_buffers[index],
        sizeof(audio_buffers[index])
    );

    next_audio_buffer = (index + 1) % AUDIO_BUFFER_COUNT;
}

void audio_init(void)
{
    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);

    memset(audio_buffers, 0, sizeof(audio_buffers));
    DCFlushRange(audio_buffers, sizeof(audio_buffers));

    next_audio_buffer = 1;

    AUDIO_RegisterDMACallback(audio_dma_callback);
    AUDIO_InitDMA(
        (u32)audio_buffers[0],
        sizeof(audio_buffers[0])
    );
    AUDIO_StartDMA();
}
