#ifndef GCSI_H
#define GCSI_H

#include <gccore.h>

void gcsi_init(void);
int gcsi_read(int port, u8 *out);

#endif
