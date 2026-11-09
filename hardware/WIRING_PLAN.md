# SpyScan Master Hardware Wiring Plan

**Document Version:** 1.0  
**Target Systems:**  
- **Board 1 (UI Master):** ESP32-S3-N16R8 Camera Board (OV3660 + 2.4" ILI9341 LCD)  
- **Board 2 (RF Coprocessor):** ESP32-S3 DevKit + TI CC1101 Sub-1 GHz Transceiver  
- **Power Stage:** 1S Li-Po (3.7V) → IP5306 Boost/Charger (5.0V, 2.1A) → Main Slide Switch  

---

## 0. Voltage Rails & Distribution Rules

| Rail | Voltage | Sourced From | Feeds | Max Current |
|---|---|---|---|---|
| **5V Rail** | 5.0 V | IP5306 OUT+ (via slide switch) | CAM Board `5V`, DevKit `5V`, LED Anodes, 5V IR Module | 2.1 A |
| **3V3_CAM** | 3.3 V | CAM Board on-board LDO | ILI9341 LCD VCC & Backlight, Button Pull-ups | ~500 mA |
| **3V3_DEV** | 3.3 V | DevKit on-board LDO | **TI CC1101 VCC ONLY** (3.3V Max!) | ~500 mA |
| **GND** | 0 V | Common Star Ground | Everything (Both Boards, Battery, LCD, LEDs, CC1101) | Reference |

> [!CAUTION]
> **CRITICAL VOLTAGE RULE FOR CC1101:**
> Connect CC1101 `VCC` **ONLY to the DevKit's 3.3V pin**. Never connect it to the 5V rail; voltages over 3.6V will permanently destroy the transceiver chip.

---

## 1. Power Stage (Wire & Test First)

```
[Li-Po Cell (3.7V)]
  Li-Po (+) ──────────► IP5306 (B+)
  Li-Po (-) ──────────► IP5306 (B-)

[IP5306 Output & Main Switch]
  IP5306 (OUT+) ──────► Slide Switch (Pin 1)
  Slide Switch (Pin 2) ──► [+5V Rail]
  IP5306 (OUT-) ──────► [Common GND Rail]

[Decoupling & Bulk Capacitors]
  Between 5V & GND (near boards) : 470 µF (10V+ Electrolytic) + 100 nF (Ceramic)
  Between 3.3V & GND (on LCD)    : 10 µF Ceramic
```

- **Charging:** Plug USB-C directly into the **IP5306 charging port** on the enclosure exterior. Charging is independent of the ESP32 boards.
- **Flashing Safety:** Before connecting a computer USB cable to either ESP32-S3 for flashing, **switch the device OFF** with the slide switch so two 5V sources do not fight each other.

---

## 2. Board 1: ESP32-S3 CAM Board (UI Master)

### A. Power Connections
* `5V` pin $\to$ **+5V Rail**
* `GND` pin $\to$ **Common GND Rail**

### B. Display Wiring (ILI9341 2.4" SPI @ 3.3V)
| LCD Module Pin | Connected to | Note |
|---|---|---|
| **VCC** | CAM Board **3V3** | 3.3V logic & panel supply |
| **GND** | Common **GND** | Ground |
| **CS** | CAM Board **GPIO 41** | SPI Chip Select |
| **RESET** | CAM Board **3V3** | Software reset used in firmware (via 10k) |
| **DC / RS** | CAM Board **GPIO 42** | Data / Command select |
| **SDI (MOSI)** | CAM Board **GPIO 21** | Hardware SPI Data |
| **SCK** | CAM Board **GPIO 47** | Hardware SPI Clock |
| **LED (Backlight)**| CAM Board **3V3** | Always-on display backlight |
| *MISO, T_CLK, T_CS, etc.* | *Unconnected* | Touch controller not used |

### C. Optical LEDs via AO3400 MOSFETs
* **AO3400 Pinout (SOT-23):** Pin 1 = Gate, Pin 2 = Source, Pin 3 = Drain.

```
1. On-Axis Ring (6× 5mm Red LEDs around Camera Lens):
   CAM GPIO 2 ──100Ω──► Q1 Gate ;  Q1 Gate ──100kΩ──► GND ;  Q1 Source ──► GND
   Q1 Drain ──► Common-Cathode Node
   6× [5V Rail ──150Ω Resistor ──► LED Anode ──► LED Cathode ──► Q1 Drain Node]

2. Off-Axis Pair (2× 5mm Red LEDs, 3.8 cm to the side of the Lens):
   CAM GPIO 14 ──100Ω──► Q2 Gate ;  Q2 Gate ──100kΩ──► GND ;  Q2 Source ──► GND
   Q2 Drain ──► Common-Cathode Node
   2× [5V Rail ──150Ω Resistor ──► LED Anode ──► LED Cathode ──► Q2 Drain Node]
```

### D. 5V Hardware IR Receiver Module
| IR Module Pin | Connected to | Note |
|---|---|---|
| **VCC** | **+5V Rail** | 5V module accepts bus power |
| **GND** | **Common GND** | Ground |
| **OUT / DATA** | CAM Board **GPIO 3** | Active-low; triggers `IR_HW_TRIGGER` |

### E. Tactile Buttons (Active-Low)
| Button | CAM Board Pin | Function |
|---|---|---|
| **UP** | **GPIO 38** $\leftrightarrow$ GND | Navigate down menu / Toggle view |
| **SELECT** | **GPIO 39** $\leftrightarrow$ GND | Enter screen / Lock target |
| **BACK** | **GPIO 40** $\leftrightarrow$ GND | Return to Main Menu |

### F. Battery Voltage Sensing Divider
```
Li-Po (+) ─── 100 kΩ ───┬─── CAM Board GPIO 1 (ADC1_CH0)
                        │
                       100 kΩ
                        │
                       GND
```

---

## 3. Board 2: ESP32-S3 DevKit (RF Coprocessor)

### A. Power Connections
* `5V` / `VIN` pin $\to$ **+5V Rail**
* `GND` pin $\to$ **Common GND Rail**

### B. TI CC1101 Sub-1 GHz Transceiver Wiring
| CC1101 Pin | DevKit Pin | Function / Note |
|---|---|---|
| **VCC** | DevKit **3V3** | **DO NOT CONNECT TO 5V! 3.3V ONLY!** |
| **GND** | DevKit **GND** | Ground |
| **CSN** | DevKit **GPIO 10** | Manual SPI Chip Select strobe |
| **MOSI** | DevKit **GPIO 11** | SPI2 Data Out |
| **SCK** | DevKit **GPIO 12** | SPI2 Clock (4 MHz) |
| **MISO** | DevKit **GPIO 13** | SPI2 Data In |
| **GDO0** | DevKit **GPIO 9** | Carrier detect / interrupt (optional) |
| **ANT** | **SMA Female Jack** | Connects to 433 MHz rubber ducky antenna |

---

## 4. Inter-Board High-Speed UART Link

| CAM Board (UI Master) | DevKit (RF Coprocessor) | Function |
|---|---|---|
| **GPIO 43 (U0TXD)** | **GPIO 44 (U0RXD)** | CAM commands to DevKit (Mode change, Sweep) |
| **GPIO 44 (U0RXD)** | **GPIO 43 (U0TXD)** | DevKit streams RF telemetry to CAM at 921,600 baud |
| **GND** | **GND** | Common reference (linked via GND rail) |

---

## 5. Master Pin Summary Table

| Board | GPIO / Pin | Connected To | Wire Color Suggestion |
|---|---|---|---|
| **CAM** | **5V** | 5V Rail (from IP5306 via Switch) | **Red (Heavy gauge 22 AWG)** |
| **CAM** | **GND** | Common GND Rail | **Black (Heavy gauge 22 AWG)** |
| **CAM** | **GPIO 1** | Battery Divider Center (VBAT / 2) | White |
| **CAM** | **GPIO 2** | AO3400 Q1 Gate (On-Axis Ring) | Blue |
| **CAM** | **GPIO 3** | 5V IR Module OUT | Yellow |
| **CAM** | **GPIO 14** | AO3400 Q2 Gate (Off-Axis Pair) | Green |
| **CAM** | **GPIO 21** | LCD SDI / MOSI | Brown |
| **CAM** | **GPIO 38** | Button UP $\leftrightarrow$ GND | Gray |
| **CAM** | **GPIO 39** | Button SELECT $\leftrightarrow$ GND | Orange |
| **CAM** | **GPIO 40** | Button BACK $\leftrightarrow$ GND | Purple |
| **CAM** | **GPIO 41** | LCD CS | Pink |
| **CAM** | **GPIO 42** | LCD DC | Tan |
| **CAM** | **GPIO 43** | DevKit GPIO 44 (UART Link) | Violet |
| **CAM** | **GPIO 44** | DevKit GPIO 43 (UART Link) | Cyan |
| **CAM** | **GPIO 47** | LCD SCK | Turquoise |
| **DevKit** | **5V** | 5V Rail | **Red (22 AWG)** |
| **DevKit** | **3V3** | CC1101 VCC (3.3V ONLY!) | Orange |
| **DevKit** | **GPIO 10** | CC1101 CSN | Yellow |
| **DevKit** | **GPIO 11** | CC1101 MOSI | Blue |
| **DevKit** | **GPIO 12** | CC1101 SCK | Purple |
| **DevKit** | **GPIO 13** | CC1101 MISO | Green |
| **DevKit** | **GPIO 9** | CC1101 GDO0 | Gray |

---

## 6. Build & Test Checklist

1. **Power Stage:** Solder Li-Po $\to$ IP5306 $\to$ switch $\to$ 5V rail. Confirm 5.0 V on DMM.
2. **CAM Board Flash Test:** Turn switch OFF. Connect USB-C to laptop. Verify flashing and boot log.
3. **Display Test:** Power CAM board $\to$ verify color bars and SpyScan HUD menu boot.
4. **Optical Strobing:** Connect Q1 & Q2 MOSFETs $\to$ verify AXIS 6-LED ring and OFF-AXIS pair alternating flash.
5. **Hardware IR:** Connect 5V IR receiver $\to$ point TV remote and verify `IR_HW_TRIGGER` in console.
6. **CC1101 & DevKit:** Solder SPI wires to DevKit $\to$ confirm `cc1101,INIT_OK,freq=433920kHz`.
7. **UART Interconnect:** Connect TX $\leftrightarrow$ RX between boards $\to$ verify live Sub-GHz waterfall and Wi-Fi radar on LCD.

