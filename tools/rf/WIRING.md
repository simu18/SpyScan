# SpyScan Hardware Wiring Guide: Sub-1 GHz RF & Hardware IR

This guide details the physical hardware connections for extending SpyScan with:
1. **TI CC1101 Sub-1 GHz Transceiver** (315 / 433 / 868 MHz RF Bug Hunter) on **ESP32-S3 DevKit**.
2. **5V Hardware IR Receiver Module** (Active IR Bug Hunter) on **ESP32-S3 CAM Board**.

---

## 1. TI CC1101 Sub-1 GHz Module $\to$ ESP32-S3 DevKit

> [!CAUTION]
> **CRITICAL VOLTAGE REQUIREMENT:**
> Connect CC1101 `VCC` **ONLY to the 3.3V (`3V3`) pin** of the ESP32-S3!
> Connecting CC1101 to `5V` or `VIN` will **permanently burn out the CC1101 chip** (its absolute maximum rating is 3.6V).

### Pin Mapping Table

| CC1101 Module Pin | ESP32-S3 DevKit Pin | Function | Notes |
|---|---|---|---|
| **VCC** | **3V3** (Pin 1 or 2) | Power Supply (3.3V) | **DO NOT CONNECT TO 5V!** |
| **GND** | **GND** | Ground Reference | Common ground |
| **CSN** | **GPIO 10** | SPI Chip Select (Active Low) | Manual CSN strobe timing |
| **MOSI** | **GPIO 11** | SPI MOSI (Data In to CC1101) | Hardware SPI2_HOST |
| **SCK** | **GPIO 12** | SPI Clock | 4 MHz SPI Mode 0 |
| **MISO** | **GPIO 13** | SPI MISO (Data Out from CC1101) | Hardware SPI2_HOST |
| **GDO0** | **GPIO 9** | Interrupt / Carrier Detect | Optional (defaults high on carrier) |
| **GDO2** | *Not Connected* | General Digital Output | Leave floating |

### Visual Pin Layout Diagram (ESP32-S3 DevKit)

```
             ESP32-S3 DevKit Header (Right Side / Near Antenna)
                  ┌───────────────┐
                  │      3V3      │ ───► CC1101 VCC (Pin 1)
                  │      GND      │ ───► CC1101 GND (Pin 2)
                  │    GPIO 9     │ ───► CC1101 GDO0 (Pin 8 - optional)
                  │    GPIO 10    │ ───► CC1101 CSN  (Pin 4)
                  │    GPIO 11    │ ───► CC1101 MOSI (Pin 3)
                  │    GPIO 12    │ ───► CC1101 SCK  (Pin 5)
                  │    GPIO 13    │ ───► CC1101 MISO (Pin 7)
                  │      ...      │
                  └───────────────┘
```

### Module Pinout (Standard CC1101 8-Pin Header)

```
        Top View of CC1101 Module
           ┌──────────────┐
     GND  1│ [•]      [•] │2  VCC (3.3V ONLY!)
    MOSI  3│ [•]      [•] │4  CSN
     SCK  5│ [•]      [•] │6  GDO2 (NC)
    MISO  7│ [•]      [•] │8  GDO0
           └──────────────┘
```

---

## 2. 5V Hardware IR Sensor Module $\to$ ESP32-S3 CAM Board

The 5V IR receiver module (e.g., TSOP38238 / VS1838B or 3-pin IR breakout) detects modulated 38 kHz infrared carriers from covert camera remote triggers and night illuminators.

### Pin Mapping Table

| IR Module Pin | ESP32-S3 CAM Pin | Function | Notes |
|---|---|---|---|
| **VCC** | **5V** (or 3V3) | Power Supply | 5V module accepts 5V from USB bus |
| **GND** | **GND** | Ground Reference | Common ground |
| **OUT / DATA** | **GPIO 3** | Demodulated IR signal | Active-low; triggers `IR_HW_TRIGGER` |

### Firmware Trigger Operation
* When the IR receiver detects any 38 kHz modulated IR pulses (from TV remotes, laser pointers, or spy camera IR beacons), the pin pulls LOW.
* The ESP32-S3 CAM firmware triggers a debounced interrupt on GPIO 3:
  ```
  I,12480,IR_HW_TRIGGER,level=0
  ```
* The laptop host tool (`tools/ir/ir_hunter.py`) correlates this physical interrupt with the OV3660 CMOS image sensor hotspot, locking a dual-verification target.

---

## 3. Verification Commands

### Test 1: Verify CC1101 Connection via Serial Console
Once flashed and wired, open the DevKit serial console:
```bash
./.venv/bin/python -m serial.tools.miniterm /dev/cu.usbmodem111401 115200
```
Type:
```
subghz init
```
**Expected Output:**
```
I,1200,cc1101,INIT_OK,part=0x00,version=0x14,freq=433920kHz
```
If not wired, it safely outputs:
```
I,1200,cc1101,NOT_DETECTED,pins=CSN:10,MOSI:11,SCK:12,MISO:13
```

### Test 2: Run Live Sub-1 GHz Bug Hunter
```bash
./.venv/bin/python tools/rf/subghz_bug_hunter.py --beep
```
Press any 433 MHz car remote or transmitter to observe the real-time RF carrier waterfall and audio proximity beeps!
