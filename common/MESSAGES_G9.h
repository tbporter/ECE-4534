#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#ifndef IS_ARM
#include <GenericTypeDefs.h>
#define uint8_t UINT8
#define uint16_t UINT16
#else
#include <stdint.h>// If this doesn't work with pic just define uint8_t as unsigned char
#endif

#define ENC_POLL_RATE	100 //ms

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

	//Navigation Task
	navMotorCmdMsg,
	navEncoderMsg,
	
	//IR/Line
	navLineFoundMsg,
	navIRDataMsg,

	//lap message from web
	navWebLapMsg,
	navWebStartMsg,
		
	//RFID
	navRFIDFoundMsg,
	
	//Webpage
	webDebugMsg,
	webPowerMsg,
	webNavMsg,
	
	//conductor request message
	conRequestMsg,
	//void, just cause
	voidMsg
} g9MsgType;


typedef enum {
    None	= 0x0,
    SpeedUp	= 0x1,
    SlowDown= 0x2,
    GoLeft	= 0x4,
    GoRight	= 0x8,
    Finish	= 0x10,
	Error	= 0x20
}RFID;



#define MAX_MSG_LEN 10

#ifdef IS_ARM

typedef struct __attribute__((__packed__)) __g9Msg {
#else

typedef struct __g9Msg {
#endif
    uint8_t msgType; // Use Enum defined above to set. Declared as uint8_t to force casting, thus reducing size.
    uint8_t length;
    uint8_t id; // Used to check for dropped messages **Won't be used until wireless**
    uint8_t buf[MAX_MSG_LEN]; // On the way in, message to be sent, on the way out, message received (if any)
} g9Msg;



// Message Lengths shall be determined by message type.
// Define max expected message lengths hur.
// NOTE: For varible length message, first byte in buf should be length
<<<<<<< HEAD
#define VAR_MSG_LEN -1
//vtI2C
#define vtI2CMsgTypeTempInitLen		2
#define vtI2CMsgTypeTempRead1Len 	1
#define vtI2CMsgTypeTempRead2Len 	1
#define vtI2CMsgTypeTempRead3Len 	1
#define TempMsgTypeTimerLen				0

//OScope
#define oScopeRead1Len					1
#define oScopeRead2Len					1
#define oScopeTimerLen					0

// LCD_Task
#define LCDMsgTypeTimerLen				1
#define LCDMsgTypePrintLen				VAR_LEN

//LCD OScope Task
#define lcdOScopeDataLen				2
#define lcdOScopeTimerLen				0

//Navigation Task
#define navMotorCmdLen					2
#define navLineFoundLen					0
#define navIRDataLen					3							
#define navRFIDFoundLen					1

//Webpage
#define webDebugLen						VAR_LEN

//Max lengths per Task, used for sizing the Queues
#define baseMAX_LEN	sizeof(g9Msg)
#define navMAX_LEN	( baseMAX_LEN + (navIRDataLen - 1)*sizeof(uint8_t))
=======
=======

>>>>>>> 4abaed625a687dbd2d2edfdf17f4e6821e9d08de
//	#define VAR_MSG_LEN -1
//	//vtI2C
//	#define vtI2CMsgTypeTempInitLen		2
//	#define vtI2CMsgTypeTempRead1Len 	1
//	#define vtI2CMsgTypeTempRead2Len 	1
//	#define vtI2CMsgTypeTempRead3Len 	1
//	#define TempMsgTypeTimerLen				0
//
//	//OScope
//	#define oScopeRead1Len					1
//	#define oScopeRead2Len					1
//	#define oScopeTimerLen					0
//	
//	// LCD_Task
//	#define LCDMsgTypeTimerLen				1
//	#define LCDMsgTypePrintLen				VAR_LEN
//
//	//LCD OScope Task
//	#define lcdOScopeDataLen				2
//	#define lcdOScopeTimerLen				0
//	
//	//Navigation Task
//	#define navMotorCmdLen					2
//	#define navLineFoundLen					0
//	#define navIRDataLen					3							
//	#define navRFIDFoundLen					1
//	
//	//Webpage
//	#define webDebugLen						VAR_LEN
//	
//	//Max lengths per Task, used for sizing the Queues
//	#define baseMAX_LEN	sizeof(g9Msg)
//	#define navMAX_LEN	( baseMAX_LEN + (navIRDataLen - 1)*sizeof(uint8_t))

#define G9_LEN_NO_BUFF 3;


//Message ERROR CODES
#define INVALID_G9MSG_TYPE 0xDEADC0DE


#endif
