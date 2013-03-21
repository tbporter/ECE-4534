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
#include "messages_g9.h"
#include "conductor.h"

/* *********************************************** */
// definitions and data structures that are private to this file

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations	-- almost certainly too large, see LCDTask.c for details on how to check the size
#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif


#define conQLen 10 /*g9Msgs*/
// end of defs
/* *********************************************** */

/* The i2cTemp task. */
static portTASK_FUNCTION_PROTO( vConductorUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartConductorTask(vtConductorStruct* params,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,vtTempStruct *temperature,oScopeStruct* oScopeData,navStruct* navData)
{
	/* Start the task */
	portBASE_TYPE retval;
	params->i2c = i2c;
	params->tempData = temperature;
	params->oScopeData = oScopeData;
	params->navData = navData;

	// Allocate a queue to be used to communicate with other tasks
	if ((params->inQ = xQueueCreate(conQLen,sizeof(g9Msg))) == NULL) {
		// free up everyone and go home
		VT_HANDLE_FATAL_ERROR(1);
	}

	if ((retval = xTaskCreate( vConductorUpdateTask, ( signed char * ) "Conductor", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE SendConductorMsg(vtConductorStruct* conPtr,g9Msg* msg,portTickType ticksToBlock){
	//Do any need processing here
		//NONE
	//Add the msg to the queue.
	return xQueueSend(conPtr->inQ,(void*)(msg), ticksToBlock);
}

// End of Public API
/*-----------------------------------------------------------*/

// This is the actual task that is run
static portTASK_FUNCTION( vConductorUpdateTask, pvParameters )
{
	uint8_t rxLen, status;
	g9Msg inMsg;
	// Get the parameters
	vtConductorStruct *param = (vtConductorStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *i2cPtr = param->i2c;
	// Get the LCD information pointer
	vtTempStruct *tempData = param->tempData;
	// Get Navigation task pointer
	navStruct* navData = param->navData;

	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from an I2C operation
		if( xQueueReceive(param->inQ,(void*)&inMsg,portMAX_DELAY) != pdTRUE ){
			VT_HANDLE_FATAL_ERROR(2);
		}
		// Decide where to send the message 
		//   This just shows going to one task/queue, but you could easily send to
		//   other Q/tasks for other message types
		// This isn't a state machine, it is just acting as a router for messages
		
		//TODO: Requests are made with conReuqestMsg, but the first byte we recieve is the msg type, so change it to that
		if(inMsg.msgType == conRequestMsg){
			//recvMsgType = Buffer[0];
		}
		switch(inMsg.msgType) {
		case vtI2CMsgTypeTempInit:
		case vtI2CMsgTypeTempRead1:
		case vtI2CMsgTypeTempRead2:
		case vtI2CMsgTypeTempRead3:
			SendTempValueMsg(tempData,inMsg.msgType,(uint8_t*)&inMsg,portMAX_DELAY);
			break;

		case navMotorCmdMsg:
			//Do nothing with this message type
			break;

		case navLineFoundMsg:
		case navIRDataMsg:
		case navRFIDFoundMsg:
			SendNavigationMsg(navData,(uint8_t*)&inMsg,portMAX_DELAY);
			break;
		case voidMsg:
			printf("VOID YO\n");
			break;
		default:
			VT_HANDLE_FATAL_ERROR(inMsg.msgType);
			break;
		}


	}
}

