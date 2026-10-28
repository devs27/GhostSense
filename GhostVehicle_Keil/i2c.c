#include "i2c.h"
#include "TM4C123GH6PM.h"

void I2C0_Init(void) {
    SYSCTL->RCGCI2C  |= (1<<0);
    SYSCTL->RCGCGPIO |= (1<<1);
    while(!(SYSCTL->PRGPIO & (1<<1)));

    GPIOB->AFSEL |= (1<<2)|(1<<3);    // PB2,PB3 alt function
    GPIOB->PCTL &= ~0x0000FF00;
    GPIOB->PCTL |=  0x00003300;       // I2C0 SCL/SDA mux value
    GPIOB->ODR  |= (1<<3);            // open-drain on SDA
    GPIOB->DEN  |= (1<<2)|(1<<3);

    I2C0->MCR = 0x10;                 // master mode
    I2C0->MTPR = 7;                   // ~100kHz @ 16MHz sysclk
}

void I2C0_Send(uint8_t slaveAddr, uint8_t data) {
    I2C0->MSA = (slaveAddr << 1);
    I2C0->MDR = data;
    I2C0->MCS = 0x07;                 // START+RUN+STOP
    while(I2C0->MCS & 0x01);
}
