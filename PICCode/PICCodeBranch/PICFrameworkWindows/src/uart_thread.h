#include "queue.h"
#include <usart.h>
#include "messages.h"
typedef struct __uart_thread_struct {
	// "persistent" data for this "lthread" would go here
	int	data;
} uart_thread_struct;

int uart_lthread(uart_thread_struct *uptr, int msgtype, int length, unsigned char *msgbuffer, Queue* rcvQ);
