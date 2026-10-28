#include "ultrasonic.h"
#include "TM4C123GH6PM.h"

#define TRIG_PIN      (1u << 6)       /* PB6 */
#define ECHO_PIN      (1u << 7)       /* PB7 */
#define TICKS_PER_US  16u             /* default 16 MHz system clock, no PLL */
#define MIN_PULSE_US  100u            /* shorter than ~1.7 cm = spurious / floating pin */
#define MAX_PULSE_US  38000u          /* HC-SR04 "nothing in range" pulse is ~38 ms */

void Ultrasonic_Init(void) {
    SYSCTL->RCGCGPIO |= (1<<1);                 /* Port B clock */
    while (!(SYSCTL->PRGPIO & (1<<1)));

    GPIOB->DIR |= TRIG_PIN;                     /* TRIG = output */
    GPIOB->DIR &= ~ECHO_PIN;                    /* ECHO = input  */
    GPIOB->DEN |= TRIG_PIN | ECHO_PIN;

    /* Timer0A as a free-running 32-bit counter: 1 tick = 1/16 microsecond */
    SYSCTL->RCGCTIMER |= (1<<0);
    while (!(SYSCTL->PRTIMER & (1<<0)));

    TIMER0->CTL  &= ~(1<<0);                    /* disable while configuring */
    TIMER0->CFG   = 0x00;                       /* 32-bit mode */
    TIMER0->TAMR  = 0x02;                       /* periodic, counting up */
    TIMER0->TAILR = 0xFFFFFFFF;
    TIMER0->CTL  |= (1<<0);                     /* enable */
}

static void Delay_us(uint32_t us) {
    uint32_t start = TIMER0->TAV;
    uint32_t ticks = us * TICKS_PER_US;
    while ((TIMER0->TAV - start) < ticks);
}

uint16_t Ultrasonic_GetDistanceCM(void) {
    uint32_t t0, startTicks, pulse_us;

    /* clean 10 us trigger pulse */
    GPIOB->DATA &= ~TRIG_PIN;
    Delay_us(2);
    GPIOB->DATA |= TRIG_PIN;
    Delay_us(10);
    GPIOB->DATA &= ~TRIG_PIN;

    /* wait for ECHO to rise (30 ms timeout) */
    t0 = TIMER0->TAV;
    while (!(GPIOB->DATA & ECHO_PIN)) {
        if ((TIMER0->TAV - t0) > (30000u * TICKS_PER_US)) return ULTRASONIC_NO_ECHO;
    }
    startTicks = TIMER0->TAV;

    /* wait for ECHO to fall (40 ms timeout) */
    while (GPIOB->DATA & ECHO_PIN) {
        if ((TIMER0->TAV - startTicks) > (40000u * TICKS_PER_US)) return ULTRASONIC_NO_ECHO;
    }
    pulse_us = (TIMER0->TAV - startTicks) / TICKS_PER_US;

    if (pulse_us < MIN_PULSE_US || pulse_us > MAX_PULSE_US) return ULTRASONIC_NO_ECHO;

    return (uint16_t)(pulse_us / 58u);          /* HC-SR04: cm = microseconds / 58 */
}
