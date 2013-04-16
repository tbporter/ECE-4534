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
#include "g9_SysFlags.h"

/* *********************************************** */
// definitions and data structures that are private to this file

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations	-- almost certainly too large, see LCDTask.c for details on how to check the size
#define INSPECT_STACK 1
#define baseStack 3
#if PRINTF_VERSION == 1
#define conSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

// end of defs
/* *********************************************** */

/* The i2cTemp task. */
static portTASK_FUNCTION_PROTO( vConductorUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartConductorTask(vtConductorStruct* params,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,vtTempStruct *temperature,oScopeStruct* oScopeData,navStruct* navData,g9ZigBeeStruct* zigBeeData)
{
	/* Start the task */
	portBASE_TYPE retval;
	params->i2c = i2c;
	params->tempData = temperature;
	params->oScopeData = oScopeData;
	params->navData = navData;
	params->zigBeeData = zigBeeData;

	if ((retval = xTaskCreate( vConductorUpdateTask, ( signed char * ) "Conductor", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
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
	// Get the zigBee task pointer
	g9ZigBeeStruct* zigBeeData = param->zigBeeData;

	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message
		#if USE_XBEE == 1
		if( xQueueReceive(zigBeeData->outQ,(void*)&inMsg,portMAX_DELAY) != pdTRUE ){
			VT_HANDLE_FATAL_ERROR(2);
		}
		#elif USE_I2C == 1
			//TODO: setup I2C for conductor
			#error "I2C Conductor not implemented... yet."
		#else
			#error "Must have XBee or I2C enabled!"
		#endif
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
		case navEncoderMsg:
			if( SendNavigationMsg(navData,&inMsg,portMAX_DELAY) != pdTRUE ){
				VT_HANDLE_FATAL_ERROR(3);
			}
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

