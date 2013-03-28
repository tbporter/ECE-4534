#include "uart_thread.h"
#include "maindefs.h"
#include <stdio.h>
#include <string.h>
#include "relpic.h"
#include "zigBee.h"

// This is a "logical" thread that processes messages from the UART
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int uart_lthread(uart_thread_struct *uptr, int msgtype, int length, unsigned char *msgbuffer, Queue* rcvQ) {
    if (msgtype == MSGT_OVERRUN) {
    }
    else if (msgtype == MSGT_UART_DATA) {
        int i;
        char k;
        // print the message (this assumes that the message
        // 		was a printable string)
        //msgbuffer[length] = '\0'; // null-terminate the array as a string
        // Now we would do something with it

#ifdef ISRELPIC
        if (strncmp(msgbuffer, tag_finish,8) == 0) {
            // append 0xF to RCVQ
            appendQueue(rcvQ, FINISH);
        } else if (strncmp(msgbuffer, tag_slwdwn,8) == 0) {
            // append 0x8 to RCVQ
            appendQueue(rcvQ, SLWDWN);
        } else if (strncmp(msgbuffer, tag_spdupp,8) == 0) {
            // append 0x4 to RCVQ
            appendQueue(rcvQ, SPDUPP);
        } else if (strncmp(msgbuffer, tag_dirrgt,8) == 0) {
            // append 0x2 to RCVQ
            appendQueue(rcvQ, DIRRGT);
        } else if (strncmp(msgbuffer, tag_dirlft,8) == 0) {
            // append 0x1 to RCVQ
            appendQueue(rcvQ, DIRLFT);
        } else {
            // send error, shouldnt get here!
            appendQueue(rcvQ, ERROR);
        }
        ToMainLow_sendmsg(0, MSGT_RFID_READ, (void*)0);
#endif
#ifdef MASTERPIC

        for(i = 0; i < length; i++)
        {
            appendQueue(rcvQ, msgbuffer[i]);
        }

        doZigBee(length, msgbuffer, rcvQ);
    }
}