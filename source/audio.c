#include "audio.h"
#include "ringbuffer.h"

#include <gccore.h>
#include <ogc/audio.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_SAMPLES 512
static s16 audio_buffer[AUDIO_SAMPLES * 2] __attribute__((aligned(32)));

void audio_submit_single_port(const u8 *port_data)
{
    /*
     * TAStm32 should send actual audio latch data, not controller inputs
     * The audio latch contains 8 bytes of audio samples from one port
     */
    u8 audio_data[8];

    /* Copy the received audio data */
    memcpy(audio_data, port_data, 8);

    /* Feed FIFO with audio data */
    fifo_write(audio_data, 8);
}

static void audio_fill_buffer(void)
{
    /*
     * FIFO contains mono signed 16-bit samples.
     * AI wants interleaved stereo samples:
     *
     * L R L R L R ...
     */

    s16 mono[AUDIO_SAMPLES];

    int samples_available = fifo_read(
        mono,
        AUDIO_SAMPLES
    );

    /* If FIFO is empty, fill with zeros */
    if (samples_available == 0) {
        memset(audio_buffer, 0, sizeof(audio_buffer));
        return;
    }

    /* Fill available samples, rest remain from previous buffer */
    for (int i = 0; i < samples_available; i++) {
        audio_buffer[i * 2 + 0] = mono[i];
        audio_buffer[i * 2 + 1] = mono[i];
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
    // Initialize audio system
    AUDIO_Init(NULL);
    AUDIO_SetDSPSampleRate(AI_SAMPLERATE_32KHZ);

    // Clear audio buffer
    memset(audio_buffer, 0, sizeof(audio_buffer));
    DCFlushRange(audio_buffer, sizeof(audio_buffer));

    // Set up callback and DMA
    AUDIO_RegisterDMACallback(audio_callback);
    AUDIO_InitDMA((u32)audio_buffer, sizeof(audio_buffer));
    AUDIO_StartDMA();
}