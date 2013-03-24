#ifndef _G9_ZIGBEE_H__
#define _G9_ZIGBEE_H__

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"


#include "g9_UART.h"
#include "vtUtilities.h"
#include "messages_g9.h"
#include "conductor.h"

// return codes for startG9ZigBeeTask()
#define g9Err -1
#define g9Success 0

// Structure that is used to hold all queues
//   It should be initialized by startg9ZigBeeTask() and then not changed by anything... ever
//   A user of the API should never change or access it, it should only pass it as a parameter
typedef struct __g9ZigBeeStruct {
	g9UARTStruct* uartDev;	 				// Memory address of the UART task information
	vtConductorStruct* conPtr;				// Used to send messages to the conductor
	unsigned portBASE_TYPE taskPriority;   	// Priority of the I2C task
	xQueueHandle inQ;						// Queue used by the zigbee task to receive messages to TX
} g9ZigBeeStruct;

int startG9ZigBeeTask(g9ZigBeeStruct* zigBeePtr, g9UARTStruct* uartDev, vtConductorStruct* conPtr, unsigned portBASE_TYPE taskPriority);

portBASE_TYPE SendZigBeeMsg(g9ZigBeeStruct* zigBeePtr,g9Msg* msg,portTickType ticksToBlock);

#endif