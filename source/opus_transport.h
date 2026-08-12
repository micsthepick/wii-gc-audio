#ifndef OPUS_TRANSPORT_H
#define OPUS_TRANSPORT_H

#define OPUS_MAX_PACKET        1500
#define OPUS_PACKET_QUEUE_SIZE 40
#define OPUS_PACKET_QUEUE_HIGH_WATER 8
#define OPUS_TRANSFER_SIZE     128
#define OPUS_TRANSFER_HEADER   1
#define OPUS_RECORD_HEADER     3

#include <stdint.h>

void opus_transport_init(void);
void opus_transport_push(const uint8_t *data, unsigned len);
void opus_transport_process(void);
int queue_full(void);
int queue_empty(void);
int opus_transport_needs_backpressure(void);


#endif
