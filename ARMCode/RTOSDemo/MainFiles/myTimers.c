/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "timers.h"

/* include files. */
#include "vtUtilities.h"
#include "g9_LCDOScopeTask.h"
#include "lcdTask.h"
#include "myTimers.h"
#include "messages.h"
#include "vtI2C.h"

#include <string.h>

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
#define fakeI2C_RATE_BASE	( ( portTickType ) 500 / portTICK_RATE_MS)

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
		uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t)*3);

		switch (count) {
		  case 0:
		  	msg.id = 1;
		  	msgBuf->msgType = navLineFoundMsg;
			msg.msgType = msgBuf->msgType;
			msgBuf->rxLen = navLineFoundLen + sizeof(g9Msg);
			break;
		  case 1:
		  	msg.id = 2;
		  	msgBuf->msgType = navIRDataMsg;
			msg.msgType = msgBuf->msgType;
			msgBuf->rxLen = navIRDataLen + sizeof(g9Msg);
			buf[0] = 1;
			buf[1] = 2;
			buf[2] = 3;
			msg.buf = buf;
			break;
		  case 2:
		  	msg.id = 3;
		  	msgBuf->msgType = navRFIDFoundMsg;
		  	msg.msgType = msgBuf->msgType;
			msgBuf->rxLen = navRFIDFoundLen + sizeof(g9Msg);
			buf[0] = 0x69;
			msg.buf = buf;
			break;
		}		

		strncpy(msgBuf->buf,&msg,navMAX_LEN);

		if (xQueueSend(ptr->outQ,(void *) (msgBuf),portMAX_DELAY) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
		count = (count+1)%3;
		free(msgBuf);
		free(buf);
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
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		//if (vtI2CEnQ(devPtr,voidMsg,0x4F,2,0xAA,0) != pdTRUE) {
		//	VT_HANDLE_FATAL_ERROR(0);
		//}
	}
}
#define conductorWRITE_RATE_BASE	( ( portTickType ) 100 / portTICK_RATE_MS)

void startTimerForConductor(vtConductorStruct *vtConData) {
	if (sizeof(long) != sizeof(vtConductorStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle ConductorTimerHandle = xTimerCreate((const signed char *)"Conductor Timer",conductorWRITE_RATE_BASE,pdTRUE,(void *) vtConData,ConductorTimerCallback);
	if (ConductorTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(ConductorTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}