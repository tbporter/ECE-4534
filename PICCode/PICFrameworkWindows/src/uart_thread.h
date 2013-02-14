#include "queue.h"
#include <usart.h>
#include "messages.h"
typedef struct __uart_thread_struct {
	// "persistent" data for this "lthread" would go here
    Queue* uartRXQ;
    int	data;
} uart_thread_struct;

int uart_lthread(uart_thread_struct *,int,int,unsigned char*, Queue *rcvQ);
