/* 
 * File:   zigBee.h
 * Author: Ben Smith
 *
 * Created on March 26, 2013, 6:46 PM
 */
#ifndef ZIGBEE_H
#define	ZIGBEE_H

#include <GenericTypeDefs.h>
#include <stdlib.h>
#include "maindefs.h"
#include "messages.h"
#include "queue.h"
#include "../../../../common/MESSAGES_G9.h"
#include <usart.h>


typedef enum{HEADER,GATHER,PROCESS,CLEANUP,RETRY} ESTATE;

// Define the zigBee API messages
#define ZigBeeStart 0x7E

#define TX_STAT_SUCCESS 0
#define TX_STAT_NO_ACK 	1
#define TX_STAT_CCA 	2
#define TX_STAT_PURGE 	3

#define TX_STATUS_MSGID	0x89

#define max(a,b)    (((a) > (b)) ? (a) : (b))

typedef struct __zigBeeTxStat{
	uint8_t cmdId;
	uint8_t frameId;
	uint8_t status;
} zigBeeTxStat;

#define MAX_DATA_LEN max(sizeof(g9Msg)+5 /*API Formatting*/,36 /*Max ZigBee API MSG size*/)

typedef struct __zigBeeMsg {

    union {

        struct {
            uint8_t start;

            struct {
                uint8_t HI;
                uint8_t LO;
            } len;
        };
        uint8_t header[3];
    } zigBeeU;
    uint8_t data[MAX_DATA_LEN]; //big cuz... fuck you malloc
    uint8_t checksum;

} zigBeeMsg;

void doZigBee(int length, unsigned char *msgbuffer, Queue *rcvQ);

#endif	/* ZIGBEE_H */

