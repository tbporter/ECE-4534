
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

#define DEMO_MSG_RECV 1
#define USE_FAKE_TX_MSG 0

// Length of the message queues to/from this task
#define g9ZigBeeQLen 20 /*g9Msgs*/

// Define the zigBee API messages
#define ZigBeeStart 0x7E

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
	printf("sendMsg: %X\n",msg->msgType);
	return xQueueSend(zigBeePtr->inQ,(void*)(msg), ticksToBlock);
}

static portTASK_FUNCTION( vZigBeeTask, pvParameters ){
	g9ZigBeeStruct* zigBeePtr = (g9ZigBeeStruct*) pvParameters;

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
				}
				g9Msg outMsg;	
				//Look at msg type
				switch(msg.data[0] /*CmdID*/){
				case 0x8A: //Modem Status
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Status Message\n");
					#endif
					break;
				case 0x08: //AT Command
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Message\n");
					#endif
					break;
				case 0x09: //AT Command - Queue	Param Value
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Queue Message\n");
					#endif
					break;
				case 0x88: //AT Command Response
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Response Message\n");
					#endif
					break;
				case 0x17: //Remote AT Command Request
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Remote Message\n");
					#endif
					break;
				case 0x97: //Remote Command Response
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received AT Remote Response Message\n");
					#endif
					break;
				case 0x89: //TX Status
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Tx Status Message\n");
					#endif
					//TODO: Check for failed message Tx
					break;
				case 0x80: //RX - 64bit
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received RX 64 Message\n");
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
						printw("ZigBee - Received Tx Message????\n");
					#endif
				default:
					#if DEMO_MSG_RECV == 1
						printw("ZigBee - Received Unknown Message\n");
					#endif
					//Shouldn't get here.
					VT_HANDLE_FATAL_ERROR(0xD34D5);
					break;
				}
					
			free(msg.data); //Free allocated array
			}
		}

	//Handle TX
		//Get Message from queue
		static g9Msg inMsg;
		//Get Message

		#if USE_FAKE_TX_MSG == 1
		 if(1){
			inMsg.msgType = navMotorCmdMsg;
			inMsg.length = navMotorCmdLen;
			inMsg.id = 0x69;
			inMsg.buf[0] = 0x27;
			inMsg.buf[1] = 0x72;
		#else
		if( xQueueReceive(zigBeePtr->inQ,(void*)&inMsg,10) == pdTRUE ){
			printf("recvMsg: %X\n",inMsg.msgType);	
		#endif
	
			//Get Len
			uint16_t len = 5 /*API Format*/ + 3 /*g9Msg*/ + inMsg.length;
			setLen(&msg,len);
	
			msg.data = (uint8_t*)malloc(sizeof(uint8_t)*len);
	
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
			uint8_t* txBuf = (uint8_t*)malloc(sizeof(uint8_t)*(len+4));
			serializeZigBeeMsg(&msg,txBuf);
			if( SendUartMsg(zigBeePtr->uartDev,len+4, txBuf) != pdTRUE ){
				VT_HANDLE_FATAL_ERROR(0xD34D7);
			}
			free(txBuf);
			free(msg.data); //Free allocated array
		}
	}
}