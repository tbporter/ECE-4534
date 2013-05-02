
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

#define MAX_SPEED	1000 // cm/s
#define MIN_SPEED	0  // cm/s
#define MAX_SPD_DELTA 20

#define MAX_TURN_ANGLE	90 //Degrees
#define MIN_TURN_ANGLE	60 //Degrees
 
uint8_t IR[6];
uint8_t oldIR[6][5];
uint8_t irIndex=0;

#define LEFT_FRONT_IR	IR[0]
#define RIGHT_FRONT_IR	IR[1]
#define LEFT_BACK_IR	IR[2]
#define RIGHT_BACK_IR	IR[3]
#define SONAR_LEFT		IR[4]
#define SONAR_RIGHT		IR[5]
#define dc 99999

//helper functions for setting motor speeds
typedef union {
	uint16_t data;
	struct {
		uint8_t left;
		uint8_t right;
	};
} motorData_t;

typedef enum {
	straight=0,
	ninety,
	turn
} navState;

typedef enum {
	none=0,
	left,
	right
} dir;

motorData_t motorData;
navState curState;
dir curDir;
uint8_t tagValue = None; // Holds any rfid tags that have been found
Bool lineFound = FALSE;
char state[MAX_MSG_LEN] = "Stopped";
webInput_t inputs = {{0,0,0,1,0}}; //NOTE: Refer to Struct Definition
int16_t oldEnc[2] = {0,0}; //Old Encoder values -- use for webserver only

unsigned int totDist = 0; //cm

portTickType enc_poll_rate = 100; //Average rate of enc messages. NOTE: This value is just a starting point.


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
	#define TICKS_PER_REV 14400
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

inline int ir2Dist(uint8_t raw1, int old){
	//float raw = ((int)raw1<<2);
	//float retVal = (raw * 3.3)/255.0;
	//printw("retVal = %f \t", retVal);
	if(raw1<2){
		return old;
	}
	//retVal = 41.543 * pow(retVal + 0.30221, -1.5281);
	int retVal = ((6762/(4*raw1-9))-4);
	//float herp = 12343.85*pow(raw,-1.15);
	//int retVal = herp;
	if(retVal<0)
		retVal = old;
	else if(retVal>100)
		retVal = 100;
	return retVal; 
}

//Get speed (cm/s) from enc value
inline float enc2Speed(short leftEnc, short rightEnc){
	//Get "middle" encoder value
	short medEnc = (leftEnc + rightEnc)/2;
	//Calculate velocity
	return ( 1000.0*enc2Dist(medEnc) )/enc_poll_rate; //  1000 ms/s * 1 cm/ms = 1000 cm/s
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

	if( inputs.printNav == 1){
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
					printw("<b>navRFIDFoundMsg</b> %#X\n",msg->buf[0]);
				break;
			case navWebInputMsg:
				{
					webInput_t* webInput = (webInput_t*)msg->buf;
					printw("<b style=color:green>navWebInputMsg</b> %d%d%d%d%d\n",webInput->start,webInput->loop,webInput->m4Demo,webInput->printNav,webInput->printZigBee);
				break;	
				}

			default:
				printw_err("Incorrect Navigation Msg\n");
		}
	}

	return(xQueueSend(nav->inQ,(void*)(msg),ticksToBlock));
}

