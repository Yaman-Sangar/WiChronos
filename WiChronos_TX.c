/* Transmit data through UART to Linx transmitter encoded as the delay time between two packets */

#include <msp430.h>

void Software_Trim();                       // Software Trim to get the best DCOFTRIM value
void set_pins_low();
void set_UART_clk();
void config_UART();
void set_xt1_clk();
void set_timer_sleep(unsigned int time);
void send_startpacket(int in);
void send_stoppacket(int out);
void set_linx_gpio_normal();
void set_linx_gpio_sleep();
void set_timer_sleep_5s();

#define MCLK_FREQ_MHZ 1                     // MCLK = 1MHz

unsigned int RXData = 0, num_sent = 0;

// num_sent counte how many packets have been sent


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    set_pins_low();

    P1OUT &= ~BIT0;
    P1DIR |= BIT0;                          // Set P1.0 to output direction
    P1DIR &= ~BIT3;                         // Set P1.3 as input

    set_linx_gpio_normal();
    P4OUT |= BIT7;
    set_xt1_clk();
    set_UART_clk();
    config_UART();
    CSCTL4 = SELA__XT1CLK | SELMS__DCOCLKDIV;    //Select Aux clock as XT1(32kHz) and Main clock as DCO (1MHz)
    TB0CCTL0 |= CCIE;                            // TBCCR0 interrupt enabled
    //TB1CCTL0 |= CCIE;                            //TBCCR1 interrupt enabled

    //set_timer_sleep_5s();


    while (num_sent<50)
    {
        // Wake up Linx from Powerdown after turning all GPIO pins to their normal state
        set_linx_gpio_normal();
        P4OUT |= BIT7;                              // Powerdown is now VDD
        set_timer_sleep(200);                       // To ensure Ready is low.

        config_UART();
        send_startpacket(1);                        // To start the timer on Rcx
        num_sent++;

        // Set Linx GPIO pins to 0, reset eUSCI, Put Linx in powerdown
        set_linx_gpio_sleep();
        UCA0CTLW0 |= UCSWRST;                     // Put eUSCI in reset
        P4OUT &= ~BIT7;

        // Time encoded data to be sent
        set_timer_sleep(5000);

        // Wake up Linx from Powerdown after turning all GPIO pins to their normal state to send stop packet
        set_linx_gpio_normal();
        P4OUT |= BIT7;                              // Powerdown is now VDD
        set_timer_sleep(200);                       // To ensure Ready is low.

        config_UART();
        send_startpacket(1);                        // To stop timer on Rcx
        num_sent++;

        // Set Linx GPIO pins to 0, reset eUSCI, Put Linx in powerdown
        set_linx_gpio_sleep();
        UCA0CTLW0 |= UCSWRST;                     // Put eUSCI in reset
        P4OUT &= ~BIT7;

        set_timer_sleep(10000);
//        set_timer_sleep_5s();
    }
    P1OUT |= BIT0;
    __bis_SR_register(LPM4_bits | GIE);         // Enter LPM3 w/ interrupt
}

void set_timer_sleep (unsigned int time)
{
    //Start timer for a small delay and go into LPM3
    TB0CTL = TBCLR;
    TB0CCR0 = time;                             // set delay
    TB0CTL = ID__1 | TBSSEL__ACLK | MC__UP;
    __bis_SR_register(LPM4_bits | GIE);         // Enter LPM3 w/ interrupt
    TB0CTL = TBCLR;
}

void set_timer_sleep_5s()
{
    //Start timer for a small delay and go into LPM3
    TB0CTL = TBCLR;
    TB0CCR0 = 10955;                             // set delay
    TB0CTL = ID__4 | TBSSEL__ACLK | MC__UP;
    __bis_SR_register(LPM4_bits | GIE);         // Enter LPM3 w/ interrupt
    TB0CTL = TBCLR;
}

void send_startpacket(int in){
    int i=0;
    for(i=0; i<in; i++){
        P1OUT ^= BIT0;
        while(P1IN & BIT3)
        {
            //P1OUT ^= BIT0;
        }
        while((!(UCA0IFG & UCTXIFG)));
        UCA0TXBUF = 0x55;

        while((!(UCA0IFG & UCTXIFG)));
        UCA0TXBUF = 0xAA;                   // Load data onto buffer
        set_timer_sleep(60);
    }
}

void send_stoppacket(int out){
    int j=0;
    for(j=0; j<out; j++){
        while(P1IN & BIT3)
        {

        }

        while((!(UCA0IFG & UCTXIFG)) & (~P1IN & BIT3));
        UCA0TXBUF = 0xAA;                   // Load data onto buffer
        TB0CTL = TBCLR;
        TB0CCR0 = 7;                             // set delay
        TB0CTL = ID__1 | TBSSEL__ACLK | MC__UP;
        __bis_SR_register(LPM4_bits | GIE);         // Enter LPM3 w/ interrupt
        TB0CTL = TBCLR;
    }
}

