#include "smcpic.h"
#include "my_adc.h"

void sendMtrCmd(unsigned char cmd)
{
    WriteUSART(cmd);
    return;
}

void readIRSensors(char* IRValues)
{
    readADC(&IRValues[0], ADC_CH0);
    readADC(&IRValues[1], ADC_CH1);
    readADC(&IRValues[2], ADC_CH2);
    readADC(&IRValues[3], ADC_CH3);
    return;
}

void readSonarSensors (char* SonarValues)
{
    readADC(&SonarValues[0], ADC_CH4);
    readADC(&SonarValues[1], ADC_CH5);
    return;
}

void readCurrentSensor(char* CurrentValue)
{
    readADC(&CurrentValue[0], ADC_CH6);
    return;
}