uint8_t avgIr(uint8_t ir[5]){
	uint8_t i=0;
	int ret=0;
	for(i=0; i<5; i++){
		ret += ir[i];	
	}
	return ret/5;
}
int zigbeetimer = 0;
// This is the actual task that is run
static portTASK_FUNCTION( navigationUpdateTask, pvParameters )
{

	// Get the parameters
	navStruct *navData = (navStruct *) pvParameters;
	// Buffer for receiving messages
	g9Msg msgBuffer;

	curState = straight;
	portTickType ticksAtStart=0; //in ticks

	// Like all good tasks, this should never exit
	for(;;)
	{
		int16_t enc[2] = {0,0}; //Encoder value

		// Wait for a message from the "otherside"
		portBASE_TYPE ret;
		if ((ret = xQueueReceive(navData->inQ,(void *) &msgBuffer,portMAX_DELAY) ) != pdTRUE) {
			//VT_HANDLE_FATAL_ERROR(ret);
			printw_err("NavTask Didn't RCV From PIC!\n");
			setMotorData(&motorData,64,64);
			setState("Stopped");
			goto end; //Skip everything send stop
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
			LEFT_FRONT_IR = ir2Dist(msgBuffer.buf[0],LEFT_FRONT_IR);
			RIGHT_FRONT_IR = ir2Dist(msgBuffer.buf[1],RIGHT_FRONT_IR);
			LEFT_BACK_IR = ir2Dist(msgBuffer.buf[2],LEFT_BACK_IR);
			RIGHT_BACK_IR = ir2Dist(msgBuffer.buf[3],RIGHT_BACK_IR);
			SONAR_LEFT = ir2Dist(msgBuffer.buf[4],SONAR_LEFT);
			SONAR_RIGHT = ir2Dist(msgBuffer.buf[5],SONAR_RIGHT);

			oldIR[0][irIndex] = LEFT_FRONT_IR;
			oldIR[1][irIndex] = RIGHT_FRONT_IR;
			oldIR[2][irIndex] = LEFT_BACK_IR;
			oldIR[3][irIndex] = RIGHT_BACK_IR;
			oldIR[4][irIndex] = SONAR_LEFT;
			oldIR[5][irIndex] = SONAR_RIGHT;

			irIndex = (irIndex+1)%5;
			/*
			LEFT_FRONT_IR = avgIr(oldIR[0]);
			RIGHT_FRONT_IR = avgIr(oldIR[1]);
			LEFT_BACK_IR = avgIr(oldIR[2]);
			RIGHT_BACK_IR = avgIr(oldIR[3]);
			SONAR_LEFT = avgIr(oldIR[4]);
			SONAR_RIGHT = avgIr(oldIR[5]);*/

			msg.msgType = webIRMsg;
			msg.id = 0; //internal
			msg.length = 6*sizeof(uint8_t);
			memcpy(msg.buf,IR,msg.length);
			SendConductorMsg(&msg,10);
			
			msg.msgType = webPowerMsg;
			msg.id = 0; //internal
			msg.length = sizeof(uint16_t);
			((uint16_t*)msg.buf)[0] = (uint16_t)(msgBuffer.buf[6])*2;
			SendConductorMsg(&msg,10);
			break;

		case navEncoderMsg:
		{
				static portTickType oldTick=0;
				portTickType newTick = xTaskGetTickCount();
				//Get encoder values
				enc[0] = ((short*)msgBuffer.buf)[0];
				enc[1] = ((short*)msgBuffer.buf)[1];
				totDist += enc2Dist((enc[0]+enc[1])/2);
				//calculate poll rate
				enc_poll_rate = (enc_poll_rate+((newTick-oldTick)/portTICK_RATE_MS))/2;

				oldEnc[0] = enc[0];
				oldEnc[1] = enc[1];

				oldTick=newTick;

			break;
		}

		case navRFIDFoundMsg:
			//Save the data and make a decision
			tagValue |= msgBuffer.buf[0];
			break;

		case navWebInputMsg:
		{
			//printw("Inputs are HERE!\n");
			uint8_t oldStart = inputs.start;
			inputs = ((webInput_t*)msgBuffer.buf)[0];
			if( oldStart == 0 && inputs.start == 1 ){
				//Get start time (in ticks)
				ticksAtStart = xTaskGetTickCount();
				//Reset Distance
				totDist = 0;
			}
			break;
		}
		
		default:
			//Invalid message type - Should have been handled in conductor
			VT_HANDLE_FATAL_ERROR(INVALID_G9MSG_TYPE);
			break;
		}

		if(inputs.start==1){
			setState("Navigate");
			stateMachine();
			handleSpecialEvents(enc);
		}
		else{
			setMotorData(&motorData,64,64);
			setState("Stopped");
		}

end:	msg.msgType = navMotorCmdMsg;
		msg.id = msgBuffer.id + 1;
		msg.length = 2;
		msg.buf[0] = motorData.left;
		msg.buf[1] = motorData.right;		
		
		vtLEDOn(0x01);
		SendZigBeeMsg(navData->zigBeePtr,&msg,portMAX_DELAY);
		vtLEDOff(0x01);

		//Send off web information
		msg.msgType = webMotorMsg;
		msg.id = 0; //internal
		msg.length = 2;
		getMotorData(&motorData,&msg.buf[0],&msg.buf[1]);
		SendConductorMsg(&msg,10);

		msg.msgType = webSpeedMsg;
		msg.id = 0; //internal
		msg.length = 2*sizeof(int);
		((int*)msg.buf)[0] = (int)enc2Speed(oldEnc[0],oldEnc[1]);
		((int*)msg.buf)[1] = (1000*totDist*portTICK_RATE_MS)/(xTaskGetTickCount()-ticksAtStart); // cm/(ticks/(ticks/ms)) = cm/ms * 1000 ms/s = cm/s
		SendConductorMsg(&msg,10);

		msg.msgType = webStateMsg;
		msg.id = 0; //internal
		msg.length = strlen(state);
		strcpy((char*)msg.buf,state);
		SendConductorMsg(&msg,10);

		msg.msgType = webNavMsg;
		msg.id = 0; //internal
		msg.length = 3*sizeof(uint8_t) + 2*sizeof(int16_t);
		msg.buf[0] = lineFound;
		msg.buf[1] = 0;
		msg.buf[2] = tagValue;
		memcpy(&(msg.buf[3]),oldEnc,2*sizeof(int16_t));

		SendConductorMsg(&msg,10);
	}
}

