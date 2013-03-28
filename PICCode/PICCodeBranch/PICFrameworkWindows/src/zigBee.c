#include <string.h>
#include "maindefs.h"
#include "zigBee.h"

#define DEMO_MSG_RECV 0
#define USE_FAKE_TX_MSG 1

typedef enum{TX_CONTINUE,TX_RETRY} ETX_STATE;

void processMsg(zigBeeMsg* msg);

uint16_t getLen(zigBeeMsg msg)
{
    //return (((uint16_t)msg.len.HI << 8) | (msg.len.LO));
    return ((((uint16_t)msg.zigBeeU.len.HI) << 8) | msg.zigBeeU.len.LO);
}

void setLen(zigBeeMsg* msg, uint16_t len){
    msg->zigBeeU.len.HI = (len >> 8) & 0x00FF;
    msg->zigBeeU.len.LO = len & 0x0FF;
//    msg->len.HI = (len >> 8)& 0x00FF;
//    msg->len.LO = len & 0x00FF;
    return;
}

uint8_t generateChecksum(zigBeeMsg* msg){
	uint8_t check = 0;
	uint16_t len = getLen(*msg);
	int i = 0;
	for(i=0; i<len; i++){
            
//            while(BusyUSART());
//            WriteUSART(msg->data[i]);
            check += msg->data[i];
	}
	check = 0xFF - (check & 0xFF);
	return (check & 0xFF);
}

void serializeZigBeeMsg(zigBeeMsg* msg, uint8_t* const buf){
    int i = 0;
    uint16_t len;
    buf[0] = msg->zigBeeU.start;
    buf[1] = msg->zigBeeU.len.HI;
    buf[2] = msg->zigBeeU.len.LO;


    len = getLen(*msg);
    for(i=0; i<len; i++){
            buf[i+3] = msg->data[i];
    }
    buf[len+3] = msg->checksum;
    return;
}

unsigned char zigBee2G9Msg(zigBeeMsg* in, g9Msg* out) {
    int i = 0;
    if (in == NULL || out == NULL) return 0;
    if (in->data[0] != 0x81) return 0;

    out->msgType = in->data[5];
    out->length = in->data[6];
    out->id = in->data[7];


    for (i = 0; i < out->length; i++) {
        out->buf[i] = in->data[i + 8];
    }

    return 1;
}

void doZigBee(int length, unsigned char *msgbuffer, Queue *rcvQ){
    int i, j = 0;
    static ESTATE state = HEADER;
    static uint16_t len = 0;
    static zigBeeMsg msg;
    static int curI=0;
    
    //RX
    switch( state ){
        case HEADER:
        {
            //See if we have a full header (should always be true)
            if( getNumMessagesQueue(rcvQ) >= 3 ){
                //Copy Header from msgbuffer
                for( i=0; i<3; i++){
                        readQueue(rcvQ,&(msg.zigBeeU.header[i]));
                }
                //Verify correct header information
                if( msg.zigBeeU.start != ZigBeeStart ){
                    //NOTE: More robust by checking every byte for start
                    break; //Leave state machine
                }
                //Get Len
                len = getLen(msg);
                //Go to the gather state
                state = GATHER;
                curI=0;
            }
            break;
        }
        case GATHER:
        {
            //Copy data to zigBeeMsg until reached length
            int numMsgs = getNumMessagesQueue(rcvQ);
            //PORTBbits.RB5 = 1;
            if(curI<len){
                for( i=0; i<numMsgs; i++, curI++){
                    readQueue(rcvQ,&(msg.data[curI]));
    //                for( j = 0; j < len+3; j++)
    //                {
    //                    while(BusyUSART());
    //                    WriteUSART(*chr++);
    //                }
                }
            }

            //If we have the message
            else{
                numMsgs = getNumMessagesQueue(rcvQ);
                //PORTBbits.RB5 = 1;
                if( numMsgs > 0){
                    //Copy checksum
                    readQueue(rcvQ, &(msg.checksum));
                    //Go to process state
                    processMsg(&msg);
                    state = HEADER;
                }
            }
            break;
        }
        default:
        {
            //How'd this happen?!
            break;
        }
    }

    //TX
    //PORTBbits.RB5 = 0;
    
}

void processMsg(zigBeeMsg* msg){

    static ETX_STATE txState = TX_CONTINUE;
    //Evaluate the message
    //Validate Checksum
    //PORTBbits.RB5 = 1;
    if( msg->checksum != generateChecksum(msg) ){
        //TODO: Handle bad checksum

        return;
    }
    WriteUSART(generateChecksum(msg));
    //PORTBbits.RB4 = 1;
    //Look at msg type
    WriteUSART(msg->data[0]);
    switch(msg->data[0] /*CmdID*/){
    case 0x8A: //Modem Status
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x08: //AT Command
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x09: //AT Command - Queue Param Value
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x88: //AT Command Response
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x17: //Remote AT Command Request
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x97: //Remote Command Response
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case TX_STATUS_MSGID: //TX Status
    {
        #if DEMO_MSG_RECV == 1
        #endif
        //Check for failed message Tx
        zigBeeTxStat* pStatus = (zigBeeTxStat*)msg->data;
        if(pStatus->status != TX_STAT_SUCCESS){
                txState = TX_RETRY;
        }
        else{
                txState = TX_CONTINUE;
                // Free previous message
        }
        break;
    }
    case 0x80: //RX - 64bit
        #if DEMO_MSG_RECV == 1
        #endif
        break;
    case 0x81: //RX - 16bit
    {
        #if DEMO_MSG_RECV == 1
        #endif
        //TODO: Validate ID
        //Send to slaves
        g9Msg outMsg;
        zigBee2G9Msg(msg, &outMsg);
        //PORTBbits.RB5 = 1;
        
        if(outMsg.msgType == navMotorCmdMsg)
            ToMainHigh_sendmsg(outMsg.length, MSGT_SEND_MTRCMD, outMsg.buf);
        break;
    }
    case 0x00: //TX Request - 64bit
    case 0x01: //TX Request - 16bit
        #if DEMO_MSG_RECV == 1
        #endif
    default:
        #if DEMO_MSG_RECV == 1
        #endif
        //Shouldn't get here.
        break;
    }
}