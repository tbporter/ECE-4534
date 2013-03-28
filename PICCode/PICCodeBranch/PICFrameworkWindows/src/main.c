#include "maindefs.h"
#include <stdio.h>
#ifndef __XC8
#include <usart.h>
#include <i2c.h>
#include <timers.h>
#else
#include <plib/usart.h>
#include <plib/i2c.h>
#include <plib/timers.h>
#endif
#include "interrupts.h"
#include "messages.h"
#include "my_uart.h"
#include "my_i2c.h"
#include "uart_thread.h"
#include "timer1_thread.h"
#include "timer0_thread.h"
#include "queue.h"
#include "encoders.h"
#include "smcpic.h"
#include "my_adc.h"

#ifdef __USE18F45J10
// CONFIG1L
#pragma config WDTEN = OFF      // Watchdog Timer Enable bit (WDT disabled (control is placed on SWDTEN bit))
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable bit (Reset on stack overflow/underflow disabled)
#ifndef __XC8
// Have to turn this off because I don't see how to enable this in the checkboxes for XC8 in this IDE
#pragma config XINST = ON       // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode enabled)
#else
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode enabled)
#endif

// CONFIG1H
#pragma config CP0 = OFF        // Code Protection bit (Program memory is not code-protected)

// CONFIG2L
#pragma config FOSC = HSPLL     // Oscillator Selection bits (HS oscillator, PLL enabled and under software control)
#pragma config FOSC2 = ON       // Default/Reset System Clock Select bit (Clock selected by FOSC as system clock is enabled when OSCCON<1:0> = 00)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)
#pragma config IESO = ON        // Two-Speed Start-up (Internal/External Oscillator Switchover) Control bit (Two-Speed Start-up enabled)

// CONFIG2H
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = DEFAULT // CCP2 MUX bit (CCP2 is multiplexed with RC1)

#else
#ifdef __USE18F2680
#pragma config OSC = IRCIO67    // Oscillator Selection bits (Internal oscillator block, port function on RA6 and RA7)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
#pragma config BORV = 3         // Brown-out Reset Voltage bits (VBOR set to 2.1V)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<4:0> pins are configured as digital I/O on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer 1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = OFF     // Stack Full/Underflow Reset Enable bit (Stack full/underflow will not cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config BBSIZ = 1024     // Boot Block Size Select bits (1K words (2K bytes) Boot Block)
#ifndef __XC8
// Have to turn this off because I don't see how to enable this in the checkboxes for XC8 in this IDE
#pragma config XINST = ON       // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode enabled)
#endif
#else
//Something is messed up
#endif
#endif

