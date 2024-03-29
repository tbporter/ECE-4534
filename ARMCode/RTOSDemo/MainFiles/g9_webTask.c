#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "g9_webTask.h"
#include "conductor.h"

static int debugIndex=DEBUG_LINES-1;
static char webDebugOut[DEBUG_LINES][DEBUG_LENGTH];

static int curspeed=0;
static int avgspeed=0;
static char state[MAX_MSG_LEN]="Stopped";

static webInput_t webInput;

static unsigned int time[2]={0}; //nominal, actual in ms

static uint16_t amps=0;
static uint8_t IR[6];
static char lap=0;
static char finished=0;
static int16_t enc[2] = {0};
static uint8_t rfid_tags = 0;
static uint8_t motors[2] = {64,64};

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

	//Set Defaults for webPage
	webInput.printNav=0;
	webInput.start=0;
	setWebInputs(&webInput);

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
				if(msgBuffer.length == sizeof(uint16_t)){
					amps = ((uint16_t*)msgBuffer.buf)[0];
				}				
				break;
			case webSpeedMsg:
				if(msgBuffer.length == 2*sizeof(int)){
					curspeed = ((int*)msgBuffer.buf)[0];
					avgspeed = ((int*)msgBuffer.buf)[1];
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
				if(msgBuffer.length == 3*sizeof(uint8_t) + 2*sizeof(short)){
					finished = msgBuffer.buf[0];
					lap = msgBuffer.buf[1];
					rfid_tags = msgBuffer.buf[2];
					memcpy(enc,&(msgBuffer.buf[3]),2*sizeof(int16_t));
				}
				break;
			case webIRMsg:
				if(msgBuffer.length == 6*sizeof(uint8_t)){
					memcpy(IR,msgBuffer.buf,msgBuffer.length);
				}
				break;
			case webTimeMsg:
				if(msgBuffer.length == 2*sizeof(unsigned int)){
					memcpy(time,msgBuffer.buf,msgBuffer.length);
				}
				break;		
			default:
				//Invalid message type
				printw_err("Invalid Web Msg! Type: 0x%X\n",msgBuffer.msgType);
			break;
		}
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
	if(++i>=DEBUG_LINES) i = 0;
	if(++debugIndex >= DEBUG_LINES) debugIndex = 0;				 
}
void getWebStatusText(char* out, const char* in){
	sprintf(out,in,state,curspeed,avgspeed,lap,amps,finished);
}

void getWebInputText(char* out, const char* in){
	#define checked(val) (((val)==1)?("checked"):(""))
	sprintf(out,in,checked(webInput.start)
				  ,checked(webInput.loop)
				  ,checked(webInput.m4Demo)
				  ,checked(webInput.printNav)
				  ,checked(webInput.printZigBee)
	);
}

void getWebIRText(char* out, const char* in){
	sprintf(out,in,IR[4],IR[0],IR[1],IR[5],IR[2],IR[3]);
}

void getWebSensorText(char* out, const char* in){
	sprintf(out,in,enc[0],enc[1],
						(rfid_tags&GoLeft)>0,
						(rfid_tags&GoRight)>0,
						(rfid_tags&SpeedUp)>0,
						(rfid_tags&SlowDown)>0,
						(rfid_tags&Finish)>0,
						(rfid_tags&Error)>0);
}

void getWebTimesText(char* out, const char* in){
	uint8_t minutes[2],seconds[2], millis[2];
	minutes[0] = time[0]/(1000*60);
	seconds[0] = (time[0]/1000)-minutes[0]*60;
	millis[0] = time[0]-1000*seconds[0]-60*minutes[0];

	minutes[1] = time[1]/(1000*60);
	seconds[1] = (time[1]/1000)-minutes[1]*60;
	millis[1] = time[1]-1000*seconds[1]-60*minutes[1];

	sprintf(out,in,minutes[0],seconds[0],millis[0],minutes[1],seconds[1],millis[1]);
}

char (*getWebDebug(int* index))[DEBUG_LENGTH]{
	*index=debugIndex;
	return webDebugOut;
}

void setWebInputs(webInput_t* in){
	if( in->data != webInput.data){ //Only updata if differet - resolves derpy webpages
		//update Values
		if( (in != &webInput) && (in != 0) ) webInput = *in;
		//Send to Navigation
		g9Msg msg;
		msg.id = 0; //Internal
		msg.msgType = navWebInputMsg;
		msg.length = sizeof(webInput);
		((webInput_t*)msg.buf)[0] = webInput;
	
		//printw("Sending Inputs\n");
		if( SendConductorMsg(&msg,portMAX_DELAY) != pdTRUE){
			printw_err("Failed to send Web Inputs to Conductor!\n");
		}
	}
}
inline char getWebStart(){
	return webInput.start;
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