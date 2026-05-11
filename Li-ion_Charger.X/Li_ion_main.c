/*
 * File:   Li_ion_main.c
 * Author: Than Htike Zaw
 *
 * Created on May 9, 2024, 6:14 PM
 */


#include <xc.h>// PIC16F883 Configuration Bit Settings
// 'C' source line config statements
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Reset Selection bits (BOR enabled)
#pragma config IESO = ON        // Internal External Switchover bit (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "TMR0.h"
#include "PWM.h"

PWM_TypeDef PWM_1;

#define _XTAL_FREQ  4000000
// Use single bit data to assign which segment to display
#define _A      (1<<5)    // bit 0 for Seg_A Display
#define _B      (1<<7)    // bit 1 for Seg_B Display
#define _C      (1<<1)    // bit 2 for Seg_C Display
#define _D      (1<<3)    // bit 3 for Seg_D Display
#define _E      (1<<4)    // bit 4 for Seg_E Display
#define _F      (1<<6)    // bit 5 for Seg_F Display
#define _G      (1<<0)    // bit 6 for Seg_G Display
#define _DP     (1<<2)    // bit 7 for Seg_G Display

#define _Digit_1    7
#define _Digit_2    6
#define _Digit_3    5
#define _Digit_4    4

#define _P25	(1<<4)
#define _P50	(1<<5)
#define _P75	(1<<6)
#define _P100	(1<<7)
#define _SD		(1<<3)

#define ADC_14	512

#define Solar_Detect    0

#define Current_Limit   20  //(20 - 2.0 Amp)
#define Current_Full_Limit 2    // (2 - 0.2 Amp)
// Built Segment Table Array to make ease access to get Display Data
const char Segment_Tab[24]={  _A | _B |_C | _D | _E | _F     , //0
                                   _B |_C                    , //1
                              _A | _B     | _D | _E      | _G, //2
                              _A | _B |_C | _D           | _G, //3
                                   _B |_C |           _F | _G, //4
                              _A |     _C | _D |      _F | _G, //5
                              _A |     _C | _D | _E | _F | _G, //6
                              _A | _B |_C                   , //7
                              _A | _B |_C | _D | _E | _F | _G, //8
                              _A | _B |_C | _D |      _F | _G, //9
                                       _C | _D | _E      | _G, //o		//10
                                       _C | _D | _E			 , //v/u	//11
                                                 _E      | _G, //r		//12
                                   _B |_C | _D | _E      | _G, //d		//13
                                                           _G, //-		//14
                              _A |     _C | _D      | _F | _G, //S	    //15
                              _A | _B          | _E | _F | _G, //P		//16
                                            _D | _E | _F     , //L		// 17
                              _P25,		//18
                              _P25 | _P50,		//19
                              _P25 | _P50 | _P75,		//20
                              _P25 | _P50 | _P75 | _P100,		//21
                              _SD,	//22
                              0  };//F Blank

char Display_Array[4]={1,2,3,19};
char SolarFlag=0;
enum State{Wait_Charge=0,Current_Charge,Volt_Charge,Full_Charge};
char State=Wait_Charge;
unsigned int PWM_Value=0;
struct{
   char Change:1;
   char Next:1;
}Flag;


void Display_Segment(char* Display_Array);
void Data2Array(char* d_Array,unsigned int data);
void WaitCharge_Function(void);
void CurrentCharge_Function(void);
void VoltCharge_Function(void);
void FullCharge_Function(void);
unsigned int AD_Value=0;
unsigned int Display_Volt=0;
unsigned int Display_Current=0;

