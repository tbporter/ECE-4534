#ifndef _MY_TIMERS_H
#define _MY_TIMERS_H
#include "g9_LCDOScopeTask.h"
#include "i2cTemp.h"
void startTimerForLCD(vtLCDStruct *vtLCDdata);
void startTimerForTemperature(vtTempStruct *vtTempdata);
#endif