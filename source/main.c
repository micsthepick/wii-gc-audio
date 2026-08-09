#include <gccore.h>
#include <ogc/si.h>
#include <stdio.h>

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

static void si_poll_handler(u32 chan, void *data)
{
    if (chan != 0)
        return;

    fifo_write((const u8 *)data, 8);
}

static void si_init(void)
{
    u8 dummy[8];

    SI_GetResponse(0, dummy);

    SI_SetCommand(0, 0x00400300);
    SI_SetSamplingRate(4000);
    SI_EnablePolling(0x80000000);

    SI_RegisterPollingHandler(si_poll_handler);
    SI_EnablePollingInterrupt(1);

    SI_TransferCommands();
}

int main(int argc, char **argv)
{
    SYS_Init();
    PAD_Init();

    video_init();

    printf("GC audio streamer\n");

    fifo_init();
    audio_init();
    si_init();

    while (SYS_MainLoop())
    {
        /*
         * SI and audio DMA are interrupt-driven.
         */
    }

    return 0;
}