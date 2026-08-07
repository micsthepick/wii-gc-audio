#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <gccore.h>

void fifo_init(void);
int fifo_write(u8 *data, int len);
int fifo_read(s16 *out, int samples);

#endif
