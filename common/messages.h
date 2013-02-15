#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "FreeRTOS.h"


typedef enum {
	//vtI2C
	vtI2CMsgTypeTempInit, 
	vtI2CMsgTypeTempRead1, 
	vtI2CMsgTypeTempRead2, 
	vtI2CMsgTypeTempRead3, 
	TempMsgTypeTimer,

	//OScope
	oScopeRead1Msg,
	oScopeRead2Msg,
	oScopeTimerMsg,
	
	// LCD_Task
	LCDMsgTypeTimer,
	LCDMsgTypePrint,

	//LCD OScope Task
	lcdOScopeData,
	lcdOScopeTimer,

	//void, just cause
	voidMsg
} g9MsgType;

typedef struct __g9Msg {
	uint8_t msgType;// Use Enum defined above to set. Declared as uint8_t to force casting, thus reducing size.
	uint8_t id;			// Used to check for dropped messages **Won't be used until wireless**
	uint8_t* buf;		// On the way in, message to be sent, on the way out, message received (if any)
} g9Msg;



// Message Lengths shall be determined by message type.
// Define max expected message lengths hur.
// NOTE: For varible length message, first byte in buf should be length
	//vtI2C
	#define vtI2CMsgTypeTempInitLen		2
	#define vtI2CMsgTypeTempRead1Len 	1
	#define vtI2CMsgTypeTempRead2Len 	1
	#define vtI2CMsgTypeTempRead3Len 	1
	#define TempMsgTypeTimerLen				0

	//OScope
	#define oScopeRead1MsgLen					1
	#define oScopeRead2MsgLen					1
	#define oScopeTimerMsgLen					0
	
	// LCD_Task
	#define LCDMsgTypeTimerLen				1
	#define LCDMsgTypePrintLen				0

	//LCD OScope Task
	#define lcdOScopeDataLen					2
	#define lcdOScopeTimerLen					0

#endif