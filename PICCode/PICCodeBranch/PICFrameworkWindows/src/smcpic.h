#ifndef SMCPIC_H
#define	SMCPIC_H

#include <usart.h>

void sendMtrCmd(unsigned char cmd);

void readIRSensors(char*);
void readSonarSensors(char*);
void readCurrentSensor(char*);


#endif	/* SMCPIC_H */

