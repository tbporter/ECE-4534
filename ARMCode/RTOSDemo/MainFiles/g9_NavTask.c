
#include <stdlib.h>
#include <stdio.h>
#include "g9_webTask.h"
#include "g9_NavTask.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define NavQLen 20 //Lots of messages
#define PRINT_MSG_RCV 1 //Notify of incoming msgs

#define MAX_SPEED	10 // m/s
#define MIN_SPEED	3  // m/s
#define TURN_ANGLE	90 //Degrees

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
uint8_t tagValue = None; // Holds any rfid tags that have been found
Bool lineFound = FALSE;

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

//Converts an encoder value to a distance (cm) traveled
inline float enc2Dist(int enc){
	#define ROLL_OUT 38.1 //cm -- guess
	#define TICKS_PER_REV 12000	// guess
	return (ROLL_OUT*enc)/TICKS_PER_REV;
}


//Converts an encoder value into an angle (degrees)
//NOTE: 0 if forward, 180 is backwards, + is left, - is right
inline float enc2Ang(int leftEnc, int rightEnc){
	#define ROVER_WIDTH 30.48 //cm -- guess
	float dS = enc2Dist(rightEnc) - enc2Dist(leftEnc); //differential
	//float R = (enc2Dist(leftEnc)/dS+1)*ROVER_WIDTH;
	return 	dS/ROVER_WIDTH*(180/3.1415927);
}

//Get speed (m/s) from enc value
inline float enc2Speed(int leftEnc, int rightEnc){
	//Get "middle" encoder value
	int medEnc = (leftEnc + rightEnc)/2;
	//Calculate velocity
	return ( 10.0*enc2Dist(medEnc) )/ENC_POLL_RATE; //  1000/100 ms*m/s*cm * 1 cm/ms = 10 cm/ms = 1 m/s
}


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
					printw_err("navMotorCmdMsg - How'd THIS Happen?\n");
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
					printw("<b>navRFIDFoundMsg</b>\n");
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
	int enc = 0; //Encoder value
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
			lineFound = TRUE;
			break;
		
		case navIRDataMsg:
			//Save the data
			leftIR = msgBuffer.buf[0];
			rightIR = msgBuffer.buf[1];
			break;

		case navEncoderMsg:
			enc = msgBuffer.buf[0];
			break;
		
		case navRFIDFoundMsg:
			//Save the data and make a decision
			tagValue |= msgBuffer.buf[0];
			break;
		
		default:
			//Invalid message type
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
		}

		stateMachine();
		handleSpecialEvents(5.3);

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

void adjustSpeed(int enc, float targetSpeed){
	//Get speed
	//Adjust to target
}
									  
void handleSpecialEvents(float speed){
	static uint8_t oldTagValue = None;
	//Check for Error
	if( tagValue & Error ){
		//Report Error, Clear Flag, and Do Nothing
		printw_err("Invalid RFID Tag!\n");
		disableTag(Error);
		return;
	}

	printw("\tHandling RFID Tag: %X\n",tagValue);
	//Depending on tag values adjust motor speed
	if( tagValue != oldTagValue ){
		//Do first occurence actions
		switch( ~(tagValue & oldTagValue) ){
			case SpeedUp:
				//Disable SPD DOWN
				disableTag(SlowDown);
				break;
			case SlowDown:
				//Disable SPD UP
				disableTag(SpeedUp);
				break;
			case GoLeft:
				//Disable RIGHT
				disableTag(GoRight);
				break;
			case GoRight:
				//Disable LEFT
				disableTag(GoLeft);
				break;
		}
	}

	if( tagValue & SpeedUp ){ //SPD UP
		//check speed
			//adjust to target	 
	}
	if( tagValue & SlowDown ){ //SPD DOWN
		
	}
	if( tagValue & GoLeft ){ //LEFT
		
	}
	if( tagValue & GoRight ){ //RIGHT
		
		
	}

	if( tagValue & Finish ){ //FIN -- AT END TO ENSURE STOPPING IN EVENT OF OTHER TAGS
		//Stop if found line
		if(lineFound == TRUE) setMotorData(&motorData,0,0);
		//else slow down?
	}

	oldTagValue = tagValue;

	return;
}

void disableTag(uint8_t tag){
	 tagValue &= (~tag & 0xFF);
}

