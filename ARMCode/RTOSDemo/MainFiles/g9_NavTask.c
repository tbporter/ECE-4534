
#include <stdlib.h>
#include <stdio.h>
#include "g9_webTask.h"
#include "g9_NavTask.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define NavQLen 20 //Lots of messages
#define PRINT_MSG_RCV 1 //Notify of incoming msgs

#define SPD_UP_TARGET	10
#define SPD_DWN_TARGET	3
#define SPD_UP_DELTA	+16 //+25%
#define SPD_DWN_DELTA	-16 //-25%
#define SPD_L_DELTA		
#define SPD_R_DELTA

//helper functions for setting motor speeds
typedef union {
	uint16_t data;
	struct {
		uint8_t left;
		uint8_t right;
	};
} motorData_t;

motorData_t motorData;
int curState;
int leftIR;
int rightIR;
// Holds any rfid tags that have been found
uint8_t tagValue = 0;

void setMotorData(motorData_t* motorData, uint8_t left, uint8_t right){
	left &= 0x7F;
	right &= 0x7F;
	motorData->data = 0x0080 | (right<<8) | left;
}

void getMotorData(motorData_t* motorData, uint8_t* left, uint8_t* right){
	*left = motorData->left & 0x7F;
	*right = motorData->right & 0x7F;
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

	#if PRINT_MSG_RCV == 1
		switch (msg->msgType){
		
			case navMotorCmdMsg:
					printw("<b style=color:red>navMotorCmdMsg - How'd THIS Happen?\n");
				break;
	
			case navEncoderMsg:
					printw("<b>navEncoderMsg</b> %d\n",msg->buf[0]);
				break;
			
			case navLineFoundMsg:
					printw("navLineFoundMsg\n");
				break;
			
			case navIRDataMsg:
					printw("<b>navIRDataMsg<\b> %d %d\n",msg->buf[0],msg->buf[1]);
				break;
			
			case navRFIDFoundMsg:
					printw("<b>navRFIDFoundMsg</b> ");
				break;
			case navWebStartMsg:
					printw("navWebStartMsg\n");
				break;	
	
			default:
				printw("Incorrect Navigation Msg\n");
		}
	#endif

	return(xQueueSend(nav->inQ,(void*)(msg),ticksToBlock));
}

// This is the actual task that is run
static portTASK_FUNCTION( navigationUpdateTask, pvParameters )
{

	// Get the parameters
	navStruct *navData = (navStruct *) pvParameters;
	// Buffer for receiving messages
	g9Msg msgBuffer;

	curState = 0;
	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from the "otherside"
		portBASE_TYPE ret;
		if ((ret = xQueueReceive(navData->inQ,(void *) &msgBuffer,portMAX_DELAY) ) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(ret);
		}

		g9Msg msg;
		msg.id = msgBuffer.id + 1;
		msg.length = 2;
		msg.msgType = navMotorCmdMsg;

		// Now, based on the type of the message, we decide on the action to take
		switch (msgBuffer.msgType){
		case navLineFoundMsg:
			//stop we have found the finish line!!!
			//setMotorData(&motorData,127,127);
			break;
		
		case navIRDataMsg:
			//Save the data
			leftIR = msgBuffer.buf[0];
			rightIR = msgBuffer.buf[1];
			//setMotorData(&motorData,1,0); //Turn left
			break;
		
		case navRFIDFoundMsg:
			//Save the data and make a decision
			//setMotorData(&motorData,0,1); //Forward
			tagValue |= msgBuffer.buf[0];
			break;
		
		default:
			//Invalid message type
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
		}

		stateMachine();
		handleSpecialEvents(5.3);
		tagValue = 0;

		msg.buf[0] = motorData.left;
		msg.buf[1] = motorData.right;		
		SendZigBeeMsg(navData->zigBeePtr,&msg,portMAX_DELAY);
	}
}



void stateMachine(){
	setMotorData(&motorData,0,0);
	switch(curState){
		#define min_dist 100
		//Simple state, lets just lean to the left or right based off the IR
		case 0:
			if(getWebStart()==1){
				if(rightIR>120){
									setMotorData(&motorData,70,90);
				}
				else if(leftIR>120){
									setMotorData(&motorData,90,70);
				}
				else if(rightIR>100){
					setMotorData(&motorData,95,110);
				}
				else if(leftIR>100){
					setMotorData(&motorData,110,95);
				}
				else {
					setMotorData(&motorData,110,110);
				}
			}
			else{
				setMotorData(&motorData,0,0);
			}	

	}

}
									  
void handleSpecialEvents(float speed){
	static uint8_t oldTagValue = 0x0;
	//Depending on tag values adjust motor speed
	if( tagValue != oldTagValue ){
		printw("Handling RFID Tag: %X\n",tagValue);
		//Do first occurence actions
		switch( ~(tagValue & oldTagValue) ){
			case 0x01:
				//Disable SPD DOWN
				disableTag(0x02);
				break;
			case 0x02:
				//Disable SPD UP
				disableTag(0x01);
				break;
			case 0x04:
				//Disable RIGHT
				disableTag(0x08);
				break;
			case 0x08:
				//Disable LEFT
				disableTag(0x04);
				break;
		}
	}

	if( tagValue & 0x01 ){ //SPD UP
		 
	}
	if( tagValue & 0x02 ){ //SPD DOWN
		
	}
	if( tagValue & 0x04 ){ //LEFT
		
	}
	if( tagValue & 0x08 ){ //RIGHT
		
		
	}

	if( tagValue & 0x10 ){ //FIN -- AT END TO ENSURE STOPPING IN EVENT OF ERROR
		//Stop
		setMotorData(&motorData,0,0);
	}

	oldTagValue = tagValue;
}

void disableTag(uint8_t tag){
	 tagValue &= (~tag & 0xFF);
}

uint8_t adjuctSpeed(char delta){
	if( abs(delta) > 64 ) return 0; //Delta too large, will change heading
	//Determine max/min delta until clipping occurs
	uint8_t left;
	uint8_t right;
	getMotorData(&motorData,&left,&right);

	float scaleLeft, scaleRight;
	
	//Scale and shift left and right to be from -1 to 1
	if( left == 0 ){
		scaleLeft = 0;
	}
	else{
		scaleLeft = (left - 64)/127;
	}

	if( right == 0 ){
		scaleRight = 0;
	}
	else{
		scaleRight = (right - 64)/127;
	}

	char maxDelta = 0;

	if(delta >=0){ //find max
		maxDelta = max(1-scaleLeft,1-scaleRight);
	}else{ //find min
		maxDelta = max(scaleLeft+1,scaleRight+1);
	}


	
	return delta;		
}

