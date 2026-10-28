/* ============================================================================
   GHOST VEHICLE DETECTION - TM4C123GH6PM (EK-TM4C123GXL), Keil MDK v5, register level
   ----------------------------------------------------------------------------
   ONE main.c, TEN build stages. Change STAGE below, press F7 (build), F8 (flash).
   Each stage only needs the hardware listed for it, so test them in order.

     STAGE 1  Onboard LED blink                 (board + debugger check)
     STAGE 2  Mic ADC -> onboard LED            (read values in the Keil Watch window)
     STAGE 3  OLED text test                    (I2C0 on PB2/PB3)
     STAGE 4  Live mic ADC value on OLED        <-- the Review 1 demonstration
     STAGE 5  Three mics raw ADC on OLED        (Front / Back-Right / Back-Left)
     STAGE 6  Buzzer test                       (PB5)
     STAGE 7  Ultrasonic distance on OLED       (HC-SR04 on PB6 / PB7)
     STAGE 8  Fusion with raw mic values       (distance triggers alerts, raw sound shown)
     STAGE 9  Filtered acoustic detector        (DC removal, band-pass, RMS, mean+3 sigma)
     STAGE 10 FINAL: ultrasonic + filtered acoustic fusion

   Pins:  Mic Front PE3 (AIN0)   Mic Back-Right PE2 (AIN1)   Mic Back-Left PE1 (AIN2)
          OLED SDA PB3, SCL PB2  Buzzer PB5   HC-SR04 TRIG PB6, ECHO PB7
   ========================================================================== */

#ifndef STAGE
#define STAGE 10
#endif

#if (STAGE < 1) || (STAGE > 10)
#error "STAGE must be between 1 and 10"
#endif

#include <stdint.h>
#include "TM4C123GH6PM.h"
#include "adc.h"
#include "i2c.h"
#include "oled.h"
#include "buzzer.h"
#include "delay.h"
#include "ultrasonic.h"
#include "util.h"
#include "dsp.h"

/* ---- Tunable values ------------------------------------------------------ */
#define SOUND_DELTA        150u   /* stage 2: ADC change from baseline that counts as "sound" */
#define DIST_DETECT_CM      30u   /* above this = MONITORING (calibrated for the 35 cm box)   */
#define DIST_APPROACH_CM    20u   /* 20-30 = CAR DETECTED                                     */
#define DIST_CRITICAL_CM    10u   /* 10-20 = APPROACHING, <=10 = CRITICAL (buzzer)            */

/* ---- Onboard RGB LED helpers (stages 1-3) -------------------------------- */
#if STAGE <= 3
#define LED_RED    (1u << 1)      /* PF1 */
#define LED_BLUE   (1u << 2)      /* PF2 */
#define LED_GREEN  (1u << 3)      /* PF3 */

static void BoardLED_Init(void) {
    SYSCTL->RCGCGPIO |= (1<<5);               /* Port F clock */
    while (!(SYSCTL->PRGPIO & (1<<5)));
    GPIOF->DIR |= (LED_RED | LED_BLUE | LED_GREEN);
    GPIOF->DEN |= (LED_RED | LED_BLUE | LED_GREEN);
    GPIOF->DATA &= ~(LED_RED | LED_BLUE | LED_GREEN);
}
#endif

/* ============================================================================
   STAGE 1 - LED blink. Proves board, Keil project and ICDI flashing all work.
   Pass: onboard RED LED blinks about once per second.
   ========================================================================== */
#if STAGE == 1
int main(void) {
    BoardLED_Init();
    while (1) {
        GPIOF->DATA ^= LED_RED;
        Delay_ms(500);
    }
}
#endif

/* ============================================================================
   STAGE 2 - Read the mics with the ADC, no display yet.
   Start the debugger (Ctrl+F5), right-click adcFront in the code -> "Add to Watch",
   then run (F5). Clap near the Front mic.
   Pass: adcFront changes with sound and the GREEN LED lights on a loud sound.
   The baseline is measured at start-up so a high resting value does not matter.
   ========================================================================== */
#if STAGE == 2
volatile uint16_t adcFront, adcBackRight, adcBackLeft;

