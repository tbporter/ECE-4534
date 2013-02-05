/////////////////////////////////////////////
// Program to test the A/Ds on the 4534 demo
// board.  Channel 0 is connected to the pot.
// Channel 1 is connected to a header where a
// sensor can be hooked up.
// The LEDs display is based on the value read
// from the A/D.
//////////////////////////////////////////////

#include <p18cxxx.h>
#include <portb.h>
#include <delays.h>
#include <adc.h>

void initADC()
{
	OpenADC(ADC_FOSC_8 & ADC_RIGHT_JUST & ADC_0_TAD,
		ADC_CH0 & ADC_CH1 &
		ADC_INT_OFF & ADC_VREFPLUS_VDD &
		ADC_VREFMINUS_VSS, 0b1011);
	// Use SetChanADC(ADC_CH1) to look at sensor channel
	SetChanADC(ADC_CH0);
	Delay10TCYx( 50 );
}

void readADC(int *value)
{
	ConvertADC(); // Start conversion
	while( BusyADC() ); // Wait for ADC conversion
	(*value) = ReadADC(); // Read result and put in temp
	Delay1KTCYx(1);  // wait a bit...
}

void stopADC()
{
	CloseADC(); // Disable A/D converter
}

void main(void)
{
	int value;

	// Set internal oscillator to 4Mhz
	OSCCON = 0x6C;

	// Set PORT B as digital outputs for the LEDs
	LATB = 0x00;
	TRISB = 0x00;

	// Initialize the A/D
	initADC();

	// Flash the LEDs
	LATB = 0x3F;
	Delay10KTCYx(50);
	LATB = 0x00;

	// Loop forever while reading the A/D
	while(1) {

			// read A/D
			readADC(&value);

			// Map the 10 bit value to an LED display
			if(value<0x08F)
				LATB = 0x00;
			else if (value<0x100)
				LATB = 0x01;
			else if (value<0x180)
				LATB = 0x03;
			else if (value<0x200)
				LATB = 0x07;
			else if (value<0x280)
				LATB = 0x0F;
			else if (value<0x300)
				LATB = 0x1F;
			else
				LATB = 0x3F;

	}

}
