#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "g9_webTask.h"


#include "web_input.c"

static char webDebugOut[DEBUG_LINES][DEBUG_LENGTH];

static int curspeed=0;
static int avgspeed=0;
static char state[MAX_MSG_LEN]="Stopped";
static int amps=0;
static char loop[10]="";
static char finished=0;
static int lap=0;
static int start =0;
static uint8_t motors[2] = {64,64};

static portTASK_FUNCTION_PROTO( webUpdateTask, pvParameters );

void startWebTask(webStruct *ptr, unsigned portBASE_TYPE uxPriority, navStruct *navData)
{
	if (ptr == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	ptr->navData = navData;
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
				if(msgBuffer.length == 1){
					amps = msgBuffer.buf[0];
				}				
				break;
			case webSpeedMsg:
				if(msgBuffer.length == 2*sizeof(float)){
					curspeed = ((float*)msgBuffer.buf)[0]*100;
					avgspeed = ((float*)msgBuffer.buf)[1]*100;
				}
				break;
			case webStateMsg:
				if(msgBuffer.length > 0){
					strcpy(state,(char*)msgBuffer.buf);
				}				
				break;
			case webMotorMsg:
				if(msgBuffer.length == 2){
					motors[0] = msgBuffer.buf[0];
					motors[1] = msgBuffer.buf[1];
				}
				break;
			//used to display nav related things from the nav task
			case webNavMsg:
				if(msgBuffer.length == 2){
					finished = msgBuffer.buf[0];
					lap = msgBuffer.buf[1];
				}
				break;		
			default:
				//Invalid message type
				printw_err("Invalid Web Msg! Type: 0x%X\n",msgBuffer.msgType);
			break;
		}
	   
		/*
		if(start == 1){
		   g9Msg msg;
			msg.id = 0;
			msg.length = 0;
			msg.msgType = navWebStartMsg;
			printw("sending start=%d",start);
			msg.msgType = navWebStartMsg;
			SendNavigationMsg(param->navData,&msg,portMAX_DELAY);
			start = 0;
		}*/
	}
}
void printw(const char* fmt, ...){
	char msg[DEBUG_LENGTH];
	va_list args;
	va_start( args, fmt );
	vsnprintf( msg, DEBUG_LENGTH, fmt, args );
	va_end(args);
	processWebDebugMsg(msg);
}

void printw_err(char* fmt, ...){
	#define DEBUG_ERR_LENGTH DEBUG_LENGTH-23
	char msg[DEBUG_LENGTH] = "<b style=color:red>";

	//Truncate fmt if necessary
	if( strlen(fmt) > DEBUG_ERR_LENGTH ) fmt[DEBUG_ERR_LENGTH-1] = '\0';

	//Append fmt
	strcat(msg,fmt);
	//Append end of stlye tags
	strcat(msg,"</b>");


	va_list args;
	va_start( args, fmt );
	vsnprintf( msg, DEBUG_LENGTH, msg, args );
	va_end(args);
	processWebDebugMsg(msg);
}

//TODO: keep track of position or linked list, moving the whole array is slow.
void processWebDebugMsg(char* msg){
	static int i = 0;
	strncpy(webDebugOut[i],msg,DEBUG_LENGTH);
	i++;
	if(i>=DEBUG_LINES){
		i = 0;
	}
	strcpy(webDebugOut[i],"----------------\n");				 
}
void getWebStatusText(char* buffer){
	sprintf(buffer,INFO_TABLE,state,loop,curspeed,avgspeed,lap,finished,amps);
}




char (*getWebDebug())[DEBUG_LENGTH]{
	return webDebugOut;
}

void setWebStart(int s){
	start = s;
	printw("start = %d\n",s);
}
int getWebStart(){
	return start;
}

void getWebMotors( uint8_t* left, uint8_t* right){
	*left = motors[0];
	*right = motors[1];
}

portBASE_TYPE SendWebMsg(webStruct* web,g9Msg* msg,portTickType ticksToBlock){
	if (msg == NULL || web == NULL ) {
		return pdFALSE;
	}

	return(xQueueSend(web->inQ,(void*)(msg),ticksToBlock));
}