#include "maindefs.h"
#include "my_adc.h"


void adc_int_handler(){
    ToMainLow_sendmsg(0, MSGT_ADC_DATA, (void *) 0);
}

void initADC()
{
	OpenADC(ADC_FOSC_16 & ADC_RIGHT_JUST & ADC_2_TAD,
		ADC_CH0 & ADC_CH1 &
		ADC_INT_ON & ADC_VREFPLUS_VDD &
		ADC_VREFMINUS_VSS, 0b1011);
	// Use SetChanADC(ADC_CH1) to look at sensor channel
	SetChanADC(ADC_CH0);
	//Delay10TCYx( 50 );
}

void readADC(int *value)
{
	ConvertADC(); // Start conversion
	while( BusyADC() ); // Wait for ADC conversion
	(*value) = ReadADC(); // Read result and put in temp
	//Delay1KTCYx(1);  // wait a bit...
}

void stopADC()
{
	CloseADC(); // Disable A/D converter
}