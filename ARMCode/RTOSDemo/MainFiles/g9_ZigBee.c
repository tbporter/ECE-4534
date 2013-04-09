
#include "g9_ZigBee.h"
#include "g9_webTask.h"

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the ZigBee operations -- it is possible/very likely these are much larger than needed (see LCDtask.c for how to check the stack size)
#define baseStack 3
#if PRINTF_VERSION == 1
#define zigBeeSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define zigBeeSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

#define DEMO_MSG_RECV 0
#define USE_FAKE_TX_MSG 0
#define RETRY_MAX 3

// Length of the message queues to/from this task
#define g9ZigBeeQLen 20 /*g9Msgs*/

typedef enum{CONTINUE,RETRY} ETX_STATE;

// Define the zigBee API messages
#define ZigBeeStart 0x7E

#define TX_STAT_SUCCESS 0
#define TX_STAT_NO_ACK 	1
#define TX_STAT_CCA 	2
#define TX_STAT_PURGE 	3

#define TX_STATUS_MSGID	0x89

typedef struct __attribute__((__packed__)) __zigBeeTxStat{
	uint8_t cmdId;
	uint8_t frameId;
	uint8_t status;
} zigBeeTxStat;

typedef struct __attribute__((__packed__)) __zigBeeMsg{
	union {
	 struct __attribute__((__packed__)){
	 	uint8_t start;
		struct {
			uint8_t HI;
			uint8_t LO;
		} len;
	 };
	 uint8_t header[3];
	};
	uint8_t* data;
	uint8_t checksum;

} zigBeeMsg;

/* The ZigBee tasks. */
static portTASK_FUNCTION_PROTO( vZigBeeTask, pvParameters );

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

portBASE_TYPE zigBee2G9Msg(zigBeeMsg* in, g9Msg* out){
	if( in == NULL || out == NULL ) return pdFALSE;
	if( in->data[0] != 0x81 ) return pdFALSE;

	out->msgType = in->data[5];
	out->length = in->data[6];
	out->id = in->data[7];

	int i=0;
	for(i=0; i<out->length; i++){
		out->buf[i] = in->data[i+8];
	}

	return pdTRUE;
}

// End of private definitions

int startG9ZigBeeTask(g9ZigBeeStruct* zigBeePtr, g9UARTStruct* uartDev, unsigned portBASE_TYPE taskPriority){
	if( zigBeePtr == 0 ) return g9Err;
	if( uartDev == 0 ) return g9Err;
	
	zigBeePtr->uartDev = uartDev;
	zigBeePtr->taskPriority = taskPriority;	

	// Allocate a queue to be used to communicate with other tasks
	if ((zigBeePtr->inQ = xQueueCreate(g9ZigBeeQLen,sizeof(g9Msg))) == NULL) {
		// free up everyone and go home
		return(g9Err);
	}

	// Allocate a queue to be used to communicate with other tasks
	if ((zigBeePtr->outQ = xQueueCreate(g9ZigBeeQLen,sizeof(g9Msg))) == NULL) {
		// free up everyone and go home
		vQueueDelete(zigBeePtr->inQ);
		return(g9Err);
	}

	if (xTaskCreate( vZigBeeTask, (signed char*)"ZigBee Task", zigBeeSTACK_SIZE,(void *) zigBeePtr, zigBeePtr->taskPriority, ( xTaskHandle * ) NULL ) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(g9Err);
	}

	return g9Success;
}

inline portBASE_TYPE SendZigBeeMsg(g9ZigBeeStruct* zigBeePtr,g9Msg* msg,portTickType ticksToBlock){
	//Do any need processing here
		//NONE
	//Add the msg to the queue.
	//printf("sendMsg: %X\n",msg->msgType);
	return xQueueSend(zigBeePtr->inQ,(void*)(msg), ticksToBlock);
}

