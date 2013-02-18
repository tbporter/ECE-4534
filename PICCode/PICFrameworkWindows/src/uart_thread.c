#include "maindefs.h"
#include <stdio.h>
#include "uart_thread.h"

// This is a "logical" thread that processes messages from the UART
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int uart_lthread(uart_thread_struct *uptr, int msgtype, int length, unsigned char *msgbuffer, Queue *rcvQ) {
    int i;
    if (msgtype == MSGT_OVERRUN) {
    } else if (msgtype == MSGT_UART_DATA) {
        // print the message (this assumes that the message
        // 		was a printable string)
        msgbuffer[length] = '\0'; // null-terminate the array as a string
#ifdef DEBUG_WRITE
        WriteUSART('a');
        //WriteUSART('\n');
#endif
        // Now we would do something with it
    }
    else if (msgtype == MSGT_UART_RCV)
    {
        for (i = 0; i < length; i++)
        {
            appendQueue(rcvQ, msgbuffer[i]);
#ifdef DEBUG_READ
            WriteUSART((char*)msgbuffer[i]);
            WriteUSART('\n');
#endif
        }
        ToMainHigh_sendmsg(0, MSGT_I2C_DATA, (void *) 0);
    }
}