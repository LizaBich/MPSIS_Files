#include <msp430.h>

/*
 * main.c
 */
// click on S1 switch comparator as active device, S2 - ADC12
int compareMode = 0; // 0 - comparator, 1 - ADC

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Leds: PAD1 - led 4, PAD4 - led 7; potenc - led 3, termo ADC - led2
    //LED config
    P8DIR |= BIT1; //led 2
    P8DIR |= BIT2; //led 3
    P1DIR |= BIT1; //led 4
    P1DIR |= BIT4; //Led 7
	
    P8OUT &= ~BIT1;
    P8OUT &= ~BIT2;
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT4;

    //Button config
    P2DIR &= ~BIT2; //button S2
    P2REN |= BIT2;
    P2OUT |= BIT2;

    P1DIR &= ~BIT7; // button S1
    P1REN |= BIT7;
    P1OUT |= BIT7;

    //enable interrupt for buttons
    //S1
    P1IES |= BIT7;
    P1IFG &= ~BIT7;
    P1IE |= BIT7;

    //S2
    P2IES |= BIT2;
    P2IFG &= ~BIT2;
    P2IE |= BIT2;
    ////////////////////////
    // comparator setup
    CBCTL0 |= CBIMEN + CBIMSEL_3; // enabling PAD1 and PAD4 like comparator inputs
    CBCTL0 |= CBIPEN + CBIPSEL_0;
    CBCTL1 |= CBPWRMD_1;        // CBMRVS=0 => select VREF1 as ref when CBOUT 
                                // is high and VREF0 when CBOUT is low (normal mode) 
    CBCTL1 |= CBF + CBFDLY_3;   // filter + filter delay
    CBCTL3 |= BIT0 + BIT3;      // Input Buffers Disable P6.1/CB0 and CB3

    CBINT &= ~(CBIFG + CBIIFG);   // Clear any interrupts  
    CBINT  |= CBIE;               // Enable CompB Interrupt on rising edge of CBIFG (CBIES=0)
    CBCTL1 |= CBON;               // Turn On ComparatorB  

    // ADC12 setup
    // potenciometr - A5 input on ADC12
    P6SEL = 0x0F;                               // Enable A/D channel inputs
    ADC12CTL0 = ADC12MSC + ADC12SHT0_2;         // Set sampling time
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_1;         // Use sampling timer, single sequence
    ADC12MCTL5 = ADC12INCH_5;                   // ref+=AVcc, channel = A0
    ADC12MCTL1 = ADC12INCH_1;                   // ref+=AVcc, channel = A1
    ADC12MCTL2 = ADC12INCH_2;                   // ref+=AVcc, channel = A2
    ADC12MCTL3 = ADC12INCH_3 + ADC12EOS;          // ref+=AVcc, channel = A3, end seq.
    ADC12IE = 0x08;                             // Enable ADC12IFG.3
    ADC12CTL0 |= ADC12ENC;                      // Enable conversions

    while(1)
    {
        ADC12CTL0 |= ADC12SC;                   // Start convn - software trigger
        
        __bis_SR_register(LPM4_bits + GIE);     // Enter LPM4, Enable interrupts
        __no_operation();                       // For debugger    
    }

    // timer setup
    TA0CCTL0 = CCIE;	// enable compare/capture interrupt
    TA0CTL = TASSEL__SMCLK + ID_1 + MC__UP + TACLR;
    TA0CTL &= ~TAIFG;
    TA0CCR0 = 0xFFFF;

    // P7SEL |= BIT7;
    // P7DIR |= BIT7;
    __bis_SR_register(LPM0_bits+GIE);
    // _enable_interrupt();
    __no_operation();
	return 0;
}

// Button S2: turn on ADC, turn off comparator
#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void){

	int i;
	for(i=0; i<2000; i++) {}
	if(!(P2IN & BIT2)) {
		P2IE &= ~BIT2;		// disabling interrupts from S2 while processing current interrupt
		
		CBCTL1 &= ~CBON;                // Turn off ComparatorB  
        ADC12CTL0 |= ADC12ON;           // Turn on ADC12
        ADC12CTL0 |= ADC12REFON;       // Turn on generator

		P2IE |= BIT2; 		// enabling interrupt 2nd button
		P2IFG &= ~BIT2; 	// reseting interrupt 2nd button
	}
}

// Button S1: turn on comparator, turn off ADC
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void){
	int i;
	for(i=0; i<2000; i++) {}
	if(!(P1IN & BIT7)) {
		P1IE &= ~BIT7;

        ADC12CTL0 &= ~ADC12ON;          // Turn off ADC12
        ADC12CTL0 &= ~ADC12REFON;       // Turn off generator
		CBCTL1 |= CBON;                 // Turn On ComparatorB  

		P1IE |= BIT7; 		// enabling interrupt 1st button
		P1IFG &= ~BIT7; 	// reseting interrupt 1nd button
	}
}

// Timer
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    if (compareMode == 0)
    {
        // Comparator block
        if(!(CBCTL1 & CBOUT))
		{
            // Turn on led 4, turn off led 7
			P1OUT &= ~BIT4;
			P1OUT |= BIT1;
		}
		else
		{
            // Turn on led 7, turn off led 4
			P1OUT &= ~BIT1;
            P1OUT |= BIT4;
		}
    } else {

    }
	
	TA0CCTL0 &= ~CCIFG;
}

// interrupt from comparator
#pragma vector = COMP_B_VECTOR
__interrupt void CB_ISR(void) {
    // if (high output of comparator)
    // {
    //      turn off led4
    //      turn on led1   
    // } else {
    //      turn off led1
    //      turn on led4
    // }
}
