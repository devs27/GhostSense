#include "adc.h"
#include "TM4C123GH6PM.h"

void ADC0_Init3Ch(void) {
    SYSCTL->RCGCADC  |= (1<<0);
    SYSCTL->RCGCGPIO |= (1<<4);
    while(!(SYSCTL->PRGPIO & (1<<4)));

    GPIOE->AFSEL |= 0x0E;             // PE1,PE2,PE3 alt function
    GPIOE->DEN   &= ~0x0E;
    GPIOE->AMSEL |= 0x0E;

    ADC0->ACTSS  &= ~(1<<1);          // disable SS1 to configure
    ADC0->EMUX   &= ~0x00F0;          // SS1 = software trigger
    ADC0->SSMUX1  = 0x00000210;       // CORRECTED: sample0=AIN0(Front,PE3), sample1=AIN1(BackRight,PE2), sample2=AIN2(BackLeft,PE1)
    ADC0->SSCTL1  = (1<<10);          // END2 set on 3rd sample (index 2)
    ADC0->IM     &= ~(1<<1);          // polling, no interrupt
    ADC0->ACTSS  |= (1<<1);
}

void ADC0_ReadAll(uint16_t *buf) {
    ADC0->PSSI |= (1<<1);             // trigger SS1
    while(!(ADC0->RIS & (1<<1)));     // wait for conversion sequence done
    buf[0] = ADC0->SSFIFO1 & 0xFFF;   // Front
    buf[1] = ADC0->SSFIFO1 & 0xFFF;   // Back-Right
    buf[2] = ADC0->SSFIFO1 & 0xFFF;   // Back-Left
    ADC0->ISC = (1<<1);               // clear completion flag
}
