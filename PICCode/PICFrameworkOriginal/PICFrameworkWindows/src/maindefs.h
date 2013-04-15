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
#define MSGT_UART_RFID 32
#define MSGT_I2C_DBG 41
#define	MSGT_I2C_DATA 40
#define MSGT_I2C_RQST 42
#define MSGT_I2C_MASTER_SEND_COMPLETE 43
#define MSGT_I2C_MASTER_SEND_FAILED 44
#define MSGT_I2C_MASTER_RECV_COMPLETE 45
#define MSGT_I2C_MASTER_RECV_FAILED 46
#define MSGT_POLL_PICS 13

#define MSGT_SEND_MTRCMD 50
#define MSGT_ADC_DATA 60
#define MSGT_RFID_READ 70
#define MSGT_POLL_ENCDRS 80
#define MSGT_POLL_LENCDR 81
#define MSGT_POLL_RENCDR 82
#define MSGT_POLL_FLINE 90


//#define MASTERPIC 1
#define RELPICADDR 0x9E
#define SLAVEPIC 1

#define POLLFLINE  0xF2
#define POLLENCD   0xF3
#define POLLRFID   0xF4
#define LENCODER   0xF5
#define RENCODER   0xF6
#define SENDMTRCMD 0xF7
#define CASEERROR  0xFF

#endif

