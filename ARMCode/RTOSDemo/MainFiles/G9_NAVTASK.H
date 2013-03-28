#ifndef __NAV_TASK_H__
#define __NAV_TASK_H__

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "messages_g9.h"
#include "g9_zigbee.h"

typedef struct __navStruct {
	g9ZigBeeStruct* zigBeePtr; //Needed for sending messages out
	xQueueHandle inQ; //Queue for incoming messages
} navStruct;

// Public API
//
// Start the task
// Args:
//   navData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   zigBeePtr: pointer to the zigBee task
void vStartNavigationTask(navStruct* navData,unsigned portBASE_TYPE uxPriority, g9ZigBeeStruct* zigBeePtr);

//Sends a message to the Navigation task
//	msg -- a pointer to a variable of type g9Msg
//	ticksToBlock -- how long the routine should wait if the queue is full
//	Return: Result of the call to xQueueSend()
portBASE_TYPE SendNavigationMsg(navStruct* nav,g9Msg* msg,portTickType ticksToBlock);


//State machine,
//Decides on commands to send the motor based off of IR

void stateMachine();



#endif