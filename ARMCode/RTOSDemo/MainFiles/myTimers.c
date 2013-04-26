/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "timers.h"

/* include files. */
#include "vtUtilities.h"
#include "g9_LCDOScopeTask.h"
#include "conductor.h"
#include "g9_NavTask.h"
#include "lcdTask.h"
#include "myTimers.h"
#include "messages_g9.h"
#include "vtI2C.h"

#include <string.h>
#include <stdlib.h>

/* **************************************************************** */
// WARNING: Do not print in this file -- the stack is not large enough for this task
/* **************************************************************** */

/* *********************************************************** */
// Functions for the LCD Task related timer
//
// how often the timer that sends messages to the LCD task should run
// Set the task up to run every 100 ms
#define lcdWRITE_RATE_BASE	( ( portTickType ) 100 / portTICK_RATE_MS)

// Callback function that is called by the LCDTimer
//   Sends a message to the queue that is read by the LCD Task
void LCDTimerCallback(xTimerHandle pxTimer)
{
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		// When setting up this timer, I put the pointer to the 
		//   LCD structure as the "timer ID" so that I could access
		//   that structure here -- which I need to do to get the 
		//   address of the message queue to send to 
		vtLCDStruct *ptr = (vtLCDStruct *) pvTimerGetTimerID(pxTimer);
		// Make this non-blocking *but* be aware that if the queue is full, this routine
		// will not care, so if you care, you need to check something
		if (SendLCDTimerMsg(ptr,lcdWRITE_RATE_BASE,0) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void startTimerForLCD(vtLCDStruct *vtLCDdata) {
	if (sizeof(long) != sizeof(vtLCDStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle LCDTimerHandle = xTimerCreate((const signed char *)"LCD Timer",lcdWRITE_RATE_BASE,pdTRUE,(void *) vtLCDdata,LCDTimerCallback);
	if (LCDTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(LCDTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

/* *********************************************************** */
// Functions for the Temperature Task related timer
//
// how often the timer that sends messages to the LCD task should run
// Set the task up to run every 500 ms
#define tempWRITE_RATE_BASE	( ( portTickType ) 500 / portTICK_RATE_MS)

// Callback function that is called by the TemperatureTimer
//   Sends a message to the queue that is read by the Temperature Task
void TempTimerCallback(xTimerHandle pxTimer)
{
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		// When setting up this timer, I put the pointer to the 
		//   Temperature structure as the "timer ID" so that I could access
		//   that structure here -- which I need to do to get the 
		//   address of the message queue to send to 
		vtTempStruct *ptr = (vtTempStruct *) pvTimerGetTimerID(pxTimer);
		// Make this non-blocking *but* be aware that if the queue is full, this routine
		// will not care, so if you care, you need to check something
		if (SendTempTimerMsg(ptr,tempWRITE_RATE_BASE,0) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void startTimerForTemperature(vtTempStruct *vtTempdata) {
	if (sizeof(long) != sizeof(vtTempStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle TempTimerHandle = xTimerCreate((const signed char *)"Temp Timer",tempWRITE_RATE_BASE,pdTRUE,(void *) vtTempdata,TempTimerCallback);
	if (TempTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(TempTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

// Functions for the LCD OScope Task related timer
//
// how often the timer that sends messages to the LCD task should run
// Set the task up to run every 30 ms
#define lcdOSCOPE_RATE_BASE	( ( portTickType ) 400 / portTICK_RATE_MS)

// Callback function that is called by the LCDTimer
//   Sends a message to the queue that is read by the LCD OScope Task
void lcdOScopeTimerCallback(xTimerHandle pxTimer)
{
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		// When setting up this timer, I put the pointer to the 
		//   LCD structure as the "timer ID" so that I could access
		//   that structure here -- which I need to do to get the 
		//   address of the message queue to send to 
		lcdOScopeStruct *ptr = (lcdOScopeStruct *) pvTimerGetTimerID(pxTimer);
		// Make this non-blocking *but* be aware that if the queue is full, this routine
		// will not care, so if you care, you need to check something
		if (SendLCDOScopeTimerMsg(ptr,lcdOSCOPE_RATE_BASE,0) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void startTimerForLCDOScope(lcdOScopeStruct *lcdOScopeData) {
	if (sizeof(long) != sizeof(lcdOScopeStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle lcdOScopeTimerHandle = xTimerCreate((const signed char *)"LCD OScope Timer",lcdOSCOPE_RATE_BASE,pdTRUE,(void *) lcdOScopeData,lcdOScopeTimerCallback);
	if (lcdOScopeTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(lcdOScopeTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

// Functions for the fake I2C Message timer
//
// how often the timer that sends messages to the LCD task should run
// Set the task up to run every 30 ms
#define fakeI2C_RATE_BASE	( ( portTickType ) 2000 / portTICK_RATE_MS)

// Callback function that is called by the LCDTimer
//   Sends a message to the queue that is read by the LCD OScope Task
void fakeI2CTimerCallback(xTimerHandle pxTimer)
{
	static int count;
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		vtI2CStruct *ptr = (vtI2CStruct *) pvTimerGetTimerID(pxTimer);

		fakeI2CMsg* msgBuf = (fakeI2CMsg*)malloc(sizeof(fakeI2CMsg));

	    msgBuf->slvAddr = 0;
		msgBuf->txLen = 0;
		g9Msg msg;

		switch (count%3) {
		  case 0:
		  	msg.msgType = navLineFoundMsg;
			msg.length = 0;
			break;
		  case 1:;
		  	msg.msgType = navIRDataMsg;
			msg.length = 2;
			msg.buf[0] = 1;
			msg.buf[1] = 2;
			break;
		  case 2:
		  	msg.msgType = navRFIDFoundMsg;
			msg.length = 1;
			msg.buf[0] = 0x69;
			break;
		}

		msg.id = count%3+1;
		msgBuf->msgType = msg.msgType;
		msgBuf->rxLen = msg.length + G9_LEN_NO_BUFF;		

		strncpy((char*)msgBuf->buf,(char*)&msg,sizeof(g9Msg));

		if (xQueueSend(ptr->outQ,(void *) (msgBuf),portMAX_DELAY) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
		count++;
		free(msgBuf);
	}
}

void startTimerForFakeI2CMsg(vtI2CStruct* i2c){
	if (sizeof(long) != sizeof(vtI2CStruct*)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle fakeI2CTimerHandle = xTimerCreate((const signed char *)"Fake I2C Timer",fakeI2C_RATE_BASE,pdTRUE,(void *) i2c, fakeI2CTimerCallback);
	if (fakeI2CTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(fakeI2CTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void ConductorTimerCallback(xTimerHandle pxTimer)
{
	static int count = 0;
	g9Msg fakeMsg;
	g9Msg encoderMsg;
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		fakeMsg.id = 0; //Denotes fake msg
		encoderMsg.id = 0; //Denotes fake msg

		//Default encoder values -- forward	@ approx. 1.75 m/s
		encoderMsg.msgType = navEncoderMsg;
		encoderMsg.length = 2*sizeof(short);
		((short*)encoderMsg.buf)[0] = 7000;
		((short*)encoderMsg.buf)[1] = 7000;

		switch( count ){
		case 0:	//Speed Up Start
			//printw("<b style=color:green>Speed Up Test Start</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = SpeedUp;
			break;
		case 5:	//Sped Up End
			//printw("<b style=color:green>Speed Up Test End</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = EndZone;
			break;


		case 10: //Slow Down Start
			//printw("<b style=color:green>Slow Down Test Start</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = SlowDown;
			break;
		case 15: //Slow Down End
			//printw("<b style=color:green>Slow Down Test End</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = EndZone;
			break;

		case 20:  //Right Start
			//printw("<b style=color:green>Go Right Start</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = GoRight;
			break;
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
			//Simulate Right circle
			((short*)encoderMsg.buf)[0] = 3500;
			((short*)encoderMsg.buf)[1] = -3500;
			break;


		case 35:  //Left Start
			//printw("<b style=color:green>Go Left Start</b>\n");
			fakeMsg.msgType = navRFIDFoundMsg;
			fakeMsg.length = 1;
			fakeMsg.buf[0] = GoLeft;
			break;
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
			//Simulate Right circle
			((short*)encoderMsg.buf)[0] = -3500;
			((short*)encoderMsg.buf)[1] = 3500;
			break;

		case 50:
			count = -1; //reset
			break;
		}

		if( fakeMsg.msgType > 0 ){
			SendConductorMsg(&fakeMsg, 500); //Send to conductor
		}
		SendConductorMsg(&encoderMsg, 500); //Send to conductor
		count++;
	}
}
#define conductorWRITE_RATE_BASE	( ( portTickType ) 1000 / portTICK_RATE_MS)

void startTimerForConductor() {
	if (sizeof(long) != sizeof(vtConductorStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle ConductorTimerHandle = xTimerCreate((const signed char *)"Conductor Timer",conductorWRITE_RATE_BASE,pdTRUE,(void *) NULL,ConductorTimerCallback);
	if (ConductorTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(ConductorTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}