#define distShort 3
#define distMed	6 
#define distLong 9

#define speedSlow 77
#define speedMed 79
#define speedFast 88
#define speedStop 64
#define speedBack 40
void stateMachine(){
	setMotorData(&motorData,64,64);

transition_state:
	switch(curState){
		case straight:
			//First lets see if we need to make a turn
			if(chkDist (dc,dc,50,50,dc,dc)){
				if(RIGHT_FRONT_IR > LEFT_FRONT_IR)
					curDir = right;
				else
					curDir = left;
				curState = ninety;
				goto transition_state;
			}
			if(LEFT_FRONT_IR<13){	
				curDir = right;
				curState = turn;
				goto transition_state;
			}
			else if(RIGHT_FRONT_IR<13){
				curDir = left;
				curState = turn;
				goto transition_state;
			}
			else if(LEFT_FRONT_IR<21){
				setMotorData(&motorData,speedFast+3,speedSlow-3);
			}
			else if(RIGHT_FRONT_IR<21){
				setMotorData(&motorData,speedSlow-3,speedFast+3);
			}
			else if(SONAR_LEFT<45){
				setMotorData(&motorData,speedFast+3,speedSlow-3);				
			}
			else if(SONAR_RIGHT<45){
				setMotorData(&motorData,speedSlow-3,speedFast+3);
			}
			else if(LEFT_FRONT_IR<RIGHT_FRONT_IR){
				 setMotorData(&motorData,speedFast,speedFast-2);
			}
			else if(RIGHT_FRONT_IR<LEFT_FRONT_IR){
				 setMotorData(&motorData,speedFast-2,speedFast);
			}
			else{
				 setMotorData(&motorData,speedFast,speedFast);
			}	

		break;
		case ninety:
			//keep turning until the front sensors read a larg val
			if (!chkDist(dc,dc,65,65,dc,dc)){
				curState = straight;
				curDir = none;
				break;
			}
			if(curDir==left){
				setMotorData(&motorData,speedStop-(speedMed-speedStop)-5,speedMed);
			}
			else if(curDir==right){
				setMotorData(&motorData,speedMed,speedStop-(speedMed-speedStop)-5);
			}
			
		break;
		case turn:
			if(curDir==right){
				setMotorData(&motorData,speedMed,speedStop-(speedMed-speedStop)-5);
				if(LEFT_FRONT_IR>12)
					curState = straight;
			}
			else if(curDir==left){				
				setMotorData(&motorData,speedStop-(speedMed-speedStop)-5,speedMed);
				if(RIGHT_FRONT_IR>12)
					curState = straight;
			}
		break;
	}

}


Bool chkDist(float left, float right, float front_left, float front_right, float side_left, float side_right){
	return (LEFT_FRONT_IR<left 			&&
			RIGHT_FRONT_IR<right 		&&
			SONAR_LEFT <front_left 		&&
			SONAR_RIGHT<front_right 	&&
			LEFT_BACK_IR < side_left	&&
			RIGHT_BACK_IR < side_right
		   );
}


void adjustSpeed(short leftEnc, short rightEnc, int targetSpeed){
	#define getSpeed(old,del) (((old)>64)?((old) + (delta)):( ((old)==64)?(64):((old)-(delta)) ))  //Increase magnitude of old if not 64
	//Get speed
	int curSpeed = enc2Speed(leftEnc,rightEnc);
	int diff = curSpeed - targetSpeed;
	int delta = 0;
	uint8_t left, right;
	int newLeft, newRight;
	//Calculate delta
	if( diff > 0 || diff < -10 ){ //If outside of tolerance of -10 to 0
		//Speed Up
		delta = diff*diff * MAX_SPD_DELTA;

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
		adjustSpeed(oldEnc[0], oldEnc[1], MAX_SPEED);	 
	}
	if( tagValue & SlowDown ){ //SPD DOWN
		setState("Go Slow");
		//adjust to target
		adjustSpeed(oldEnc[0], oldEnc[1], MIN_SPEED);		
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
		//if(lineFound == TRUE) setMotorData(&motorData,64,64);
		setMotorData(&motorData,64,64);
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

