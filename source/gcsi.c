#include "gcsi.h"

#include <gccore.h>
#include <ogc/si.h>

static volatile int transfer_done;


static void si_callback(s32 chan, u32 type)
{
    transfer_done = 1;
}


void gcsi_init(void)
{
    transfer_done = 0;

    /*
     * SI is already initialised by libogc.
     * No SI_Init() exists in current libogc.
     */
}


int gcsi_read(int port, u8 *out)
{
    u8 command[3];

    /*
       GameCube controller poll command:

       0x41 = status/poll
       0x00
       0x00
    */

    command[0] = 0x41;
    command[1] = 0x00;
    command[2] = 0x00;


    transfer_done = 0;


    SI_Transfer(
        port,
        command,
        sizeof(command),
        out,
        8,
        si_callback,
        0
    );


    while (!transfer_done)
        ;

    return 0;
}