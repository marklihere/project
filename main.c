#include "mytm4c123gh6pm.h"
#include "init.h"
#include "LCD.c"

#define SSI_SR_RNE 0x00000004 // SSI Rx FIFO Not Empty
#define SSI_SR_TFE 0x00000001 // SSI Tx FIFO Empty
#define rxmax 0xBC9
#define rxmin 0x516
#define rymax 0x5E2
#define rymin 0x315
#define yxmax 0xB34
#define yxmin 0x426
#define yymax 0xC89
#define yymin 0xA65
#define gxmax 0xBA0
#define gxmin 0x5E1
#define gymax 0x7FD
#define gymin 0x670


unsigned short xarray[100];
unsigned short yarray[100];
int xtotal = 0;
int ytotal = 0;
int r = 0;  // status of red button
int g = 0;  // status of green button
int y = 0;  // status of yellow button

int i = 0;

void yellowLED(void) {
  if (y==1) {  // LED is on, turn it off, yellow is PD2
	  GPIOD->DATA |= 0x4;  // X1XX
	} else {
		GPIOD->DATA &= 0xFB; // 1011  Turns it on
	}
}


void redLED(void) {
  if (r==1) {  // LED is on, turn it off, red is PD0
	  GPIOD->DATA |= 0x1;  // XXX1
	} else {
		GPIOD->DATA &= 0xFE; // 1110  Turns it on
	}
}

void greenLED(void) {
  if (g==1) {  // LED is on, turn it off, red is PD1
	  GPIOD->DATA |= 0x2;  // XX1X
	} else {
		GPIOD->DATA &= 0xFD; // 1101  Turns it on
	}
}

void mysetArea(unsigned short x1, unsigned short x2, unsigned short y1, unsigned short y2) {
  // Column Address Set 0x2A
	// columns range from 0 to 239
	mywriteCmd(0x2A);
	mywriteDat2(x1);
	mywriteDat2(x2);
	
	// Page Address Set 0x2B
	// pages range from 0 to 319
	mywriteCmd(0x2B);
	mywriteDat2(y1);
	mywriteDat2(y2);
}


void mywriteColor(unsigned short color) {
	int i;
	int cols;
	int rows;
	cols = 240;
	rows = 320;

  mywriteCmd(0x2C);
	for(i = 0; i < cols*rows; i++)
	  mywriteDat2(color);
}

	
void fillGreen(void) {
  mysetArea(67, 137, 128, 193);
  if (g==0)
		mywriteColor(green);
	else
		mywriteColor(black);
}

void fillRed(void) {
  mysetArea(67, 137, 45, 110);
	if (r==0)
	  mywriteColor(red);
	else
		mywriteColor(black);
}
void fillYellow(void) {
  mysetArea(67, 137, 211, 277);
	if (y==0)
  	mywriteColor(yellow);
	else
		mywriteColor(black);
}

void GPIO_INT_INIT(void) {
	// enable interrupts
	GPIOE->IM |=0x2;             // allow pin[1] to interrupt
	NVIC->IP[4] = 0x40;          // interrupt priority 2
	NVIC->ISER[0] = 1 << 4;      // Enable interrupt 4 [GPIOE]
}

void getX(void) {
	// Temporary variables to store data as we read
	unsigned short data = 0;

	GPIOE->DATA &= 0xFE; // TS chip select low
  sendAfterWaiting(0xD0);  // read x-coord
  data = sendAfterWaiting(0) << 5;   // sends 16-bits of 0
	data += sendAfterWaiting(0) >> 3;   // sends 16-bits of 0
	GPIOE->DATA |= 0x1; // TS chip select high
	xarray[i%100] = data;
}

void getY(void) {
	// Temporary variables to store data as we read
	unsigned short data = 0;
	
	GPIOE->DATA &= 0xFE; // chip select low
  sendAfterWaiting(0x90);  // read x-coord
  data = sendAfterWaiting(0) << 5;   // sends 16-bits of 0
	data += sendAfterWaiting(0) >> 3;   // sends 16-bits of 0
	GPIOE->DATA |= 0x1; // chip select high
	yarray[i%100] = data;
}


