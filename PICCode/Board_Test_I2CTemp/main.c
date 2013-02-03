/////////////////////////////////////////////////////
// Program example to test 4534 example board.  The
// program configures and reads the i2c temperature
// sensor.  The lower three bits of the sensor address
// are displayed on the yellow LEDs. The red LEDs display
// any temperature change.  The prgram has the three 
// temperature sensor address bits set to 111, but it
// is easy enough to change this.
////////////////////////////////////////////////////

#include <p18cxxx.h>
#include <plib/portb.h>
#include <delays.h>
#include <plib/i2c.h>

void setTempSensorAddress(unsigned char address)
{
	// Don't want to set all eight bits as with the following
	// LATB = address;	// Zero out Port B
}

// Configure and start temperature sensor
void configStartTempSensor(void)
{
	// Set configuration register
	StartI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0xAC); // access configuration
	IdleI2C();
	WriteI2C(0x00); // set config
	IdleI2C();
	StopI2C();

	// Need at least a 10msec delay (from data sheet)
	Delay10KTCYx(40);

	// Start temperature conversion
	StartI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0xEE); // start conversion
	IdleI2C();
	StopI2C();

	// Another delay
	Delay10KTCYx(40);
}

// Configure and start temperature sensor
void stopTempSensor(void)
{
	StartI2C();
	IdleI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0x22); // stop conversion
	IdleI2C();
	StopI2C();
	// Another delay
	Delay10KTCYx(40);
}

void readTempSensor(unsigned char *byte1, unsigned char *byte2,
					unsigned char *slope, unsigned char *counter)
{
	StartI2C();
	IdleI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0xAA); // read two bytes command
	IdleI2C();

	RestartI2C();
	IdleI2C();
	WriteI2C(0x9F); // slave address + R
	IdleI2C();
	(*byte1) = ReadI2C();
	IdleI2C();
	AckI2C();
	IdleI2C();
	(*byte2) = ReadI2C();
	IdleI2C();
	NotAckI2C();
	IdleI2C();
	StopI2C();

	Delay10KTCYx(1);

	// read counter
	StartI2C();
	IdleI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0xA8); // read counter command
	IdleI2C();

	RestartI2C();
	IdleI2C();
	WriteI2C(0x9F); // slave address + R
	IdleI2C();

	(*counter) = ReadI2C();
	IdleI2C();
	NotAckI2C();
	IdleI2C();
	StopI2C();

	Delay10KTCYx(1);

	// read slope
	StartI2C();
	IdleI2C();
	WriteI2C(0x9E); // slave address + W
	IdleI2C();
	WriteI2C(0xA9); // read slope command
	IdleI2C();

	RestartI2C();
	IdleI2C();
	WriteI2C(0x9F); // slave address + R
	IdleI2C();

	(*slope) = ReadI2C();
	IdleI2C();
	NotAckI2C();
	IdleI2C();
	StopI2C();

	Delay10KTCYx(1);
}

void main(void)
{
	unsigned char address;
	unsigned char byte1, byte2, slope, counter;
	int temp, old_temp;
	int diff, last_diff;

	// zero our variables to save iteration values
	old_temp = 0;
	last_diff = 0;

	// Set internal oscillator to 4Mhz
	OSCCON = 0x6C;

	// Set PORT C (TRISC<3:4> bits) as digital outputs 
	TRISC = 0x18;
	LATC = 0x18;

	// Set Port B for digital output (LEDs)
	TRISB = 0x00;
	LATB = 0x3F;

	address = 0x07; // don't use this right now!
	setTempSensorAddress(address);

	// According to the datasheet, with a 4Mhz clock we
	// need SSPADD set to 0x28 to get a 100KHz clock for
	// the i2c master.  However, this is clearly wrong
	// when you look at the i2c clock with the logic tool.
	// The i2c frequency is about 23 KHz.  An errata sheet
	// for the processor says there is an error in the data
	// sheet.  SSPADD = 0x0A appears to get 83KHz.
	SSPADD = 0x0A;

	// Init PIC I2C hardware as Master
	OpenI2C (MASTER, SLEW_OFF);
	
	configStartTempSensor();

	while(1) {

		// Flash top red LED to indicate that the program is running
		LATBbits.LATB5 = ~(LATBbits.LATB5 & 1);

		// read temp sensor, get bytes for more accurate result
		readTempSensor(&byte1, &byte2, &slope, &counter);

		// check to see if we have a read error. If not, check to 
		// see if the temperature has changed.  If so, do something
		// with the red LEDs.
		if(byte1!=0xFF) {
			temp = (int) byte1; 
			temp = temp << 1;
			byte2 = byte2 >> 7;
			temp = temp + (int) byte2;
			diff = temp - old_temp;
			if(diff<0) diff = -diff;
			if((diff+last_diff)>=3) {
				LATB = 0x3F;
			} else if ((diff+last_diff)==2) {
				LATB = 0x1F;
			} else if ((diff+last_diff)==1) {
				LATB = 0x0F;
			} else {
				LATB = 0x07;
			}

			// save temp as old_temp to compute difference
			old_temp = temp;
			last_diff = diff;
		} else {
				LATB = 0x3F;
		}

		// delay for some time
		Delay10KTCYx(100);

	}

}
