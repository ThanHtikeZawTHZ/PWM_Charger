#include <xc.h>
#include "PWM.h"

#define _XTAL_FREQ 4000000

void PWM_Init(PWM_TypeDef *PWMData)
{
    char presc[3]={1,4,16};
    char presc_val[]={0,1,2};
    char index;
    unsigned long Fclk;
    unsigned int PR2_Val;
    for(index=0;index<3;index++)
    {
        Fclk = _XTAL_FREQ /(presc[index]*4);
        PR2_Val= PWMData->Period*Fclk/1000000 -1;
        if (PR2_Val<=255)
            break;
    }
    PR2=PR2_Val;
    T2CON = presc_val[index];
    if ( 1==PWMData->CCP )
    {
        CCPR1L = PWMData->Ton>>2;
        CCP1CON= (PWMData->Ton &0x3)<<4 | // Ton LSB 2 bits
                 0xC;  // PWM Mode
        TRISC &= ~(1<<2);  // RC2 - Output
    }
    else
    {
        CCPR2L = PWMData->Ton>>2;
        CCP2CON= (PWMData->Ton &0x3)<<4 | // Ton LSB 2 bits
                 0xC;  // PWM Mode
         TRISC &= ~(1<<1);  // RC1 - Output
    }
}

void PWM_Start(void)
{
    T2CON |= 1<<2;
}

void PWM_Stop(void)
{
    T2CON &= ~(1<<2);
}

void PWM_SetPeriod(PWM_TypeDef * PWMData)
{
    if ( 1==PWMData->CCP )
    {
        CCPR1L = PWMData->Ton>>2;
        CCP1CON= (PWMData->Ton &0x3)<<4 | // Ton LSB 2 bits
                 0xC;  // PWM Mode
       // TRISC &= ~(1<<2);  // RC2 - Output
    }
    else
    {
        CCPR2L = PWMData->Ton>>2;
        CCP2CON= (PWMData->Ton &0x3)<<4 | // Ton LSB 2 bits
                 0xC;  // PWM Mode
       //  TRISC &= ~(1<<1);  // RC1 - Output
    }
}

void PWM_SetPulseWidth(PWM_TypeDef * PWMData)
{
    
}

void PWM_SetFrequency(unsigned int Freq)
{
    unsigned int Period = 1000000L/Freq;
    char presc[3]={1,4,16};
    char presc_val[]={0,1,2};
    char index;
    unsigned long Fclk;
    unsigned int PR2_Val;
    for(index=0;index<3;index++)
    {
        Fclk = _XTAL_FREQ /(presc[index]*4);
        PR2_Val= Period*Fclk/1000000 -1;
        if (PR2_Val<=255)
            break;
    }
    PR2=PR2_Val;
    T2CON &= ~(3);  // clear TMR2 prescaler
    T2CON |= presc_val[index];
}

void PWM_OutputEnable(CCPModule M)
{
    
}

void PWM_OutputDisable(CCPModule M)
{
    
}

inline void PWM_InterruptEnabled(void)
{
    
}

