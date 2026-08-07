#include <gccore.h>
#include <string.h>

#include "gcsi.h"
#include "audio.h"
#include "ringbuffer.h"

int main(int argc, char **argv)
{
    u8 p1[8];
    u8 p2[8];
    u8 frame[16];

    gcsi_init();
    fifo_init();
    audio_init();

    while (1) {
        gcsi_read(0, p1);
        gcsi_read(1, p2);

        memcpy(frame, p1, 8);
        memcpy(frame + 8, p2, 8);

        fifo_write(frame, 16);
    }

    return 0;
}
