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
unsigned char readQueue(Queue *Q);
int createQueue(Queue *Q, int Qsize);

#endif	/* QUEUE_H */

