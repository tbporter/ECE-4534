#include <stdlib.h>
#include <stdio.h>

#include "g9_webTask.h"



// This is the actual task that is run
static portTASK_FUNCTION( webUpdateTask, pvParameters )
{
	// Get the parameters
	webStruct *param = (webStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *i2cPtr = param->i2c;
	// Buffer for receiving messages
	g9Msg msgBuffer;

	// Like all grrood tasks, this should never exit
	for(;;)
	{
		// Wait for a message from the nether
		if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		// Now, based on the type of the message, we decide on the action to take
		switch (msgBuffer.msgType){
			//Other tasks will send debug messages
			case webDebugMsg:
				
				break;
			//Used to display power related things from the power PIC
			case webPowerMsg:
				break;
			//used to display nav related things from the nav task
			case webNavMsg:
				break;		
			default:
				//Invalid message type
				VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
				break;
		}


	}
}