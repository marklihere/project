#include "mytm4c123gh6pm.h"

void EEPROM_INIT(void) {
	int a = 0;
	SYSCTLb->RCGCEEPROM |= 0x01;   // enable EEPROM CLock
	for(a = 0; a < 10; a++);       // wait 6 clock cycles
	while(EEPROM->EEDONE &= 0x01);  // Wait until EEDONE[0] == 0 (EEPROM is not busy)
	if(EEPROM->EESUPP &= 0xC);   // if this ever fails, EEPROM chip worn out by too many cycles
	SYSCTLb->SREEPROM |= 0x1;       // Start resetting EEPROM
	for(a = 0; a < 10; a++);       // wait 6 clock cycles
	SYSCTLb->SREEPROM &= 0xFFFE;       // Finish resetting EEPROM
	for(a = 0; a < 10; a++);       // wait 6 clock cycles
	while(EEPROM->EEDONE &= 0x01);  // Wait until EEDONE[0] == 0 (EEPROM is not busy)
	if(EEPROM->EESUPP &= 0xC);   // if this ever fails, EEPROM chip worn out by too many cycles
}

// Initialize I2C  for fast mode
void INIT_I2C(void){
    // We want I2C Frequency of 400 KHz (fast mode)
    // TPR = (System Clock/(2*(SCL_LP + SCL_HP)*SCL_CLK))-1;
	  // TPR = 40000000 / (2*(6+4)*400000)-1
    // TPR = 4 (dec) = 0x4
    #define TPR 0x4
    SYSCTLb->RCGCI2C |= 0x0001;   // activate I2C0
    SYSCTLb->RCGCGPIO |= 0x0002;  // activate port B
    while((SYSCTLb->PRGPIO&0x0002) == 0){};  // ready?
    GPIOB->AFSEL |= 0x0C;        // enable alt func on PB2, 3 
    GPIOB->ODR |= 0x08;          // enable open drain PB3, i2c0 SDA
    GPIOB->PCTL = (GPIOB->PCTL&0xFFFF00FF) + 0x00003300;  // I2C
    GPIOB->DEN |= 0x0C;          // enable digital I/O on PB2,3
			
    I2C0->MCR = 0x00000010;     // master function enable
    I2C0->MTPR = TPR;           // cfg for 100 Kbps initially

	// send the master code @ 100kbps standard speed
	//while(I2C0->MCS_I2C0_ALT & 0x00000001) {};  //wait
	//I2C0->MSA = 0x08;             // master code byte
	//I2C0->MCS = 0x13;             // set HS byte only when Tx master code byte
	//while(I2C0->MCS_I2C0_ALT & 0x00000001) {};  //wait

  // now in High Speed mode		
	// right now HS bit is set, but manual says subsequent Tx do not need to set HS bit
	//I2C0->MTPR = 0x01;   // reset TPR?

}

void INIT_PLL(void) {
    SYSCTLb->RCC2 |= 0x80000000;     // Use RCC2
    SYSCTLb->RCC2 |= 0x00000800;   // bypass PLL while initializing
    SYSCTLb->RCC = (SYSCTLb->RCC &~0x000007C0) + 0x00000540;  // 16 MHz
    SYSCTLb->RCC2 &= ~0x00000070;  // Cfg for main OSC
    SYSCTLb->RCC2 &= ~0x00002000;  // activate PLL by clearing PWRDN
    //SYSCTLb->RCC2 |= 0x40000000;   // use 400 MHz PLL   
    SYSCTLb->RCC2 = (SYSCTLb->RCC2 & ~0x1FC00000) + (4<<23);  // 80 MHz
	  //SYSCTLb->RCC2 = (SYSCTLb->RCC2 & ~0x1FC00000) + (9<<23);  // 40 MHz
    while((SYSCTLb->RIS & 0x00000040)==0){};  // Wait for PLL to lock
    SYSCTLb->RCC2 &= ~0x00000800;  // enable PLL by clearing bypass
}

void GPIO_INIT(void) {
	  SYSCTLb->RCGCSSI |= 0x01;     //   same as below
    SYSCTLb->RCGCGPIO |= 0x001B;  // activate port A, B, D, E 
    while((SYSCTLb->PRGPIO&0x001B) == 0){};  // ready?

		// Port A
		// PA2 = SSI0CLK  = SDO, LCD_SCK, pin 23

	  // PA4 = SSI0Rx = TP_SO, pin 21
    // PA5 = SSI0Tx   = LCD_SI, pin 19
		// GPIOA->DIR |= 0x2C;          // PA2,3,4,5 output
		GPIOA->AFSEL |= 0x3C;        // Enable alt func on PA2,3,4,5
    GPIOA->DEN |= 0x3C;          // enable digital I/O on PA2,3,4,5

		// Port B
		// PB[0] = Data/CMD Pin for LCD (LCD_RS)
		GPIOB->DIR |= 0x01;          // PB[0] output
		GPIOB->AFSEL |= 0x00;        // disable alt func on PB0
		GPIOB->PUR |= 0x01;          // default to data Tx
    GPIOB->DEN |= 0x01;          // enable digital I/O on PB0

		// Port D
    // PD0 = RED LED
    // PD1 = GREEN LED
    // PD2 = Yellow LED
		GPIOD->DIR |= 0x07;          // PD[2:0] output, but we sink current into TM4C to turn on
		GPIOD->AFSEL |= 0x00;        // disable alt func on PB0
		GPIOD->PUR |= 0x07;          // write 0 to turn on LED
    GPIOD->DEN |= 0x07;          // enable digital I/O on PB0
		GPIOD->DATA |= 0x7;          // turn off all LEDs by default
		
		// Port E
		GPIOE->DIR |= 0xFD;  // PE[1] is input, PE[0] is output, PE[2] is output
		GPIOE->AFSEL &= 0x00;  
		GPIOE->PUR |= 0x5;   // pull up on output LCD/TS chip select pins
		GPIOE->DEN |= 0x7;     // PE[2:0] enabled
}

void INIT_SSI0(void) {
	
	// SYSCTLb->RCGC1 |= 0x10;       // Activate clk for SSI0
	SSI0b->CR1 &= 0xFFFFFFFD;        // Disable SSI while configuring
	SSI0b->CR1 &= 0xFFFFFFFB;        // Set to Master mode
	// Time for ludicrous speed
	// 80 MHz SysClk SSIClk = SysClk / (CPSDVSR * (1 + SCR)) 
	//               10 MHz = 40 MHz / (CPSDVSR * (1 + ))
  // CPSDVSR from 2 to 254 (SSICPSR REG) SCR is a value from 0-255
  // from above, 4 = (CPSDVSR * (1 + SCR)) = 2 * (1+SCR)
	// we choose CPSDVSR = 1, then SCR = 1
	// so CPSR = 0x02, SSI0->CR0 = 0x0107
	SSI0b->CPSR = 0x02;             // see above
	SSI0b->CR0  = 0x0107;           // see above
	SSI0b->CR1 |= 0x2;              // Enable SSI
}
