
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
	*data = 0x0080 | (left<<8) | right;
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
	if ((navData->inQ = xQueueCreate(NavQLen,navMAX_LEN)) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	navData->i2c = i2c;
	if ((retval = xTaskCreate( navigationUpdateTask, ( signed char * ) "Navi", navSTACK_SIZE, (void *) navData, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE SendNavigationMsg(navStruct* nav,g9Msg* msg,portTickType ticksToBlock){
	if (msg == NULL || nav == NULL ) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	int length = sizeof(&msg);
	
	switch (msg->msgType){
		case navMotorCmdMsg:
			//Shouldn't be receiving this type of message, but just in case...
			if(navMotorCmdLen > 0) length += sizeof(uint8_t)*(navMotorCmdLen-1);
			break;
		
		case navLineFoundMsg:
			if(navLineFoundMsg > 0) length += sizeof(uint8_t)*(navLineFoundMsg-1);
			break;
		
		case navIRDataMsg:
			if(navIRDataMsg > 0) length += sizeof(uint8_t)*(navIRDataMsg-1);
			break;
		
		case navRFIDFoundMsg:
			if(navRFIDFoundMsg > 0) length += sizeof(uint8_t)*(navRFIDFoundMsg-1);
			break;
		
		default:
			//Invalid message type
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
	}
	
	if (length > navMAX_LEN) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(length);
	}
	return(xQueueSend(nav->inQ,(void *)msg,ticksToBlock));
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

		// Now, based on the type of the message, we decide on the action to take
		switch (msgBuffer.msgType){
		case navLineFoundMsg:
			//stop we have found the finish line!!!
			setMotorData(&(motorData.data),64,64);
			break;
		
		case navIRDataMsg:
			//Save the data and make a decision
			setMotorData(&(motorData.data),1,127); //Turn left
			break;
		
		case navRFIDFoundMsg:
			//Save the data and make a decision
			setMotorData(&(motorData.data),127,127); //Forward
			break;
		
		default:
			//Invalid message type
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
	}


	}
}