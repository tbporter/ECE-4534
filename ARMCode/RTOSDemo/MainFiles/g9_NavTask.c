
#include <stdlib.h>
#include <stdio.h>
#include "g9_webTask.h"
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

#define IRBUF_SIZE 20;

//static uint8_t IRBuf[IRBUF_SIZE];

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations	-- almost certainly too large, see LCDTask.c for details on how to check the size
#define baseStack 5
#if PRINTF_VERSION == 1
#define navSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define navSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

/* The navigation task. */
static portTASK_FUNCTION_PROTO( navigationUpdateTask, pvParameters );


void vStartNavigationTask(navStruct* navData,unsigned portBASE_TYPE uxPriority, g9ZigBeeStruct* zigBeePtr){
	// Create the queue that will be used to talk to this task
	if ((navData->inQ = xQueueCreate(NavQLen,sizeof(g9Msg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	navData->zigBeePtr = zigBeePtr;
	if ((retval = xTaskCreate( navigationUpdateTask, ( signed char * ) "Navi", navSTACK_SIZE, (void *) navData, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE SendNavigationMsg(navStruct* nav,g9Msg* msg,portTickType ticksToBlock){
	if (msg == NULL || nav == NULL ) {
		return pdFALSE;
	}

	switch (msg->msgType){
		case navMotorCmdMsg:
			printw("navMotorCmdMsg");
			break;
		
		case navLineFoundMsg:
			printw("navLineFoundMsg");
			break;
		
		case navIRDataMsg:
			printw("navIRDataMsg");
			break;
		
		case navRFIDFoundMsg:
			printw("navRFIDFoundMsg");
			break;	

	default:
		printw("Incorrect Navigation Msg");
		VT_HANDLE_FATAL_ERROR(0xDEAD);
	}
	return(xQueueSend(nav->inQ,(void*)(msg),ticksToBlock));
}

// This is the actual task that is run
static portTASK_FUNCTION( navigationUpdateTask, pvParameters )
{
	static motorData_t motorData;
	// Get the parameters
	navStruct *navData = (navStruct *) pvParameters;
	// Buffer for receiving messages
	g9Msg msgBuffer;

	// Assumes that the I2C device (and thread) have already been initialized

	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from the "otherside"
		if (xQueueReceive(navData->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		g9Msg msg;
		msg.id = msgBuffer.id + 1;
		msg.length = 2;
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

		msg.buf[0] = motorData.left;
		msg.buf[1] = motorData.right;		
		SendZigBeeMsg(navData->zigBeePtr,&msg,portMAX_DELAY);
	}
}