// Triggers on touchscreen press
// PE1 == 0 causes this interrupt
void GPIOE_Handler(void) {
	int x = 0;
	int divideby = 0;

	// GET TOUCHED COORDINATES
  // while PENIRQ is low, continuously load touchscreen values into our array
	// GPIOE->DATA &= 0xFE;    // put CS low at start of Tx
	while ((GPIOE->DATA & 0x2) == 0x0) {  // polling PE[1], the interrupt
	  getX();   
	  getY();   
	  i++;     // array index for our read X and Y coordinates
	}
	// return chip select to high
	// because this means we have already released the touchscreen and should
	// be calculating a coordinate that was touched
	// GPIOE->DATA |= 0x1;    // put CS high at end of Tx
	
	// PROCESS TOUCH
	// sum all array values, divide by divideby variable
	
		// check if i > 100, if so, we change our divide by value when averaging the actual position
		// because this means someone touched and held for longer than 100 read cycles
		// in which case we need to keep filling array (loop around) and then average
		// the entire array, Ex: touch & hold & drag, then we want to read the final release coordinate
		// and not the initial touch point
	if (i > 100) divideby = 100;
		else divideby = i;
	
  if (divideby!=100) {
		for(x = 0; x < i; x++) {
				xtotal += xarray[x];
				ytotal += yarray[x];
	  }
		xtotal /= i;
		ytotal /= i;
	} else {
				for(x = 0; x < 100; x++) {
				xtotal += xarray[x];
				ytotal += yarray[x];
	  }
		xtotal /= 100;
		ytotal /= 100;
	}
	// at this point xtotal and ytotal represent the actual x and y coordinates
	
	// reset array index at the end of reading an actual coordinate
	// invalidates entire temporary xarray and yarray read tables
	i = 0;
	
	GPIOE->DATA &= 0xFB; // LCD CS = 0  1011  PE[2]
		if(xtotal > rxmin && xtotal < rxmax && ytotal > rymin && ytotal < rymax) {
			if(r==1) {  // r == 1 means already filled
				fillRed();
  			redLED();
				r = 0;
			} else {
				fillRed();
				redLED();
				r = 1;
			}
		} else if (xtotal > yxmin && xtotal < yxmax && ytotal > yymin && ytotal < yymax) {
			if(y==1) {
				fillYellow();
				yellowLED();
				y = 0;
			} else {
				fillYellow();
				yellowLED();
				y = 1;
			}
		} else if (xtotal > gxmin && xtotal < gxmax && ytotal > gymin && ytotal < gymax) {
			if(g==1) {
				fillGreen();
				greenLED();
				g = 0;
			} else {
				fillGreen();
				greenLED();
				g = 1;
			}
		}
	GPIOE->DATA |= 0x4; // LCD CS = 1, done writing to LCD
	GPIOE->ICR |= 0x2;    // clear interrupt on pin [1]
}

int main(void)
{
	INIT_PLL();
	GPIO_INIT();
	INIT_SSI0();
    GPIO_INT_INIT();  // Enable interrupts
	
	// Enable LCD
	GPIOE->DATA &= 0xFB; // LCD CS = 0  1011  PE[2] = 0
	LCD_Init();
	
	// Draw Initial screen
  mysetArea(0, 239, 0, 319);
  mywriteColor(black);	
  mysetArea(5, 75, 85, 155);
  mywriteColor(white);
  mysetArea(86, 155, 85, 155);
  mywriteColor(cyan);
  mysetArea(165, 235, 85, 155);
  mywriteColor(green);
  mysetArea(5, 75, 165, 235);
  mywriteColor(blue);
  mysetArea(86, 155, 165, 235);
  mywriteColor(magenta);
  mysetArea(165, 235, 165, 235);
  mywriteColor(orange);
  mysetArea(5, 75, 245, 315);
  mywriteColor(yellow);
  mysetArea(86, 155, 245, 315);
  mywriteColor(brown);
  mysetArea(165, 235, 245, 315);
  mywriteColor(red);
	GPIOE->DATA |= 0x4; // LCD CS = 1  // 0100  setting PE[2] = 1
	while(1);

 }