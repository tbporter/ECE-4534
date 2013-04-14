#ifndef __my_uart_h
#define __my_uart_h

#include "messages.h"

#ifdef MASTERPIC
#define MAXUARTBUF 1
#else
#define MAXUARTBUF 16
#endif


typedef struct __uart_comm {
	unsigned char buffer[MAXUARTBUF];
	unsigned char	buflen;
} uart_comm;

void init_uart_recv(uart_comm *);
void uart_recv_int_handler(void);

#endif