char Display_Change=0;
void main(void) 
{
    TIM0_init();
    TIM0_Start();
    ANSELH=0;
    TRISB =0;
    TRISC &= ~(0xF0);
    
    TRISC |= (1<<Solar_Detect); // Solar Detect pin as input
    
    ANSEL |= (1<<0);    // AN0 as analog
    ANSEL |= (1<<1);    // AN1 as analog
    TRISA |= (1<<0) | (1<<1);    // RA0 and RA1 as input
    ADCON0 |= (2<<6);   // FOSC/32
    ADCON0 &= ~(0x0F<<2);   // clear Channel
    //ADCON0 |= (0<<2);   // Channel 0(AN0)
    ADCON0 |= (1<<2);   // Channel 1(AN1)
    ADCON1 |=(1<<7);    // Right Justify
    ADCON0 |= (1<<0);   // AD ON

    PWM_1.CCP=CCPM1;
    PWM_1.Period=1000;
    PWM_1.Ton=0;
    PWM_Init(&PWM_1);
    PWM_Start();
    while(1)
    {
        switch(State)
        {
            case Wait_Charge:
                WaitCharge_Function();
                break;
            case Current_Charge:
                CurrentCharge_Function();
                break;
            case Volt_Charge:
                VoltCharge_Function();
                break;
            case Full_Charge:
                FullCharge_Function();
                break;
            default:
                State=Wait_Charge;
                break;
        }
        
    }
}

void WaitCharge_Function(void)
{
    unsigned int Volt_Value=0;
    char Total_Count=0;
    Volt_Value=0;
    PWM_Value=0;
    CCPR1L = PWM_Value>>2;
    CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
             0xC;  // PWM Mode
    while(State==Wait_Charge)
    {
        if(!(PORTC & (1<<Solar_Detect)))
        {
            SolarFlag=1;
            State=Current_Charge;
        }
        else
        {
            SolarFlag=0;
            
        }
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (1<<2);   // Channel 1(AN1)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Volt_Value+=(AD_Value>>4);
        Total_Count++;
        if(Total_Count>=4)
        {
            Total_Count=0;
            Volt_Value=Volt_Value>>2;
            Display_Volt=((long)140*Volt_Value)/ADC_14;        
            Data2Array(Display_Array,Display_Volt);
            Volt_Value=0;
        }
        if(Display_Volt >=120)
        {
            Display_Array[3] = 21;
        }
        else if(Display_Volt >= 114)
        {
            Display_Array[3] = 20;
        }
        else if(Display_Volt >= 108)
        {
            Display_Array[3] = 19;
        }
        else if(Display_Volt >= 102)
        {
            Display_Array[3] = 18;
        }
        else
        {
            Display_Array[3] = 23;
        }
        //__delay_ms(1000);
    }
}

void CurrentCharge_Function(void)
{
    unsigned int Volt_Value=0;
    unsigned int Current_Value=0;
    char Total_Count=0;
    char C_Count=0;
    PWM_Value=0;
    CCPR1L = PWM_Value>>2;
    CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
             0xC;  // PWM Mode
    while(State==Current_Charge)
    {
        if(!(PORTC & (1<<Solar_Detect)))
        {
            SolarFlag=1;
        }
        else
        {
            SolarFlag=0;
            State=Wait_Charge;
        }
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (0<<2);   // Channel 0(AN0)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Current_Value+=(AD_Value>>4);
        C_Count++;
        if(C_Count>=4)
        {
            C_Count=0;
            Current_Value=Current_Value>>2;
//            Current_Value+=2;
            //Display_Current=Current_Value;
//            if(Current_Value>=512)
//            {
//                Display_Current=(Current_Value-512)*50/100;
//            }
//            else
//            {
//                Display_Current=(512-Current_Value)*50/100;
//            }
//            Current_Value=0;
            unsigned long A_To_V = ((unsigned long)Current_Value * 500) / 1023;
            Display_Current = (A_To_V * 10) / 24;
            Current_Value = 0;
            if(Display_Current < Current_Limit && PWM_Value < 900)
            {
                PWM_Value++;
                CCPR1L = PWM_Value>>2;
                CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
                         0xC;  // PWM Mode
            }
            else if(Display_Current > Current_Limit && PWM_Value > 5)
            {
                PWM_Value--;
                CCPR1L = PWM_Value>>2;
                CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
                         0xC;  // PWM Mode
            }
            else
            {
                // do nothing
            }
        }
        
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (1<<2);   // Channel 1(AN1)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Volt_Value+=(AD_Value>>4);
        Total_Count++;
        if(Total_Count>=4)
        {
            Total_Count=0;
            Volt_Value=Volt_Value>>2;
            Display_Volt=((long)140*Volt_Value)/ADC_14;  
            if(Display_Volt>=126)
            {
                State=Volt_Charge;
            }
            
            Volt_Value=0;
        }
        if(Display_Volt >=120)
        {
            Display_Array[3] = 21;
        }
        else if(Display_Volt >= 114)
        {
            Display_Array[3] = 20;
        }
        else if(Display_Volt >= 108)
        {
            Display_Array[3] = 19;
        }
        else if(Display_Volt >= 102)
        {
            Display_Array[3] = 18;
        }
        else
        {
            Display_Array[3] = 23;
        }
        if(Display_Change==0)
        {
            Data2Array(Display_Array,Display_Current);
        }
        else if(Display_Change==1)
        {
            Data2Array(Display_Array,Display_Volt);
        }
    }
}

