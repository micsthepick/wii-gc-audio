#ifndef OPUS_AUDIO_H
#define OPUS_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <gctypes.h>

#define OPUS_SAMPLE_RATE 48000
#define OPUS_CHANNELS    2
#define OPUS_FRAME_SIZE  960

int opus_audio_init(void);
int opus_audio_decode(const uint8_t *packet, size_t len);
int opus_audio_decode_missing(void);

extern volatile u32 opus_errors;
extern volatile u32 opus_count;

#endif