void main(void) {
    char c;
    signed char length;
    unsigned char msgtype;
    unsigned char last_reg_recvd;
    uart_comm uc;
    i2c_comm ic;
    unsigned char msgbuffer[MSGLEN + 1];
    unsigned char i;
    //int lchange, rchange;
    uart_thread_struct uthread_data; // info for uart_lthread
    timer1_thread_struct t1thread_data; // info for timer1_lthread
    timer0_thread_struct t0thread_data; // info for timer0_lthread
    //unsigned char msg;

#ifdef MASTERPIC
    unsigned char LSonar = 0;
    unsigned char RSonar = 0;
    unsigned char FLIR = 0;
    unsigned char BLIR = 0;
    unsigned char FRIR = 0;
    unsigned char BRIR = 0;
    unsigned char linesensor = 0;
    unsigned int encoder1 = 0;
    unsigned int encoder2 = 0;
    unsigned int current = 0;
    unsigned char RFID = 0;
    Queue uartRXQ;
#endif
#ifdef ISRELPIC
    Queue rfidRXQ;
    Queue uartRXQ;
#endif
#ifdef ISSMCPIC
    Queue uartTXQ;
    
#endif

#ifdef __USE18F2680
    OSCCON = 0xFC; // see datasheet
    // We have enough room below the Max Freq to enable the PLL for this chip
    OSCTUNEbits.PLLEN = 1; // 4x the clock speed in the previous line
#else
    OSCCON = 0x82; // see datasheeet
    OSCTUNEbits.PLLEN = 0; // Makes the clock exceed the PIC's rated speed if the PLL is on
#endif

    // initialize my uart recv handling code
    init_uart_recv(&uc);

    // initialize the i2c code
    init_i2c(&ic);

    initADC();

    // init the timer1 lthread
    init_timer1_lthread(&t1thread_data);

    // initialize message queues before enabling any interrupts
    init_queues();

    // set direction for PORTB to output
    TRISB = 0x0;
    LATB = 0x0;

    // how to set up PORTA for input (for the V4 board with the PIC2680)
    /*
            PORTA = 0x0;	// clear the port
            LATA = 0x0;		// clear the output latch
            ADCON1 = 0x0F;	// turn off the A2D function on these pins
            // Only for 40-pin version of this chip CMCON = 0x07;	// turn the comparator off
            TRISA = 0x0F;	// set RA3-RA0 to inputs
     */

    // initialize Timers
    //OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_128);
   OpenTimer1(TIMER_INT_ON & T1_8BIT_RW & T1_PS_1_1 & T1_SOURCE_INT & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF);

    // Peripheral interrupts can have their priority set to high or low
    // enable high-priority interrupts and low-priority interrupts
    enable_interrupts();

    // Decide on the priority of the enabled peripheral interrupts
    // 0 is low, 1 is high
    // Timer1 interrupt
    IPR1bits.TMR1IP = 0;
#ifdef ISRELPIC
    // USART RX interrupt
    IPR1bits.RCIP = 0;
    // Encoder Interrupt on Change enable and set low priority
    //INTCONbits.RBIE = 1;
    INTCON2bits.RBIP = 0;
#endif
    // I2C interrupt
    IPR1bits.SSPIP = 1;

// Note that the temperature sensor Address bits (A0, A1, A2) are also the
// least significant bits of LATB -- take care when changing them
// They *are* changed in the timer interrupt handlers if those timers are
//   enabled.  They are just there to make the lights blink and can be
//   disabled.
#ifdef MASTERPIC
    i2c_configure_master();
    IPR1bits.RCIP = 0;
    createQueue(&uartRXQ, 10);
#elif ISRELPIC
    // configure slave accordingly
    i2c_configure_slave(RELPICADDR);
    // create rfid message queue and an uart rx queue
    createQueue(&rfidRXQ, 5);
    createQueue(&uartRXQ, 5);
    // initialize encoders
    initEncoders();
#elif ISSMCPIC
    i2c_configure_slave(SMCPICADDR);
    createQueue(&uartTXQ, 5);
#endif


    // must specifically enable the I2C interrupts
    PIE1bits.SSPIE = 1;

    // configure the hardware USART device
    OpenUSART(USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE & USART_EIGHT_BIT &
            USART_CONT_RX & USART_BRGH_LOW, 0x33);

    /* Junk to force an I2C interrupt in the simulator (if you wanted to)
    PIR1bits.SSPIF = 1;
    _asm
    goto 0x08
    _endasm;
     */

    // printf() is available, but is not advisable.  It goes to the UART pin
    // on the PIC and then you must hook something up to that to view it.
    // It is also slow and is blocking, so it will perturb your code's operation
    // Here is how it looks: printf("Hello\r\n");


    // loop forever
    // This loop is responsible for "handing off" messages to the subroutines
    // that should get them.  Although the subroutines are not threads, but
    // they can be equated with the tasks in your task diagram if you
    // structure them properly.
    while (1) {
        // Call a routine that blocks until either on the incoming
        // messages queues has a message (this may put the processor into
        // an idle mode)
        block_on_To_msgqueues();

        // At this point, one or both of the queues has a message.  It
        // makes sense to check the high-priority messages first -- in fact,
        // you may only want to check the low-priority messages when there
        // is not a high priority message.  That is a design decision and
        // I haven't done it here.
        length = ToMainHigh_recvmsg(MSGLEN, &msgtype, (void *) msgbuffer);
        if (length < 0) {
            // no message, check the error code to see if it is concern
            if (length != MSGQUEUE_EMPTY) {
                // This case be handled by your code.
            }
        } else {
            switch (msgtype) {
                case MSGT_TIMER0:
                {
                    timer0_lthread(&t0thread_data, msgtype, length, msgbuffer);
                    break;
                };
                case MSGT_I2C_DATA:
                {
                    last_reg_recvd = msgbuffer[0];
#ifdef MASTERPIC
                    switch (last_reg_recvd) {
                        case SAMPLEDISTANCE:
                        {
                            LSonar = msgbuffer[1];
                            RSonar = msgbuffer[2];
                            FLIR = msgbuffer[3];
                            BLIR = msgbuffer[4];
                            FRIR = msgbuffer[5];
                            BRIR = msgbuffer[6];
                            break;
                        };
                        case SAMPLEENCODERL:
                        {
                            encoder1 = msgbuffer[1];
                            encoder1 = encoder1 << 8;
                            encoder1 = encoder1 | msgbuffer[2];
                            break;
                        };
                        case SAMPLEENCODERR:
                        {
                            encoder2 = msgbuffer[1];
                            encoder2 = encoder2 << 8;
                            encoder2 = encoder2 | msgbuffer[2];
                            break;
                        };
                        case SAMPLELINE:
                        {
                            linesensor = msgbuffer[1];
                            break;
                        };
                        case SAMPLECURRENT:
                        {
                            current = msgbuffer[1];
                            break;
                        };
                        case SAMPLERFID:
                        {
                            if(msgbuffer[1] != 0)
                                RFID = msgbuffer[1];
                            break;
                        };
                        default:
                        {
                            //putsUSART((char*) "Oh no! I2C Unknown Message Recvd.");
                            break;
                        };
                    };
#endif
                    break;
                };
                case MSGT_I2C_DBG:
                {
                    // Here is where you could handle debugging, if you wanted
                    // keep track of the first byte received for later use (if desired)
                    last_reg_recvd = msgbuffer[0];
                    break;
                };
                case MSGT_I2C_RQST:
                {
                    // Generally, this is *NOT* how I recommend you handle an I2C slave request
                    // I recommend that you handle it completely inside the i2c interrupt handler
                    // by reading the data from a queue (i.e., you would not send a message, as is done
                    // now, from the i2c interrupt handler to main to ask for data).
                    //
                    // The last byte received is the "register" that is trying to be read
                    // The response is dependent on the register.
                    switch (last_reg_recvd) {
#ifdef ISRELPIC
                        // command to send rfid tag, if there is something in the queue, send it else send 0
                        case 0xf6:
                        {
                            length = 2;
                            msgbuffer[0] = 0xf6;
                            if (!isQEmpty(&rfidRXQ))
                                msgbuffer[1] = readQueue(&rfidRXQ, &msg);
                            else
                                msgbuffer[1] = 0x30;
                            break;
                        }
                        // command to send line sensor readings
                        case 0xf5:
                        {
                            length = 2;
                            msgbuffer[0] = 0x3A;
                            msgbuffer[1] = 0x3B;
                            break;
                        }
                        case 0xf2:
                        {
                            length = 3;
                            lchange = reportLChange();
                            msgbuffer[0] = 0xf2;
                            msgbuffer[1] = (char)(lchange >> 8);
                            msgbuffer[2] = (char)lchange;
                            break;
                        }
                        case 0xf3:
                        {
                            length = 3;
                            rchange = reportRChange();
                            msgbuffer[0] = 0xf3;
                            msgbuffer[1] = (char)(rchange >> 8);
                            msgbuffer[2] = (char)rchange;
                            break;
                        }
                        case 0xf4:
                        {
                            length = 2;
                            msgbuffer[0] = 0xf4;
                            msgbuffer[1] = 0x30;
                            break;
                        }
                        default:
                        {
                            length = 3;
                            msgbuffer[0] = 0xff;
                            msgbuffer[1] = 0xff;
                            msgbuffer[2] = 0xff;
                        }
#endif
                    };
                    start_i2c_slave_reply(length, msgbuffer);
                    break;
                };

                case MSGT_SEND_MTRCMD:
                {
#ifdef MASTERPIC
                    unsigned char buf[4];
                    buf[0] = RELPICADDR;
                    buf[1] = SENDMTRCMD;
                    buf[2] = msgbuffer[0];
                    buf[3] = msgbuffer[1];
                    i2c_master_send(4, &buf);
                    //WriteUSART(0x99);
                    PORTBbits.RB5 = 1;
#endif
                    break;
                };

                default:
                {
                    // Your code should handle this error
                    break;
                };
            };
        }

        // Check the low priority queue
        length = ToMainLow_recvmsg(MSGLEN, &msgtype, (void *) msgbuffer);
        if (length < 0) {
            // no message, check the error code to see if it is concern
            if (length != MSGQUEUE_EMPTY) {
                // Your code should handle this situation
            }
        } else {
            switch (msgtype) {
                case MSGT_TIMER1:
                {
                    timer1_lthread(&t1thread_data, msgtype, length, msgbuffer);
                    break;
                };
                case MSGT_OVERRUN:
                case MSGT_UART_DATA:
                {
#ifdef ISRELPIC
                    uart_lthread(&uthread_data, msgtype, length, msgbuffer, &rfidRXQ);
#endif
#ifdef MASTERPIC
                    uart_lthread(&uthread_data, msgtype, length, msgbuffer, &uartRXQ);
#endif

                    break;
                };
                default:
                {
                    // Your code should handle this error
                    break;
                };
            };
        }
    }

}