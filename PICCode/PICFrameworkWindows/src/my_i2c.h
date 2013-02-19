#ifndef __my_i2c_h
#define __my_i2c_h

#include "messages.h"
#include <p18cxxx.h>


#define MAXI2CBUF MSGLEN
typedef struct __i2c_comm {
	unsigned char buffer[MAXI2CBUF];
        volatile unsigned char explen;
	volatile unsigned char buflen;
	unsigned char event_count;
	unsigned char status;
	unsigned char error_code;
	unsigned char error_count;
	unsigned char outbuffer[MAXI2CBUF];
	unsigned char outbuflen;
	unsigned char outbufind;
	unsigned char slave_addr;
} i2c_comm;


#define I2C_STOP 0x4
#define I2C_IDLE 0x5
#define I2C_STARTED 0x6
#define	I2C_RCV_DATA 0x7
#define I2C_SLAVE_SEND 0x8
#define I2C_MASTER_SEND 0x9
#define I2C_MASTER_RECV 0xA
#define I2C_MASTER_SEND_ADDR 0xB
#define I2C_MASTER_RECV_ADDR1 0xC
#define I2C_MASTER_RECV_ADDR2 0xF
#define I2C_MASTER_RECV_ADDR3 0x10
#define I2C_MASTER_DATA_READ 0xD
#define I2C_MASTER_RECV_RECN 0xE

#define I2C_ERR_THRESHOLD 1
#define I2C_ERR_OVERRUN 0x4
#define I2C_ERR_NOADDR 0x5
#define I2C_ERR_NODATA 0x6
#define I2C_ERR_MSGTOOLONG 0x7
#define I2C_ERR_MSG_TRUNC 0x8

void init_i2c(i2c_comm *);
void i2c_int_handler(void);
void i2c_master_handler(void);
void i2c_slave_handler(void);
void start_i2c_slave_reply(unsigned char,unsigned char *);
void i2c_configure_slave(unsigned char);
void i2c_configure_master(unsigned char);
unsigned char i2c_master_send(unsigned char,unsigned char *);
unsigned char i2c_master_recv(unsigned char, unsigned char);

#endif