#ifndef I2CTASK_MSG_TYPES_H
#define I2CTASK_MSG_TYPES_H

// Here is where I define the types of the messages that I am passing to the I2C task
//   --Note that none of these message types (as I have implemented this) actually go over the I2C bus, but they
//     are useful for matching up what is send to/from the I2C task message queues
//
// I have defined them all here so that they are unique

enum
   {
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
	
	//void, just cause
	voidMsg
   };
#endif