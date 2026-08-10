#include <gccore.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <sys/types.h>
#include <wiiuse/wpad.h>

#include "gcsi.h"
#include "audio.h"
#include "ringbuffer.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

/*
#define SI_RATE_NUM   60
#define SI_RATE_DENOM 128000

#define SI_PERIOD_TICKS \
    (((u64)PPC_BUS_CLOCK / 4) * SI_RATE_NUM / SI_RATE_DENOM)
*/

//static int timer_offset = 0;

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
    SYS_Init();
    PAD_Init();

    video_init();

    printf("GC audio streamer\n");

    fifo_init();
    audio_init();

    //u64 next_poll = gettime();

    // first poll (next poll starts from cb)
    si_poll();

    while (SYS_MainLoop()) {
        for (int i = 0; i < 100; i++) {
            VIDEO_WaitVSync();
        }

       printf("wpos - rpos: %u \n", (u32)(wpos - rpos));
    }

    return 0;
}