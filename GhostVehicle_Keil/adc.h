#ifndef ADC_H
#define ADC_H
#include <stdint.h>
void ADC0_Init3Ch(void);
void ADC0_ReadAll(uint16_t *buf);   // buf[0]=Front, buf[1]=BackRight, buf[2]=BackLeft
#endif
