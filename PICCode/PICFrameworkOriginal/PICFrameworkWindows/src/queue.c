#include "queue.h"

int appendQueue(Queue *Q, unsigned char msg) {
    if (isQFull(Q)) {
        return 0;
    } else {
        if (Q->last == -1) {
            Q->messages[0] = msg;
            Q->last++;
        } else {
            Q->last++;
            Q->messages[Q->last] = msg;

        }
        return 1;
    }
}

unsigned char readQueue(Queue *Q, unsigned char* msg) {
    int i;
    if (msg == 0) return 0;
    if (isQEmpty(Q))
    {
        // indicates error
        return 0;
    } else {
        *msg = Q->messages[0];
        for (i = 0; i < Q->last; i++)
        {
            Q->messages[i] = Q->messages[i+1];
        }
        Q->last--;
        return 1;
    }
}

int createQueue(Queue *Q, int Qsize) {
    if (Qsize > 0) {
        Q->size = Qsize;
        Q->first = 0;
        Q->last = -1;
        Q->messages[Q->size];
        return 1;
    } else
        return 0;
}

int getNumMessagesQueue(Queue *Q)
{
    return ((Q->last+1)-Q->first);
}

unsigned char isQEmpty(Queue* Q)
{
    return (Q->last == -1);
}

unsigned char isQFull(Queue *Q)
{
    return (Q->last == Q->size-1);
}