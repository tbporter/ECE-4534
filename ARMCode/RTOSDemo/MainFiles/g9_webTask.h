#ifndef __WEB_TASK_H__
#define __WEB_TASK_H__

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

#include "vtUtilities.h"
#include "vtI2C.h"
#include "messages.h"


#define DEBUG_LINES 20
#define DEBUG_LENGTH 50

typedef struct __webStruct {
	vtI2CStruct *i2c; //Needed for sending messages out
	xQueueHandle inQ; //Queue for incoming messages
} webStruct;

// Public API
//
// Start the task
// Args:
//   webData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   i2c: pointer to the data structure for an i2c task
void startWebTask(webStruct* webData,unsigned portBASE_TYPE uxPriority);

//Sends a message to the web task
//	msg -- a pointer to a variable of type g9Msg
//	ticksToBlock -- how long the routine should wait if the queue is full
//	Return: Result of the call to xQueueSend()
portBASE_TYPE SendWebMsg(webStruct* web,g9Msg* msg,portTickType ticksToBlock);

void printw(char* msg);
void processWebDebugMsg(char* msg);

char (*getWebDebug())[DEBUG_LENGTH];
void getWebStatusText(char* buf);
#endif