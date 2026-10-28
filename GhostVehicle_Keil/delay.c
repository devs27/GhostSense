#include "delay.h"
#include "TM4C123GH6PM.h"

void Delay_ms(uint32_t ms) {
    volatile uint32_t count;
    uint32_t i;
    for (i = 0; i < ms; i++) {
        count = 3180;
        while (count--);
    }
}
