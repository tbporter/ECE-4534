#ifndef __NAV_TASK_H__
#define __NAV_TASK_H__

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "vtI2C.h"
#include "messages.h"

typedef struct __navStruct {
	vtI2CStruct *dev;
	xQueueHandle inQ;
} navStruct;


#endif