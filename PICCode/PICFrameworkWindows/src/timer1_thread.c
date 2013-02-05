#include "maindefs.h"
#include <stdio.h>
#include "messages.h"
#include "timer1_thread.h"
#include "my_adc.h"

void init_timer1_lthread(timer1_thread_struct *tptr) {

}

// This is a "logical" thread that processes messages from TIMER1
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int value;

int timer1_lthread(timer1_thread_struct *tptr, int msgtype, int length, unsigned char *msgbuffer) {
    readADC(&value);
    msgbuffer[1] = value >> 8;
    msgbuffer[0] = value & 0xFF;

}