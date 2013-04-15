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
#include "zigBee.h"
#include "my_adc.h"
#include "relpic.h"
#include "../../../../common/MESSAGES_G9.h"
#include <string.h>
#include "encoders.h"

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


/*******************************************************************************/
// 670071014A5D
//char tag_finish[] = {'\x02', '6', '7', '0', '0', '7', '1', '0', '1', '4', 'A', '5', 'D', '\xd', '\xa', '\x03', '\0'};
char tag_finish[] = {'\x02', '6', '7', '0', '0', '7', '1', '0', '1'};
// 670072BEE04B
//char tag_slwdwn[] = {'\x02', '6', '7', '0', '0', '7', '2', 'B', 'E', 'E', '0', '4', 'B', '\xd', '\xa', '\x03', '\0'};
char tag_slwdwn[] = {'\x02', '6', '7', '0', '0', '7', '2', 'B', 'E'};
// 670072818B1F
//char tag_spdupp[] = {'\x02', '6', '7', '0', '0', '7', '2', '8', '1', '8', 'B', '1', 'F', '\xd', '\xa', '\x03', '\0'};
char tag_spdupp[] = {'\x02', '6', '7', '0', '0', '7', '2', '8', '1'};
// 670072C05184
//char tag_dirrgt[] = {'\x02', '6', '7', '0', '0', '7', '2', 'C', '0', '5', '1', '8', '4', '\xd', '\xa', '\x03', '\0'};
char tag_dirrgt[] = {'\x02', '6', '7', '0', '0', '7', '2', 'C', '0'};
// 670072AF48F2
//char tag_dirlft[] = {'\x02', '6', '7', '0', '0', '7', '2', 'A', 'F', '4', '8', 'F', '2', '\xd', '\xa', '\x03', '\0'};
char tag_dirlft[] = {'\x02', '6', '7', '0', '0', '7', '2', 'A', 'F'};
/*******************************************************************************/

