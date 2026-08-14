#include "gcsi.h"
#include "ringbuffer.h"
#include "opus_transport.h"

volatile u32 si_callback_count = 0;
volatile u32 si_error_count = 0;
volatile u32 si_last_error = 0;

static u8 si_response[128] __attribute__((aligned(32)));

static u8 si_request[1] = {
    0x55
};

static void si_transfer_callback(s32 chan, u32 error)
{
    (void)chan;

    si_callback_count++;

    if (error != 0) {
        si_error_count++;
        si_last_error = error;
    } else {
        opus_transport_push(si_response, 128);
    }

    if (fifo_count() >= FIFO_HIGH_WATER ||
        opus_transport_needs_backpressure()) {
        stalled = 1;
    }
}

int si_poll(void)
{
    if (fifo_count() >= FIFO_HIGH_WATER ||
        opus_transport_needs_backpressure()) {
        return 0;
    }

    return SI_Transfer(
        0,
        si_request,
        1,          // 1 TX byte
        si_response,
        128,        // 128 RX bytes
        si_transfer_callback,
        1000        // At most one poll per millisecond
    );
}

void si_service(void)
{
    if (fifo_count() >= FIFO_HIGH_WATER ||
        opus_transport_needs_backpressure()) {
        stalled = 1;
        return;
    }

    stalled = 0;
    si_poll();
}

void si_maybe_resume(void)
{
    if (stalled)
        si_service();
}
