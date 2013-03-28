#ifndef __my_uart_h
#define __my_uart_h

#include "messages.h"

#define MAXUARTBUF 1

typedef struct __uart_comm {
	unsigned char buffer[MAXUARTBUF];
	unsigned char	buflen;
} uart_comm;

void init_uart_recv(uart_comm *);
void uart_recv_int_handler(void);

#endif
