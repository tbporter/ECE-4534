#include "encoders.h"

static int enL_setA, enL_setB, enR_setA, enR_setB;
volatile unsigned int encoderLPos, encoderRPos;
unsigned int lastReportedLPos, lastReportedRPos;
int lchange, rchange;

void initEncoders(void)
{

    encoderLPos = 0;
    encoderRPos = 0;
    lastReportedLPos = 0;
    lastReportedRPos = 0;
    enL_setA = PORTBbits.KBI0;
    enL_setB = PORTBbits.KBI1;
    enR_setA = PORTBbits.KBI2;
    enR_setB = PORTBbits.KBI3;
}

void encoderIntHandler(void)
{
    // figure out which of the 4 bits changed
    if (enL_setA != PORTBbits.KBI0)
    {
        enL_setA = PORTBbits.KBI0;
        encoderLPos += (enL_setA != enL_setB) ? +1 : -1;
    }
    else if (enL_setB != PORTBbits.KBI1)
    {
        enL_setB = PORTBbits.KBI1;
        encoderLPos += (enL_setA == enL_setB) ? +1: -1;
    }
    else if (enR_setA != PORTBbits.KBI2)
    {
        enR_setA = PORTBbits.KBI2;
        encoderRPos += (enR_setA != enR_setB) ? +1 : -1;
    }
    else if (enR_setB != PORTBbits.KBI3)
    {
        enR_setB = PORTBbits.KBI3;
        encoderRPos += (enR_setA == enR_setB) ? +1 : -1;
    }

    calculateLChange();
    calculateRChange();

    return;

}

void calculateLChange(void)
{
    lchange += encoderLPos - lastReportedLPos;
    lastReportedLPos = encoderLPos;
    return;
}

void calculateRChange(void)
{
    rchange += encoderRPos - lastReportedRPos;
    lastReportedRPos = encoderRPos;
    return;
}

int reportLChange(void)
{
    int prevChange = lchange;
    lchange = 0;
    //return prevChange;
    return (prevChange * -1);
}

int reportRChange(void)
{
    int prevChange = rchange;
    rchange = 0;
    //return prevChange;
    return (prevChange * -1);
}