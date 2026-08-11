// gcsi.h
#ifndef GCSI_H
#define GCSI_H

#include <ogc/si.h>

int si_poll(void);

extern volatile int stalled;
extern volatile u32 si_callback_count;

#endif