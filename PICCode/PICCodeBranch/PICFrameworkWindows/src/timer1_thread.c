#include "maindefs.h"
#include <stdio.h>
#include "messages.h"
#include "my_i2c.h"
#include "timer1_thread.h"
#include "my_adc.h"

void init_timer1_lthread(timer1_thread_struct *tptr) {
    tptr->counter = 0;
    tptr->message[0] = '0';
    tptr->message[1] = '0';
}

// This is a "logical" thread that processes messages from TIMER1
// It is not a "real" thread because there is only the single main thread
// of execution on the PIC because we are not using an RTOS.

int value;

int timer1_lthread(timer1_thread_struct *tptr, int msgtype, int length, unsigned char *msgbuffer) {
    tptr->counter++;
#ifdef MASTERPIC
    
//    if(tptr->counter == 10)
//    {
//        tptr->message[0] = SMCPICADDR;
//        tptr->message[1] = SAMPLEDISTANCE;
//        i2c_master_send(2, tptr->message);
//        i2c_master_recv(7, SMCPICADDR);
//    }
//    if(tptr->counter == 5)
//    {
//        tptr->message[0] = RELPICADDR;
//        tptr->message[1] = SAMPLEENCODERL;
//        i2c_master_send(2,tptr->message);
//        i2c_master_recv(3,RELPICADDR);
//
//    }
//    if(tptr->counter == 15)
//    {
//        tptr->message[1] = SAMPLEENCODERR;
//        i2c_master_send(2,tptr->message);
//        i2c_master_recv(3,RELPICADDR);
//    }
//
//    if(tptr->counter == 20)
//    {
//        tptr->message[0] = RELPICADDR;
//        tptr->message[1] = SAMPLERFID;
//        i2c_master_send(2,tptr->message);
//        i2c_master_recv(2,RELPICADDR);
//    }
//    if(tptr->counter == 25)
//    {
//        tptr->message[0] = RELPICADDR;
//        tptr->message[1] = SAMPLELINE;
//        i2c_master_send(2,tptr->message);
//        i2c_master_recv(2,RELPICADDR);
//    }
//    if(tptr->counter == 30)
//    {
//        tptr->message[0] = SMCPICADDR;
//        tptr->message[1] = SAMPLECURRENT;
//        i2c_master_send(2,tptr->message);
//        i2c_master_recv(2, SMCPICADDR);
//        tptr->counter = 0;
//    }
#endif
#ifdef ISSMCPIC
    if (tptr->counter % 5 == 0)
    {
        //ToMainLow_sendmsg(0, MSGT_ADC_DATA, (void*)0);
        tptr->counter = 0;
    }
#endif
}