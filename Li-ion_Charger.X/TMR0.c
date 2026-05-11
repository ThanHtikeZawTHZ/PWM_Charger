#include <xc.h>
#include "TMR0.h"

void TIM0_init(void)
{
   OPTION_REG &= ~(1<<5 |   // TMR0 Clock is FOSC/4
                  1<<3);    // Prescaler is for TMR0
   OPTION_REG &= ~(7);      // clear prescaler value
   OPTION_REG |= 3;         // prescaler = 1:16
   
   INTCON |= (1<<7) |       // GIE Enable
             (1<<6);        // PEIE Enable
}

void TIM0_Start(void)
{
    INTCON &= ~(1<<2);            // T0IF = 0;
    INTCON |= (1<<5);             // T0IE = 1;
    TMR0 = 0;
}

void TIM0_Stop(void)
{
    INTCON &= ~((1<<5) |          // T0IE = 0;
                (1<<2) );         // T0IF = 0;
    TMR0 = 0;
}
