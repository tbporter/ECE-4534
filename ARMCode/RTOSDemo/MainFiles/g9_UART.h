#ifndef __G9_UART_H__
#define __G9_UART_H__

#include <stdlib.h>
#include <stdio.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "lpc17xx_uart.h"
#include "vtUtilities.h"

#include "lpc17xx_libcfg_default.h"
#include "lpc17xx_pinsel.h"

// return codes for g9UartInit()
#define g9UartErrInit -1
#define g9UartInitSuccess 0

// The maximum length of a message to be sent/received over UART 
#define g9UARTMLen 64

// Structure that is used to define the operate of an UART peripheral using the g9Uart routines
//   It should be initialized by UartInit() and then not changed by anything... ever
//   A user of the API should never change or access it, it should only pass it as a parameter
typedef struct __g9UARTStruct {
	uint8_t devNum;	  						// Number of the UART peripheral (0,1,2 on the 1768)
	LPC_UART_TypeDef *devAddr;	 			// Memory address of the UART peripheral
	unsigned portBASE_TYPE taskPriority;   	// Priority of the UART task
	xSemaphoreHandle binSemaphore;		   	// Semaphore used between UART task and UART interrupt handler
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the UART task
	xQueueHandle outQ;						// Queue used by the UART task to send out results
} g9UARTStruct;

/* ********************************************************************* */
// The following are the public API calls that other tasks should use to work with the UART task

// Args:
//   dev: pointer to the g9UARTStruct data structure
//   uartDevNum: The number of the uart device -- 0, 1, or 2
//   taskPriority: At what priority should this task be run?
//   uartCfg: Contains the setting for baudrate, parity, stop bits, and data bits
// Return:
//   if successful, returns UartInitSuccess
//   if not, should return UartErrInit
// Must be called for each UART device initialized (0, 1, or 2) and used
int UartInit(g9UARTStruct *devPtr,uint8_t uartDevNum,unsigned portBASE_TYPE taskPriority,UART_CFG_Type* uartCfg);

// A simple routine to use for filling out and sending a message to the UART thread
//   You may want to make your own versions of these as they are not suited to all purposes
// Args
//   dev: pointer to the g9UARTStruct data structure
//   msgType: The message type value -- does not get sent on the wire, but is included in the response in the message queue
//   slvAddr: The address of the uart slave device you are addressing
//   txLen: The number of bytes you want to send
//   txBuf: The buffer holding the bytes you want to send
//   rxLen: The number of bytes that you would like to receive
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE UartEnQ(g9UARTStruct *dev,uint8_t msgType,uint8_t slvAddr,uint8_t txLen,const uint8_t *txBuf,uint8_t rxLen);

// A simple routine to use for retrieving a message from the UART thread
// Args
//   dev: pointer to the g9UARTStruct data structure
//   maxRxLen: The maximum number of bytes that your receive buffer can hold
//   rxBuf: The buffer that you are providing into which the message will be copied
//   rxLen: The number of bytes that were actually received
//   msgType: The message type value -- does not get sent/received on the wire, but is included in the response in the message queue
//   status: Return code of the operation (you will need to dive into the code to understand the status values)
// Return:
//   Result of the call to xQueueReceive()
portBASE_TYPE UartDeQ(g9UARTStruct *dev,uint8_t maxRxLen,uint8_t *rxBuf,uint8_t *rxLen,uint8_t *msgType,uint8_t *status);

#endif