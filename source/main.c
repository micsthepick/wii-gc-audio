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

#define TICKS_PER_SECOND 18225000ULL

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

static u64 stats_time;

static void print_stats(void)
{
    u64 now = gettime();

    if (stats_time == 0) {
        stats_time = now;
        return;
    }

    u32 ms = diff_msec(stats_time, now);

    if (ms < 5000)
        return;

    printf(
        "SI: %.1f/s %.1f kbit/s | "
        "Opus: %.1f/s errors=%u underruns=%u\n",
        si_callback_count * 1000.0f / ms,
        si_callback_count * 128 * 8.0f / ms,
        opus_count * 1000.0f / ms,
        opus_errors,
        underruns
    );
    printf("fifo=%u\n", fifo_count());

    si_callback_count = 0;
    opus_count = 0;
    opus_errors = 0;
    underruns = 0;
    stats_time = now;
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

    // first poll (next poll starts from cb)
    si_poll();

    while (SYS_MainLoop()) {
        opus_transport_process();
        print_stats();
    }

    return 0;
}