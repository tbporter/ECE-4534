#include "my_adc.h"
#include "maindefs.h"


void adc_int_handler() {
    ToMainLow_sendmsg(0, MSGT_ADC_DATA, (void*) 0);
    return;
}

void initADC() {
    OpenADC(ADC_FOSC_16 & ADC_RIGHT_JUST & ADC_2_TAD,
            ADC_CH0 & ADC_CH1 & ADC_CH2 & ADC_CH3 & ADC_CH4 & ADC_CH5 & ADC_CH6 &
            ADC_INT_OFF & ADC_VREFPLUS_VDD &
            ADC_VREFMINUS_VSS, 0b1011);
    return;
}

void readADC(unsigned char* value, unsigned char channel) {
    int reading = 0;
    SetChanADC(channel);
    ConvertADC();
    while (BusyADC());
    reading = ReadADC();
    *value = reading >> 2;
    return;
}

void stopADC() {
    CloseADC();
    return;
}
