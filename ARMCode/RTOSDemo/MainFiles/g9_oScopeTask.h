#ifndef G9_OSCOPETASK_H
#define G9_OSCOPETASK_H
#include "vtI2C.h"
#include "g9_LcdOscopeTask.h"

typedef struct __oScopeStruct {
	vtI2CStruct *dev;
	lcdOScopeStruct *lcdData;
	xQueueHandle inQ;
} oScopeStruct;

#define oScopeMaxLen   (sizeof(portTickType))

// Public API
//
// Start the task
// Args:
//   tempData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   i2c: pointer to the data structure for an i2c task
//   lcd: pointer to the data structure for an LCD task (may be NULL)
void startOScopeTask(oScopeStruct *oScopeData,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,lcdOScopeStruct *lcd);
//
// Send a timer message to the OScope task
// Args:
//   tempData -- a pointer to a variable of type vtLCDStruct
//   ticksElapsed -- number of ticks since the last message (this will be sent in the message)
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendOScopeTimerMsg(oScopeStruct *tempData,portTickType ticksElapsed,portTickType ticksToBlock);
//
// Send a value message to the OScope task
// Args:
//   tempData -- a pointer to a variable of type vtLCDStruct
//   msgType -- the type of the message to send
//   value -- The value to send
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendOScopeValueMsg(oScopeStruct *tempData,uint8_t msgType,uint8_t value,portTickType ticksToBlock);

#endif