void VoltCharge_Function(void)
{
    unsigned int Volt_Value=0;
    unsigned int Current_Value=0;
    char Total_Count=0;
    char C_Count=0;
    //PWM_Value=0;
    CCPR1L = PWM_Value>>2;
    CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
             0xC;  // PWM Mode
    while(State==Volt_Charge)
    {
        if(!(PORTC & (1<<Solar_Detect)))
        {
            SolarFlag=1;
        }
        else
        {
            SolarFlag=0;
            State=Wait_Charge;
        }
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (0<<2);   // Channel 0(AN0)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Current_Value+=(AD_Value>>4);
        C_Count++;
        if(C_Count>=4)
        {
            C_Count=0;
            Current_Value=Current_Value>>2;
            //Current_Value+=2;
            //Display_Current=Current_Value;
//            if(Current_Value>=512)
//            {
//                Display_Current=(Current_Value-512)*50/100;
//            }
//            else
//            {
//                Display_Current=(512-Current_Value)*50/100;
//            }
//            Current_Value=0;
            unsigned long A_To_V = ((unsigned long)Current_Value * 500) / 1023;
            Display_Current = (A_To_V * 10) / 24;
            Current_Value = 0;
            if(Display_Current <= Current_Full_Limit)
            {
                State=Full_Charge;
            }
        }
        
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (1<<2);   // Channel 1(AN1)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Volt_Value+=(AD_Value>>4);
        Total_Count++;
        if(Total_Count>=4)
        {
            Total_Count=0;
            Volt_Value=Volt_Value>>2;
            Display_Volt=((long)140*Volt_Value)/ADC_14;  
            if(Display_Volt<126 && PWM_Value<900 && Display_Current < Current_Limit)
            {
                PWM_Value++;
                CCPR1L = PWM_Value>>2;
                CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
                         0xC;  // PWM Mode
            }
            else if(Display_Volt>126 && PWM_Value>5)
            {
                PWM_Value--;
                CCPR1L = PWM_Value>>2;
                CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
                         0xC;  // PWM Mode
            }
            else
            {
                // do nothing
            }
            if(Display_Current > Current_Limit && PWM_Value > 5)
            {
                PWM_Value--;
                CCPR1L = PWM_Value>>2;
                CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
                         0xC;  // PWM Mode
            }
            
            Volt_Value=0;
        }
        if(Display_Volt >=120)
        {
            Display_Array[3] = 21;
        }
        else if(Display_Volt >= 114)
        {
            Display_Array[3] = 20;
        }
        else if(Display_Volt >= 108)
        {
            Display_Array[3] = 19;
        }
        else if(Display_Volt >= 102)
        {
            Display_Array[3] = 18;
        }
        else
        {
            Display_Array[3] = 23;
        }
        if(Display_Change==0)
        {
            Data2Array(Display_Array,Display_Current);
        }
        else if(Display_Change==1)
        {
            Data2Array(Display_Array,Display_Volt);
        }
    }
}

