#include "maindefs.h"
#include <stdio.h>
#include "uart_thread.h"
#include "zigBee.h"

#include "relpic.h"
#include "../../../../common/MESSAGES_G9.h"




// This is a "logical" thread that processes messages from the UART
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int uart_lthread(uart_thread_struct *uptr, int msgtype, int length, unsigned char *msgbuffer, Queue* rcvQ) {
    if (msgtype == MSGT_OVERRUN) {
    } else if (msgtype == MSGT_UART_DATA) {
        int i;
        // print the message (this assumes that the message
        // 		was a printable string)
        //msgbuffer[length] = '\0'; // null-terminate the array as a string
        // Now we would do something with it

#ifdef MASTERPIC
        for (i = 0; i < length; i++) {
            appendQueue(rcvQ, msgbuffer[i]);
        }

        doZigBee(length, msgbuffer, rcvQ);
#endif
    } else if (msgtype == MSGT_UART_RFID) {

        char i, count;
        PORTCbits.RC0 = 1;

        for (i = 0; i < 16; i++)
        {
            WriteUSART(msgbuffer[i]);
            while (BusyUSART());
        }

//                for (i = 0; i < 16; i++)
//                {
//                    if (msgbuffer[i] == tag_spdupp[i])
//                    {
//                        WriteUSART(i);
//                        while(BusyUSART());
//                    }
//                }

//        // check what kind of tag it is, we only care about the first 8 bits
//        if (strncmp(msgbuffer, tag_finish, 9) == 0) {
//            PORTCbits.RC1 = 1;
//            appendQueue(rcvQ, Finish);
//        }
        //else if (strcmp(msgbuffer, tag_slwdwn) == 0) {
//            PORTCbits.RC2 = 1;
//            appendQueue(rcvQ, SlowDown);
//        } else if (strncmp(msgbuffer, tag_spdupp, 9) == 0) {
//            appendQueue(rcvQ, SpeedUp);
//        } else if (strncmp(msgbuffer, tag_dirlft, 9) == 0) {
//            appendQueue(rcvQ, GoLeft);
//        } else if (strncmp(msgbuffer, tag_dirrgt, 9) == 0) {
//            appendQueue(rcvQ, GoRight);
//        } else {
//            appendQueue(rcvQ, Error);
//        }
        //ToMainLow_sendmsg(0, MSGT_RFID_READ, (void *) 0);
    }
}