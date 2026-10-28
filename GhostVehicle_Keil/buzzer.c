#include "buzzer.h"
#include "TM4C123GH6PM.h"

void Buzzer_Init(void) {
    SYSCTL->RCGCGPIO |= (1<<1);       // Port B clock
    while(!(SYSCTL->PRGPIO & (1<<1)));
    GPIOB->DIR |= (1<<5);             // PB5 output
    GPIOB->DEN |= (1<<5);
}
void Buzzer_On(void)  { GPIOB->DATA |= (1<<5); }
void Buzzer_Off(void) { GPIOB->DATA &= ~(1<<5); }
