#include <gccore.h>
#include <stdio.h>
#include <string.h>

#include "gcsi.h"
#include "audio.h"
#include "ringbuffer.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void video_init(void)
{
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(
        SYS_AllocateFramebuffer(rmode)
    );

    console_init(
        xfb,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth * VI_DISPLAY_PIX_SZ
    );

    VIDEO_Configure(rmode);

    VIDEO_SetNextFramebuffer(xfb);

    VIDEO_SetBlack(FALSE);

    VIDEO_Flush();

    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
}


int main(int argc, char **argv)
{
    u8 p1[8];

    // Initialize system
    SYS_Init();
    PAD_Init();

    video_init();


    printf("GC audio streamer\n");


    gcsi_init();
    fifo_init();
    audio_init();


    while(SYS_MainLoop())
    {

        /*
         * Read one GC port.
         *
         * Each port provides
         * one 8-byte latch.
         */

        gcsi_read(
            0,
            p1
        );

        /*
         * Submit audio data from single port
         */
        audio_submit_single_port(p1);

        /*
         * Wait for vsync
         */
        VIDEO_WaitVSync();
        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
    }

    return 0;
}
