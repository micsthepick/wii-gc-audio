#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <gctypes.h>
#include "opus_audio.h"

#define FIFO_SIZE (1<<14)
#define FIFO_MASK (FIFO_SIZE-1)
#define FIFO_HIGH_WATER (FIFO_SIZE - 2048)

_Static_assert(
    FIFO_SIZE >= OPUS_MAX_FRAME_SIZE,
    "FIFO must hold the largest Opus frame"
);

void fifo_init(void);

/*
 * Called from the SI interrupt.
 *
 * Writes unsigned 8-bit samples, converting them to signed
 * 16-bit samples.
 */
int fifo_write(s16 *data, int len);

/*
 * Called from the main loop.
 */
int fifo_read(s16 *out, int samples);

u32 fifo_free(void);

u32 fifo_count(void);

extern volatile u32 rpos;
extern volatile u32 wpos;

#endif
