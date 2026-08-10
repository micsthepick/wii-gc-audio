#include "ringbuffer.h"
#include "gcsi.h"

#include <string.h>

/*
 * FIFO_SIZE is the number of mono samples.
 * Each entry is stored as L/R s16, so the actual array
 * contains FIFO_SIZE * 2 s16 values.
 */
static s16 fifo[FIFO_SIZE * 2];

volatile u32 rpos;
volatile u32 wpos;

volatile int stalled = 0;

void fifo_init(void)
{
    rpos = 0;
    wpos = 0;
}

int fifo_write(const u8 point)
{
    u32 w = wpos;

    s16 sample = ((s16)point - 128) << 10;

    fifo[(w & FIFO_MASK) * 2 + 0] = sample;
    fifo[(w & FIFO_MASK) * 2 + 1] = sample;

    wpos = w + 1;

    return 1;
}

int fifo_read(s16 *out, int samples)
{
    u32 r = rpos;
    u32 w = wpos;
    u32 available = w - r;

    if (available > FIFO_SIZE)
        available = FIFO_SIZE;

    int actual = samples;

    if ((u32)actual > available)
        actual = available;

    for (int i = 0; i < actual; i++) {
        u32 pos = (r + i) & FIFO_MASK;

        out[i * 2 + 0] = fifo[pos * 2 + 0];
        out[i * 2 + 1] = fifo[pos * 2 + 1];

        fifo[pos * 2 + 0] = 0;
        fifo[pos * 2 + 1] = 0;
    }

    rpos = r + actual;

    if (stalled) {
        si_poll();
        stalled = 0;
    }

    return actual;
}