/* 
 * File:   zigBee.h
 * Author: Ben Smith
 *
 * Created on March 26, 2013, 6:46 PM
 */
#ifndef ZIGBEE_H
#define	ZIGBEE_H

#include <stdint.h>
#include "maindefs.h"
#include "queue.h"


typedef enum{HEADER,GATHER,PROCESS,CLEANUP,RETRY} ESTATE;

// Define the zigBee API messages
#define ZigBeeStart 0x7E

#define TX_STAT_SUCCESS 0
#define TX_STAT_NO_ACK 	1
#define TX_STAT_CCA 	2
#define TX_STAT_PURGE 	3

#define TX_STATUS_MSGID	0x89

typedef struct __zigBeeTxStat{
	uint8_t cmdId;
	uint8_t frameId;
	uint8_t status;
} zigBeeTxStat;

typedef struct __zigBeeMsg{
	union {
	 struct {
	 	uint8_t start;
		struct {
			uint8_t HI;
			uint8_t LO;
		} len;
	 };
	 uint8_t header[3];
	};
	uint8_t data[32]; //big cuz... fuck you malloc
	uint8_t checksum;

} zigBeeMsg;

void doZigBee(int length, unsigned char *msgbuffer, Queue *rcvQ);

#endif	/* ZIGBEE_H */

