
#include "g9_UART.h"

/* ************************************************ */
// Private definitions used in the Public API
// Structure used to define the messages that are sent to/from the UART thread 
//typedef struct __g9UARTMsg {
//	uint8_t msgType; // A field you will likely use in your communications between processors (and for debugging)
//	uint8_t	rxLen;	 // Length of the message you *expect* to receive (or, on the way back, the length that *was* received)
//	uint8_t txLen;   // Length of the message you want to sent (or, on the way back, the length that *was* sent)
//	uint8_t status;  // status of the completed operation -- I've not done anything much here, you probably should...
//	uint8_t buf[g9UARTMLen]; // On the way in, message to be sent, on the way out, message received (if any)
//} g9UARTMsg;
// Length of the message queues to/from this task
#define g9UARTQLen 10 /*FIFOs*/

#define g9UARTTransferFailed -2
#define g9UARTIntPriority 7

// Here is where we define an array of pointers that lets communication occur between the interrupt handler and the rest of the code in this file
static 	g9UARTStruct *devStaticPtr[3];

// Variables to control read and write or UART
static Bool bReadReady[3] = {0,0,0};
static Bool bWriteReady[3] = {0,0,0};

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the UART operations -- it is possible/very likely these are much larger than needed (see LCDtask.c for how to check the stack size)
#define baseStack 3
#if PRINTF_VERSION == 1
#define uartSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define uartSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

/* The UART monitor tasks. */
static portTASK_FUNCTION_PROTO( vUARTMonitorTask, pvParameters );
// End of private definitions
/* ************************************************ */

