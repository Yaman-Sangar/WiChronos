/* Receive data through Linx transmitter encoded as the delay time between two packets */
#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int addr = 0;
void Software_Trim();                       // Software Trim to get the best DCOFTRIM value
void set_pins_low();
void set_UART_clk();
void config_UART();
void set_xt1_clk();
void config_timer(void);
#define MCLK_FREQ_MHZ 1                     // MCLK = 1MHz

unsigned int RXData[100], new_data=0, pre=0, new=0;
unsigned int TXData = 0xAA, end=1, fend=100, new1;
unsigned int i=0, j=0, k=0, counting = 0, RSSI_count = 0, rx = 0, rssi = 0;
unsigned long CountVal[800];

// CountVal holds the timer value which is, in effect, the data
// j is the number of packets received

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
    set_pins_low();
    set_xt1_clk();
    set_UART_clk();
    config_UART();
    config_timer();

    CSCTL4 = SELA__XT1CLK | SELMS__DCOCLKDIV;    // Select Aux clock as XT1(32kHz) and Main clock as DCO (1MHz)
    //TB0CCTL0 |= CCIE;                            // TBCCR0 interrupt enabled

    // Configure ADC A1 pin
    P1SEL0 |= BIT1;
    P1SEL1 |= BIT1;

    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // Configure ADC
    ADCCTL0 |= ADCSHT_2 | ADCON;                             // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                       // ADCCLK = MODOSC; sampling timer
    ADCCTL2 &= ~ADCRES;                                      // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2;                                     // 12-bit conversion results
    ADCMCTL0 |= ADCINCH_1;                                   // A1 ADC input select; Vref=AVCC
    ADCIE |= ADCIE0;                                         // Enable ADC conv complete interrupt


    while (1)
    {
                //ADCCTL0 |= ADCENC | ADCSC;                           // Sampling and conversion start
                //while(ADCIFG & BIT0);

        //        P1OUT = 0;
        //        __bis_SR_register(LPM0_bits | GIE);                  // LPM0, ADC_ISR will force exit

        __bis_SR_register(LPM0_bits | GIE);         // Enter LPM3 w/ interrupt
        //ADCCTL0 |= ADCENC | ADCSC;                           // Sampling and conversion start
        // __bis_SR_register(LPM0_bits | GIE);                  // LPM0, ADC_ISR will force exit
        __no_operation();                           // For debugging
        //        P1OUT = BIT0;
    }
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
                 // Stops the timer
//        ADCCTL0 |= ADCENC | ADCSC;                           // Sampling and conversion start
//        while(ADCIFG & BIT0);
//        ADC_Result[i] = ADCMEM0;        // This clears ADC flag as well
        UCA0IFG &=~ UCRXIFG;            // Clear UART interrupt
        RXData[i] = UCA0RXBUF;             // Clear buffer
        pre = RXData[fend+i-1];
        new = RXData[i]<<8;

        new_data = pre|new;
        new1 = new_data^0xAA55;
       if((new_data^0xAA55) == 0x0000){
           CountVal[j] = TB0R;
           TB0CTL &= ~MC_3;
//           result[j] = RXData[i];
           P1OUT ^= BIT0;
//           ADC_Result1[j] = ADC_Result[i];
           //cnt[j] = i;
           //time[j] = CountVal[i];
           j++;
           TB0CTL = TBCLR;         // Clear Timer
           TB0CCR0 = 0xFFFF;
           TB0CTL = ID__1 | TBSSEL__ACLK | MC__UP | TBCLR; // ACLK, Continuous mode
       }


       //__bic_SR_register_on_exit(LPM3_bits | GIE);         // Exit LPM3 w/ interrupt

       i++;
       if(i == 100){
           i = 0;
           fend = 100;
       }
       else
           fend = 0;

        break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    }
}

void config_timer(void)
{
    TB0CTL = ID__1 | TBSSEL__ACLK | MC__UP;     // Input divider /4, ACLK, UP mode
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
    case ADCIV_NONE:
        break;
    case ADCIV_ADCOVIFG:
        break;
    case ADCIV_ADCTOVIFG:
        break;
    case ADCIV_ADCHIIFG:
        break;
    case ADCIV_ADCLOIFG:
        break;
    case ADCIV_ADCINIFG:
        break;
    case ADCIV_ADCIFG:
//        ADC_Result[i] = ADCMEM0;
        //ADC_Result1[i] = ADCMEM0;
        RSSI_count++;
//        i++;
        //            P1OUT = 0;
        //__bic_SR_register_on_exit(LPM3_bits | GIE);            // Clear CPUOFF bit from LPM0
        break;
    default:
        break;
    }
}


void config_UART()
{
    // Configure UART pins
    P1SEL0 |= BIT6 | BIT7;                    // set 2-UART pin as second function

    // Configure UART
    UCA0CTLW0 |= UCSWRST;                     // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK | UCRXEIE | UCBRKIE;
    // Baud Rate calculation
    UCA0BR0 = 6;                              // 1000000/115200 = 8.68 --> 1000000/9600 = 104.166666667  which is greater than 16. Hence, (1000000/(16*9600)) = 6.51041666667, decimal portion is 6 and fraction is 0.51.
    //Fraction for MCTLW reg is .166 = 0x11. UCBRFx = int ( (104.167-104)*16) = 2
    UCA0MCTLW = 0x1100 | UCOS16 | UCBRF_2;    // 1000000/115200 - INT(1000000/115200)=0.68 --> done above.
    // UCBRSx value = 0xD6 (See UG - see table 22.4 in section 22.3.10, other code example says table 17.4, that's not correct.
    UCA0BR1 = 0;
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
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

void set_pins_low()
{
    P1DIR = 0xff;
    P2DIR = 0xff;
    P3DIR = 0xff;
    P4DIR = 0xff;
    P5DIR = 0xff;
    P6DIR = 0xff;

    P1OUT = 0x00;
    P2OUT = 0x00;
    P3OUT = 0x00;
    P4OUT = 0x00;
    P5OUT = 0x00;
    P6OUT = 0x00;

}

void set_UART_clk()
{
    __bis_SR_register(SCG0);                // Disable FLL
    CSCTL3 = SELREF__XT1CLK;                // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_0;// DCOFTRIM=3, DCO Range = 1MHz
    CSCTL2 = FLLD_0 + 30;                   // DCODIV = 1MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // Enable FLL
    Software_Trim();                        // Software Trim to get the best DCOFTRIM value
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
