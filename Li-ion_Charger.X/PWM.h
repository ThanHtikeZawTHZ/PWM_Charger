/* 
 * File:   PWM.h
 * Author: thanh
 *
 * Created on June 15, 2022, 11:36 PM
 */

#ifndef PWM_H
#define	PWM_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef   enum {CCPM1=1,CCPM2=2}CCPModule;
typedef struct
{
    unsigned int  Period;
    unsigned int  Ton;
    unsigned char CCP;
}PWM_TypeDef;

void PWM_Init(PWM_TypeDef *PWMData);
void PWM_Start(void);
void PWM_Stop(void);
void PWM_SetPeriod(PWM_TypeDef * PWMData);
void PWM_SetPulseWidth(PWM_TypeDef * PWMData);
void PWM_SetFrequency(unsigned int Freq);
void PWM_OutputEnable(CCPModule M);
void PWM_OutputDisable(CCPModule M);
inline void PWM_InterruptEnabled(void);



#ifdef	__cplusplus
}
#endif

#endif	/* PWM_H */

