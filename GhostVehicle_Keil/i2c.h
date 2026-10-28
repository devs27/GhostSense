#ifndef I2C_H
#define I2C_H
#include <stdint.h>
void I2C0_Init(void);
void I2C0_Send(uint8_t slaveAddr, uint8_t data);
#endif