void FullCharge_Function(void)
{
    unsigned int Volt_Value=0;
    char Total_Count=0;
    Volt_Value=0;
    PWM_Value=0;
    CCPR1L = PWM_Value>>2;
    CCP1CON= (PWM_Value &0x3)<<4 | // Ton LSB 2 bits
             0xC;  // PWM Mode
    while(State==Full_Charge)
    {
        if(!(PORTC & (1<<Solar_Detect)))
        {
            SolarFlag=1;
            
        }
        else
        {
            SolarFlag=0;
            
        }
        ADCON0 &= ~(0x0F<<2);   // clear ADC Channel
        ADCON0 |= (1<<2);   // Channel 1(AN1)
        AD_Value=0;
        for(char i=0;i<16;i++)
        {
            ADCON0 |= (1<<1);   // Set Go/Done bit
            while(ADCON0 & (1<<1))
                ;
            AD_Value+=(int)ADRESH<<8 | ADRESL; 
            __delay_ms(1);
        }
        Volt_Value+=(AD_Value>>4);
        Total_Count++;
        if(Total_Count>=4)
        {
            Total_Count=0;
            Volt_Value=Volt_Value>>2;
            Display_Volt=((long)140*Volt_Value)/ADC_14;        
            Data2Array(Display_Array,Display_Volt);
            Volt_Value=0;
        }
        if(Display_Volt < 126 && SolarFlag == 1)
        {
            State = Current_Charge;
        }
        if(Display_Volt >=120)
        {
            Display_Array[3] = 21;
        }
        else if(Display_Volt >= 114)
        {
            Display_Array[3] = 20;
        }
        else if(Display_Volt >= 108)
        {
            Display_Array[3] = 19;
        }
        else if(Display_Volt >= 102)
        {
            Display_Array[3] = 18;
        }
        else
        {
            Display_Array[3] = 23;
        }
        //__delay_ms(1000);
    }
}

void __interrupt() ISR(void)
{
    static int Display_Count=0;
    if(INTCON & (1<<2)) // TOIF==1
    {
        INTCON &= ~(1<<2);  // clear T0IF Flag
        Display_Segment(Display_Array);
        Display_Count++;
        if(Display_Count>=1000)
        {
            Display_Count=0;
            Display_Change^=1;
        }
    }
}

//===========================================================
void Display_Segment(char* Display_Array)
{
    static char count=1;  // Segment number to light up data
    PORTB = 0;
    switch(count)
    {
        case 1:
		  PORTC &= ~(1<<_Digit_4);
          if(Display_Array[0] != 0)
          {
			  PORTB = Segment_Tab[Display_Array[0]]; 
          }    
          else
          {
              PORTB = Segment_Tab[23];
          }             
          count=2;
          PORTC |= (1<<_Digit_1);
          break;
        case 2:
          PORTC &= ~(1<<_Digit_1);
          PORTB = Segment_Tab[Display_Array[1]];
          PORTB |= _DP;
          count=3;
          PORTC |= (1<<_Digit_2);
          break;
        case 3:
          PORTC &= ~(1<<_Digit_2);
          PORTB = Segment_Tab[Display_Array[2]];
          count=4;
          PORTC |= (1<<_Digit_3);
          break;
	    case 4:
          PORTC &= ~(1<<_Digit_3);          
          if(SolarFlag==1)
          {
              PORTB = Segment_Tab[Display_Array[3]] | _SD;
          }
          else
          {
              PORTB = Segment_Tab[Display_Array[3]];
          }
          count=1;
          PORTC |= (1<<_Digit_4);
          break;
        default:
          count = 1;  // reset to 0 if any error occur
          break;
    }
}

// Input Argument 2 - Display Data
// Input Argument 1 - Display Array but return in pointer type
void Data2Array(char* d_Array,unsigned int data)
{
    d_Array[0]=0;   // clear display array[0]
    while(data >= 100)  // while data is hundred value
    {
        data-=100;
        d_Array[0]++;
    }

    d_Array[1]=0;   // clear display array[1]
    while(data >= 10)  // while data is ten value
    {
        data-=10;
        d_Array[1]++;
    }

    // last decimal data is placed in last Array
    d_Array[2]=data;

}

//===========================================================



