#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "g9_webTask.h"



static char webDebugOut[DEBUG_LINES][DEBUG_LENGTH];

static int curspeed;
static int avgspeed;
static char state[10];
static int amps;
static char loop[10];
static char finished;
static int lap;




static portTASK_FUNCTION_PROTO( webUpdateTask, pvParameters );

void startWebTask(webStruct *ptr, unsigned portBASE_TYPE uxPriority)
{
	if (ptr == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}

	// Create the queue that will be used to talk to this task
	if ((ptr->inQ = xQueueCreate(10,sizeof(g9Msg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( webUpdateTask, ( signed char * ) "WEB", configMINIMAL_STACK_SIZE*5, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

// This is the actual task that is run
static portTASK_FUNCTION( webUpdateTask, pvParameters )
{
	// Get the parameters
	webStruct *param = (webStruct *) pvParameters;
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
				//processWebDebugMsg(msgBuffer.buf[1]);
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
void printw(char* msg){

	processWebDebugMsg(msg);
}
//TODO: keep track of position or linked list, moving the whole array is slow.
void processWebDebugMsg(char* msg){
	int i;
	for(i=0; i<DEBUG_LINES-1; i++){
		strncpy(webDebugOut[i+1], webDebugOut[i], DEBUG_LENGTH);
	}
	strncpy(webDebugOut[0], msg, DEBUG_LENGTH);	
}
void getWebStatusText(char* buffer){
	strcpy(state,"Stopped");
	curspeed = 0;
	avgspeed = 0;
	amps = 0;
	strcpy(loop,"");
	lap = 0;
	finished=0;
	sprintf(buffer,"<table border=1><tr><td><table border=0><tr><td>State</td><td>%s</td></tr><tr><td>Cur Speed</td><td>%d</td></tr><tr><td>Avg  peed</td><td>%d</td></tr><tr><td>Amps</td><td>%d</td></tr></table></td><td><table border=0><tr><td>Loop?</td><td><input type=\"checkbox\" name=\"loop\" value=\"1\" %s></td></tr><tr><td>Lap</td><td>%d</td></tr><tr><td>Finished?</td><td>%d</td></tr></table></td></tr></table>"
	,state,curspeed,avgspeed,amps,loop,lap,finished);
}




char (*getWebDebug())[DEBUG_LENGTH]{
	return webDebugOut;
}