/* ************************************************ */
// Public API Functions
//
// Note: This will startup an UART thread, once for each call to this routine
int g9UartInit(g9UARTStruct *devPtr,uint8_t uartDevNum,unsigned portBASE_TYPE taskPriority,UART_CFG_Type* uartCfg, UART_FIFO_CFG_Type* fifoCfg)
{
	PINSEL_CFG_Type PinCfg;

	devPtr->devNum = uartDevNum;
	devPtr->taskPriority = taskPriority;

	int retval = g9UartInitSuccess;
	switch (devPtr->devNum) {
		case 0: {
			devStaticPtr[0] = devPtr; // Setup the permanent variable for use by the interrupt handler
			devPtr->devAddr = LPC_UART0;
			// Start with the interrupts disabled *and* make sure we have the priority correct
			NVIC_SetPriority(UART0_IRQn,g9UARTIntPriority);	
			NVIC_DisableIRQ(UART0_IRQn);
			// Init UART pin connect
			PinCfg.OpenDrain = 0;
			PinCfg.Pinmode = 0;
			PinCfg.Funcnum = 1;
			PinCfg.Pinnum = 2;
			PinCfg.Portnum = 0;
			PINSEL_ConfigPin(&PinCfg);
			PinCfg.Pinnum = 3;
			PINSEL_ConfigPin(&PinCfg);
			break;
		}
		case 1: {
			devStaticPtr[1] = devPtr; // Setup the permanent variable for use by the interrupt handler
			devPtr->devAddr = LPC_UART1;
			// Start with the interrupts disabled *and* make sure we have the priority correct
			NVIC_SetPriority(UART1_IRQn,g9UARTIntPriority);	
			NVIC_DisableIRQ(UART1_IRQn);
			// Init UART pin connect
			PinCfg.OpenDrain = 0;
			PinCfg.Pinmode = 0;
			PinCfg.Funcnum = 1;
			PinCfg.Pinnum = 15;
			PinCfg.Portnum = 0;
			PINSEL_ConfigPin(&PinCfg);
			PinCfg.Pinnum = 16;
			PINSEL_ConfigPin(&PinCfg);
			break;
		}
		//TODO: define UART2/3
		default: {
			return(g9UartErrInit);
			break;
		}
	}

	// Create semaphore to communicate with interrupt handler
	vSemaphoreCreateBinary(devPtr->binSemaphore);
	if (devPtr->binSemaphore == NULL) {
		return(g9UartErrInit);
	}
	// Need to do an initial "take" on the semaphore to ensure that it is initially blocked
	if (xSemaphoreTake(devPtr->binSemaphore,0) != pdTRUE) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		return(g9UartErrInit);
	}

	// Allocate the two queues to be used to communicate with other tasks
	if ((devPtr->inQ = xQueueCreate(g9UARTQLen*g9UARTMLen,1)) == NULL) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		return(g9UartErrInit);
	}
	if ((devPtr->outQ = xQueueCreate(g9UARTQLen*g9UARTMLen,1)) == NULL) {
		// free up everyone and go home
		vQueueDelete(devPtr->binSemaphore);
		vQueueDelete(devPtr->inQ);
		return(g9UartErrInit);
	}


	// Initialize  UART peripheral
	UART_Init(devPtr->devAddr, uartCfg);

	//Setup FIFO
	UART_FIFOConfig(devPtr->devAddr, fifoCfg);

	// Enable interrupts
	UART_IntConfig(devPtr->devAddr, UART_INTCFG_THRE, ENABLE);
	UART_IntConfig(devPtr->devAddr, UART_INTCFG_RBR, ENABLE);
	NVIC_EnableIRQ(UART1_IRQn);
	
	// Enable  UART operation
	UART_TxCmd(devPtr->devAddr, ENABLE);

	/* Start the task */
	char taskLabel[8];
	sprintf(taskLabel,"UART%d",devPtr->devNum);
	if ((retval = xTaskCreate( vUARTMonitorTask, (signed char*) taskLabel, uartSTACK_SIZE,(void *) devPtr, devPtr->taskPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
		return(g9UartErrInit); // return is just to keep the compiler happy, we will never get here
	} else {
		return g9UartInitSuccess;
	}
}

// A simple routine to use for filling out and sending a message to the UART thread
//   You may want to make your own versions of these as they are not suited to all purposes
//portBASE_TYPE g9UARTEnQ(g9UARTStruct *dev,uint8_t msgType,uint8_t txLen,const uint8_t *txBuf,uint8_t rxLen)
//{
//	g9UARTMsg msgBuf;
//	int i;
//
//	msgBuf.msgType = msgType;
//	msgBuf.rxLen = rxLen;
//	if (msgBuf.rxLen > g9UARTMLen) {
//		VT_HANDLE_FATAL_ERROR(0);
//	}
//	msgBuf.txLen = txLen;
//	if (msgBuf.txLen > g9UARTMLen) {
//		VT_HANDLE_FATAL_ERROR(0);
//	}
//	for (i=0;i<msgBuf.txLen;i++) {
//		msgBuf.buf[i] = txBuf[i];
//	}
//	return(xQueueSend(dev->inQ,(void *) (&msgBuf),portMAX_DELAY));
//}
//
//// A simple routine to use for retrieving a message from the UART thread
//portBASE_TYPE g9UARTDeQ(g9UARTStruct *dev,uint8_t maxRxLen,uint8_t *rxBuf,uint8_t *rxLen,uint8_t *msgType,uint8_t *status)
//{
//	g9UARTMsg msgBuf;
//	int i;
//
//	if (xQueueReceive(dev->outQ,(void *) (&msgBuf),portMAX_DELAY) != pdTRUE) {
//		return(pdFALSE);
//	}
//	(*status) = msgBuf.status;
//	(*rxLen) = msgBuf.rxLen;
//	if (msgBuf.rxLen > maxRxLen) msgBuf.rxLen = maxRxLen;
//	for (i=0;i<msgBuf.rxLen;i++) {
//		rxBuf[i] = msgBuf.buf[i];
//	}
//	(*msgType) = msgBuf.msgType;
//	return(pdTRUE);
//}

// End of public API Functions
/* ************************************************ */

// uart interrupt handler
static __INLINE void g9UARTIsr(g9UARTStruct* devPtr) {
	static uint8_t data[UART_TX_FIFO_SIZE];
	vtLEDOff(0xC0);
	uint32_t IIR = UART_GetIntId(devPtr->devAddr);
	if(IIR & UART_IIR_INTID_RDA){
		vtLEDOn(0x80);
		//Grab bytes put in queue
		uint32_t bRecv = UART_Receive(devPtr->devAddr,data,UART_TX_FIFO_SIZE,NONE_BLOCKING);
		int i=0;
		for(i=0;i<bRecv;i++){
			if( xQueueSendToBackFromISR(devPtr->outQ,&data[i],0) != pdTRUE ){
				//OOPS.....
				VT_HANDLE_FATAL_ERROR(0xC0DE1);
			}
		}
	}

	if(IIR & UART_IIR_INTID_THRE){
		vtLEDOn(0x40);
		//Load up the FIFO
		unsigned portBASE_TYPE len = uxQueueMessagesWaitingFromISR(devPtr->inQ);
		if( len > UART_TX_FIFO_SIZE) len = UART_TX_FIFO_SIZE;
		int i=0;
		for(i=0;i<len;i++){
			if( xQueueReceiveFromISR(devPtr->inQ,&data[i],0) != pdTRUE ){
				//OOPS.....
				VT_HANDLE_FATAL_ERROR(0xC0DE2);
			}
		}
		uint32_t bSent = UART_Send(devPtr->devAddr,data,len,NONE_BLOCKING);
	}
//	UART_MasterHandler(devAddr);
//	if (UART_MasterTransferComplete(devAddr)) {
//		static signed portBASE_TYPE xHigherPriorityTaskWoken;
//		xHigherPriorityTaskWoken = pdFALSE;
//		xSemaphoreGiveFromISR(*binSemaphore,&xHigherPriorityTaskWoken);
//		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
//	}
}
// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple uart peripheral units on the LPC1768
void g9UART0Isr(void) {
	g9UARTIsr(devStaticPtr[0]);
}

// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple uart peripheral units on the LPC1768
void g9UART1Isr(void) {
	g9UARTIsr(devStaticPtr[1]);
	//NVIC_ClearPendingIRQ(UART1_IRQn);   // Clear Interrupt
}

// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple uart peripheral units on the LPC1768
void g9UART2Isr(void) {
	g9UARTIsr(devStaticPtr[2]);
}

// Simply pass on the information to the real interrupt handler above (have to do this to work for multiple uart peripheral units on the LPC1768
void g9UART3Isr(void) {
	g9UARTIsr(devStaticPtr[3]);
}

// This is the actual task that is run
static portTASK_FUNCTION( vUARTMonitorTask, pvParameters )
{
	// Get the uart structure for this task/device
	g9UARTStruct *devPtr = (g9UARTStruct *) pvParameters;
	//g9UARTMsg msgBuffer;
	//uint8_t tmpRxBuf[g9UARTMLen];
	//int i;

	for (;;) {
		// wait for a message from another task telling us to send/recv over uart
//		if (xQueueReceive(devPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
//			VT_HANDLE_FATAL_ERROR(0);
//		}
//		//Log that we are processing a message
//		vtITMu8(vtITMPortUARTMsg,msgBuffer.msgType);
//		UART_SendByte(devPtr->devAddr,'1');
//		UART_SendByte(devPtr->devAddr,'2');
//		UART_SendByte(devPtr->devAddr,'3');
//		UART_SendByte(devPtr->devAddr,'4');
//		UART_SendByte(devPtr->devAddr,'5');
//		UART_SendByte(devPtr->devAddr,'6');
//		UART_SendByte(devPtr->devAddr,'7');
//		UART_SendByte(devPtr->devAddr,'8');

		uint8_t buf[8] = {'1','2','3','4','5','6','7','8'};
		if(UART_CheckBusy(devPtr->devAddr)==RESET){
			UART_Send(devPtr->devAddr, buf, 8, NONE_BLOCKING);
		}

		// process the messsage and perform the UART transaction
//		transferMCfg.tx_data = msgBuffer.buf;
//		transferMCfg.tx_length = msgBuffer.txLen;
//		transferMCfg.rx_data = tmpRxBuf;
//		transferMCfg.rx_length = msgBuffer.rxLen;
//		transferMCfg.retransmissions_max = 3;
//		transferMCfg.retransmissions_count = 0;	 // this *should* be initialized in the LPC code, but is not for interrupt mode
//		msgBuffer.status = UART_MasterTransferData(devPtr->devAddr, &transferMCfg, UART_TRANSFER_INTERRUPT);

		

//		// Block until the UART operation is complete -- we *cannot* overlap operations on the UART bus...
//		if (xSemaphoreTake(devPtr->binSemaphore,portMAX_DELAY) != pdTRUE) {
//			// something went wrong 
//			VT_HANDLE_FATAL_ERROR(0);
//		}
//		msgBuffer.txLen = transferMCfg.tx_count;
//		msgBuffer.rxLen = transferMCfg.rx_count;
//		// Now send out a message with the data that was read
//		// First, copy over the buffer that was received (if any)
//		for (i=0;i<msgBuffer.rxLen;i++) {
//			msgBuffer.buf[i] = tmpRxBuf[i];
//		}
//		// now put a message in the message queue
//		if (xQueueSend(devPtr->outQ,(void*)(&msgBuffer),portMAX_DELAY) != pdTRUE) {
//			// something went wrong 
//			VT_HANDLE_FATAL_ERROR(0);
//		} 
	}
}