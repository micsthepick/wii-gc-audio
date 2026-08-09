#include "ringbuffer.h"

static s16 fifo[FIFO_SIZE];

/*
 * Only the SI interrupt modifies wpos.
 * Only the consumer modifies rpos.
 */
static volatile u32 rpos;
static volatile u32 wpos;

void fifo_init(void)
{
    rpos = 0;
    wpos = 0;
}

int fifo_write(const u8 *data, int len)
{
    int written = 0;

    while (written < len)
    {
        u32 next = wpos + 1;

        if (next >= FIFO_SIZE)
            next = 0;

        /*
         * FIFO full.
         *
         * Don't overwrite unread audio.
         */
        if (next == rpos)
            break;

        /*
         * Convert unsigned 8-bit PCM:
         *
         *   0   -> -32768
         *   128 ->      0
         *   255 ->  32512
         */
        fifo[wpos] = (s16)((s32)(data[written] - 128) * 256);

        wpos = next;
        written++;
    }

    return written;
}

int fifo_read(s16 *out, int samples)
{
    int read = 0;

    while (read < samples)
    {
        if (rpos == wpos)
            break;

        out[read++] = fifo[rpos];

        rpos++;

        if (rpos >= FIFO_SIZE)
            rpos = 0;
    }

    return read;
}

int fifo_available(void)
{
    if (wpos >= rpos)
        return wpos - rpos;

    return FIFO_SIZE - rpos + wpos;
}