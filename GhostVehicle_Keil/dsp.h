#ifndef DSP_H
#define DSP_H
#include <stdint.h>

/* Acoustic signal chain for the three MAX4466 microphones.
   Per burst and per channel:
     1. sample 256 points at a fixed 8 kHz (paced by SysTick)
     2. remove the DC bias (subtract the burst mean)
     3. band-pass filter, biquad centred about 1 kHz (Q = 0.7), integer maths only
     4. RMS of the filtered samples
   Then a 4-burst moving average, and a detector with hysteresis whose threshold
   is  quiet mean + K_SIGMA x quiet standard deviation, measured at start-up. */

#define DSP_CHANNELS    3u
#define DSP_BURST_LEN   256u
#define DSP_FS_HZ       8000u
#define DSP_CAL_BURSTS  32u     /* quiet bursts used to learn the baseline (about 1 s) */
#define DSP_K_SIGMA     3u      /* threshold = mean + K x sigma                         */
#define DSP_MIN_MARGIN  2u      /* counts; stops a zero-noise baseline giving threshold = mean */

void     Dsp_Init(void);                 /* configure SysTick pacing (call after ADC init) */
void     Dsp_Calibrate(void);            /* keep the room quiet while this runs            */
void     Dsp_Update(void);               /* one burst on all channels, updates the results */

uint16_t Dsp_Rms(uint8_t ch);            /* latest filtered RMS of the last burst           */
uint16_t Dsp_Avg(uint8_t ch);            /* 4-burst moving average of RMS                   */
uint16_t Dsp_Threshold(uint8_t ch);      /* ON threshold for this channel                   */
uint16_t Dsp_Baseline(uint8_t ch);       /* quiet mean RMS for this channel                 */
uint8_t  Dsp_Active(uint8_t ch);         /* 1 = acoustic detection with hysteresis          */
uint8_t  Dsp_AnyActive(void);
uint8_t  Dsp_Strongest(void);            /* channel with largest (avg - baseline)           */

#endif
