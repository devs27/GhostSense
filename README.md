# GhostSense: Pre-Visual Vehicle Detection in Dense Fog via Acoustic-Pressure Sensing & Ultrasonic Fusion

[![Platform](https://img.shields.io/badge/Platform-TI%20TM4C123GH6PM%20(ARM%20Cortex--M4F)-blue.svg)](https://www.ti.com/tool/EK-TM4C123GXL)
[![Firmware](https://img.shields.io/badge/Firmware-Bare--Metal%20Register--Level%20Embedded%20C-orange.svg)](#-firmware-architecture)
[![IDE](https://img.shields.io/badge/IDE-Keil%20uVision%205%20(C99)-darkgreen.svg)](#-setup-steps--environment-variables)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Design Document](https://img.shields.io/badge/TDD-Technical%20Design%20Doc-purple.svg)](TECHNICAL_DESIGN.md)
[![Test Suite](https://img.shields.io/badge/Tests-10%2F10%20Stages%20Passing-brightgreen.svg)](sample_data/TEST_CASES.md)

> **GhostSense** is a non-visual, edge-processed collision warning embedded system designed to detect approaching vehicles in dense fog long before headlights or optical Advanced Driver Assistance Systems (ADAS) become visible. Built from the ground up on the Texas Instruments **TM4C123GH6PM** ARM Cortex-M4F microcontroller using bare-metal register-level Embedded C and fixed-point digital signal processing.

---

### 📑 Quick Project Links
* 📘 **[Technical Design Document (TDD)](TECHNICAL_DESIGN.md)** — Architectural block diagrams, mathematical formulation of Q14 biquad filter, finite state machine, and hardware resource analysis.
* 🧪 **[Test Cases & Verification Suite](sample_data/TEST_CASES.md)** — Stage 1–10 validation matrix, distance boundary tests, and fail-safe exception protocols.
* 📊 **[Sample Sensor Data (CSV)](sample_data/sample_sensor_readings.csv)** — Calibrated experimental dataset logged during vehicle approach tests.
* 📄 **[MIT License](LICENSE)** — Open-source permissive license.

---

## 📌 Problem Overview & Physical Insight

* **The Challenge:** Dense fog attenuates and scatters visible light, creating severe optical contrast degradation. Traditional camera-based vision systems (e.g., YOLO/CNNs) degrade rapidly, and headlights provide insufficient preview distance at highway speeds. LiDAR similarly suffers from droplet backscatter. Moving vehicles appear at dangerously short distances ("ghost vehicles"), precipitating chain-reaction pileups.
* **The Non-Visual Solution:** Moving vehicles produce mechanical and acoustic pressure disturbances (engine vibration, tire-road displacement friction, aerodynamic turbulence) that propagate through atmospheric fog at rates orders of magnitude less attenuated than visible light.
* **Our Approach:** By coupling a multi-channel array of low-frequency analog acoustic/pressure sensors with an ultrasonic Time-of-Flight (ToF) rangefinder, GhostSense continuously profiles ambient baselines, detects approaching pressure signatures, and deterministically measures distance without relying on light.

---

## ⚙️ System Architecture

```mermaid
graph TD
    subgraph Sensing Layer
        M1["Front Mic (PE3 / AIN0)"]
        M2["Back-Right Mic (PE2 / AIN1)"]
        M3["Back-Left Mic (PE1 / AIN2)"]
        US["HC-SR04 Ultrasonic (PB6 TRIG / PB7 ECHO)"]
    end

    subgraph TM4C123GH6PM ARM Cortex-M4F Core
        ADC["ADC0 Sequencer 1 (12-bit Simultaneous)"]
        TMR["Timer0 Microsecond Pulse Timer"]
        DSP["Fixed-Point DSP (Q14 Biquad IIR + RMS + Hysteresis)"]
        FUS["Asymmetric Sensor Fusion & Classification Engine"]
    end

    subgraph Actuation & Display Layer
        OLED["SSD1306 OLED (I2C0 - PB2/PB3)"]
        BUZZ["Active Buzzer (PB5)"]
        LED["Graduated Alert LEDs (PB4 / Onboard RGB)"]
    end

    M1 & M2 & M3 --> ADC
    ADC --> DSP
    US --> TMR
    TMR --> FUS
    DSP --> FUS
    FUS --> OLED
    FUS --> BUZZ
    FUS --> LED
```

---

## 🔬 Signal Processing & Embedded Algorithms

Implemented in [`GhostVehicle_Keil/dsp.c`](GhostVehicle_Keil/dsp.c) with zero vendor library dependence:

1. **Quiet Baseline Profiling:**
   * At power-up, the system samples 32 quiet bursts (256 samples per burst at 8 kHz paced by SysTick) to determine ambient baseline RMS and variance ($\sigma$) for each channel.
2. **Fixed-Point Q14 Biquad Bandpass Filter:**
   * Eliminates DC bias and isolates vehicular acoustic frequencies (~1 kHz, Q = 0.7) using 16-bit fixed-point integer arithmetic with no floating-point hardware overhead.
3. **Adaptive Thresholding with Hysteresis:**
   * Dynamic threshold trigger:
     $$\text{Threshold}_{\text{ON}} = \text{Baseline} + 3\sigma + 2$$
   * Release threshold is set to $\frac{2}{3}$ of the margin to prevent chatter/rapid oscillating state switches.
4. **Asymmetric Proximity Classification:**
   * Ultrasonic distance acts as primary proximity classifier; acoustic channels provide corroboration and directional angle of approach.

---

## 📊 Proximity Bands & Alert Hierarchy

| Distance Range (Scaled 35 cm Box) | Proximity Classification | OLED Display Status | Alert Output |
| :--- | :--- | :--- | :--- |
| **> 30 cm** | `MONITORING...` | Real-time distance + Sound level | LED OFF / Buzzer OFF |
| **20 cm – 30 cm** | `CAR DETECTED` | Real-time distance + Direction | LED ON / Buzzer OFF |
| **10 cm – 20 cm** | `APPROACHING!` | Alert banner + Direction | LED ON / Buzzer OFF |
| **< 10 cm** | `!!CRITICAL!!` | Flashing alert + Range | LED ON + **Buzzer ACTIVE** |
| **Timeout / No Pulse** | `NO ECHO / CLEAR` | Idle monitor | LED OFF / Buzzer OFF |

---

## 🔌 Hardware Pinout Mapping

| Component | Pin Function | TM4C123GH6PM Pin | Description |
| :--- | :--- | :--- | :--- |
| **MAX4466 Mic 1** | Front Analog Out | **PE3 (AIN0)** | ADC0 Sample Sequencer 1 (Step 0) |
| **MAX4466 Mic 2** | Back-Right Analog Out | **PE2 (AIN1)** | ADC0 Sample Sequencer 1 (Step 1) |
| **MAX4466 Mic 3** | Back-Left Analog Out | **PE1 (AIN2)** | ADC0 Sample Sequencer 1 (Step 2) |
| **SSD1306 OLED** | I2C Serial Data | **PB3** | I2C0 SDA (Fast 400 kHz line padded) |
| **SSD1306 OLED** | I2C Serial Clock | **PB2** | I2C0 SCL |
| **HC-SR04** | Ultrasonic Trigger | **PB6** | 10 µs trigger pulse |
| **HC-SR04** | Ultrasonic Echo | **PB7** | Timer0 ToF capture (100 µs – 38 ms) |
| **Active Buzzer** | Alert Signal | **PB5** | GPIO Output (Critical warning) |
| **Alert Indicator** | Visual Indicator | **PB4** | GPIO Output |
| **Power Bus** | VCC / GND | **3.3V / GND** | Common ground across all breadboards |

---

## 📁 Repository Structure

```
GhostSense/
├── GhostVehicle_Keil/             # Complete Bare-Metal C Firmware
│   ├── main.c                     # Multi-stage execution loop & fusion engine
│   ├── adc.c / adc.h              # 3-channel ADC0 Sequencer 1 register driver
│   ├── dsp.c / dsp.h              # Fixed-point Q14 biquad IIR filter & RMS logic
│   ├── ultrasonic.c / .h          # Timer0-based microsecond ToF ultrasonic driver
│   ├── oled.c / oled.h            # Optimized line-buffered SSD1306 I2C driver
│   ├── font.c / font.h            # 5x7 ASCII monospace bitmap font table
│   ├── buzzer.c / buzzer.h        # GPIO buzzer control
│   ├── delay.c / delay.h          # Hardware calibrated delay utilities
│   ├── util.c / util.h            # Safe string and integer-to-string formatters
│   └── README_KEIL_STEPS.txt      # Step-by-step Keil compilation guide
├── sample_data/                   # Validation Datasets & Test Suite
│   ├── TEST_CASES.md              # 10-stage test matrix & fault handling cases
│   ├── sample_sensor_readings.csv # Calibrated approach run sensor logs
│   └── Threshold_Evidence_Workbook_SAMPLE.xlsx
├── TECHNICAL_DESIGN.md            # In-depth architectural & mathematical design doc
├── LICENSE                        # MIT License
├── .gitignore                     # Keil build artifacts and OS ignores
└── README.md                      # Primary project documentation
```

---

## 🛠️ Setup Steps & Environment Variables

### 1. Prerequisites & Toolchain Installation
* **IDE:** [Keil µVision 5](https://www.keil.com/demo/eval/arm.htm) (MDK-ARM v5.38 or higher)
* **Device Pack:** Keil Texas Instruments Tiva C Series Device Family Pack (`Keil::TM4C_DFP`)
* **Hardware Drivers:** [TI Stellaris In-Circuit Debug Interface (ICDI) Drivers](https://www.ti.com/tool/SW-ICDI-DRIVERS)

### 2. Environment Variables & System Paths (Windows)
For command-line build and flashing tools, ensure the following environment paths are configured:

| Variable Name | Typical Path | Purpose |
| :--- | :--- | :--- |
| `KEIL_ROOT` | `C:\Keil_v5` | Base Keil MDK directory |
| `ARM_TOOLCHAIN` | `C:\Keil_v5\ARM\ARMCC\bin` | Keil Arm Compiler 5 toolchain |
| `PATH` | `%PATH%;C:\Keil_v5\UV4` | Allows invoking `UV4.exe` from terminal for CI/CD |

### 3. Keil µVision Project Setup
1. **Launch Keil µVision 5:** Click `Project` > `New µVision Project...`
2. **Select Device:** Choose `Texas Instruments` > `TM4C123 Series` > **`TM4C123GH6PM`** > Click **OK**.
3. **Manage Run-Time Environment:**
   * Tick **`CMSIS`** > **`CORE`**
   * Tick **`Device`** > **`Startup`** (This adds `startup_TM4C123.s` and `system_TM4C123.c`).
4. **Add Source Files:**
   * Right-click `Source Group 1` in the Project explorer > `Add Existing Files to Group...`
   * Select all `.c` and `.h` files from [`GhostVehicle_Keil/`](GhostVehicle_Keil/):
     `main.c`, `adc.c`, `dsp.c`, `i2c.c`, `oled.c`, `font.c`, `buzzer.c`, `ultrasonic.c`, `delay.c`, `util.c`.
5. **Configure Target Options (`Alt + F7`):**
   * **Target Tab:** Set System Clock to **16.0 MHz** (or configure PLL for 80 MHz).
   * **C/C++ Tab:** Select **Language C: C99** (crucial to permit inline `for(uint8_t i...)` declarations).
   * **Debug Tab:** Set debugger to **`Stellaris ICDI`**. Under *Settings*, verify mode is **JTAG/SWD** and check **Reset and Run**.
   * **Utilities Tab:** Select *Use Target Driver for Flash Programming* (`Stellaris ICDI`).
6. **Build & Flash:**
   * Press **`F7`** to compile (target: `0 Errors, 0 Warnings`).
   * Connect TM4C123G LaunchPad via USB and press **`F8`** to flash.

---

## 🚀 Incremental Bring-Up Stages

Configure `#define STAGE <N>` at the top of [`GhostVehicle_Keil/main.c`](GhostVehicle_Keil/main.c) to isolate each subsystem:

* **Stage 1:** Onboard LaunchPad RGB LED blink test (System clock verify).
* **Stage 2:** Single-channel ADC read verification via Watch window.
* **Stage 3:** SSD1306 I2C bus probing and static text display.
* **Stage 4:** Live single-microphone waveform/bar display on OLED.
* **Stage 5:** 3-channel simultaneous acoustic sampling and visualizer.
* **Stage 6:** Buzzer hardware trigger test.
* **Stage 7:** Ultrasonic distance measurement with live OLED feedback.
* **Stage 8:** Asymmetric sensor fusion (distance + raw acoustic corroborate).
* **Stage 9:** Dynamic acoustic noise profiling and fixed-point DSP bandpass filter.
* **Stage 10 (FINAL):** Full integrated fusion system (DSP-filtered acoustic verification + ultrasonic distance hierarchy + multi-tier alerts).

---

## 👥 Team & Mentorship

### Team Members :
* **Akshaya M V** (`CH.EN.U4CCE24003`)
* **Devadharshini P** (`CH.EN.U4CCE24004`)
* **Mukesh M** (`CH.EN.U4CCE24015`)

### Faculty Mentor:
* **Dr. C. Ganesh Kumar** 
  *Amrita School of Engineering, Amrita Vishwa Vidyapeetham, Chennai*
