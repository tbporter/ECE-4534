#include "maindefs.h"
#include <stdio.h>
#include "messages.h"
#include "timer1_thread.h"

void init_timer1_lthread(timer1_thread_struct *tptr) {
    tptr->msgcount = 0;
}

// This is a "logical" thread that processes messages from TIMER1
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int timer1_lthread(timer1_thread_struct *tptr, int msgtype, int length, unsigned char *msgbuffer) {
    tptr->msgcount++;
    // Every tenth message we get from timer1 we
    // send something to the High Priority Interrupt
    if ((tptr->msgcount % 2) == 0) {
        ToMainHigh_sendmsg(0, MSGT_POLL_PICS, (void*) 0);
        PORTBbits.RB4 = !PORTBbits.RB4;
    }
}