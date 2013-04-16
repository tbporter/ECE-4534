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
    if ((tptr->msgcount % 100) == 0) {
#ifdef MASTERPIC
        //ToMainLow_sendmsg(0, MSGT_ADC_DATA, (void*) 0);
        ToMainHigh_sendmsg(0, MSGT_POLL_PICS, (void*) 0);
#endif
        tptr->msgcount = 0;
    }
#ifdef MASTERPIC
    else if ((tptr->msgcount % 50) == 0)
    {
        ToMainHigh_sendmsg(0, MSGT_POLL_FLINE, (void*) 0);
        
    }
#endif
#ifdef MASTERPIC
    else if ((tptr->msgcount % 20) == 0)
    {
        ToMainHigh_sendmsg(0, MSGT_POLL_ENCDRS, (void*) 0);
        //tptr->msgcount = 0;
    }

#endif

}