int main(void) {
    uint16_t adc[3];
    uint32_t sum = 0;
    uint16_t baseline, delta;
    uint8_t  i;

    BoardLED_Init();
    ADC0_Init3Ch();

    for (i = 0; i < 64; i++) {                /* average a quiet period */
        ADC0_ReadAll(adc);
        sum += adc[0];
        Delay_ms(5);
    }
    baseline = (uint16_t)(sum / 64u);

    while (1) {
        ADC0_ReadAll(adc);
        adcFront = adc[0];
        adcBackRight = adc[1];
        adcBackLeft = adc[2];

        delta = (adc[0] > baseline) ? (uint16_t)(adc[0] - baseline)
                                    : (uint16_t)(baseline - adc[0]);
        if (delta > SOUND_DELTA) GPIOF->DATA |= LED_GREEN;
        else                     GPIOF->DATA &= ~LED_GREEN;
        Delay_ms(10);
    }
}
#endif

/* ============================================================================
   STAGE 3 - OLED text test over I2C0.
   Pass: text appears and the GREEN LED is on.
   Fail: RED LED blinks fast = OLED did not acknowledge (check SDA PB3, SCL PB2,
         VCC 3.3V, GND, and that the I2C address is 0x3C).
   ========================================================================== */
#if STAGE == 3
int main(void) {
    BoardLED_Init();
    I2C0_Init();

    if (!OLED_Probe()) {
        while (1) { GPIOF->DATA ^= LED_RED; Delay_ms(100); }
    }

    OLED_Init();
    OLED_Clear();
    OLED_PrintLine(0, "GHOST VEHICLE");
    OLED_PrintLine(1, "OLED OK");
    OLED_PrintLine(2, "ABCDEFGHIJKLMNOPQRSTU");
    OLED_PrintLine(3, "0123456789 !?.:-%");
    GPIOF->DATA |= LED_GREEN;
    while (1);
}
#endif

/* ============================================================================
   STAGE 4 - REVIEW 1 DEMO: live microphone ADC value on the OLED.
   Pass: ADC value changes when you clap or speak near the Front mic, and the
         bar graph grows with it.
   ========================================================================== */
#if STAGE == 4
int main(void) {
    uint16_t adc[3];
    char     line[32];
    char    *p;
    uint8_t  i, n;

    ADC0_Init3Ch();
    I2C0_Init();
    OLED_Init();
    OLED_Clear();
    OLED_PrintLine(0, "MIC ADC TEST");

    while (1) {
        ADC0_ReadAll(adc);

        p = Util_AppendStr(line, "ADC:");
        Util_AppendNum(p, adc[0]);
        OLED_PrintLine(2, line);

        n = (uint8_t)((adc[0] * 21u) / 4096u);   /* 0..20 bar segments */
        for (i = 0; i < n; i++) line[i] = '#';
        line[n] = 0;
        OLED_PrintLine(4, line);

        Delay_ms(100);
    }
}
#endif

/* ============================================================================
   STAGE 5 - All three mics, raw values.
   Pass: F, BR and BL each react when you tap near that mic.
   Note: a quiet-room value near 2048 is ideal. If all three sit near 3100-3300,
         turn each module's gain trimmer down and check each module is on 3.3V.
   ========================================================================== */
#if STAGE == 5
int main(void) {
    uint16_t adc[3];
    char     line[32];
    char    *p;

    ADC0_Init3Ch();
    I2C0_Init();
    OLED_Init();
    OLED_Clear();
    OLED_PrintLine(0, "RAW ADC TEST");

    while (1) {
        ADC0_ReadAll(adc);

        p = Util_AppendStr(line, "F:");   Util_AppendNum(p, adc[0]);  OLED_PrintLine(1, line);
        p = Util_AppendStr(line, "BR:");  Util_AppendNum(p, adc[1]);  OLED_PrintLine(2, line);
        p = Util_AppendStr(line, "BL:");  Util_AppendNum(p, adc[2]);  OLED_PrintLine(3, line);

        Delay_ms(100);
    }
}
#endif

/* ============================================================================
   STAGE 6 - Buzzer test.
   Pass: a short beep (PB5) about once a second, repeating.
   Silent or only clicks = the buzzer is passive (needs an ACTIVE buzzer) or the
   wiring/transistor is wrong.
   ========================================================================== */
#if STAGE == 6
int main(void) {
    Buzzer_Init();
    Buzzer_Off();

    while (1) {
        Buzzer_On();  Delay_ms(200);
        Buzzer_Off(); Delay_ms(1000);
    }
}
#endif

