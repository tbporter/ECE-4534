#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "vtI2C.h"
#include "LCDtask.h"
#include "g9_oScopeTask.h"
#include "I2CTaskMsgTypes.h"

#define oScopeQLen 10

typedef struct __oScopeMsg {
	uint8_t msgType;
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[oScopeMaxLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} oScopeMsg;

#define baseStack 3
#if PRINTF_VERSION == 1
#define i2cSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define i2cSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

/* The oscilloscope task. */
static portTASK_FUNCTION_PROTO( oScopeUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void startOScopeTask(oScopeStruct *params,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,vtLCDStruct *lcd)
{
	// Create the queue that will be used to talk to this task
	if ((params->inQ = xQueueCreate(oScopeQLen,sizeof(oScopeMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	params->dev = i2c;
	params->lcdData = lcd;
	if ((retval = xTaskCreate( oScopeUpdateTask, ( signed char * ) "oScope", i2cSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE sendOScopeTimerMsg(oScopeStruct *oScopeData,portTickType ticksElapsed,portTickType ticksToBlock)
{
	if (oScopeData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	oScopeMsg oScopeBuffer;
	oScopeBuffer.length = sizeof(ticksElapsed);
	if (oScopeBuffer.length > oScopeMaxLen) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(oScopeBuffer.length);
	}
	memcpy(oScopeBuffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
	oScopeBuffer.msgType = oScopeTimerMsg;
	return(xQueueSend(oScopeData->inQ,(void *) (&oScopeBuffer),ticksToBlock));
}

portBASE_TYPE sendOScopeValueMsg(oScopeStruct *oScopeData,uint8_t msgType,uint8_t value,portTickType ticksToBlock)
{
	oScopeMsg oScopeBuffer;

	if (oScopeData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	oScopeBuffer.length = sizeof(value);
	if (oScopeBuffer.length > oScopeMaxLen) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(oScopeBuffer.length);
	}
	memcpy(oScopeBuffer.buf,(char *)&value,sizeof(value));
	oScopeBuffer.msgType = msgType;
	return(xQueueSend(oScopeData->inQ,(void *) (&oScopeBuffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
int getMsgType(oScopeMsg *Buffer)
{
	return(Buffer->msgType);
}
uint8_t getValue(oScopeMsg *Buffer)
{
	uint8_t *ptr = (uint8_t *) Buffer->buf;
	return(*ptr);
}

// I2C commands for the oScopeerature sensor
	const uint8_t i2cCmdInit[]= {0xAC,0x00};
	const uint8_t i2cCmdStartConvert[]= {0xEE};
	const uint8_t i2cCmdStopConvert[]= {0x22};
	const uint8_t i2cCmdReadByte1[]= {0xAA};
	const uint8_t i2cCmdReadByte2[]= {0xA8};
// end of I2C command definitions

// Definitions of the states for the FSM below
const uint8_t fsmRead1 = 0;
const uint8_t fsmRead2 = 1;	  
// This is the actual task that is run
static portTASK_FUNCTION( oScopeUpdateTask, pvParameters )
{
	uint8_t rcvByte[2];
	// Get the parameters
	oScopeStruct *param = (oScopeStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the LCD information pointer
	vtLCDStruct *lcdData = param->lcdData;
	
	// Buffer for receiving messages
	oScopeMsg msgBuffer;
	uint8_t currentState;

	// Assumes that the I2C device (and thread) have already been initialized

	// This task is implemented as a Finite State Machine.  The incoming messages are examined to see
	//   whether or not the state should change.
	//
	// Temperature sensor configuration sequence (DS1621) Address 0x4F
	if (vtI2CEnQ(devPtr,vtI2CMsgTypeTempInit,0x4F,sizeof(i2cCmdInit),i2cCmdInit,0) != pdTRUE) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	currentState = fsmRead1;
	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from either a timer or from an I2C operation
		if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		// Now, based on the type of the message and the state, we decide on the new state and action to take
		switch(getMsgType(&msgBuffer)) {
		case oScopeTimerMsg: {
			if(1){
				// Read in the values from the temperature sensor
				// We have three transactions on i2c to read the full temperature 
				//   we send all three requests to the I2C thread (via a Queue) -- responses come back through the conductor thread
				// Temperature read -- use a convenient routine defined above
				if (vtI2CEnQ(devPtr,oScopeRead1Msg,0x4F,sizeof(i2cCmdReadByte1),i2cCmdReadByte1,2) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				// Read in the read counter
				if (vtI2CEnQ(devPtr,oScopeRead2Msg,0x4F,sizeof(i2cCmdReadByte2),i2cCmdReadByte2,1) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				// Read in the slope;
				/*if (vtI2CEnQ(devPtr,vtI2CMsgTypeTempRead3,0x4F,sizeof(i2cCmdReadSlope),i2cCmdReadSlope,1) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}*/
			} else {
				// just ignore timer messages until initialization is complete
			} 
			break;
		}
		case oScopeRead1Msg: {
			if (currentState == fsmRead1) {
				currentState = fsmRead2;
				rcvByte[0] = getValue(&msgBuffer);
			} else {
				// unexpectedly received this message
				VT_HANDLE_FATAL_ERROR(0);
			}
			break;
		}
		case oScopeRead2Msg: {
			if (currentState == fsmRead2) {
				
				currentState = fsmRead1;
				rcvByte[1] = getValue(&msgBuffer);
				uint16_t msgData = (rcvByte[0])|(rcvByte[1]<<8);
				//if (sendLCDOScopeMsg(lcdData,msgData,portMAX_DELAY) != pdTRUE) {
				//	VT_HANDLE_FATAL_ERROR(0);
				//}
			} else {
				// unexpectedly received this message
				VT_HANDLE_FATAL_ERROR(0);
			}
			break;
		}
		default: {
			VT_HANDLE_FATAL_ERROR(getMsgType(&msgBuffer));
			break;
		}
		}


	}
}