static portTASK_FUNCTION( vZigBeeTask, pvParameters ){	 //Red is due to #defines below
	g9ZigBeeStruct* zigBeePtr = (g9ZigBeeStruct*) pvParameters;
	g9Msg inMsg, outMsg;
	ETX_STATE txState = CONTINUE;
	uint8_t* txBuf = 0;
	uint8_t retries = 0;

	for(;;){
	//Handle RX
		zigBeeMsg msg;
		portBASE_TYPE success = pdTRUE;
		//Get message from UART
			//Get ZigBee Header
		if( uxQueueMessagesWaiting(zigBeePtr->uartDev->outQ) >= 3 ){
			//TODO: Semaphore? - To ensure the queue doesn't get too full in case of incomplete message
			//			May not be needed.... needs investigating
			success &= xQueueReceive(zigBeePtr->uartDev->outQ,(void*)&(msg.header[0]),10);
			success &= xQueueReceive(zigBeePtr->uartDev->outQ,(void*)&(msg.header[1]),10);
			success &= xQueueReceive(zigBeePtr->uartDev->outQ,(void*)&(msg.header[2]),10);
			if( success != pdTRUE ) VT_HANDLE_FATAL_ERROR(0xD34D);
			//Validate
			if( msg.start != ZigBeeStart ) VT_HANDLE_FATAL_ERROR(0xD34D2);
			//Has the data+checksum arrived yet?
			if( uxQueueMessagesWaiting(zigBeePtr->uartDev->outQ) < getLen(msg)+1){
				//reset the queue
				success &= xQueueSendToFront(zigBeePtr->uartDev->outQ,(void*)&(msg.header[2]),10);
				success &= xQueueSendToFront(zigBeePtr->uartDev->outQ,(void*)&(msg.header[1]),10);
				success &= xQueueSendToFront(zigBeePtr->uartDev->outQ,(void*)&(msg.header[0]),10);
				if( success != pdTRUE ) VT_HANDLE_FATAL_ERROR(0xD34D3);
			}else{
				//End Semaphore?
				//Get data
				uint16_t len = getLen(msg);
				msg.data = (uint8_t*)malloc(sizeof(uint8_t)*len);
				if(msg.data == NULL){
					VT_HANDLE_FATAL_ERROR(MALLOC_ERROR);
				}
				int i =0;
				for(i=0; i<len; i++){
					success &= xQueueReceive(zigBeePtr->uartDev->outQ,(void*)&(msg.data[i]),10);	
				}
				//Get Checksum
				uint8_t check;
				success &= xQueueReceive(zigBeePtr->uartDev->outQ,(void*)&(check),10);	
				msg.checksum = check;
				if( success != pdTRUE ) VT_HANDLE_FATAL_ERROR(0xD34D4);
				//Validate Checksum
				if( msg.checksum != generateChecksum(&msg) ){
					//TODO: Handle bad checksum
					printw("ZigBee - Corrupt Msg Received!");
				}
					
				//Look at msg type
				switch(msg.data[0] /*CmdID*/){
				case 0x8A: //Modem Status
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Status Message");
					#endif
					break;
				case 0x08: //AT Command
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Message");
					#endif
					break;
				case 0x09: //AT Command - Queue	Param Value
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Queue Message");
					#endif
					break;
				case 0x88: //AT Command Response
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Response Message");
					#endif
					break;
				case 0x17: //Remote AT Command Request
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Remote Message");
					#endif
					break;
				case 0x97: //Remote Command Response
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Remote Response Message");
					#endif
					break;
				case TX_STATUS_MSGID: //TX Status
					{
						#if DEMO_MSG_RECV == 1
							printw("ZigBee - Received Tx Status Message");
						#endif
						//Check for failed message Tx
						zigBeeTxStat* pStatus = (zigBeeTxStat*)msg.data;
						if(pStatus->status != TX_STAT_SUCCESS){
							//txState = RETRY;
							if(++retries == RETRY_MAX){
								printw("ZigBee - Error: TX Failed");
								txState = CONTINUE; //Give up;
								retries = 0;
							}
						}
						else{
							txState = CONTINUE;
							retries = 0;
							free(txBuf); // Free previous message
						}
						break;
					}
				case 0x80: //RX - 64bit
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received RX 64 Message");
					#endif
					break;
				case 0x81: //RX - 16bit
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received RX 16 Message");
					#endif
					//TODO: Validate ID
					//Send to conductor
					zigBee2G9Msg(&msg,&outMsg);
					if( xQueueSend(zigBeePtr->outQ,(void*)&outMsg,10) != pdTRUE){
						VT_HANDLE_FATAL_ERROR(0xD34D8);
					}
					break;
				case 0x00: //TX Request - 64bit
				case 0x01: //TX Request - 16bit
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Tx Message?");
					#endif
				default:
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Unknown Message");
					#endif
					//Shouldn't get here.
					VT_HANDLE_FATAL_ERROR(0xD34D5);
					break;
				}
					
			free(msg.data); //Free allocated array
			}
		}//RX

	//Handle TX
		//Get Message from queue
		if( txState == CONTINUE){
		#if USE_FAKE_TX_MSG == 1
			inMsg.msgType = navMotorCmdMsg;
			inMsg.length = navMotorCmdLen;
			inMsg.id = 0x69;
			inMsg.buf[0] = 0x60;
			inMsg.buf[1] = 0xE0;
		#else
			if(xQueueReceive(zigBeePtr->inQ,(void*)&inMsg,10) == pdTRUE ){	
		#endif
	
				//Get Len
				uint16_t len = 5 /*API Format*/ + 3 /*g9Msg*/ + inMsg.length;
				setLen(&msg,len);
		
				msg.data = (uint8_t*)malloc(sizeof(uint8_t)*len);
				if(msg.data == NULL){
					VT_HANDLE_FATAL_ERROR(MALLOC_ERROR);
				}
		
				//Wrap in ZigBee headers
				msg.start = ZigBeeStart;
					//Fill in API Message Info
				msg.data[0] = 0x01; //TX 16bit
				msg.data[1] = 0x01;	//Response Frame
				msg.data[2] = 0x00;	//Dest. Addr.
				msg.data[3] = 0x02;
				msg.data[4] = 0x00; //Options
		
				msg.data[5] = inMsg.msgType;
				msg.data[6] = inMsg.length;
				msg.data[7] = inMsg.id;
		
				int i = 0;
				for(i=8; i<len; i++){
					msg.data[i] = inMsg.buf[i-8];
				}
					//Generate checksum
				msg.checksum = generateChecksum(&msg);
		
				//Send to UART
				txBuf = (uint8_t*)malloc(sizeof(uint8_t)*(len+4));
				if(txBuf == NULL){
					VT_HANDLE_FATAL_ERROR(MALLOC_ERROR);
				}
				serializeZigBeeMsg(&msg,txBuf);
				if( SendUartMsg(zigBeePtr->uartDev,len+4, txBuf) != pdTRUE ){
					VT_HANDLE_FATAL_ERROR(0xD34D7);
				}
				//txState = RETRY; //Stops from sending new data before TX_SUCCESS status rcv
				free(msg.data); //Free allocated array
			}// Rcvd Msg
		}//TX - Continue
		else{
			uint16_t len = getLen(*(zigBeeMsg*)txBuf) + 4; //Type cast to zigBeeMsg isn't exact as data* is actual data after being serialized
			if( SendUartMsg(zigBeePtr->uartDev, len, txBuf) != pdTRUE ){
				VT_HANDLE_FATAL_ERROR(0xD34D7);
			}
		}//TX - Retry
	} //for
}//zigBee task