void set_linx_gpio_normal()
{
    // Use pin 4.6 for standby and 4.7 for Powerdown.
    P4DIR |= BIT6 | BIT7;
    //Standby = 0 for normal operation
    P4OUT &= ~BIT6;
    //Powerdown is VCC for normal operation
    //P4OUT |= BIT7;

    // Linx Pin 18 is VDD for transmit mode, connect to Pin 6.4 on MSP
    P6DIR |= BIT4;
    P6OUT |= BIT4;

    // Linx Pin 10 and 13 are VDD for appropriate channel select. Connect them to MSP pin 6.0, 6.1 respectively
    P6DIR |= BIT0 | BIT1;
    P6OUT |= BIT0 | BIT1;

    //Linx Pin 14 is level adj at VDD for max power out. Connect it to Pin 6.2 on MSP
    P6DIR |= BIT2;
    P6OUT |= BIT2;

    //Linx Pin 9 is VDD to choose transparent mode. Connect to MSP Pin 6.3
    P6DIR |= BIT3;
    P6OUT |= BIT3;
}

void set_linx_gpio_sleep()
{
    // Put Linx in Powerdown
    // Pull all VDD pins to 0
    P6OUT &= ~BIT0;
    P6OUT &= ~BIT1;
    P6OUT &= ~BIT2;
    P6OUT &= ~BIT3;
    P6OUT &= ~BIT4;
    P1DIR |= BIT7;
    P1OUT &= ~BIT7;

}

void set_UART_clk()
{
    __bis_SR_register(SCG0);                // Disable FLL
    CSCTL3 = SELREF__XT1CLK;               // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_0;// DCOFTRIM=3, DCO Range = 1MHz
    CSCTL2 = FLLD_0 + 30;                   // DCODIV = 1MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // Enable FLL
    Software_Trim();                        // Software Trim to get the best DCOFTRIM value

}

void config_UART()
{
    // Configure UART pins
    P1SEL0 |= BIT6 | BIT7;                    // set 2-UART pin as second function

    // Configure UART
    UCA0CTLW0 |= UCSWRST;                     // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK;
    // Baud Rate calculation
    UCA0BR0 = 6;                              // 1000000/115200 = 8.68 --> 1000000/9600 = 104.166666667  which is greater than 16. Hence, (1000000/(16*9600)) = 6.51041666667, decimal portion is 6 and fraction is 0.51.
    //Fraction for MCTLW reg is .166 = 0x11. UCBRFx = int ( (104.167-104)*16) = 2
    UCA0MCTLW = 0x1100 | UCOS16 | UCBRF_2;    // 1000000/115200 - INT(1000000/115200)=0.68 --> done above.
    // UCBRSx value = 0xD6 (See UG - see table 22.4 in section 22.3.10, other code example says table 17.4, that's not correct.
    UCA0BR1 = 0;
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

void set_pins_low()
{
    P1OUT = 0x00;
    P2OUT = 0x00;
    P3OUT = 0x00;
    P4OUT = 0x00;
    P5OUT = 0x00;
    P6OUT = 0x00;

    P1DIR = 0xff;
    P2DIR = 0xff;
    P3DIR = 0xff;
    P4DIR = 0xff;
    P5DIR = 0xff;
    P6DIR = 0xff;
}

void set_xt1_clk()
{
    P2SEL1 |= BIT6 | BIT7;                  // P2.6~P2.7: crystal pins
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag
}


#pragma vector = TIMER0_B0_VECTOR
__interrupt void TB0_ISR(void) {
    //    P1OUT ^= BIT0;   // Indicating interrupt
    //TB0CTL = TBCLR;
    //TB1CTL = TBCLR;
    __bic_SR_register_on_exit(LPM4_bits | GIE);
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void TB1_ISR(void) {
    //    P1OUT ^= BIT0;   // Indicating interrupt
    TB0CTL = TBCLR;
    TB1CTL = TBCLR;
    __bic_SR_register_on_exit(LPM4_bits | GIE);
}

void Software_Trim()
{
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do
    {
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do
        {
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        }while (CSCTL7 & DCOFFG);               // Test DCO fault flag

        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);// Wait FLL lock status (FLLUNLOCK) to be stable
        // Suggest to wait 24 cycles of divided FLL reference clock
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0));

        csCtl0Read = CSCTL0;                   // Read CSCTL0
        csCtl1Read = CSCTL1;                   // Read CSCTL1

        oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070)>>4;// Get DCOFTRIM value

        if(newDcoTap < 256)                    // DCOTAP < 256
        {
            newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
            if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }
        else                                   // DCOTAP >= 256
        {
            newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
            if(oldDcoTap < 256)                // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }

        if(newDcoDelta < bestDcoDelta)         // Record DCOTAP closest to 256
        {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    }while(endLoop == 0);                      // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
}
