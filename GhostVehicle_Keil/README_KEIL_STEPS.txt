GHOST VEHICLE DETECTION - KEIL MDK v5 BUILD FROM SCRATCH
=========================================================
Board: EK-TM4C123GXL (TM4C123GH6PM), Keil uVision 5, register-level Embedded C.

PART A - CREATE THE KEIL PROJECT (once)
---------------------------------------
1. Project > New uVision Project. Make a new empty folder, name the project (e.g. GhostVehicle).
2. Device: Texas Instruments > Tiva C Series > TM4C123GH6PM > OK.
3. In "Manage Run-Time Environment" tick:
      CMSIS > CORE
      Device > Startup
   Click OK. This adds startup_TM4C123.s and system_TM4C123.c.
4. Copy ALL .c and .h files from this folder into the project folder.
5. In the Project window, right-click "Source Group 1" > Add Existing Files to Group, and add
   exactly these ten .c files:
      main.c  adc.c  i2c.c  oled.c  font.c  buzzer.c  delay.c  ultrasonic.c  util.c  dsp.c
   Do NOT add any other file with a main() function.
6. Project > Options for Target:
      C/C++ tab ........ Language C = c99
      Debug tab ........ Use: Stellaris ICDI, then Settings: Mode JTAG/SWD as shown, tick "Reset and Run"
                         so the program starts right after flashing
      Utilities tab .... Use Debug Driver, tick "Update Target before Debugging"
7. Press F7 to build. Expect 0 errors. (Warnings about "last line ends without a newline" are
   already fixed in these files.)

PART B - BUILD AND TEST IN STAGES
---------------------------------
Open main.c and change this line near the top:   #define STAGE 1   (default is 10)
Build (F7), flash (F8). Move to the next stage only when the current one passes.

 Stage  What it does                              Hardware needed            Pass looks like
 -----  ----------------------------------------  -------------------------  ---------------------------
   1    Onboard LED blink                         LaunchPad only             Red LED blinks about 1 Hz
   2    Mic ADC, Watch window + onboard LED       MAX4466 on PE3 (+PE2, PE1) adcFront changes, green LED
                                                                             on loud sound
   3    OLED text test                            OLED on PB3 / PB2          Text shows, green LED on
                                                                             (fast red blink = no ACK)
   4    Live mic ADC on OLED  (REVIEW 1 DEMO)     Mic PE3 + OLED             ADC number and bar react
                                                                             to sound
   5    Three mics raw on OLED                    3 mics + OLED              F / BR / BL each react
   6    Buzzer test                               Buzzer on PB5              Short beep about once a second
   7    Ultrasonic distance on OLED               HC-SR04 PB6 / PB7 + OLED   DIST follows your hand
   8    Fusion with raw mic values                Everything                 See table below
   9    Filtered acoustic detector (needs no     3 mics + OLED              Rows show avg/threshold per
        ultrasonic)                                                         mic; HIGH when car is near
  10    FINAL: ultrasonic + filtered acoustic     Everything                 Stage 8 table + "SND OK"

STAGE 8 / 10 BEHAVIOUR (thresholds are for the 35 cm test box)
   DIST > 30 cm .... MONITORING...   OLED only
   20 - 30 cm ...... CAR DETECTED    OLED only
   10 - 20 cm ...... APPROACHING!    OLED only
   <= 10 cm ........ !!CRITICAL!!    OLED + buzzer
   NO ECHO ......... NO SIGNAL       buzzer off
Change DIST_DETECT_CM, DIST_APPROACH_CM, DIST_CRITICAL_CM at the top of main.c after running
stage 7 with your RC car in the real box.

FILTERING (dsp.c, stages 9 and 10)
----------------------------------
 Start-up: KEEP QUIET for about 1 s. 32 quiet bursts give each mic's baseline RMS and sigma.
 Per burst (256 samples at 8 kHz, paced by SysTick): subtract burst mean, band-pass biquad
 (about 1 kHz, Q 0.7, Q14 integers, no floating point), RMS, 4-burst moving average.
 Threshold ON = baseline + 3 x sigma + 2 counts; OFF = two thirds of that margin (hysteresis).
 Alerts and the buzzer still come from the ultrasonic distance only; the filtered mics
 corroborate and show the loudest direction. Values are RMS counts, not raw ADC counts.
 Sampling pace uses SystemCoreClock, so it follows whatever clock the startup file sets.

PIN MAP
-------
   Mic 1 Front ........ PE3 (AIN0)       OLED SDA ..... PB3     Buzzer + ..... PB5
   Mic 2 Back-Right ... PE2 (AIN1)       OLED SCL ..... PB2     HC-SR04 TRIG . PB6
   Mic 3 Back-Left .... PE1 (AIN2)       Buzzer - ..... GND     HC-SR04 ECHO . PB7
   VCC for mics and OLED = 3.3V.  All grounds common.
   There is no external LED in this project. The LEDs used in stages 1-3 are the
   LaunchPad's own onboard RGB LED (PF1/PF2/PF3), so nothing needs wiring for them.

WHAT CHANGED FROM THE PREVIOUS PACKAGE
--------------------------------------
 - Lowercase letters no longer read outside the font table (the old "cm" printed garbage).
 - OLED_PrintLine pads each line, so the screen is no longer fully cleared every loop (no flicker,
   faster loop).
 - OLED_Probe lets stage 3 tell a wiring fault from a software fault.
 - Ultrasonic timeouts use the hardware timer, and pulses shorter than 100 us (floating or
   spurious echo) or longer than 38 ms are rejected as NO ECHO.
 - The buzzer is forced off before anything else starts in stage 8.
 - The external LED (led.c / led.h) has been removed from the project.
 - Single main.c with a STAGE switch, so there is no duplicate-main linker error.

KNOWN ISSUES AND FIXES SEEN DURING BRING-UP
-------------------------------------------
 1. "type name is not allowed" on for(uint8_t i...)  -> set Language C = c99 (Part A step 6).
 2. "I2C0 undefined" in oled.c                        -> oled.c includes TM4C123GH6PM.h itself (done).
 3. A mic always reads above 2800                     -> ADC0->SSMUX1 must be 0x00000210 (done in adc.c).
 4. L6200E "main multiply defined"                    -> only one file with main(); remove backups
                                                         from the Source Group.
 5. "Could not find a Cortex-M device in the JTAG chain"
                                                      -> install the Stellaris ICDI driver manually:
                                                         ti.com/tool/download/SW-ICDI-DRIVERS, then
                                                         Device Manager > In-Circuit Debug Interface >
                                                         Update driver > Browse my computer.
 6. Buzzer always on                                  -> wire the HC-SR04 ECHO to PB7; a floating pin
                                                         looks like a very close object.
 7. Quiet-room mic value near 3100-3300 instead of ~2048
                                                      -> turn each module's gain trimmer down and check
                                                         each module is powered from 3.3V, not 5V.
                                                         Still open at last test; use stage 5 to adjust.
 8. HC-SR04 erratic at 3.3V                           -> power it from the 5V pin and put a 1k/2k divider
                                                         on ECHO before PB7.
