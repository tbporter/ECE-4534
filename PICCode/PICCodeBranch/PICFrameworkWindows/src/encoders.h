#ifndef ENCODERS_H
#define	ENCODERS_H
#include <portb.h>

void initEncoders(void);
void encoderIntHandler(void);
void calculateLChange(void);
void calculateRChange(void);
int reportLChange(void);
int reportRChange(void);

#endif	/* ENCODERS_H */

