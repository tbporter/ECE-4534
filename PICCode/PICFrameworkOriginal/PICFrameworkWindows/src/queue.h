#ifndef QUEUE_H
#define	QUEUE_H

typedef struct Queue
{
    int size;
    int first;
    int last;
    unsigned char* messages;
}Queue;

int appendQueue(Queue *Q, unsigned char msg);
unsigned char readQueue(Queue *Q, unsigned char* msg);
int createQueue(Queue *Q, int Qsize);
int getNumMessagesQueue(Queue* Q);
unsigned char isQEmpty(Queue *Q);
unsigned char isQFull(Queue *Q);

#endif	/* QUEUE_H */

