# SpyScan Master Hardware Wiring & Assembly Guide

**Document Version:** 2.0 (Final Handheld Build)  
**Target Systems:**  
- **Board 1 (UI Master):** ESP32-S3-N16R8 Camera Board (OV3660 + 2.4" ILI9341 LCD)  
- **Board 2 (RF Coprocessor):** ESP32-S3 DevKit + TI CC1101 Sub-1 GHz Transceiver  
- **Power Stage:** 1S Li-Po (3.7V) → IP5306 / FM5324 Power Module (5.0V Boost) with Soft-Power Push Button (`K` pad)  
- **Filter Stage:** Single 470 µF 16V Bulk Electrolytic Capacitor  
- **User Interface:** 4× 6×6 mm Tactile Pushbuttons (1× Power, 3× Navigation)  
- **Sensors:** 8× 5mm Red LEDs (6 On-Axis + 2 Off-Axis), YS-IRTM 38 kHz IR Transceiver  

---

## 0. Preparation & Bill of Materials

Before heating up your soldering iron, lay out and confirm all components:

### A. Active Modules
1. **ESP32-S3-N16R8 Camera Board** with OV3660 camera module installed in FPC connector.
2. **2.4" ILI9341 SPI TFT LCD** (320×240 resolution, 3.3V logic).
3. **FM5324 / IP5306 USB-C Power Bank Module** (Black PCB with `K`, `+`, `-`, `5V`, `O` pads and 4 battery LEDs).
4. **1S Li-Po Battery** (3.7V nominal, 4.2V max).
5. **YS-IRTM Serial IR Module** (Blue PCB with STC11F02E MCU, 22.1184 MHz crystal, VS1838B receiver, and 940 nm IR LED).
6. **ESP32-S3 DevKit Board** (Board 2: RF Coprocessor).
7. **TI CC1101 Sub-1 GHz Transceiver Module** with SMA female jack and 433 MHz rubber ducky antenna.

### B. Discrete Components & Resistors
- **4× 6×6 mm momentary tactile pushbuttons** (through-hole).
- **1× 470 µF 16V electrolytic capacitor** (Bulk decoupling for 5V rail).
- **2× AO3400 N-channel MOSFETs** (SOT-23 on breakout adapters or wired directly).
- **8× 5mm High-Brightness Red LEDs** (6 for On-Axis ring, 2 for Off-Axis pair).
- **Resistors (1/4 W through-hole or 0805 SMD):**
  - **8× 150 Ω** (Current limiting resistors: 1 per LED).
  - **2× 100 Ω** (MOSFET gate damping resistors).
  - **2× 100 kΩ** (MOSFET gate pull-down resistors).
  - **2× 100 kΩ** (Li-Po battery voltage divider into GPIO 1).
  - **1× 1 kΩ + 1× 2 kΩ** (YS-IRTM UART 5V $\to$ 3.3V level divider into GPIO 3).
- **Wire:** 28–30 AWG flexible silicone wire for logic signals; 22–24 AWG for 5V and GND power rails.
- **Tools:** Soldering iron, rosin core solder / flux, wire strippers, flush cutters, tweezers, and a Multimeter (DMM).

---

## 1. 6×6 mm Tactile Button Pinout & Wiring

Standard 6×6 mm tactile switches have 4 pins arranged in two internally connected pairs:

```
         Top View (6×6 mm Button)
         ┌─────────┬─────────┐
 Pin A1 ─┤         O         ├─ Pin B1
         │     (Plunger)     │
 Pin A2 ─┤                   ├─ Pin B2
         └─────────┴─────────┘
   Pin A1 and Pin A2 are PERMANENTLY CONNECTED internally.
   Pin B1 and Pin B2 are PERMANENTLY CONNECTED internally.
   Pressing the button CONNECTS pair [A1/A2] to pair [B1/B2].
```

> [!IMPORTANT]
> To wire each button safely:
> 1. Use **diagonally opposite pins** (e.g., Pin A1 and Pin B2).
> 2. Verify with your multimeter in Continuity / Beep mode:
>    - Unpressed $\to$ No beep (Open circuit).
>    - Pressed down $\to$ Continuous beep ($\approx 0\ \Omega$).
> 3. **Common GND:** All 4 buttons have one terminal connected to Common GND. Run a single black GND wire that daisy-chains across all 4 buttons!

### Button Assignments:
| Button ID | Location | Wire From | Wire To | Function |
|---|---|---|---|---|
| **BTN_PWR** | Top Edge | Power Board **`K`** Pad | Common **GND** | Single-click = Power ON 5V rail; Double-click = Power OFF |
| **BTN_UP** | Front Left | CAM Board **GPIO 38** | Common **GND** | Scroll menu down / Cycle mode / Increase value |
| **BTN_SEL** | Front Center| CAM Board **GPIO 39** | Common **GND** | Select / Enter screen / Target reticle lock |
| **BTN_BACK** | Front Right | CAM Board **GPIO 40** | Common **GND** | Return to Main Menu / Exit |

---

## 2. Voltage Rails & Distribution Rules

| Rail | Voltage | Sourced From | Feeds | Max Current |
|---|---|---|---|---|
| **5V Rail** | 5.0 V | Power Board `5V` Pad (activated via `K` button) | CAM Board `5V`, DevKit `5V`, LED Anodes, YS-IRTM Module | 2.1 A |
| **3V3_CAM** | 3.3 V | CAM Board on-board LDO | ILI9341 LCD VCC & Backlight, Button Pull-ups | ~500 mA |
| **3V3_DEV** | 3.3 V | DevKit on-board LDO | **TI CC1101 VCC ONLY** (3.3V Max!) | ~500 mA |
| **GND** | 0 V | Common Star Ground | Everything (Both Boards, Battery, LCD, LEDs, CC1101) | Reference |

> [!CAUTION]
> **CRITICAL VOLTAGE RULE FOR CC1101:**
> Connect CC1101 `VCC` **ONLY to the DevKit's 3.3V pin**. Never connect it to the 5V rail; voltages over 3.6V will permanently destroy the transceiver chip.

---

## 3. Power Stage (Step 1 — Wire & Test First)

Look at your black FM5324 / IP5306 module pads:

```
[Li-Po Battery (3.7V)]
  Li-Po (+) Red wire   ────────► Power Board (+) Pad
  Li-Po (-) Black wire ────────► Power Board (-) Pad

[Soft-Power Button: 6×6 mm Tactile Button on Top Edge]
  Button Leg 1 ────────────────► Power Board (K) Pad (Key Trigger)
  Button Leg 2 ────────────────► Common GND Rail / (-) Pad
  • Single Click : Powers ON 5.0V output & lights battery LEDs (100, 75, 50, 25)
  • Double Click : Powers OFF 5.0V output completely (<50 µA quiescent)

[Power Output & 470 µF 16V Bulk Capacitor]
  Power Board (5V Pad) ────────► [+5V Rail Node] (Red 22 AWG wire)
  Power Board (O / GND Pad) ───► [Common GND Rail Node] (Black 22 AWG wire)

  Across +5V Rail and Common GND:
  • POSITIVE (+) Long leg (no stripe)  ─────► [+5V Rail Node]
  • NEGATIVE (-) Short leg (white stripe '—') ─► [Common GND Rail Node]
```

```
     470 µF 16V Capacitor Polarity:
     ┌───────────┐
     │           │
     │   470µF   │
     │    16V    │
     │           │
     │   [— —]   │ ◄─── White / Gray stripe with MINUS signs = NEGATIVE (GND)
     └─────┬─────┘
           │   │
           │   └─── Shorter lead = NEGATIVE (GND)
           └─────── Longer lead  = POSITIVE (+5V Rail)
```

### Safety & USB Flashing Protocol:
- **Charging:** Plug USB-C directly into the **Power Board USB-C port** on the enclosure exterior. Charging is independent of the ESP32 boards.
- **Flashing Safety:** Before connecting a computer USB cable to either ESP32-S3 for flashing, **double-click the power button to turn the 5V rail OFF** so two 5V sources do not fight each other.

---

## 4. Board 1: ESP32-S3 CAM Board (UI Master)

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

### D. YS-IRTM Serial IR Transceiver Module (VS1838B + STC11F02E)
The YS-IRTM module contains an onboard 8051 MCU, a 22.1184 MHz crystal, a 940 nm IR transmitter LED, and a 38 kHz VS1838B receiver. It is mounted facing out the **TOP edge** of the case.

| YS-IRTM Pin | Connected to | Function / Level Shifting |
|---|---|---|
| **5V** | **+5V Rail** | 5V supply |
| **GND** | **Common GND** | Ground |
| **TXD** | **GPIO 3** (via divider) | Module UART output: `TXD ──1kΩ──┬──► GPIO 3`, `┬──2kΩ──► GND` (shifts 5V to 3.3V) |
| **RXD** | *Unconnected* | Module UART input (leave open or connect to spare GPIO if transmitting) |

*The UART line idles HIGH (3.3V) and drops LOW on the Start bit when an IR packet is received, immediately triggering `IR_HW_TRIGGER` in firmware.*

### E. Tactile Navigation Buttons (Active-Low)
Internal pull-ups are enabled in firmware. Pressing a button pulls the GPIO to GND:
| Button | CAM Board Pin | Function |
|---|---|---|
| **UP** | **GPIO 38** $\leftrightarrow$ GND | Navigate down menu / Toggle view / Cycle options |
| **SELECT** | **GPIO 39** $\leftrightarrow$ GND | Enter screen / Lock target reticle |
| **BACK** | **GPIO 40** $\leftrightarrow$ GND | Return to Main Tactical Menu |

### F. Battery Voltage Sensing Divider
```
Li-Po (+) ─── 100 kΩ ───┬─── CAM Board GPIO 1 (ADC1_CH0)
                         │
                        100 kΩ
                         │
                        GND
```
*Reads $V_{\text{BAT}} / 2$. At 4.2V fully charged cell, the ADC sees 2.10V (well within ESP32-S3 ADC linear range).*

---

## 5. Board 2: ESP32-S3 DevKit (RF Coprocessor)

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
| **ANT** | **SMA Female Jack** | Connects to 433 MHz rubber ducky antenna on top face |

---

## 6. Inter-Board High-Speed UART Link

Connects the UI Master CAM Board and the RF Coprocessor DevKit:

| CAM Board (UI Master) | DevKit (RF Coprocessor) | Function |
|---|---|---|
| **GPIO 43 (U0TXD)** | **GPIO 44 (U0RXD)** | CAM commands to DevKit (Mode change, Sweep start) |
| **GPIO 44 (U0RXD)** | **GPIO 43 (U0TXD)** | DevKit streams RF telemetry to CAM at 921,600 baud |
| **GND** | **GND** | Common reference (linked via GND rail) |

---

## 7. Master Pin Summary Table

| Board | GPIO / Pin | Connected To | Wire Color Suggestion |
|---|---|---|---|
| **CAM** | **5V** | 5V Rail (from Power Board) | **Red (Heavy gauge 22 AWG)** |
| **CAM** | **GND** | Common GND Rail | **Black (Heavy gauge 22 AWG)** |
| **CAM** | **GPIO 1** | Battery Divider Center ($V_{\text{BAT}} / 2$) | White |
| **CAM** | **GPIO 2** | AO3400 Q1 Gate (On-Axis Ring) | Blue |
| **CAM** | **GPIO 3** | YS-IRTM Module TXD (via 1k/2k divider) | Yellow |
| **CAM** | **GPIO 14** | AO3400 Q2 Gate (Off-Axis Pair) | Green |
| **CAM** | **GPIO 21** | LCD SDI / MOSI | Brown |
| **CAM** | **GPIO 38** | Button UP $\leftrightarrow$ GND | Gray |
| **CAM** | **GPIO 39** | Button SELECT $\leftrightarrow$ GND | Orange |
| **CAM** | **GPIO 40** | Button BACK $\leftrightarrow$ GND | Purple |
| **CAM** | **GPIO 41** | LCD CS | Pink |
| **CAM** | **GPIO 42** | LCD DC | Tan |
| **CAM** | **GPIO 43** | DevKit GPIO 44 (UART TX) | Violet |
| **CAM** | **GPIO 44** | DevKit GPIO 43 (UART RX) | Cyan |
| **CAM** | **GPIO 47** | LCD SCK | Turquoise |
| **DevKit** | **5V** | 5V Rail | **Red (22 AWG)** |
| **DevKit** | **3V3** | CC1101 VCC (3.3V ONLY!) | Orange |
| **DevKit** | **GPIO 10** | CC1101 CSN | Yellow |
| **DevKit** | **GPIO 11** | CC1101 MOSI | Blue |
| **DevKit** | **GPIO 12** | CC1101 SCK | Purple |
| **DevKit** | **GPIO 13** | CC1101 MISO | Green |
| **DevKit** | **GPIO 9** | CC1101 GDO0 | Gray |
| **Power Board**| **K** | Button POWER $\leftrightarrow$ GND | Yellow |

---

## 8. Step-by-Step Soldering & Testing Workflow

Follow this exact sequence. **Do not solder the next stage until the current stage passes its test!**

### Step 1: Power Subsystem & Soft-Switch
1. Solder Li-Po battery Red wire to `+` and Black wire to `-` on the power board.
2. Solder two wires to your **Power tactile button**:
   - One wire to the `K` pad.
   - One wire to the `-` / GND pad.
3. Solder the **470 µF 16V capacitor** across the `5V` and `O`/GND pads:
   - Long leg $\to$ `5V`.
   - Striped leg (`—`) $\to$ `O`/GND.
4. Solder heavy red and black leads (22 AWG) from `5V` and `O`/GND to act as your main power bus.
5. **CHECKPOINT 1:**
   - Single-click the Power button: The 4 battery LEDs illuminate, and your DMM measures **$5.05\text{ V} \pm 0.1\text{ V}$** between 5V and GND.
   - Double-click the Power button: The LEDs turn off and voltage drops to **$0.0\text{ V}$**.
   - Plug phone charger into USB-C: Fuel gauge LEDs flash, indicating battery charging.

### Step 2: CAM Board & Display HUD
1. Double-click the power button to turn power OFF.
2. Solder 5V and GND from your power bus to the CAM Board `5V` and `GND` pins.
3. Solder the 7 display wires (VCC, GND, CS, RESET, DC, MOSI, SCK, LED) according to Section 4.B.
4. Solder the 3 front navigation buttons (UP $\to$ GPIO 38, SEL $\to$ GPIO 39, BACK $\to$ GPIO 40, all with common leg to GND).
5. Solder the two 100 kΩ resistors for the battery voltage divider to GPIO 1.
6. **CHECKPOINT 2:**
   - Single-click the power button to turn power ON.
   - The ILI9341 display should immediately light up, show initialization color bars, and launch the **SpyScan Tactical HUD Menu**.
   - Check the top status bar: Battery voltage (e.g. `4.1V 95%`) should match your DMM reading.
   - Press the **UP** button: Menu cursor moves down smoothly.
   - Press **SELECT**: Opens the selected screen.
   - Press **BACK**: Returns to the main menu.

### Step 3: Optical Strobing LEDs (On-Axis Ring & Off-Axis Pair)
1. Assemble the 2× AO3400 MOSFET circuits on small perfboard or adapter boards:
   - Gate resistors: 100 Ω from GPIO 2 (Q1) and GPIO 14 (Q2).
   - Gate pull-downs: 100 kΩ from Gate to GND.
   - Sources to GND.
2. Wire the 6× On-Axis LEDs:
   - Anodes connected to 5V rail through individual 150 Ω resistors.
   - Cathodes connected together to Q1 Drain.
3. Wire the 2× Off-Axis LEDs:
   - Anodes to 5V rail through 150 Ω resistors.
   - Cathodes to Q2 Drain.
4. **CHECKPOINT 3:**
   - Navigate to **Screen 1 (OPTICAL RETROREFLECTION)** and press SELECT.
   - The On-Axis 6-LED ring strobes brightly at 10 Hz.
   - The live camera viewfinder shows synchronous contrast subtraction with green reticle lock on any shiny optical target (e.g., a phone camera or mirror).

### Step 4: YS-IRTM UART Infrared Probe
1. Wire 5V and GND of the YS-IRTM module to the 5V and GND rails.
2. Build the voltage divider on `TXD`:
   - `TXD` $\to$ 1 kΩ resistor $\to$ Node $\to$ CAM Board GPIO 3.
   - Node $\to$ 2 kΩ resistor $\to$ GND.
3. Leave `RXD` unconnected for now.
4. Mount the YS-IRTM pointing out the **top edge** of the case (next to the antenna).
5. **CHECKPOINT 4:**
   - Navigate to **Screen 2 (PASSIVE IR HUNTER)**.
   - Take any standard TV or AC infrared remote control, point it at the top probe, and press any button.
   - The screen should instantly flash with an amber/red **`IR ALERT`** banner and register the signal in real-time.

### Step 5: DevKit & CC1101 Sub-GHz Coprocessor
1. Solder 5V and GND from power bus to DevKit `5V` and `GND`.
2. Solder CC1101 module to DevKit:
   - **`VCC` $\to$ DevKit 3.3V pin** (Double check this! Never 5V!).
   - `GND` $\to$ DevKit GND.
   - SPI lines: CSN $\to$ GPIO 10, MOSI $\to$ GPIO 11, SCK $\to$ GPIO 12, MISO $\to$ GPIO 13, GDO0 $\to$ GPIO 9.
3. Connect the inter-board UART link:
   - DevKit GPIO 43 (TX) $\to$ CAM Board GPIO 44 (RX).
   - DevKit GPIO 44 (RX) $\to$ CAM Board GPIO 43 (TX).
4. **CHECKPOINT 5:**
   - Power ON the system and navigate to **Screen 5 (SUB-1 GHz BUG HUNTER)**.
   - Press a 433 MHz car remote or wireless doorbell button.
   - The rolling waterfall display on the LCD should light up with energy spikes, showing the frequency and classified duty cycle!
   - Navigate to **Screen 3 (WI-FI RADAR)**: Proximity signal strength ("Warmer / Colder") tracks active Wi-Fi emitters in real time.

---

## 9. Final Bench Inspection & Case Enclosure Assembly
- Verify no exposed wires can touch each other inside the enclosure. Cover all resistor joints with heat-shrink tubing or Kapton tape.
- Mount the 3 front navigation buttons into the front shell (using the 3D-printed button carrier or a small strip of perfboard).
- Insert the ILI9341 LCD into its retaining shelf and press the ESP32-S3 camera lens into the chamfered optical cone.
- Mount the 6-LED ring into the 6 outer holes around the lens, and the 2-LED pair into the side holes.
- Slide the YS-IRTM probe components (VS1838B dark dome and 5mm IR LED) into the top cutouts.
- Screw the SMA antenna connector into the top Ø6.5 mm antenna hole.
- Place the Li-Po battery and power board into their retaining bays in the back shell.
- Fasten the front and back shells together using 4× M3 screws into the corner heat-set inserts.
- Turn on your fully assembled SpyScan handheld TSCM detector and begin sweeping!
