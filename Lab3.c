#include <msp430.h>

#define DELTA 6250;

/*  Глобальные переменные  */
volatile short LPM_MODE = 0; // 0 - active mode, 1 - LPM1 mode.
volatile short VCORE_MODE = 0; // 0 - Vcore = 1.4, 1 - Vcore = 1.6
volatile short FREQ_MODE = 0; // 0 - DCOCLK = 0.5; 1 - DCOCLK = 1
volatile short upCounter = 0;

void SwitchLed(short ledNumber, short mode); // 0 - turn off, 1 - turn on
void SwitchLPM1();
void SwitchVcoreMode(short isUp, unsigned int level);
void setFreq();


/*  Объявление функций  */

// LED1: port - P1; bit - 0
// LED3: P8.2
// LED4: P1.1
// LED5: P1.2
// LED8: P1.5
// S1: P1.7
// S2: port - P2; bit - 2
// TA2:  port - ; bit -

// PxSEL – выбор функции вывода: 0 – I/O, 1 – периферия;
// PxREN – разрешение подтягивающего резистора;
// PxIES – выбор направления перепада для генерации запроса на прерывание: 0 – по фронту, 1 – по спаду;
// PxIE – разрешение прерывания;
// PxIFG – флаг прерывания.

void main(void) {
    WDTCTL = WDTPW + WDTHOLD;    // отключаем сторожевой таймер
//	P1SEL &= ~BIT0;
//	P1SEL &= ~BIT1;
//	P1SEL &= ~BIT2;
//	P1SEL &= ~BIT5;
//	P8SEL &= ~BIT2;

    SetFreq();

	P1DIR |= BIT0;                          // P1.0 output 	LED1
	P1DIR |= BIT1;							// P1.1 output  LED4
	P1DIR |= BIT2;                          // P1.2 output 	LED5
	P1DIR |= BIT5;							// P1.5 output  LED8
	P8DIR |= BIT2;                          // P8.2 output 	LED3
	P2DIR &= ~BIT2;							// P2.2 input   S2
	P1DIR &= ~BIT7;							// P1.7 input   S1

	P1OUT |= BIT7;
	P1REN |= BIT7;
	P2OUT |= BIT2;
	P2REN |= BIT2;

	SwitchLed(1, 1);
	SwitchLed(3, 0);
	SwitchLed(4, 1);
	SwitchLed(5, 0);
	SwitchLed(8, 0);

	// config of interrupts for S2
    P2IE |= BIT2;    						// interruptions are awailable now
    P2IES |= BIT2;
    P2IFG &= ~BIT2;

    // config of interrupts for S1
    P1IE |= BIT7;    						// interruptions are awailable now
    P1IES |= BIT7;
    P1IFG &= ~BIT7;

    // timer config
    TA2CCTL0 = CCIE; // Enable counter interrupt on counter compare register 0
    TA2CTL = TASSEL__ACLK +ID__1 + MC__UP + TACLR; // Use the SMCLK to clock the counter, SMCLK/8, count up mode 8tick/s
    TA2CTL &= ~TAIFG;
    TA0CCR0 = 10000;



    // __bis_SR_register(LPM0_bits + GIE);     // Turn on 'Save power' mode
	_enable_interrupt();
    __no_operation();
}


#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A(void){
	P1OUT |= BIT5;
}

// interrupt handler for S2
#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	P2IE &= ~BIT2;		// disabling interrupts from S2 while processing current interrupt

	if (LPM_MODE == 1)
	{
		// __bic_SR_register_on_exit(LPM1_bits);
		__bic_SR_register(LPM1_bits);
		LPM_MODE = 0;
		P8OUT &= ~BIT2;
		P1OUT |= BIT0;
	} else {
		P1OUT &= ~BIT0;
		P8OUT |= BIT2;
		LPM_MODE = 1;
		P2IE |= BIT2; 		// enabling interrupt 2nd button
		P2IFG &= ~BIT2; 	// reseting interrupt 2nd button
		__bis_SR_register(LPM1_bits + GIE);

	}

   	P2IE |= BIT2; 		// enabling interrupt 2nd button
    	P2IFG &= ~BIT2; 	// reseting interrupt 2nd button
}

