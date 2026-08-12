#include <gccore.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <sys/types.h>
#include <wiiuse/wpad.h>

#include "gcsi.h"
#include "audio.h"
#include "ringbuffer.h"
#include "opus_audio.h"
#include "opus_transport.h"

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
    SYS_Init();
    PAD_Init();

    video_init();

    printf("GC audio streamer\n");

    fifo_init();
    audio_init();

    opus_audio_init();
    opus_transport_init();


    while (SYS_MainLoop()) {
        si_service();
        opus_transport_process();
    }

    return 0;
}
