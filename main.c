#include "io430.h"
#include <math.h>
#define LED_R BIT6       // 1.6 led R
#define LED_G BIT2       // 2.2 led G
#define LED_B BIT4       // 2.4 LED B
#define ADC10INCH INCH_5 // 1.5 Resisgtor
#define ADCPIN BIT5      // 1.5
#define PWM_MODE OUTMOD_7//Set/Reset
#define FREQ 255
#define smooth 5     //color-changing smooth
#define k1 P2IN_bit.P3  //P2.3
#define k2 P1IN_bit.P7  //P2.6
#define ctr1 P1OUT_bit.P0       //scanled P1.0
#define ctr2 P1OUT_bit.P1       //scanled P1.1
#define ctr3 P1OUT_bit.P2       //scanled P1.2
#define ctr4 P1OUT_bit.P3       //scanled P1.3
#define SHCP BIT4     // CLK    P1.4 
#define DS   BIT0     // dataIC P2.0
#define STCP BIT1     // boots  P2.1
//--------------------khai bao bien ban dau----------------------------
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
float percent,color;
int r, g, b;
float Rpercent = 10;
float Gpercent = 0;
float Bpercent = 0;
int st =0;
float value =0;
char check =0;
//----------------------------------------------------------------------
float GetADC(void)             
{
  ADC10CTL0 |= ADC10SC;
  while (ADC10CTL1 & 1);
  return ADC10MEM;
}
//----------------------------------------------------------------------
void Cycling()
{
  percent = map(GetADC(), 0, 1023, 0, 100);
  int BUFFER = smooth;        // %
  r = Rpercent* (percent/100);
  g = Gpercent* (percent/100);
  b = Bpercent* (percent/100);
    if(Rpercent<=0)
    {
      if(Bpercent >= Gpercent)  {Gpercent+= BUFFER;}
      else                      {Bpercent-= BUFFER;};
    };
    if(Gpercent<=0)
    {
      if(Rpercent >= Bpercent)  {Bpercent+= BUFFER;}
      else                      {Rpercent-= BUFFER;};
    };
    if(Bpercent<=0)
    {
      if(Gpercent >= Rpercent)  {Rpercent+= BUFFER;}
      else                      {Gpercent-= BUFFER;}
    };
}
//---------------4 led 7doan anode------------------
char maled[]={0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90,0xBF};      //G/F/E/D/C/B/A
char ctr[4] = {BIT3,BIT2,BIT1,BIT0};                                            //scanled
char buff[4] = {0,0,0,0};
int idx=0;
void importIC(char data)                                                        
{
  P2OUT &=~ STCP;
  for(int i=0;i<8;i++)
  {
    P1OUT &=~ SHCP;
    if(data & 0x80)
      P2OUT |= DS;
    else
      P2OUT &=~ DS;
    P1OUT |= SHCP;
    data <<=1;
  }
  P2OUT |= STCP;
}
void scanled()
{
  for(int z=0;z<=4;z++)
  {
    P1OUT &=~ (BIT0|BIT1|BIT2|BIT3);
    importIC(maled[buff[idx]]);
    __delay_cycles(100);
    P1OUT |= ctr[idx];
    __delay_cycles(10000);
    if(idx>=3) idx=0;
    else     idx++;
  }
  idx=0;
}
void printled(int color)
{
  buff[0] =  st;
  buff[1] =  color/100;
  buff[2] =  (color%100)/10;
  buff[3] =  color%10;
}
//-------------------------custom mode------------------------------
void RED()
{
  r = map(GetADC(), 0, 1023, 0, 255); 
  printled(r);                   //adjust precent of red
}
void GREEN()
{
  g = map(GetADC(), 0, 1023, 0, 255);
  printled(g);                   //adjust percent of green
}
void BLUE()
{
  b = map(GetADC(), 0, 1023, 0, 255);
  printled(b);                   //adjust percent of blue
}
//----------------------------main----------------------------------
int main(void)
{
  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer to prevent time out reset
//-----------------------Configure LED-----------------------
  P1DIR |= LED_R; // Set P1.6 as output for red LED
  P1SEL |= LED_R;
  P2DIR |= LED_G | LED_B; // Set P2.2 and P2.4 as outputs for green and blue LEDs
  P2SEL |= LED_G | LED_B;
//-----------------------Configure PWM-----------------------
  TA0CCR0 = FREQ; // Set period
  TA1CCR0 = FREQ;
  TA0CTL = TASSEL_2 + MC_1; // Set Timer A source to SMCLK and up mode
  TA1CTL = TASSEL_2 + MC_1;
  TA0CCTL1 = PWM_MODE;           // Set red LED to set/reset mode
  TA1CCTL1 = PWM_MODE;           // Set green LED to set/reset mode
  TA1CCTL2 = PWM_MODE;           // Set blue LED to set/reset mode
 //-----------------------Configure button-----------------------
  P2DIR &= ~(BIT3);   //Input P2.3 
  P2REN |= BIT3;
  P2OUT |= BIT3;
  P1DIR &= ~(BIT7);   //Input P1.7
  P1REN |= BIT7;
  P1OUT |= BIT7;
//-----------------------Configure ADC-----------------------
  ADC10CTL1 = ADC10INCH | ADC10SSEL_0; // 5Mhz
  ADC10AE0 |= ADCPIN;
  ADC10CTL0 = SREF_0;
  ADC10CTL0 |= ADC10ON + ENC;
  //--------------------Configure scan LED-----------------------
  P1DIR |= BIT0|BIT1|BIT2|BIT3|SHCP;
  P2DIR &=~(BIT3|BIT5|BIT6|BIT7);
  P2REN |= BIT3|BIT5|BIT6|BIT7;
  P2OUT |= BIT3|BIT5|BIT6|BIT7;
  P2DIR |= DS|STCP;
  __bis_SR_register(GIE);
  while (1)
  {
    if(st == 0)
    {
      Cycling();
      printled(map(GetADC(), 0, 1023, 0, 100));
    }
    else 
    {
      if(st == 1){RED();};
      if(st == 2){GREEN();};
      if(st == 3){BLUE();};
    }
   TA0CCR1 = r;
   TA1CCR1 = g;
   TA1CCR2 = b;
   if(k1==0)                    //k1 press
  {
    if(st >= 3)
    {st=0;}
    else st++;
    __delay_cycles(100000);
  }
   if(k2==0)                    //k2 press
  {
    if(st <= 0)
    {st=3;}
    else st--;
    __delay_cycles(100000);
  }
    scanled(); 
    __delay_cycles(1000);
  }
}
/*#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0(void)
{
  scanled();
}*/


