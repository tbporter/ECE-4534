
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "g9_webTask.h"
#include "g9_NavTask.h"
#include "conductor.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define NavQLen 20 //Lots of messages
#define PRINT_MSG_RCV 1 //Notify of incoming msgs

#define MAX_SPEED	10 // m/s
#define MIN_SPEED	0  // m/s
#define MAX_SPD_DELTA 20

#define MAX_TURN_ANGLE	90 //Degrees
#define MIN_TURN_ANGLE	60 //Degrees
 
int IR[6];

#define LEFT_FRONT_IR	IR[0]
#define RIGHT_FRONT_IR	IR[1]
#define LEFT_BACK_IR	IR[2]
#define RIGHT_BACK_IR	IR[3]
#define SONAR_LEFT		IR[4]
#define SONAR_RIGHT		IR[5]


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
uint8_t tagValue = None; // Holds any rfid tags that have been found
Bool lineFound = FALSE;
char state[MAX_MSG_LEN] = "Stopped";


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

//Set state Text
inline void setState(const char* msg){
	strcpy(state,msg);
}

//Converts an encoder value to a distance (cm) traveled
inline float enc2Dist(short enc){
	#define ROLL_OUT 45.01 //cm
	#define TICKS_PER_REV 18000	// guess
	return (ROLL_OUT*enc)/TICKS_PER_REV;
}											


//Converts an encoder value into an angle (degrees)
//NOTE: 0 if forward/backwards, + is left, - is right
inline float enc2Ang(short leftEnc, short rightEnc){
	#define ROVER_WIDTH 34.29 //cm
	float dS = enc2Dist(rightEnc) - enc2Dist(leftEnc); //differential
	//float R = (enc2Dist(leftEnc)/dS+1)*ROVER_WIDTH;
	return 	dS/ROVER_WIDTH*(180/3.1415927);
}

//Get speed (m/s) from enc value
inline float enc2Speed(short leftEnc, short rightEnc){
	//Get "middle" encoder value
	short medEnc = (leftEnc + rightEnc)/2;
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
					printw("<b>navEncoderMsg</b> %d %d\n",((short*)msg->buf)[0], ((short*)msg->buf)[1]);
				break;
			
			case navLineFoundMsg:
					printw("<b>navLineFoundMsg</b> %d\n",msg->buf[0]<<8 | msg->buf[1]);
				break;
			
			case navIRDataMsg:
					printw("<b>navIRDataMsg</b> %d %d %d %d %d %d\n",msg->buf[0],msg->buf[1],msg->buf[2],msg->buf[3],msg->buf[4],msg->buf[5]);
					printw("<b>Current = %d</b>\n",msg->buf[6]);
				break;
			
			case navRFIDFoundMsg:
					printw("<b>navRFIDFoundMsg</b>\n");
				break;
			case navWebStartMsg:
					printw("<b style=color:green>navWebStartMsg\n</b>");
				break;	
	
			default:
				printw_err("Incorrect Navigation Msg\n");
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
		int16_t enc[2] = {0,0}; //Encoder value

		// Wait for a message from the "otherside"
		portBASE_TYPE ret;
		if ((ret = xQueueReceive(navData->inQ,(void *) &msgBuffer,portMAX_DELAY) ) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(ret);
		}

		g9Msg msg;

		// Now, based on the type of the message, we decide on the action to take
		switch (msgBuffer.msgType){
		case navLineFoundMsg:
			//stop we have found the finish line!!!
			if( (((int*)msgBuffer.buf)[0] >= LINE_FOUND_THRE) && (tagValue & Finish) ) lineFound = TRUE;
			break;
		
		case navIRDataMsg:
			//Save the data
			LEFT_FRONT_IR = msgBuffer.buf[0];
			RIGHT_FRONT_IR = msgBuffer.buf[1];
			LEFT_BACK_IR = msgBuffer.buf[2];
			RIGHT_BACK_IR = msgBuffer.buf[3];
			SONAR_LEFT = msgBuffer.buf[4];
			SONAR_RIGHT = msgBuffer.buf[5];

			msg.msgType = webPowerMsg;
			msg.id = 0; //internal
			msg.length = 1;
			msg.buf[0] = msgBuffer.buf[6];
			SendConductorMsg(&msg,10);
			break;

		case navEncoderMsg:
				//Get encoder values
				enc[0] = ((short*)msgBuffer.buf)[0];
				enc[1] = ((short*)msgBuffer.buf)[1];
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


		if(getWebStart()==1){
			setState("Navigate");
			stateMachine();
			handleSpecialEvents(enc);
		}
		else{
			setMotorData(&motorData,64,64);
			setState("Stopped");
		}

		msg.msgType = navMotorCmdMsg;
		msg.id = msgBuffer.id + 1;
		msg.length = 2;
		msg.buf[0] = motorData.left;
		msg.buf[1] = motorData.right;		
		SendZigBeeMsg(navData->zigBeePtr,&msg,portMAX_DELAY);

		//Send off web information
		msg.msgType = webMotorMsg;
		msg.id = 0; //internal
		msg.length = 2;
		getMotorData(&motorData,&msg.buf[0],&msg.buf[1]);
		SendConductorMsg(&msg,10);

		msg.msgType = webSpeedMsg;
		msg.id = 0; //internal
		msg.length = 2*sizeof(float);
		((float*)msg.buf)[0] = enc2Speed(enc[0],enc[1]);
		((float*)msg.buf)[1] = enc2Speed(enc[0],enc[1]); //TODO: calculate Avg
		SendConductorMsg(&msg,10);

		msg.msgType = webStateMsg;
		msg.id = 0; //internal
		msg.length = strlen(state);
		strcpy((char*)msg.buf,state);
		SendConductorMsg(&msg,10);

		msg.msgType = webNavMsg;
		msg.id = 0; //internal
		msg.length = 2;
		msg.buf[0] = lineFound;
		msg.buf[1] = 0;
		SendConductorMsg(&msg,10);
	}
}



