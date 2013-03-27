/* 
 * File:   queue.h
 * Author: ECE4534
 *
 * Created on February 11, 2013, 6:21 PM
 */

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
int getNumMessagesQueue(Queue *Q);
inline unsigned char isQueueEmpty(Queue *Q);
inline unsigned char isQueueFull(Queue *Q);

#endif	/* QUEUE_H */

