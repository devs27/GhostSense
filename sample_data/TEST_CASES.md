# GhostSense: Test Cases & Validation Suite

This document outlines the structured test cases, hardware-in-the-loop (HIL) verification protocols, and boundary condition validations for the **GhostSense** embedded collision-warning system on the TI TM4C123GH6PM.

---

## 1. Incremental Bring-Up Test Matrix (`main.c` Stages)

The firmware is designed with an incremental bring-up switch (`#define STAGE <N>`). Each stage must pass before progressing to full multi-modal fusion.

| Test ID | Stage | Subsystem Under Test | Hardware Required | Input Stimulus | Expected Output / Pass Criteria | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Stage 1 | System Clock & GPIO | LaunchPad only | Power-on reset | Red LED on `PF1` blinks reliably at ~1.0 Hz | **PASS** |
| **TC-02** | Stage 2 | ADC0 Sequencer 1 Single-Ch | MAX4466 on `PE3` | Ambient noise & clap | `adcFront` changes in Keil Watch; Green LED toggles on loud sound | **PASS** |
| **TC-03** | Stage 3 | I2C0 Bus & SSD1306 OLED | OLED on `PB2/PB3` | Initialization commands | Display shows "OLED OK"; fast red blink if I2C NACK detected | **PASS** |
| **TC-04** | Stage 4 | Live Single-Ch Waveform | Mic `PE3` + OLED | Sound impulses | OLED displays live numeric ADC counts and dynamic horizontal bar | **PASS** |
| **TC-05** | Stage 5 | 3-Channel ADC Sampling | 3 Mics on `PE3/PE2/PE1` | Directional acoustic noise | OLED displays simultaneous live levels for `F` (Front), `BR`, `BL` | **PASS** |
| **TC-06** | Stage 6 | Buzzer Actuation Circuit | Buzzer on `PB5` | 100 ms pulse trigger | Crisp, periodic 100 ms audible beep once per second | **PASS** |
| **TC-07** | Stage 7 | Ultrasonic Time-of-Flight | HC-SR04 on `PB6/PB7` | Hand moving 5 cm – 35 cm | OLED distance readout tracks true physical distance within ±1.0 cm | **PASS** |
| **TC-08** | Stage 8 | Asymmetric Sensor Fusion | Complete Hardware Rig | RC car approach in box | OLED updates proximity band; buzzer activates only below critical threshold | **PASS** |
| **TC-09** | Stage 9 | Fixed-Point DSP Filter | 3 Mics + LaunchPad | 1 kHz acoustic signal | Fixed-point Q14 biquad filter rejects out-of-band noise; dynamic baseline adapts | **PASS** |
| **TC-10** | Stage 10| Full Production Integration | All Modules + Test Box | Scaled foggy box approach | Autonomous detection, corroboration, and graduated multi-tier alerts | **PASS** |

---

## 2. Distance Proximity Threshold Boundary Test Cases

Validated inside the 35 cm enclosed acoustic test chamber:

| Test ID | Measured Distance ($d$) | Expected Proximity State | OLED Display Line 3 | LED Alert (`PB4`) | Buzzer Alert (`PB5`) | Result |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-DIST-01** | $d > 30\text{ cm}$ (e.g. 34 cm) | `MONITORING` | `MONITORING...` | OFF | OFF | **PASS** |
| **TC-DIST-02** | Boundary $d = 30\text{ cm}$ | `CAR DETECTED` | `CAR DETECTED` | ON | OFF | **PASS** |
| **TC-DIST-03** | $20\text{ cm} \le d < 30\text{ cm}$ (e.g. 25 cm) | `CAR DETECTED` | `CAR DETECTED` | ON | OFF | **PASS** |
| **TC-DIST-04** | Boundary $d = 20\text{ cm}$ | `APPROACHING` | `APPROACHING!` | ON | OFF | **PASS** |
| **TC-DIST-05** | $10\text{ cm} < d < 20\text{ cm}$ (e.g. 15 cm) | `APPROACHING` | `APPROACHING!` | ON | OFF | **PASS** |
| **TC-DIST-06** | Boundary $d = 10\text{ cm}$ | `CRITICAL` | `!!CRITICAL!!` | ON | **ACTIVE (Continuous)** | **PASS** |
| **TC-DIST-07** | $d < 10\text{ cm}$ (e.g. 5 cm) | `CRITICAL` | `!!CRITICAL!!` | ON | **ACTIVE (Continuous)** | **PASS** |
| **TC-DIST-08** | No echo received / Timeout | `NO SIGNAL` | `NO SIGNAL` | OFF | OFF | **PASS** |

---

## 3. Acoustic DSP & Baseline Calibration Tests

| Test ID | Test Scenario | Input Signal Condition | Expected Algorithm Behavior | Acceptance Criteria |
| :--- | :--- | :--- | :--- | :--- |
| **TC-DSP-01** | Power-on Quiet Calibration | Ambient silence for 1.5 seconds | Samples 32 bursts (256 samples each) via SysTick; computes per-channel $\mu_{\text{quiet}}$ and $\sigma$. | Variance $\sigma \le 15$ ADC counts under nominal conditions. |
| **TC-DSP-02** | DC Bias Subtraction | DC baseline offset ~1.65V (ADC ~2048) | Subtracts burst mean $\bar{x}$ from every sample before filtering. | Mean of filtered buffer equals 0 ± 1 count. |
| **TC-DSP-03** | Q14 Biquad Bandpass Filter | White noise vs. 1 kHz vehicular hum | Attenuates high-frequency hiss (>2 kHz) and DC rumble (<300 Hz). | >12 dB attenuation outside 800 Hz – 1.2 kHz passband. |
| **TC-DSP-04** | Hysteresis Thresholding | Signal amplitude oscillating near threshold | Trigger ON: $\text{RMS} \ge \mu + 3\sigma + 2$; Trigger OFF: $\text{RMS} < \mu + 2\sigma$. | Zero rapid alert chatter at boundary. |

---

## 4. Fault Tolerance & Exception Handling Test Cases

| Test ID | Fault Injected | System Response | Fail-Safe Mechanism |
| :--- | :--- | :--- | :--- |
| **TC-ERR-01** | Disconnected Ultrasonic ECHO pin | Timer0 overflows 38 ms without echo pulse | Returns sentinel `0xFFFF`, suppresses false buzzer triggers. |
| **TC-ERR-02** | Spurious short pulse (<100 µs) on ECHO | Noise spike on GPIO line | Filter rejects pulses <100 µs as invalid reflection. |
| **TC-ERR-03** | Disconnected OLED I2C lines (`PB2/PB3`) | `I2C0_Wait()` timeout detection | Firmware bypasses OLED write loop, lights onboard red warning LED, maintains alert logic. |
| **TC-ERR-04** | Acoustic impulse (sudden clap / horn) | Brief single-burst acoustic spike | 4-burst moving average filter prevents spurious proximity transitions without ultrasonic corroboration. |
