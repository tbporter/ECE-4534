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

#define TRACK_MEM_SIZE 300
#define TRACK_MEM_DIST 5 //how often we take readings in cm

typedef struct __navStruct {
	g9ZigBeeStruct* zigBeePtr; //Needed for sending messages out
	xQueueHandle inQ; //Queue for incoming messages
} navStruct;

typedef struct {
	uint16_t heading;
	uint8_t left;
	uint8_t right;

}trackMem_t;

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
Bool chkDist(float left, float right, float front_left, float front_right, float side_left, float side_right);


//Adjust motor speed to account for RFID tag events
void handleSpecialEvents(short* enc);

//Tells Navigation to stop acting on a tag
inline void disableTag(uint8_t tag);

//Tells Navigation to start acting on a tag
inline void enableTag(uint8_t tag);

//Trys to adjust speed by delta while maintaining heading
//returns delta of adjustment
uint8_t adjuctSpeed(char delta);

trackMem_t *trackMem[TRACK_MEM_SIZE];
uint8_t curMemLoc;
#endif