void main(void) {
    unsigned char i;
    signed char length, slength, rlength;
    unsigned char msgtype;
    unsigned char last_reg_recvd;
    uart_comm *uc;
    i2c_comm *ic;
    unsigned char msgbuffer[MSGLEN + 1];
    uart_thread_struct uthread_data; // info for uart_lthread
    timer1_thread_struct t1thread_data; // info for timer1_lthread
    timer0_thread_struct t0thread_data; // info for timer0_lthread

#ifdef MASTERPIC
    unsigned char FLIR = 0;
    unsigned char FRIR = 0;
    unsigned char BLIR = 0;
    unsigned char BRIR = 0;
    unsigned char SONL = 0;
    unsigned char SONR = 0;
    unsigned char CURS = 0;

#endif

#ifdef SLAVEPIC
    int sen0 = 0, sen1 = 0, sen2 = 0, sen3 = 0, sen4 = 0, sen5 = 0, sen6 = 0, sen7 = 0;
    int sencount = 0;
#endif

    unsigned char RTAG = Error;
    int lencoder = 0;
    int rencoder = 0;
    Queue* uartRXQ;
    Queue* uartTXQ;
    Queue* rfidRXQ;
    g9Msg txMsg;
    createQueue(uartRXQ, 10);
    createQueue(uartTXQ, 10);
    createQueue(rfidRXQ, 5);
    

#ifdef __USE18F2680
    OSCCON = 0xFC; // see datasheet
    // We have enough room below the Max Freq to enable the PLL for this chip
    OSCTUNEbits.PLLEN = 1; // 4x the clock speed in the previous line
#else
    OSCCON = 0x82; // see datasheeet
    OSCTUNEbits.PLLEN = 0; // Makes the clock exceed the PIC's rated speed if the PLL is on
#endif

    // initialize my uart recv handling code
    init_uart_recv(uc);

    // initialize the i2c code
    init_i2c(ic);

    // init the timer1 lthread
    init_timer1_lthread(&t1thread_data);

    // initialize message queues before enabling any interrupts
    init_queues();

    // init ADC
    initADC();

    // set direction for PORTB to output
    TRISC = 0x0;
    LATC = 0x0;
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
    OpenTimer1(TIMER_INT_ON & T1_PS_1_1 & T1_16BIT_RW & T1_SOURCE_INT & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF);

    // Peripheral interrupts can have their priority set to high or low
    // enable high-priority interrupts and low-priority interrupts
    enable_interrupts();

    // Decide on the priority of the enabled peripheral interrupts
    // 0 is low, 1 is high
    // Timer1 interrupt
    IPR1bits.TMR1IP = 0;
    // USART RX interrupt
    IPR1bits.RCIP = 0;
    // I2C interrupt
    IPR1bits.SSPIP = 1;

#ifdef SLAVEPIC
    INTCON2bits.RBIP = 0;
    initEncoders();
#endif

    // configure the hardware i2c device as a slave (0x9E -> 0x4F) or (0x9A -> 0x4D)
#if 1
    // Note that the temperature sensor Address bits (A0, A1, A2) are also the
    // least significant bits of LATB -- take care when changing them
    // They *are* changed in the timer interrupt handlers if those timers are
    //   enabled.  They are just there to make the lights blink and can be
    //   disabled.
#ifdef MASTERPIC
    i2c_configure_master(0x9E);
#else
    i2c_configure_slave(0x9E);
#endif
#else
    // If I want to test the temperature sensor from the ARM, I just make
    // sure this PIC does not have the same address and configure the
    // temperature sensor address bits and then just stay in an infinite loop
    i2c_configure_slave(0x9A);
    LATBbits.LATB1 = 1;
    LATBbits.LATB0 = 1;
    LATBbits.LATB2 = 1;
    for (;;);
#endif

    // must specifically enable the I2C interrupts
    PIE1bits.SSPIE = 1;
    // must specifically enable IOC interrupts
    INTCONbits.RBIE = 1;

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
#ifdef MASTERPIC
                    if (msgbuffer[0] == POLLRFID)
                    {
                        RTAG = msgbuffer[1];
                        if (RTAG != None)
                        {
                            txMsg.buf[0] = RTAG;
                            txMsg.length = 1;
                            txMsg.msgType = navRFIDFoundMsg;
                            sendZigBeeMsg(&txMsg);
                        }
                    }
                    else if (msgbuffer[0] == POLLENCD)
                    {
                        txMsg.buf[0] = msgbuffer[1];
                        txMsg.buf[1] = msgbuffer[2];
                        txMsg.buf[2] = msgbuffer[3];
                        txMsg.buf[3] = msgbuffer[4];
                        txMsg.length = 4;
                        txMsg.msgType = navEncoderMsg;
                        sendZigBeeMsg(&txMsg);
                    }
                    else if (msgbuffer[0] == LENCODER)
                    {
                        lencoder = (msgbuffer[1] << 8) + msgbuffer[2];
                    }
                    else if (msgbuffer[0] == RENCODER)
                    {
                        txMsg.buf[0] = lencoder >> 8;
                        txMsg.buf[1] = lencoder & 0xFF;
                        txMsg.buf[2] = msgbuffer[1];
                        txMsg.buf[3] = msgbuffer[2];
                        txMsg.length = 4;
                        txMsg.msgType = navEncoderMsg;
                        sendZigBeeMsg(&txMsg);
                    }
                    else if (msgbuffer[0] == POLLFLINE)
                    {
                        txMsg.buf[0] = msgbuffer[1];
                        txMsg.buf[1] = msgbuffer[2];
                        txMsg.length = 2;
                        txMsg.msgType = navLineFoundMsg;
                        sendZigBeeMsg(&txMsg);
                    }
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
                        //PORTBbits.RB5 = 1;
                        case 0xaa:
                        {
                            length = 2;
                            msgbuffer[0] = 0x55;
                            msgbuffer[1] = 0xAA;
                            break;
                        }
                        case 0xa8:
                        {
                            length = 2;
                            msgbuffer[0] = 0x3A;
                            msgbuffer[1] = 0x3B;
                            break;
                        }
                        case 0xa9:
                        {
                            length = 1;
                            msgbuffer[0] = 0xA3;
                            break;
                        }
                        case POLLRFID:
                        {
                            length = 2;
                            msgbuffer[0] = POLLRFID;
                            msgbuffer[1] = RTAG;
                            RTAG = None;
                            break;
                        }
                        case SENDMTRCMD:
                        {
                            length = 2;
                            msgbuffer[0] = SENDMTRCMD;
                            msgbuffer[1] = 0x01;
                            break;
                        }
                        case POLLENCD:
                        {
                            lencoder = reportLChange();
                            rencoder = reportRChange();
                            length = 5;
                            msgbuffer[0] = POLLENCD;
                            msgbuffer[1] = lencoder >> 8;
                            msgbuffer[2] = lencoder & 0xFF;
                            msgbuffer[3] = rencoder >> 8;
                            msgbuffer[4] = rencoder & 0xFF;
                            break;

                        }
                        case LENCODER:
                        {
                            lencoder = reportLChange();
                            length = 3;
                            msgbuffer[0] = LENCODER;
                            msgbuffer[1] = lencoder >> 8;
                            msgbuffer[2] = lencoder & 0xFF;
                            break;
                        }
                        case RENCODER:
                        {
                            rencoder = reportRChange();
                            length = 3;
                            msgbuffer[0] = RENCODER;
                            msgbuffer[1] = rencoder >> 8;
                            msgbuffer[2] = rencoder & 0xFF;
                            break;
                        }
                        case POLLFLINE:
                        {
                            length = 3;
                            msgbuffer[0] = POLLFLINE;
                            msgbuffer[1] = sencount >> 8;
                            msgbuffer[2] = sencount & 0xFF;
                            break;
                        }
                        default:
                        {
                            length = 2;
                            msgbuffer[0] = CASEERROR;
                            msgbuffer[1] = CASEERROR;
                            break;
                        }
                    };
                    start_i2c_slave_reply(length, msgbuffer);
                    break;
                };
                case MSGT_POLL_PICS:
                {
                    length = 2;
                    msgbuffer[0] = RELPICADDR;
                    msgbuffer[1] = POLLRFID;

                    i2c_master_send(length, msgbuffer);
                    i2c_master_recv(length, RELPICADDR);
                    break;
                };
                case MSGT_POLL_ENCDRS:
                {
                    slength = 2;
                    rlength = 5;
                    msgbuffer[0] = RELPICADDR;
                    msgbuffer[1] = POLLENCD;

                    i2c_master_send(slength, msgbuffer);
                    i2c_master_recv(rlength, RELPICADDR);
                    break;
                }
                case MSGT_POLL_LENCDR:
                {
                    slength = 2;
                    rlength = 3;
                    msgbuffer[0] = RELPICADDR;
                    msgbuffer[1] = LENCODER;

                    i2c_master_send(slength, msgbuffer);
                    i2c_master_recv(rlength, RELPICADDR);
                    break;
                }
                case MSGT_POLL_RENCDR:
                {
                    slength = 2;
                    rlength = 3;
                    msgbuffer[0] = RELPICADDR;
                    msgbuffer[1] = RENCODER;

                    i2c_master_send(slength, msgbuffer);
                    i2c_master_recv(rlength, RELPICADDR);
                    break;
                }
                case MSGT_SEND_MTRCMD:
                {
                    unsigned char buf[4];
                    buf[0] = RELPICADDR;
                    buf[1] = SENDMTRCMD;
                    buf[2] = msgbuffer[0];
                    buf[3] = msgbuffer[1];
                    //i2c_master_send(4, &buf);
                    break;
                }
                case MSGT_POLL_FLINE:
                {
                    slength = 2;
                    rlength = 3;
                    msgbuffer[0] = RELPICADDR;
                    msgbuffer[1] = POLLFLINE;

                    i2c_master_send(slength, msgbuffer);
                    i2c_master_recv(rlength, RELPICADDR);
                }
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
                    uart_lthread(&uthread_data, msgtype, length, msgbuffer, uartRXQ);
                    break;
                };
                case MSGT_UART_RFID:
                {
                    PORTCbits.RC0 = 1;
                    if (strncmp(msgbuffer, tag_spdupp, 9) == 0)
                    {
                        PORTCbits.RC1 = 1;
                        RTAG = SpeedUp;
                    }
                    else if (strncmp(msgbuffer, tag_slwdwn, 9) == 0)
                    {
                        PORTCbits.RC2 = 1;
                        RTAG = SlowDown;
                    }
                    else if (strncmp(msgbuffer, tag_finish, 9) == 0)
                    {
                        RTAG = Finish;
                    }
                    else if (strncmp(msgbuffer, tag_dirrgt, 9) == 0)
                    {
                        RTAG = GoRight;
                    }
                    else if (strncmp(msgbuffer, tag_dirlft, 9) == 0)
                    {
                        RTAG = GoLeft;
                    }
                    else
                    {
                        RTAG = Error;
                    }
                    //uart_lthread(&uthread_data, msgtype, length, msgbuffer, rfidRXQ);
                    break;
                };
                case MSGT_ADC_DATA:
                {
#ifdef MASTERPIC
                    readADC(&FLIR, ADC_CH0);
                    readADC(&FRIR, ADC_CH1);
                    readADC(&BLIR, ADC_CH2);
                    readADC(&BRIR, ADC_CH3);
                    readADC(&SONL, ADC_CH4);
                    readADC(&SONR, ADC_CH5);
                    readADC(&CURS, ADC_CH6);
                    txMsg.msgType = navIRDataMsg;
                    txMsg.length = 7;
                    txMsg.buf[0] = FLIR;
                    txMsg.buf[1] = FRIR;
                    txMsg.buf[2] = BLIR;
                    txMsg.buf[3] = BRIR;
                    txMsg.buf[4] = SONL;
                    txMsg.buf[5] = SONR;
                    txMsg.buf[6] = CURS;
                    //sendZigBeeMsg(&txMsg);
                    //WriteUSART(FLIR);
                    //WriteUSART(FRIR);
#endif
                    break;
                }
                case MSGT_RFID_READ:
                {
                    if (!isQEmpty(rfidRXQ))
                        readQueue(rfidRXQ, &RTAG);
                    break;
                }
                case MSGT_POLL_FLINE:
                {
                    readIADC(&sen0, ADC_CH0);
                    readIADC(&sen1, ADC_CH1);
                    readIADC(&sen2, ADC_CH2);
                    readIADC(&sen3, ADC_CH3);
                    readIADC(&sen4, ADC_CH4);
                    readIADC(&sen5, ADC_CH5);
                    readIADC(&sen6, ADC_CH6);
                    readIADC(&sen7, ADC_CH7);
                    sencount = sen0 + sen1 + sen2 + sen3 + sen4 + sen5 + sen6 + sen7;
                    break;
                }
                default:
                {
                    // Your code should handle this error
                    break;
                };
            };
        }
    }

}