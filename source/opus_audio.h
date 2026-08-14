#ifndef OPUS_AUDIO_H
#define OPUS_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <gctypes.h>

#define OPUS_SAMPLE_RATE 48000
#define OPUS_CHANNELS    2
#define OPUS_FRAME_SIZE  960
#define OPUS_MAX_FRAME_SIZE OPUS_FRAME_SIZE

int opus_audio_init(void);
void opus_audio_reset(void);
int opus_audio_decode(const uint8_t *packet, size_t len);
int opus_audio_decode_missing(void);

extern volatile u32 opus_errors;
extern volatile s32 opus_last_error;
extern volatile s32 opus_last_packet_samples;
extern volatile u32 opus_last_packet_length;
extern volatile u32 opus_last_packet_toc;
extern volatile u32 opus_count;

#endif
