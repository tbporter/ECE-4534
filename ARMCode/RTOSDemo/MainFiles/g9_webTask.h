#ifndef __WEB_TASK_H__
#define __WEB_TASK_H__

#include <stdarg.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

#include "vtUtilities.h"
#include "vtI2C.h"
#include "g9_NavTask.h"
#include "messages_g9.h"


#define DEBUG_LINES 20
#define DEBUG_LENGTH 75

typedef struct __webStruct {
	vtI2CStruct *i2c; //Needed for sending messages out
	xQueueHandle inQ; //Queue for incoming messages
	navStruct* navData;
} webStruct;

typedef union{
	struct __attribute__((__packed__)){ //Used for data compression
		unsigned start 		: 1;
		unsigned loop 		: 1;
		unsigned m4Demo		: 1;
		unsigned printNav 	: 1;
		unsigned printZigBee: 1;
	};
	uint8_t data;
} webInput_t;

// Public API
//
// Start the task
// Args:
//   webData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   i2c: pointer to the data structure for an i2c task
void startWebTask(webStruct* webData,unsigned portBASE_TYPE uxPriority, navStruct* navData);

//Sends a message to the web task
//	msg -- a pointer to a variable of type g9Msg
//	ticksToBlock -- how long the routine should wait if the queue is full
//	Return: Result of the call to xQueueSend()
portBASE_TYPE SendWebMsg(webStruct* web,g9Msg* msg,portTickType ticksToBlock);

void printw(const char* fmt, ...);
void printw_err(char* fmt, ...);
void processWebDebugMsg(char* msg);

char (*getWebDebug(int* index))[DEBUG_LENGTH];
void getWebStatusText(char* out, const char* in);
void getWebInputText(char* out, const char* in);

inline char getWebStart();
void setWebInputs(webInput_t* in);
void getWebMotors( uint8_t* left, uint8_t* right);

#endif