void stateMachine(){
	setMotorData(&motorData,64,64);
	switch(curState){
		#define min_dist 100
		//Simple state, lets just lean to the left or right based off the IR
		case 0:
			if(RIGHT_FRONT_IR>120){
				setMotorData(&motorData,70,90);
			}
			else if(LEFT_FRONT_IR>120){
				setMotorData(&motorData,90,70);
			}
			else if(RIGHT_FRONT_IR>100){
				setMotorData(&motorData,95,110);
			}
			else if(LEFT_FRONT_IR>100){
				setMotorData(&motorData,110,95);
			}
			else {
				setMotorData(&motorData,110,110);
			}
			break;
	}

}

void adjustSpeed(short leftEnc, short rightEnc, int targetSpeed){
	#define getSpeed(old,del) (((old)>64)?((old) + (delta)):( ((old)==64)?(64):((old)-(delta)) ))  //Increase magnitude of old if not 64
	//Get speed
	float curSpeed = enc2Speed(leftEnc,rightEnc);
	float diff = curSpeed - targetSpeed;
	int delta = 0;
	uint8_t left, right;
	int newLeft, newRight;
	//Calculate delta
	if( diff > 0.0 || diff < -0.1 ){ //If outside of tolerance of -0.1 to 0
		//Speed Up
		delta = pow( diff , 2) * MAX_SPD_DELTA;

		if( delta > MAX_SPD_DELTA ) delta = MAX_SPD_DELTA;
		//Slow Down
		if( targetSpeed < curSpeed ) delta = -delta;
		//printw("delta = %d\ttarget = %d\t current = %f\n",delta,targetSpeed,curSpeed);
	}
	//else do nothing;

	//Get motor values
	getMotorData(&motorData, &left, &right);

	newLeft  = left + delta;//getSpeed(left,delta);
	newRight = right + delta;//getSpeed(right,delta);

	//Contrain values
	if( newLeft > 127 || newLeft < 1 || \
		newRight > 127 || newLeft < 1){
		//Delta is too large, adjust it
		if( delta > 0 ){
			uint8_t maxMotor = max( newLeft, newRight);
			delta -= maxMotor-127;
		}
		else{
			uint8_t minMotor = min( newLeft, newRight );
			delta += 1-minMotor;
		}
		//Set new motor values
		newLeft  = getSpeed(left,delta);
		newRight = getSpeed(right,delta);
	}

	//apply motor values
	setMotorData(&motorData, newLeft, newRight);

}

