#include "ringbuffer.h"

#define FIFO_SIZE 32768

static s16 fifo[FIFO_SIZE];
static volatile int rpos, wpos;
static volatile int count;

void fifo_init(void)
{
    rpos = wpos = 0;
    count = 0;
}

int fifo_write(u8 *data, int len)
{
    int written = 0;
    for (int i = 0; i < len; i++) {
        if (count >= FIFO_SIZE)
            break;

        fifo[wpos++] = (s16)(data[i] - 128) * 256;
        count++;
        written++;

        if (wpos >= FIFO_SIZE)
            wpos = 0;
    }

    return written;
}

int fifo_read(s16 *out, int samples)
{
    int actual_samples = (samples < count) ? samples : count;

    for (int i = 0; i < actual_samples; i++) {
        out[i] = fifo[rpos++];
        if (rpos >= FIFO_SIZE)
            rpos = 0;
    }

    count -= actual_samples;
    return actual_samples;
}
