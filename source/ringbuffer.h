#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <gctypes.h>

#define FIFO_SIZE 32768

void fifo_init(void);

/*
 * Called from the SI interrupt.
 *
 * Writes unsigned 8-bit samples, converting them to signed
 * 16-bit samples.
 */
int fifo_write(const u8 *data, int len);

/*
 * Called from the main loop.
 */
int fifo_read(s16 *out, int samples);

int fifo_available(void);

#endif