char doTurn(int curAngle, int minAngle, int maxAngle){
	if( curAngle < MAX_TURN_ANGLE ){
		if( curAngle >= MIN_TURN_ANGLE ){
			//TODO: Check IR to see if can proceed yet, if not, continue turning
			if(0){
			 	return 1; //Done, Clear to Proceed
			}
		}
	}
	else{
		//TODO: Check IR to see if safe to proceed, if not, return 2
		if(0){
			return 2;//Done, Path Is Blocked
		}
		return 1; //Done, Clear To Proceed
	}
	return 0; //Continue turning
}
									  
void handleSpecialEvents(short* enc){
	static uint8_t oldTagValue = None;
	static short elapsedEnc[2] = {0,0};
	//Check for Error
	if( tagValue & Error ){
		//Report Error, Clear Flag, and Do Nothing
		printw_err("Invalid RFID Tag!\n");
		disableTag(Error);
		return;
	}

	//Depending on tag values adjust motor speed
	if( tagValue != oldTagValue ){
		//Do first occurence actions
		uint8_t diff = tagValue & ~oldTagValue;
		//printw("diff = %X\n",diff);
		if( diff & SpeedUp){
			//Disable SPD DOWN
			disableTag(SlowDown);
		}

		if( diff & SlowDown ){
			//Disable SPD UP
			disableTag(SpeedUp);
		}

		if( diff & GoLeft ){
			//Disable RIGHT
			disableTag(GoRight);
			elapsedEnc[0] = 0;
			elapsedEnc[1] = 0;
		}
		
		if( diff & GoRight ){
			//Disable LEFT
			disableTag(GoLeft);
			elapsedEnc[0] = 0;
			elapsedEnc[1] = 0;
		}
		
		if( diff & EndZone){
			disableTag(SpeedUp);
			disableTag(SlowDown);
			disableTag(EndZone);
		}
	}

	if( tagValue & SpeedUp ){ //SPD UP
		setState("Go Fast");
		//adjust to target
		adjustSpeed(enc[0], enc[1], MAX_SPEED);	 
	}
	if( tagValue & SlowDown ){ //SPD DOWN
		setState("Go Slow");
		//adjust to target
		adjustSpeed(enc[0], enc[1], MIN_SPEED);		
	}
	if( tagValue & GoLeft ){ //LEFT
		setState("Go Left");
		//Update Encoder counts
		elapsedEnc[0] += enc[0];
		elapsedEnc[1] += enc[1];
		//Check angle
		float angle = enc2Ang(elapsedEnc[0], elapsedEnc[1]);
		switch( doTurn(angle,MIN_TURN_ANGLE,MAX_TURN_ANGLE) ){
			case 0: //Continue
				setMotorData(&motorData,32,96); //Half Speed 0-Radius Left Turn
				break;
			case 1: //Done, Proceed
				disableTag(GoLeft);
				break;
			case 2:	//Done, Blocked
				disableTag(GoLeft);
				enableTag(GoRight);
				//TODO: Handle stuck at corner situation
				break;
		} 
			
	}
	if( tagValue & GoRight ){ //RIGHT
		setState("Go Right");
		//Update Encoder counts
		elapsedEnc[0] += enc[0];
		elapsedEnc[1] += enc[1];
		//Check angle
		float angle = -enc2Ang(elapsedEnc[0], elapsedEnc[1]);
		switch( doTurn(angle,MIN_TURN_ANGLE,MAX_TURN_ANGLE) ){
			case 0: //Continue
				setMotorData(&motorData,96,32); //Half Speed 0-Radius Right Turn
				break;
			case 1: //Done, Proceed
				disableTag(GoRight);
				break;
			case 2:	//Done, Blocked
				disableTag(GoRight);
				enableTag(GoLeft);
				//TODO: Handle stuck at corner situation
				break;
		} 
		
	}

	if( tagValue & Finish ){ //FIN -- AT END TO ENSURE STOPPING IN EVENT OF OTHER TAGS
		//Stop if found line
		if(lineFound == TRUE) setMotorData(&motorData,64,64);
		//else slow down?
	}

	oldTagValue = tagValue;

	return;
}

inline void disableTag(uint8_t tag){
	 tagValue &= (~tag & 0xFF);
}

inline void enableTag(uint8_t tag){
	 tagValue |= tag;
}

