#ifndef __G9_SYS_FLAGS__
#define __G9_SYS_FLAGS__

/* Define whether or not to start the standard FreeRTOS demo tasks (the code is still included in the project
   unless the files are actually removed from the project */
#define USE_FREERTOS_DEMO 0
// Define whether or not to use I2C
#define USE_I2C 0
// Define whether or not to use XBEE
#define USE_XBEE 1
// Define whether or not to use my LCD task
#define USE_MTJ_LCD 0
// Define whether or not to use the OScope task
#define USE_G9_OSCOPE 0
// Define whether to use my temperature sensor read task (the sensor is on the PIC v4 demo board, so if that isn't connected
//   then this should be off
#define USE_MTJ_V4Temp_Sensor 0
// Define whether to use my USB task
#define USE_MTJ_USE_USB 0
// Define whether to use the navigation task
#define USE_NAV_TASK 1
// Define whether or not to send fake I2C messages
#define USE_FAKE_I2C 0
// Define where or not to use web server
#define USE_WEB_SERVER 1


#endif