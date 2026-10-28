#ifndef ULTRASONIC_H
#define ULTRASONIC_H
#include <stdint.h>

#define ULTRASONIC_NO_ECHO 0xFFFFu   /* no echo, echo too short (spurious) or out of range */

void     Ultrasonic_Init(void);
uint16_t Ultrasonic_GetDistanceCM(void);

#endif
