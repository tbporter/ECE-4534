#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* include files. */
#include "GLCD.h"
#include "vtUtilities.h"
#include "g9_LCDOScopeTask.h"
#include "string.h"

// I have set this to a larger stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the LCD operations
// I actually monitor the stack size in the code to check to make sure I'm not too close to overflowing the stack
//   This monitoring takes place if INPSECT_STACK is defined (search this file for INSPECT_STACK to see the code for this) 
#define INSPECT_STACK 1
#define baseStack 3
#if PRINTF_VERSION == 1
#define lcdSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define lcdSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

// definitions and data structures that are private to this file
// Length of the queue to this task
#define vtLCDQLen 10
// OScope Axis Scales
#define OScope_YMAX 3000 //mV
#define OScope_XMAX 34000 //uS
#define OScope_DX 0
#define OScope_DY 5
#define OScope_BORDER 20
//OScope Buffer Size
#define OSCOPE_BUFF_SIZE (((192*OScope_XMAX)/10000)+1)
// Colors
#define FG_Color Orange
#define BG_Color Maroon
// a timer message -- not to be printed
#define LCDMsgTypeTimer 1
// a message to be printed
#define LCDMsgTypePrint 2
// a wave value to be displayed
#define LCDMsgTypeWave 3
// actual data structure that is sent in a message
typedef struct __vtLCDMsg {
	uint16_t msgType;
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[vtOScopeMaxLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} vtLCDMsg;
// end of defs

/* definition for the LCD task. */
static portTASK_FUNCTION_PROTO( vLCDUpdateTask, pvParameters );

/*-----------------------------------------------------------*/

void startLcdOScopeTask(lcdOScopeStruct *ptr, unsigned portBASE_TYPE uxPriority)
{
	if (ptr == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}

	// Create the queue that will be used to talk to this task
	if ((ptr->inQ = xQueueCreate(vtLCDQLen,sizeof(vtLCDMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( vLCDUpdateTask, ( signed char * ) "LCD", lcdSTACK_SIZE, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

portBASE_TYPE SendLCDOScopeMsg(lcdOScopeStruct *lcdData,uint16_t msgData,portTickType ticksToBlock)
{
	if (lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	vtLCDMsg lcdBuffer;

	lcdBuffer.length = sizeof(msgData);
	lcdBuffer.msgType = LCDMsgTypeWave;
	strncpy((char *)lcdBuffer.buf,(char*)&msgData,vtOScopeMaxLen);
	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
}

void DrawLCDAxes(){
	int x=OScope_BORDER;
	int y=OScope_BORDER;
	
	for( y=OScope_BORDER; y<(220-OScope_BORDER-OScope_DY); y++ ){
		GLCD_PutPixel(x,y);
		if( (y-OScope_BORDER)%(((220-2*OScope_BORDER-OScope_DY)*1000)/OScope_YMAX) == 0 ){
			GLCD_PutPixel(x-1,y);
			GLCD_PutPixel(x-2,y);
		}
	}
	
	for( x=OScope_BORDER; x<(320-OScope_BORDER-OScope_DX); x++){
		GLCD_PutPixel(x,y);
		if( (x-OScope_BORDER)%(((320-2*OScope_BORDER-OScope_DX)*1000)/OScope_XMAX) == 0 ){
			GLCD_PutPixel(x,y+1);
			GLCD_PutPixel(x,y+2);
		}
	}

	GLCD_DisplayChar(2,0,1,'V');
	GLCD_DisplayChar(3,0,1,'o');
	GLCD_DisplayChar(4,0,1,'l');
	GLCD_DisplayChar(5,0,1,'t');
	GLCD_DisplayChar(6,0,1,'s');

	GLCD_DisplayChar(9,9,1,'m');
	GLCD_DisplayChar(9,10,1,'s');
	
		
}

// Private routines used to unpack the message buffers
//   I do not want to access the message buffer data structures outside of these routines
int unpackWaveMsg(vtLCDMsg* lcdBuffer)
{
	int* ptr =(int*) lcdBuffer->buf;
	return (*ptr);
}

int getMsgType(vtLCDMsg *lcdBuffer)
{
	return(lcdBuffer->msgType);
} 

int getMsgLength(vtLCDMsg *lcdBuffer)
{
	return(lcdBuffer->msgType);
}

void copyMsgString(char *target,vtLCDMsg *lcdBuffer,int targetMaxLen)
{
	strncpy(target,(char *)(lcdBuffer->buf),targetMaxLen);
}

// End of private routines for message buffers

static unsigned short hsl2rgb(float H,float S,float L);
//Variables for OScope data stored in circular buffer
static int nOScopeBuf[OSCOPE_BUFF_SIZE];
static int nBufStart;
static int nBufEnd;

// This is the actual task that is run
static portTASK_FUNCTION( vLCDUpdateTask, pvParameters )
{
	unsigned short screenColor = 0;
	unsigned short tscr;

	vtLCDMsg msgBuffer;
	lcdOScopeStruct *lcdPtr = (lcdOScopeStruct *) pvParameters;

	#ifdef INSPECT_STACK
	// This is meant as an example that you can re-use in your own tasks
	// Inspect to the stack remaining to see how much room is remaining
	// 1. I'll check it here before anything really gets started
	// 2. I'll check during the run to see if it drops below 10%
	// 3. You could use break points or logging to check on this, but
	//    you really don't want to print it out because printf() can
	//    result in significant stack usage.
	// 4. Note that this checking is not perfect -- in fact, it will not
	//    be able to tell how much the stack grows on a printf() call and
	//    that growth can be *large* if version 1 of printf() is used.   
	unsigned portBASE_TYPE InitialStackLeft = uxTaskGetStackHighWaterMark(NULL);
	unsigned portBASE_TYPE CurrentStackLeft;
	float remainingStack = InitialStackLeft;
	remainingStack /= lcdSTACK_SIZE;
	if (remainingStack < 0.10) {
		// If the stack is really low, stop everything because we don't want it to run out
		// The 0.10 is just leaving a cushion, in theory, you could use exactly all of it
		VT_HANDLE_FATAL_ERROR(0);
	}
	#endif

	/* Initialize the LCD and set the initial colors */
	GLCD_Init();
	tscr = Orange; // may be reset in the LCDMsgTypeTimer code below
	screenColor = Maroon; // may be reset in the LCDMsgTypeTimer code below
	GLCD_SetTextColor(tscr);
	GLCD_SetBackColor(screenColor);
	GLCD_Clear(screenColor);
	DrawLCDAxes();

	// This task should never exit
	for(;;)
	{	
		#ifdef INSPECT_STACK   
		CurrentStackLeft = uxTaskGetStackHighWaterMark(NULL);
		float remainingStack = CurrentStackLeft;
		remainingStack /= lcdSTACK_SIZE;
		if (remainingStack < 0.10) {
			// If the stack is really low, stop everything because we don't want it to run out
			VT_HANDLE_FATAL_ERROR(0);
		}
		#endif

		
		// Wait for a message
		if (xQueueReceive(lcdPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		
		//Log that we are processing a message -- more explanation of logging is given later on
		vtITMu8(vtITMPortLCDMsg,getMsgType(&msgBuffer));
		vtITMu8(vtITMPortLCDMsg,getMsgLength(&msgBuffer));

		// Take a different action depending on the type of the message that we received
		switch(getMsgType(&msgBuffer)) {
		case LCDMsgTypeWave: {
			int point = unpackWaveMsg(&msgBuffer);
			break;
		}
		default: {
			// In this configuration, we are only expecting to receive timer messages
			VT_HANDLE_FATAL_ERROR(getMsgType(&msgBuffer));
			break;
		}
		} // end of switch()

		// Here is a way to do debugging output via the built-in hardware -- it requires the ULINK cable and the
		//   debugger in the Keil tools to be connected.  You can view PORT0 output in the "Debug(printf) Viewer"
		//   under "View->Serial Windows".  You have to enable "Trace" and "Port0" in the Debug setup options.  This
		//   should not be used if you are using Port0 for printf()
		// There are 31 other ports and their output (and port 0's) can be seen in the "View->Trace->Records"
		//   windows.  You have to enable the prots in the Debug setup options.  Note that unlike ITM_SendChar()
		//   this "raw" port write is not blocking.  That means it can overrun the capability of the system to record
		//   the trace events if you go too quickly; that won't hurt anything or change the program execution and
		//   you can tell if it happens because the "View->Trace->Records" window will show there was an overrun.
		//vtITMu16(vtITMPortLCD,screenColor);	
	}
}

// Convert from HSL colormap to RGB values in this weird colormap
// H: 0 to 360
// S: 0 to 1
// L: 0 to 1
// The LCD has a funky bitmap.  Each pixel is 16 bits (a "short unsigned int")
//   Red is the most significant 5 bits
//   Blue is the least significant 5 bits
//   Green is the middle 6 bits
static unsigned short hsl2rgb(float H,float S,float L)
{
	float C = (1.0 - fabs(2.0*L-1.0))*S;
	float Hprime = H / 60;
	unsigned short t = Hprime / 2.0;
	t *= 2;
	float X = C * (1-abs((Hprime - t) - 1));
	unsigned short truncHprime = Hprime;
	float R1, G1, B1;

	switch(truncHprime) {
		case 0: {
			R1 = C; G1 = X; B1 = 0;
			break;
		}
		case 1: {
			R1 = X; G1 = C; B1 = 0;
			break;
		}
		case 2: {
			R1 = 0; G1 = C; B1 = X;
			break;
		}
		case 3: {
			R1 = 0; G1 = X; B1 = C;
			break;
		}
		case 4: {
			R1 = X; G1 = 0; B1 = C;
			break;
		}
		case 5: {
			R1 = C; G1 = 0; B1 = X;
			break;
		}
		default: {
			// make the compiler stop generating warnings
			R1 = 0; G1 = 0; B1 = 0;
			VT_HANDLE_FATAL_ERROR(Hprime);
			break;
		}
	}
	float m = L - 0.5*C;
	R1 += m; G1 += m; B1 += m;
	unsigned short red = R1*32; if (red > 31) red = 31;
	unsigned short green = G1*64; if (green > 63) green = 63;
	unsigned short blue = B1*32; if (blue > 31) blue = 31;
	unsigned short color = (red << 11) | (green << 5) | blue;
	return(color); 
}
