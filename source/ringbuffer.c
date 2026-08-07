#include "ringbuffer.h"

#define FIFO_SIZE 32768

static s16 fifo[FIFO_SIZE];
static volatile int rpos, wpos;

void fifo_init(void)
{
    rpos = wpos = 0;
}

int fifo_write(u8 *data, int len)
{
    for (int i = 0; i < len; i++) {
        fifo[wpos++] = ((s16)data[i] - 128) << 8;
        if (wpos >= FIFO_SIZE)
            wpos = 0;
    }

    return len;
}

int fifo_read(s16 *out, int samples)
{
    for (int i = 0; i < samples; i++) {
        out[i] = fifo[rpos++];
        if (rpos >= FIFO_SIZE)
            rpos = 0;
    }

    return samples;
}
