#include <string.h>
#include "zigBee.h"

uint16_t getLen(zigBeeMsg msg){
	return (((uint16_t)msg.len.HI << 8) | (msg.len.LO));
}

void setLen(zigBeeMsg* msg, uint16_t len){
	msg->len.HI = (len >> 8)& 0x00FF;
	msg->len.LO = len & 0x00FF;
	return;
}

uint8_t generateChecksum(zigBeeMsg* msg){
	uint8_t check = 0;
	uint16_t len = getLen(*msg);
	int i = 0;
	for(i=0; i<len; i++){
		check += msg->data[i];
	}
	check = 0xFF - (check & 0xFF);
	return (check & 0xFF);
}

void serializeZigBeeMsg(zigBeeMsg* msg, uint8_t* const buf){
	buf[0] = msg->start;
	buf[1] = msg->len.HI;
	buf[2] = msg->len.LO;

	int i = 0;
	uint16_t len = getLen(*msg);
	for(i=0; i<len; i++){
		buf[i+3] = msg->data[i];
	}
	buf[len+3] = msg->checksum;
	return;
}

void doZigBee(int length, unsigned char *msgbuffer, Queue *rcvQ){
    static ESTATE state = HEADER;
    static uint16_t len = 0;
    static zigBeeMsg msg;
    static int curI=0;

    int i = 0;

    switch( state ){
        case HEADER:
        {
            //Header from msgbuffer
            memcpy(msg.header,msgbuffer,sizeof(unsigned char)*3);
            //Verify correct header information
            if( msg.start != ZigBeeStart ){
                //Remove incorrect data from the queue
                //NOTE: More robust my checking msgbuffer for correct start
                for( i=0; i<length; i++){
                    readQueue(rcvQ,0);
                }
                break; //Leave state machine
            }
            //Get Len
            len = getLen(msg);
            //Go to the gather state
            state = GATHER;
            break;
        }
        case GATHER:
        {
            //Copy data to zigBeeMsg until reached length
            int numMsgs = getNumMessagesQueue(rcvQ);
            for( i=0; i<numMsgs || curI<len ; i++, curI++){
                readQueue(rcvQ,&(msg.data[curI]));
            }

            //If we have the message
            if( curI == len){
                numMsgs = getNumMessagesQueue(rcvQ);
                if( numMsgs > 0){
                    //Copy checksum
                    readQueue(rcvQ, &(msg.checksum));
                    //Go to process state
                    state = PROCESS;
                }
            }
            break;
        }
        case PROCESS:
        {
            //Evaluate the message
            //Go to cleanup state
            break;
        }
        case CLEANUP:
        {
            //Reset variable
            curI = 0;
        }
        default:
        {
            //How'd this happen?!
            break;
        }
    }
}