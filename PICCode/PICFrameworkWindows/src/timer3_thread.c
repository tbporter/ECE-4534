#include "maindefs.h"
#include <stdio.h>
#include "messages.h"
#include "timer3_thread.h"
#include "my_adc.h"

void init_timer3_lthread(timer3_thread_struct *tptr) {

}

// This is a "logical" thread that processes messages from TIMER1
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int reading;

int timer3_lthread(timer3_thread_struct *tptr, int msgtype, int length, unsigned char *msgbuffer) {
    readADC(&reading);
    msgbuffer[1] = reading >> 8;
    msgbuffer[0] = reading & 0xFF;

}
