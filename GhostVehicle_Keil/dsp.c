#include "dsp.h"
#include "adc.h"
#include "TM4C123GH6PM.h"

/* Band-pass biquad, fs = 8 kHz, f0 = 1 kHz, Q = 0.7, coefficients in Q14.
   y[n] = ( B0*x[n] + B2*x[n-2] - A1*y[n-1] - A2*y[n-2] ) >> 14      (B1 = 0)
   Zero gain at DC and at Nyquist, so mains hum, rumble and hiss are rejected.
   To move the band, recompute with the RBJ band-pass formulas. */
#define COEF_B0    5498
#define COEF_B2   (-5498)
#define COEF_A1  (-15395)
#define COEF_A2    5388

#define SETTLE_SAMPLES   32u    /* skip the filter start-up transient in the RMS */
#define HIST_LEN          4u

typedef struct {
    uint16_t rms;
    uint16_t hist[HIST_LEN];
    uint8_t  histIdx;
    uint16_t avg;
    uint16_t baseline;
    uint16_t thrOn;
    uint16_t thrOff;
    uint8_t  active;
} Chan_t;

static Chan_t   ch[DSP_CHANNELS];
static int16_t  buf[DSP_CHANNELS][DSP_BURST_LEN];

static uint32_t ISqrt(uint32_t v) {
    uint32_t res = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= res + bit) { v -= res + bit; res = (res >> 1) + bit; }
        else                { res >>= 1; }
        bit >>= 2;
    }
    return res;
}

void Dsp_Init(void) {
    uint32_t clk = SystemCoreClock ? SystemCoreClock : 16000000u;
    SysTick->LOAD = (clk / DSP_FS_HZ) - 1u;
    SysTick->VAL  = 0;
    SysTick->CTRL = 5u;                          /* core clock, enabled, no interrupt */
}

/* Capture one burst on all three channels at DSP_FS_HZ. */
static void CaptureBurst(void) {
    uint16_t raw[3];
    uint16_t n;
    (void)SysTick->CTRL;                         /* clear any stale COUNTFLAG */
    for (n = 0; n < DSP_BURST_LEN; n++) {
        while (!(SysTick->CTRL & (1u << 16)));   /* wait for the next 125 us tick */
        ADC0_ReadAll(raw);
        buf[0][n] = (int16_t)raw[0];
        buf[1][n] = (int16_t)raw[1];
        buf[2][n] = (int16_t)raw[2];
    }
}

/* DC removal + band-pass + RMS for one channel of the captured burst. */
static uint16_t ProcessChannel(uint8_t c) {
    int32_t  sum = 0, mean;
    int32_t  x, x1 = 0, x2 = 0, y = 0, y1 = 0, y2 = 0;
    uint64_t sumsq = 0;
    uint16_t n;

    for (n = 0; n < DSP_BURST_LEN; n++) sum += buf[c][n];
    mean = sum / (int32_t)DSP_BURST_LEN;

    for (n = 0; n < DSP_BURST_LEN; n++) {
        x = (int32_t)buf[c][n] - mean;
        y = (COEF_B0 * x + COEF_B2 * x2 - COEF_A1 * y1 - COEF_A2 * y2) >> 14;
        if (y >  8191) y =  8191;
        if (y < -8191) y = -8191;
        x2 = x1;  x1 = x;
        y2 = y1;  y1 = y;
        if (n >= SETTLE_SAMPLES) sumsq += (uint64_t)(y * y);
    }
    return (uint16_t)ISqrt((uint32_t)(sumsq / (DSP_BURST_LEN - SETTLE_SAMPLES)));
}

static void AllChannelsOneBurst(void) {
    uint8_t c;
    CaptureBurst();
    for (c = 0; c < DSP_CHANNELS; c++) ch[c].rms = ProcessChannel(c);
}

void Dsp_Calibrate(void) {
    uint32_t sum[DSP_CHANNELS]   = {0, 0, 0};
    uint32_t sumsq[DSP_CHANNELS] = {0, 0, 0};
    uint8_t  c, i;
    uint32_t mean, var, sigma, margin;

    AllChannelsOneBurst();                       /* discard the first burst */
    AllChannelsOneBurst();

    for (i = 0; i < DSP_CAL_BURSTS; i++) {
        AllChannelsOneBurst();
        for (c = 0; c < DSP_CHANNELS; c++) {
            sum[c]   += ch[c].rms;
            sumsq[c] += (uint32_t)ch[c].rms * ch[c].rms;
        }
    }

    for (c = 0; c < DSP_CHANNELS; c++) {
        mean = sum[c] / DSP_CAL_BURSTS;
        var  = (sumsq[c] / DSP_CAL_BURSTS);
        var  = (var > mean * mean) ? (var - mean * mean) : 0u;
        sigma  = ISqrt(var);
        margin = DSP_K_SIGMA * sigma + DSP_MIN_MARGIN;

        ch[c].baseline = (uint16_t)mean;
        ch[c].thrOn    = (uint16_t)(mean + margin);
        ch[c].thrOff   = (uint16_t)(mean + (margin * 2u) / 3u);   /* hysteresis */
        ch[c].avg      = (uint16_t)mean;
        ch[c].active   = 0;
        ch[c].histIdx  = 0;
        for (i = 0; i < HIST_LEN; i++) ch[c].hist[i] = (uint16_t)mean;
    }
}

void Dsp_Update(void) {
    uint8_t  c, i;
    uint32_t s;

    AllChannelsOneBurst();
    for (c = 0; c < DSP_CHANNELS; c++) {
        ch[c].hist[ch[c].histIdx] = ch[c].rms;
        ch[c].histIdx = (uint8_t)((ch[c].histIdx + 1u) % HIST_LEN);
        s = 0;
        for (i = 0; i < HIST_LEN; i++) s += ch[c].hist[i];
        ch[c].avg = (uint16_t)(s / HIST_LEN);

        if (!ch[c].active && ch[c].avg > ch[c].thrOn)  ch[c].active = 1;
        else if (ch[c].active && ch[c].avg < ch[c].thrOff) ch[c].active = 0;
    }
}

uint16_t Dsp_Rms(uint8_t c)       { return ch[c].rms; }
uint16_t Dsp_Avg(uint8_t c)       { return ch[c].avg; }
uint16_t Dsp_Threshold(uint8_t c) { return ch[c].thrOn; }
uint16_t Dsp_Baseline(uint8_t c)  { return ch[c].baseline; }
uint8_t  Dsp_Active(uint8_t c)    { return ch[c].active; }

uint8_t Dsp_AnyActive(void) {
    return (uint8_t)(ch[0].active | ch[1].active | ch[2].active);
}

uint8_t Dsp_Strongest(void) {
    uint8_t c, best = 0;
    int32_t e, bestE = -100000;
    for (c = 0; c < DSP_CHANNELS; c++) {
        e = (int32_t)ch[c].avg - (int32_t)ch[c].baseline;
        if (e > bestE) { bestE = e; best = c; }
    }
    return best;
}
