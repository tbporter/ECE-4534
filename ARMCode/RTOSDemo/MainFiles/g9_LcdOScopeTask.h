#ifndef LCD_OSCOPE_TASK_H
#define LCD_OSCOPE_TASK_H
#include "queue.h"
#include "timers.h"

// NOTE: This is a reasonable API definition file because there is nothing in it that the
//   user of the API does not need (e.g., no private definitions) and it defines the *only*
//   way a user of the API is allowed to interact with the task


// Define a data structure that is used to pass and hold parameters for this task
// Functions that use the API should not directly access this structure, but rather simply
//   pass the structure as an argument to the API calls
typedef struct __vtOScopeStruct {
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the LCD task to print
} vtOScopeStruct;

// Structure used to define the messages that are sent to the LCD thread
//   the maximum length of a message to be printed is the size of the "buf" field below
#define vtOScopeMaxLen 2

/* ********************************************************************* */
// The following are the public API calls that other tasks should use to work with the LCD task
//   Note: This is *not* the API for actually manipulating the graphics -- that API is defined in GLCD.h
//         and is accessed by the LCD task (other tasks should not access it or conflicts may occur).
//
// Start the task
// Args:
//   lcdData -- a pointer to a variable of type vtLCDStruct
//   uxPriority -- the priority you want this task to be run at
void StartOScopeTask(vtOScopeStruct *lcdData,unsigned portBASE_TYPE uxPriority);
// Loads a msg onto the queue
// Args:
//	 lcdData -- a pointer to a variable of type vtLCDStruct
//   msgData -- The payload of the message
//   tickToBlock -- how long the routine should wait if the queue is full
portBASE_TYPE SendLCDOScopeMsg(vtOScopeStruct *lcdData,uint16_t msgData,portTickType ticksToBlock);
// Draw the axes of the OScope
// Args:
//	 lcdData -- a pointer to a variable of type vtLCDStruct
//	 yMax -- Maximum value on the y-axis, in milliVolts
//	 xMax -- Maximum value of the x-axis, in milliSeconds
//   tickToBlock -- how long the routine should wait if the queue is full
void DrawLCDAxes();
/* ********************************************************************* */


void LCDOScopeTimerCallback(xTimerHandle);

#endif