/* ============================================================================
   STAGE 7 - HC-SR04 distance on the OLED, no alerts yet.
   Pass: DIST follows a hand or the RC car moving toward the sensor.
   "NO ECHO" = nothing in range, sensor not wired, or a spurious pulse was rejected.
   ========================================================================== */
#if STAGE == 7
int main(void) {
    uint16_t dist;
    char     line[32];
    char    *p;

    I2C0_Init();
    OLED_Init();
    Ultrasonic_Init();
    OLED_Clear();
    OLED_PrintLine(0, "ULTRASONIC TEST");

    while (1) {
        dist = Ultrasonic_GetDistanceCM();

        p = Util_AppendStr(line, "DIST:");
        if (dist == ULTRASONIC_NO_ECHO) {
            Util_AppendStr(p, "NO ECHO");
        } else {
            p = Util_AppendNum(p, dist);
            Util_AppendStr(p, " CM");
        }
        OLED_PrintLine(2, line);
        Delay_ms(200);
    }
}
#endif

/* ============================================================================
   Shared by stages 8 and 10: distance text, status text and the buzzer rule.
   ========================================================================== */
#if STAGE == 8 || STAGE == 10
static void UpdateAlerts(uint16_t distCM) {
    if (distCM != ULTRASONIC_NO_ECHO && distCM <= DIST_CRITICAL_CM) Buzzer_On();
    else                                                            Buzzer_Off();
}

static const char *GetStatusText(uint16_t distCM) {
    if (distCM == ULTRASONIC_NO_ECHO)  return "NO SIGNAL";
    if (distCM > DIST_DETECT_CM)       return "MONITORING...";
    if (distCM > DIST_APPROACH_CM)     return "CAR DETECTED";
    if (distCM > DIST_CRITICAL_CM)     return "APPROACHING!";
    return "!!CRITICAL!!";
}

static void ShowDistance(uint16_t distCM) {
    char  line[32];
    char *p = Util_AppendStr(line, "DIST:");
    if (distCM == ULTRASONIC_NO_ECHO) {
        Util_AppendStr(p, "NO ECHO");
    } else {
        p = Util_AppendNum(p, distCM);
        Util_AppendStr(p, " CM");
    }
    OLED_PrintLine(0, line);
}
#endif

/* ============================================================================
   STAGE 8 - Ultrasonic + raw microphone fusion (single ADC sample per loop).
   Ultrasonic distance is the trigger for the alerts; the three mics give the
   sound level (SND) and the loudest direction as corroboration on the display.

     > 30 cm   MONITORING     OLED only
     20-30     CAR DETECTED   OLED only
     10-20     APPROACHING!   OLED only
     <= 10     !!CRITICAL!!   OLED + buzzer
   ========================================================================== */
#if STAGE == 8
typedef enum { DIR_FRONT = 0, DIR_BACKRIGHT = 1, DIR_BACKLEFT = 2 } Direction_t;

static const char *const dirName[3] = { "FRONT", "BACK-RIGHT", "BACK-LEFT" };

static Direction_t GetDirection(const uint16_t *adc, uint16_t *maxVal) {
    uint8_t maxIdx = 0;
    uint8_t i;
    for (i = 1; i < 3; i++)
        if (adc[i] > adc[maxIdx]) maxIdx = i;
    *maxVal = adc[maxIdx];
    return (Direction_t)maxIdx;
}

int main(void) {
    uint16_t    adc[3];
    uint16_t    maxVal, distCM;
    Direction_t dir;
    char        line[32];
    char       *p;

    /* Buzzer first, forced off, so a floating pin cannot sound it */
    Buzzer_Init();
    Buzzer_Off();

    ADC0_Init3Ch();
    I2C0_Init();
    OLED_Init();
    Ultrasonic_Init();

    OLED_Clear();
    OLED_PrintLine(0, "GHOST VEHICLE");
    OLED_PrintLine(1, "FUSION MODE");
    Delay_ms(1500);
    OLED_Clear();

    while (1) {
        ADC0_ReadAll(adc);
        dir = GetDirection(adc, &maxVal);
        distCM = Ultrasonic_GetDistanceCM();

        ShowDistance(distCM);

        p = Util_AppendStr(line, "SND:");
        p = Util_AppendNum(p, maxVal);
        p = Util_AppendStr(p, " ");
        Util_AppendStr(p, dirName[dir]);
        OLED_PrintLine(1, line);

        OLED_PrintLine(2, GetStatusText(distCM));

        UpdateAlerts(distCM);
        Delay_ms(150);
    }
}
#endif

