
#include <stdlib.h>
#include <stdio.h>

#include "g9_NavTask.h"

#define NavQLen 20 //Lots of messages

//helper functions for setting motor speeds
typedef union {
	uint16_t data;
	struct {
		uint8_t left;
		uint8_t right;
	};
} motorData_t;

void setMotorData(uint16_t* data, uint8_t left, uint8_t right){
	left &= 0x7F;
	right &= 0x7F;
	*data = 0x0080 | (right<<8) | left;
}



// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations	-- almost certainly too large, see LCDTask.c for details on how to check the size
#define baseStack 3
#if PRINTF_VERSION == 1
#define navSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define navSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

/* The navigation task. */
static portTASK_FUNCTION_PROTO( navigationUpdateTask, pvParameters );


void vStartNavigationTask(navStruct* navData,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c){
	// Create the queue that will be used to talk to this task
	if ((navData->inQ = xQueueCreate(NavQLen,10)) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	navData->i2c = i2c;
	if ((retval = xTaskCreate( navigationUpdateTask, ( signed char * ) "Navi", navSTACK_SIZE, (void *) navData, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE SendNavigationMsg(navStruct* nav,uint8_t* buffer,portTickType ticksToBlock){
	if (buffer == NULL || nav == NULL ) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	g9Msg* msg = (g9Msg*) malloc(sizeof(g9Msg));
	msg->msgType = buffer[0];
	switch (msg->msgType){
		case navMotorCmdMsg:
			printf("navMotorCmdMsg: ");
			msg->length = navMotorCmdLen;
			break;
		
		case navLineFoundMsg:
			printf("navLineFoundMsg: ");
			msg->length = navLineFoundLen;
			break;
		
		case navIRDataMsg:
			printf("navIRDataMsg: ");
			msg->length = navIRDataLen;
			break;
		
		case navRFIDFoundMsg:
			printf("navRFIDFoundMsg: ");
			msg->length = navRFIDFoundLen;
			break;	

	default:
		VT_HANDLE_FATAL_ERROR(0xDEAD);
	}
	if (msg->length > navMAX_LEN) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(0);
	}
	//msg->buf = malloc(sizeof(uint8_t) * msg->length);
	int i =0;
	for(; i<msg->length; i++){
		msg->buf[i]=buffer[i];
		printf("%X|",msg->buf[i]);
	}
	printf("\n");
	return(xQueueSend(nav->inQ,(void*)(msg),ticksToBlock));
}

// This is the actual task that is run
static portTASK_FUNCTION( navigationUpdateTask, pvParameters )
{
	static motorData_t motorData;
	// Get the parameters
	navStruct *param = (navStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *i2cPtr = param->i2c;
	// Buffer for receiving messages
	g9Msg msgBuffer;

	// Assumes that the I2C device (and thread) have already been initialized

	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from the "otherside"
		if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		g9Msg msg;
		msg.id = msgBuffer.id + 1;
		msg.msgType = navMotorCmdMsg;

		// Now, based on the type of the message, we decide on the action to take
		switch (msgBuffer.msgType){
		case navLineFoundMsg:
			//stop we have found the finish line!!!
			setMotorData(&(motorData.data),127,127);
			break;
		
		case navIRDataMsg:
			//Save the data and make a decision
			setMotorData(&(motorData.data),1,0); //Turn left
			break;
		
		case navRFIDFoundMsg:
			//Save the data and make a decision
			setMotorData(&(motorData.data),0,1); //Forward
			break;
		
		default:
			//Invalid message type
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
	}

	//printf("Motors:    %X %X\n",0x7F & motorData.left, 0x7F & motorData.right);
	//msg.buf = &(motorData.data);
	//printf("Re-Motors: %X %X\n",msg.buf[0],msg.buf[1]);

	//vtI2CEnQ(i2cPtr,navMotorCmdMsg,0x4F,sizeof(navMAX_LEN),(uint8_t*)&msg,0);
	const uint8_t test[]= {0xAA};
	vtI2CEnQ(i2cPtr,conRequestMsg,0x4F,sizeof(test),test,3);
	}
}