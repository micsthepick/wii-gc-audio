#ifndef AUDIO_H
#define AUDIO_H

#include <gccore.h>

void audio_init(void);
void audio_submit_single_port(const u8 *port_data);

#endif