// interrupt handler for S1
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void){
	P1IE &= ~BIT7;		// disabling interrupts from S1 while processing current interrupt

	if (FREQ_MODE == 1)
	{
		// add code to confige Vcore
		// set values for low frequency mode
		__bis_SR_register(SCG0); // Disable FLL control

		UCSCTL0 |= DCO0;  // set lowest posible DCOx, MODx

		UCSCTL1 = DCORSEL_0; //select DCO range 1(was 3) 0.07 - 1.7MGz;

		UCSCTL2 = FLLD_1 + 8;
		UCSCTL2 = FLLN9; // incorrect value, should be changed
    		UCSCTL3 = FLLREFDIV_1; // incorrect value, should be changed

		__bic_SR_register(SCG0);

		FREQ_MODE = 0;
	} else {
		// add code to confige Vcore
		// set values for hige frequency mode
		__bis_SR_register(SCG0); // Disable FLL control

		UCSCTL0 |= DCO0;  // set lowest posible DCOx, MODx

		UCSCTL1 = DCORSEL_0; //select DCO range 1(was 3) 0.07 - 1.7MGz;

		UCSCTL2 = FLLD_1 + 8;
		UCSCTL2 = FLLN9; // incorrect value, should be changed
		UCSCTL3 = FLLREFDIV_1; // incorrect value, should be changed

		__bic_SR_register(SCG0);

		FREQ_MODE = 1;
	}

    P1IE |= BIT7; 		// enabling interrupt 1st button
    P1IFG &= ~BIT7; 	// reseting interrupt 1st button
}

void SwitchLed(short ledNumber, short mode)
{
	switch(ledNumber)
	{
		case 1:
			if (mode == 1)
			{
				P1OUT |= BIT0;
			} else {
				P1OUT &= ~BIT0;
			}
		 	break;
		case 3:
			if (mode == 1)
			{
				P8OUT |= BIT2;
			} else {
				P8OUT &= ~BIT2;
			}
			break;
		case 4:
			if (mode == 1)
			{
				P1OUT |= BIT1;
			} else {
				P1OUT &= ~BIT1;
			}
			break;
		case 5:
			if (mode == 1)
			{
				P1OUT |= BIT2;
			} else {
				P1OUT &= ~BIT2;
			}
			break;
		case 8:
			if (mode == 1)
			{
				P1OUT |= BIT5;
			} else {
				P1OUT &= ~BIT5;
			}
			break;
	}
}

void SwitchVcoreMode(short isUp, unsigned int level)
{
	if (upCounter == 3) return;
	if (isUp == 1)
	{
  		PMMCTL0_H = PMMPW_H;	// Subroutine to change core voltage. Open PMM registers for write
  		SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;	// Set SVS/SVM high side new level

  		SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level; 	// Set SVM low side to new level

  		while ((PMMIFG & SVSMLDLYIFG) == 0); 	// Wait till SVM is settled
  		PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);	// Clear already set flags
  		PMMCTL0_L = PMMCOREV0 * level;	// Set VCore to new level
  		if ((PMMIFG & SVMLIFG))		// Wait till new level reached
    	{
			while ((PMMIFG & SVMLVLRIFG) == 0);
		}
  		SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;	// Set SVS/SVM low side to new level
  		PMMCTL0_H = 0x00;	// Lock PMM registers for write access
	} else {
		PMMCTL0_H = PMMPW_H;

    	SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;

   	 	while ((PMMIFG & SVSMLDLYIFG) == 0);
    	PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    	PMMCTL0_L = PMMCOREV0 * level;
    	PMMCTL0_H = 0x00;
	}
}

void setFreq ()
{
    	UCSCTL3 |= SELREF__REFOCLK; //set DCO reference = REFO (32 KGz)
    	UCSCTL4 |= SELM__DCOCLK | SELA__DCOCLK | SELS_DCOCLK; //set MCLK and ACLK to DCOCLK

    	__bis_SR_register(SCG0); // Disable FLL control

    	UCSCTL0 |= DCO0;  // set lowest posible DCOx, MODx

    	UCSCTL1 = DCORSEL_0; //select DCO range 1(was 3) 0.07 - 1.7MGz;

    	UCSCTL2 = FLLD_1 + 8;
    	UCSCTL2 = FLLN9; // incorrect value, should be changed
    	UCSCTL3 = FLLREFDIV_1; // incorrect value, should be changed

    	__bic_SR_register(SCG0);
}
