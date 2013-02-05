#ifndef __my_adc_h
#define __my_adc_h

#include "messages.h"
#include "adc.h"

void adc_int_handler(void);
/* ADC CODE */
void initADC(void);

void readADC(int *value);

void stopADC(void);

/* END ADC CODE */
#endif