/* ============================================================================
   STAGE 9 - FILTERED ACOUSTIC DETECTOR (no ultrasonic needed).
   Start-up: "KEEP QUIET" for about 1 s while the quiet baseline is measured.
   Then each loop captures a 256-sample burst at 8 kHz on all three mics, removes
   the DC bias, band-pass filters, takes the RMS and compares the 4-burst average
   with the threshold (quiet mean + 3 sigma, with hysteresis).
   OLED rows:  F  avg/thr   BR avg/thr   BL avg/thr   and the detection state.
   Pass: the numbers sit near the baseline in a quiet room and rise above THR
         when the RC car runs close to a mic. Note the values for the workbook.
   ========================================================================== */
#if STAGE == 9
static const char *const micName[3] = { "F ", "BR", "BL" };

int main(void) {
    char    line[32];
    char   *p;
    uint8_t c;

    ADC0_Init3Ch();
    I2C0_Init();
    OLED_Init();
    Dsp_Init();

    OLED_Clear();
    OLED_PrintLine(0, "ACOUSTIC FILTER");
    OLED_PrintLine(2, "KEEP QUIET...");
    Dsp_Calibrate();
    OLED_Clear();
    OLED_PrintLine(0, "ACOUSTIC FILTER");

    while (1) {
        Dsp_Update();

        for (c = 0; c < 3; c++) {
            p = Util_AppendStr(line, micName[c]);
            p = Util_AppendStr(p, " ");
            p = Util_AppendNum(p, Dsp_Avg(c));
            p = Util_AppendStr(p, "/");
            p = Util_AppendNum(p, Dsp_Threshold(c));
            p = Util_AppendStr(p, Dsp_Active(c) ? " HIGH" : " LOW");
            OLED_PrintLine((uint8_t)(1 + c), line);
        }

        if (Dsp_AnyActive()) {
            p = Util_AppendStr(line, "SOUND: ");
            Util_AppendStr(p, micName[Dsp_Strongest()]);
        } else {
            Util_AppendStr(line, "SOUND: QUIET");
        }
        OLED_PrintLine(5, line);
    }
}
#endif

/* ============================================================================
   STAGE 10 - FINAL: ultrasonic + FILTERED acoustic sensor fusion.
   Distance decides the alert level and the buzzer (<= 10 cm). The filtered
   microphones corroborate: ACOUSTIC CONFIRMED appears when a mic channel is
   above its calibrated threshold, with the loudest direction.
   Start the car away from the sensors only AFTER the "KEEP QUIET" screen ends.

     > 30 cm   MONITORING     OLED only
     20-30     CAR DETECTED   OLED only
     10-20     APPROACHING!   OLED only
     <= 10     !!CRITICAL!!   OLED + buzzer
   ========================================================================== */
#if STAGE == 10
static const char *const dirName10[3] = { "FRONT", "BACK-RIGHT", "BACK-LEFT" };

int main(void) {
    uint16_t distCM;
    char     line[32];
    char    *p;

    Buzzer_Init();
    Buzzer_Off();

    ADC0_Init3Ch();
    I2C0_Init();
    OLED_Init();
    Ultrasonic_Init();
    Dsp_Init();

    OLED_Clear();
    OLED_PrintLine(0, "GHOST VEHICLE");
    OLED_PrintLine(1, "CALIBRATING");
    OLED_PrintLine(2, "KEEP QUIET...");
    Dsp_Calibrate();
    OLED_Clear();

    while (1) {
        Dsp_Update();                         /* about 32 ms burst on three mics */
        distCM = Ultrasonic_GetDistanceCM();

        ShowDistance(distCM);
        OLED_PrintLine(1, GetStatusText(distCM));

        if (Dsp_AnyActive()) {
            p = Util_AppendStr(line, "SND OK ");
            Util_AppendStr(p, dirName10[Dsp_Strongest()]);
        } else {
            Util_AppendStr(line, "SND: QUIET");
        }
        OLED_PrintLine(2, line);

        p = Util_AppendStr(line, "RMS:");
        p = Util_AppendNum(p, Dsp_Avg(Dsp_Strongest()));
        p = Util_AppendStr(p, " THR:");
        Util_AppendNum(p, Dsp_Threshold(Dsp_Strongest()));
        OLED_PrintLine(3, line);

        UpdateAlerts(distCM);
        Delay_ms(50);
    }
}
#endif
