#include "gcsi.h"
#include "ringbuffer.h"

#include <ogc/si.h>

static u8 si_response[8] __attribute__((aligned(32)));

static const u8 si_command[3] = {
    0x40,
    0x03,
    0x00
};

static void add_bits(u8 data, u8 len)
{
    fifo_write(data & 0xf);

    if (len == 8)
        fifo_write((data >> 4) & 0xf);
}

inline static void decode4(u8 data)
{
    add_bits(data, 4);
}

inline static void decode8(u8 data)
{
    add_bits(data, 8);
}

static void si_transfer_callback(s32 chan, u32 error)
{
    (void)chan;
    (void)error;

    decode8(si_response[0]);
    decode4(si_response[1]);
    decode8(si_response[2]);
    decode8(si_response[3]);
    decode8(si_response[4]);
    decode8(si_response[5]);
    decode8(si_response[6]);
    decode8(si_response[7]);

    while ((u32)(wpos - rpos) >= FIFO_SIZE / 2) {
        stalled = 1;
        return;
    }
    si_poll();
}

int si_poll(void)
{
    return SI_Transfer(
        0,
        (void *)si_command,
        sizeof(si_command),
        si_response,
        sizeof(si_response),
        si_transfer_callback,
        0
    );
}