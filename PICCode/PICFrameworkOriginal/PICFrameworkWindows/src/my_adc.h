#ifndef MY_ADC_H
#define	MY_ADC_H
#include <adc.h>
#include "messages.h"


void adc_int_handler(void);

void initADC(void);

void readADC(unsigned char* value, unsigned char channel);

void stopADC(void);

#endif	/* MY_ADC_H */

