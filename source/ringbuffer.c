#include "ringbuffer.h"
#include "gcsi.h"

#include <string.h>

static s16 fifo[FIFO_SIZE * 2];

volatile u32 rpos;
volatile u32 wpos;

volatile int stalled = 0;

void fifo_init(void)
{
    rpos = 0;
    wpos = 0;
}

int fifo_write(s16 *data, int len)
{
    u32 r = rpos;
    u32 w = wpos;

    u32 used = w - r;

    if (used > FIFO_SIZE)
        used = FIFO_SIZE;

    u32 free = FIFO_SIZE - used;

    int actual = len;
    if ((u32)actual > free)
        actual = free;

    for (int i = 0; i < actual; i++) {
        u32 pos = (w + i) & FIFO_MASK;

        fifo[pos * 2 + 0] = data[i * 2 + 0];
        fifo[pos * 2 + 1] = data[i * 2 + 1];
    }

    wpos = w + actual;

    return actual;
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

u32 fifo_count(void)
{
    u32 r = rpos;
    u32 w = wpos;

    u32 used = w - r;

    if (used > FIFO_SIZE)
        used = FIFO_SIZE;

    return used;
}

u32 fifo_free(void)
{
    return FIFO_SIZE - fifo_count();
}