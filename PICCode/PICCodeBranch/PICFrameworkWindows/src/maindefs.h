#ifndef __maindefs
#define __maindefs

#ifdef __XC8
#include <xc.h>
#ifdef _18F45J10
#define __USE18F45J10 1
#else
#ifdef _18F2680
#define __USE18F2680 1
#endif
#endif
#else
#ifdef __18F45J10
#define __USE18F45J10 1
#else
#ifdef __18F2680
#define __USE18F2680 1
#endif
#endif
#include <p18cxxx.h>
#endif

// Message type definitions
#define MSGT_TIMER0 10
#define MSGT_TIMER1 11
#define MSGT_MAIN1 20
#define	MSGT_OVERRUN 30
#define MSGT_UART_DATA 31
#define MSGT_I2C_DBG 41
#define	MSGT_I2C_DATA 40
#define MSGT_I2C_RQST 42
#define MSGT_I2C_MASTER_SEND_COMPLETE 43
#define MSGT_I2C_MASTER_SEND_FAILED 44
#define MSGT_I2C_MASTER_RECV_COMPLETE 45
#define MSGT_I2C_MASTER_RECV_FAILED 46
#define MSGT_POLL_PICS 13
#define MSGT_RFID_READ 70

// comment/uncomment based on what kind of PIC this code is running on
//#define MASTERPIC 1
// Rfid, Encoder, and Line PIC and PIC Address
#define ISRELPIC 1
#define RELPICADDR 0x9C
// Sensors and Motor Control PIC
//#define ISSMCPIC 1
#define SMCPICADDR 0x9E

// defines for sampling commands
#define SAMPLEDISTANCE 0xF1
#define SAMPLEENCODERL 0xF2
#define SAMPLEENCODERR 0xF3
#define SAMPLELINE     0xF4
#define SAMPLECURRENT  0xF5
#define SAMPLERFID     0xF6
#define SENDMTRCMD     0xF7

#endif