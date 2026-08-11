#include "gcsi.h"
#include "ringbuffer.h"
#include "opus_transport.h"


volatile u32 si_callback_count = 0;

static u8 si_response[128] __attribute__((aligned(32)));

static u8 si_request[1] = {
    0x55
};

static void si_transfer_callback(s32 chan, u32 error)
{
    (void)chan;
    (void)error;

    si_callback_count++;

    opus_transport_push(si_response, 128);

    if (fifo_count() >= FIFO_HIGH_WATER) {
        stalled = 1;
        return;
    }

    si_poll();
}

int si_poll(void)
{
    return SI_Transfer(
        0,
        si_request,
        1,          // 1 TX byte
        si_response,
        128,        // 128 RX bytes
        si_transfer_callback,
        0
    );
}