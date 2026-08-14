// gcsi.h
#ifndef GCSI_H
#define GCSI_H

#include <ogc/si.h>

int si_poll(void);
void si_maybe_resume(void);

extern volatile int stalled;
extern volatile u32 si_callback_count;
extern volatile u32 si_error_count;
extern volatile u32 si_last_error;

#endif
