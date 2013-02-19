#ifndef __NAV_TASK_H__
#define __NAV_TASK_H__

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "vtI2C.h"
#include "messages.h"

typedef struct __navStruct {
	vtI2CStruct *i2c; //Needed for sending messages out
	xQueueHandle inQ; //Queue for incoming messages
} navStruct;

// Public API
//
// Start the task
// Args:
//   navData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   i2c: pointer to the data structure for an i2c task
void vStartNavigationTask(navStruct* navData,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c);

//Sends a message to the Navigation task
//	msg -- a pointer to a variable of type g9Msg
//	ticksToBlock -- how long the routine should wait if the queue is full
//	Return: Result of the call to xQueueSend()
portBASE_TYPE SendNavigationMsg(navStruct* nav,uint8_t* msg,portTickType ticksToBlock);
#endif