#ifndef _MY_TIMERS_H
#define _MY_TIMERS_H
#include "g9_LCDOScopeTask.h"
#include "lcdTask.h"
#include "vtI2C.h"
#include "i2cTemp.h"

typedef struct{
	uint8_t msgType; // A field you will likely use in your communications between processors (and for debugging)
	uint8_t slvAddr; // Address of the device to whom the message is being sent (or was sent)
	uint8_t	rxLen;	 // Length of the message you *expect* to receive (or, on the way back, the length that *was* received)
	uint8_t txLen;   // Length of the message you want to sent (or, on the way back, the length that *was* sent)
	uint8_t status;  // status of the completed operation -- I've not done anything much here, you probably should...
	uint8_t buf[vtI2CMLen]; // On the way in, message to be sent, on the way out, message received (if any)
}fakeI2CMsg;

void startTimerForLCD(vtLCDStruct *vtLCDdata);
void startTimerForTemperature(vtTempStruct *vtTempdata);
void startTimerForLCDOScope(lcdOScopeStruct *lcdOScopeData);
void startTimerForFakeI2CMsg(vtI2CStruct* i2c);
#endif