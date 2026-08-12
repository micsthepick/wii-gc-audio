#include "gcsi.h"
#include "ringbuffer.h"
#include "opus_transport.h"

#include <ogc/irq.h>


volatile u32 si_callback_count = 0;
volatile u32 si_error_count = 0;
volatile u32 si_last_error = 0;
volatile u32 si_poll_count = 0;
volatile u32 si_poll_accepted_count = 0;
volatile u32 si_poll_rejected_count = 0;

static volatile int si_in_flight = 0;

static u8 si_response[128] __attribute__((aligned(32)));

static u8 si_request[1] = {
    0x55
};

static void si_transfer_callback(s32 chan, u32 error)
{
    (void)chan;

    /* libogc has completed the transfer, including error completions. */
    si_in_flight = 0;
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
    u32 level = IRQ_Disable();

    if (si_in_flight ||
        fifo_count() >= FIFO_HIGH_WATER ||
        opus_transport_needs_backpressure()) {
        IRQ_Restore(level);
        return 0;
    }

    si_poll_count++;
    si_in_flight = 1;

    if (!SI_Transfer(
        0,
        si_request,
        1,          // 1 TX byte
        si_response,
        128,        // 128 RX bytes
        si_transfer_callback,
        1000        // At most one poll per millisecond
    )) {
        /* The request was not accepted; retry from a later service call. */
        si_in_flight = 0;
        si_poll_rejected_count++;
        IRQ_Restore(level);
        return 0;
    }

    si_poll_accepted_count++;
    IRQ_Restore(level);
    return 1;
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
