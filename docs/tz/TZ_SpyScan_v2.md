# Technical Specification (ТЗ) — SpyScan

**Portable embedded device for detecting hidden video cameras**
*(semester prototype V1)*

| | |
|---|---|
| Course | Embedded Systems (ITMO), project track, stream 1.3 |
| Instructor | Zinovichev E. S. |
| Developer | Rahaman Md Afifur, group P3330 |
| Document version | **2.0** (revision of the defended draft v1 "CamGuard 360") |
| Date | 2026-10-01 |
| Status | Draft for instructor review |
| Basis | ГОСТ 19.201-78; course ТЗ template |

### Revision history

| Version | Date | Changes |
|---|---|---|
| 1.0 | 09.2026 | Draft defended as "CamGuard 360". |
| 2.0 | 2026-10-01 | The goal is now stated in terms of the problem, following the defense feedback. Requirements are measurable and have IDs. The optical method changed from a brightness threshold to differential (LED ON/OFF) retroreflection imaging. The wireless method changed from an AP/BLE list scan to passive Wi-Fi traffic analysis with stimulus–response correlation. Added a bill of materials, a dated schedule and acceptance tests with pass criteria. Renamed to SpyScan. |

### Conventions used in this document

Every technical statement that matters for the design is tagged with how much we actually know:

| Tag | Meaning |
|---|---|
| **[F] Fact** | Supported by physics, datasheets, standards or published research (with a reference). |
| **[A] Assumption** | Believed true, not yet verified; must be checked. |
| **[H] Hypothesis** | Something we think may work; tested by a named experiment (E-x). |
| **[D] Decision** | A design choice we make deliberately, with justification. |
| **[T] Target** | A numerical requirement we aim for; the final value is confirmed or revised by measurement. |
| **[M] Measured** | An experimental result. *(None exist yet. This document contains no measured results.)* |

Requirement priorities: **M** = mandatory for acceptance; **S** = should (planned, not blocking); **C** = could (if time permits).

---

## 1. General information

### 1.1 Problem

Covert video recording in places where people expect privacy is a documented and growing problem. These places include rented apartments, hotel rooms, changing rooms, toilets, offices and meeting rooms. The enabling factor is technical: complete Wi-Fi cameras are now sold in packages of a few cubic centimetres (screw-sized "pinhole" lenses, chargers, clocks, smoke detectors), cost little and need no expertise to install.

A person entering an unfamiliar room has no practical way to check it:

1. **Visual inspection is unreliable.** The exposed part of a hidden camera can be an aperture of 1–3 mm, which is easy to miss. [F]
2. **Cheap consumer detectors give ambiguous evidence.**
   - *RF "bug detectors"* react to any transmitter (phones, routers, Bluetooth), so they cannot say *which* device is a camera, and they miss cameras that do not transmit. [F]
   - *Optical "lens finders"* (an LED ring plus a viewing window) depend on a person noticing a small glint among many reflections from glass, metal and screws. [F]
3. **Smartphone apps that list Wi-Fi networks** only see devices that broadcast their own access point. A camera connected as a client to the room's router does not appear in such a list. [F, ESP-IDF Wi-Fi scan semantics, ref. 3]
4. **Professional equipment** (spectrum analysers, non-linear junction detectors, professional optical detectors) is expensive and needs a trained operator. [F, category-level; specific models and prices are to be cited in Section 4.3]

**The core problem** is not the lack of signals; a camera always has a lens, and a wireless camera always produces traffic. The problem is that existing affordable tools do not turn these signals into **evidence that is specific to a camera** that a non-specialist can act on.

### 1.2 Relevance and physical basis of the approach

The device is built on two physical properties that *every camera of the target class* has. Both can be measured with low-cost embedded hardware.

**(a) A camera must have a lens focused on an image sensor, and this makes it a retroreflector ("cat's-eye effect") [F, ref. 6, 7].**
- Light entering the lens is focused onto the sensor plane. Part of it is reflected by the sensor surface and the filter stack, then leaves back through the same lens *towards the light source* in a narrow cone.
- A flat mirror or glass reflects light back only when it is viewed at a perpendicular angle. A self-luminous object (a lamp or an indicator LED) does not depend on our illumination at all.
- Therefore, *illuminating the scene from a point next to our own camera, and comparing frames with and without that illumination*, isolates objects that return our light to its source. Camera lenses are among those objects.

**(b) A wireless camera must transmit video, and modern video encoders produce a bit rate that depends on the content of the scene [F, ref. 8, 9, 10].**
- The Wi-Fi payload is encrypted, but frame sizes, timing, transmitter addresses and signal strength are visible to any receiver on the channel.
- If we deliberately change what the camera sees (motion or a change in lighting) on a known time pattern, *only a device that is actually filming the scene* will show a matching change in its uplink traffic.
- This turns "there is an unknown Wi-Fi device" into "this specific device reacts to what happens in the room". This is published research (DeWiCam, Singh et al., Lumos, ref. 8–10). It has mostly been demonstrated on smartphones and laptops, not on a low-cost standalone embedded device.

The two methods are **complementary**:
- The optical method works for cameras that do not transmit (local SD-card recording) if the lens is in line of sight.
- The wireless method works for streaming cameras even when the lens is well hidden or outside the scanned area.

### 1.3 Goal of the project

> **To reduce the risk of undetected covert video recording in temporary and private premises by giving a non-specialist a portable, affordable means to obtain camera-specific evidence: locating camera lenses by their optical retroreflection, and identifying wireless cameras by the response of their network traffic to a controlled change of the scene.**

The result is measured by the detection and false-alarm rates the prototype achieves under the test conditions in Section 6, not by the existence of the device.

### 1.4 Tasks

To achieve the goal, the following tasks shall be completed:

1. Analyse the physical principles of hidden-camera detection, existing devices and published methods. Define the class of target cameras and the limits of detectability.
2. Experimentally confirm the feasibility of both methods on the selected hardware (experiments E1–E4, Section 6.2). Determine the numerical parameters (distances, LED geometry, thresholds) from measured data.
3. Design the hardware architecture (structural diagram, interfaces, wiring, power budget) and the software architecture (modules, tasks, data flows).
4. Implement the firmware:
   - drivers;
   - synchronised camera–LED acquisition;
   - differential retroreflection detection;
   - a Wi-Fi passive monitor;
   - a stimulus–response correlation detector;
   - the user interface;
   - logging.
5. Build a battery-powered prototype in a 3D-printed enclosure.
6. Run the test programme (Section 6). Measure detection rate, false-alarm rate, range, angle, response time and battery life.
7. Prepare the documentation, report and demonstration. State the limitations honestly.

### 1.5 Brief description of the system and its use

SpyScan is a handheld device (roughly the size of a smartphone, with a thicker optical head). It contains:
- an ESP32-S3 microcontroller;
- a small camera surrounded by a tight ring of red LEDs;
- additional off-axis LEDs;
- a colour display;
- three buttons;
- a Li-Po battery.

It provides two inspection modes.

**Mode 1 — Optical scan (lens finder).**
- The user slowly sweeps the device across the room.
- The device alternates LED-ON and LED-OFF frames and computes their difference. It marks on the live preview the points that return its own light, then checks how each point responds to on-axis versus off-axis illumination.
- Points that behave like a retroreflector are shown as **lens candidates**, together with a strength indicator.

**Mode 2 — Wireless scan (traffic analysis).**
1. *Survey.* The device passively listens on 2.4 GHz channels 1–13. It builds a table of active transmitters (access points *and* client devices) with MAC address, vendor (from the OUI), channel, RSSI and uplink data rate. Devices that stream continuously are highlighted.
2. *Challenge.* For a selected device or the top candidates, the device generates a pseudo-random ON/OFF timing pattern. The user follows on-screen prompts ("move" / "stay still", or the lights on/off). The device correlates each device's uplink rate with the pattern and reports a statistical camera-likelihood score.
3. *Locate.* For the flagged device, a smoothed RSSI indicator ("warmer / colder") guides the user towards the source. The optical mode is then used to find the lens.

A **developer mode** streams raw data (frames or difference maps, per-MAC traffic time series, RSSI, detector statistics) over USB to a PC for experiments and for the defense.

### 1.6 Scope of the semester prototype (V1)

| In scope (V1) | Out of scope (future, Section 8) |
|---|---|
| Differential optical retroreflection detection (red LEDs, visible camera) | NIR (850/940 nm) illumination and multi-wavelength analysis |
| On-/off-axis illumination test for false-candidate reduction | ML-based lens classification |
| Passive 2.4 GHz Wi-Fi monitor (APs and clients, RSSI, traffic rate, OUI) | 5 GHz Wi-Fi (not supported by ESP32-S3 hardware [F]) |
| Stimulus–traffic correlation detector | SDR spectrum analysis, analogue video transmitters, sub-GHz |
| RSSI-based approach guidance | Direction finding with directional antennas |
| BLE advertisement list (informational, priority C) | EM side-channel detection of offline cameras, thermal sensing, NLJD |
| Battery operation, 3D-printed enclosure, USB logging | Multi-sensor fusion beyond the two V1 methods |

### 1.7 Development team

The project is carried out individually.

| Member | Role | Responsibilities |
|---|---|---|
| Rahaman Md Afifur (P3330) | Developer (all roles) | Research and experiments; hardware design and assembly; firmware (ESP-IDF, C); signal-processing and detection algorithms; PC analysis tools (Python); UI; enclosure; testing; documentation, report, defense |

---

## 2. Technical requirements

### 2.1 Functional requirements

#### 2.1.1 General

| ID | Requirement | Pri. |
|---|---|---|
| FR-G1 | The device shall provide two inspection modes, **Optical scan** and **Wireless scan**, selectable from a menu with physical buttons. | M |
| FR-G2 | All detection functions shall work autonomously, without Internet, cloud services, a smartphone or a PC. | M |
| FR-G3 | The device shall never present a result as proof. Results shall be worded as *candidates* or *likelihood* levels, with the evidence shown (for example "retroreflection: strong, off-axis response: none"). | M |
| FR-G4 | A **developer mode** shall stream raw and processed data over USB (CSV/binary over serial) for offline analysis. | M |
| FR-G5 | Scan results (time, mode, candidates, flagged MACs, scores) shall be kept in RAM for the session and be viewable in a history screen. | S |
| FR-G6 | Results shall be saved to non-volatile storage (flash/microSD). | C |

#### 2.1.2 Optical scan (Method 1: differential retroreflection)

| ID | Requirement | Pri. |
|---|---|---|
| FR-O1 | The camera and the **on-axis** LED group shall be driven synchronously so that frames are captured alternately with the LEDs ON and OFF. [D] | M |
| FR-O2 | Camera auto-exposure, auto-gain and auto-white-balance shall be locked during a scan. Without this, ON/OFF frames are not comparable. [D] | M |
| FR-O3 | The detector shall compute the difference image (ON − OFF). It shall detect local maxima and blobs exceeding an **adaptive threshold** derived from the noise statistics of the difference image (for example mean + k·σ, with k set from E1 data). Fixed brightness thresholds shall not be used. [D] | M |
| FR-O4 | A candidate shall be confirmed only if it persists across N consecutive ON/OFF pairs within a position tolerance of r pixels. N and r are determined experimentally (E1). | M |
| FR-O5 | Self-luminous sources (lamps, indicator LEDs, screens) shall be suppressed by the differential principle and not reported as candidates. | M |
| FR-O6 | **Off-axis test:** for each confirmed candidate, the device shall compare the response to on-axis LEDs with the response to off-axis LEDs. It shall classify the candidate as *retroreflector-like* (on-axis ≫ off-axis) or *specular/diffuse-like*. [H1, tested in E3] | S |
| FR-O7 | Candidates shall be shown on the live preview with markers and a strength level (for example 3 bars), and the number of candidates shall be displayed. | M |
| FR-O8 | An audible or vibration cue shall indicate a strong lens candidate in the centre of the field of view, so the user can find it without looking at the screen. | C |

#### 2.1.3 Wireless scan (Method 2: Wi-Fi traffic analysis)

| ID | Requirement | Pri. |
|---|---|---|
| FR-W1 | The device shall operate the Wi-Fi radio in **passive (promiscuous / monitor) mode**. It shall not transmit except where needed for the optional AP scan. | M |
| FR-W2 | **Survey:** the device shall sweep channels 1–13. For every observed transmitter it shall maintain: MAC, role (AP / client, from the 802.11 DS bits), BSSID it is associated with, channel, RSSI (smoothed), frames/s, and uplink bytes/s. | M |
| FR-W3 | The vendor name shall be derived from the MAC OUI using an on-device table of relevant vendors (camera/IoT SoC vendors, common phone/PC vendors). Locally administered (randomised) MACs shall be marked as such. | S |
| FR-W4 | Devices with sustained uplink traffic (a "streaming" pattern) shall be highlighted as candidates for the challenge. The criterion shall be defined from E2 data. | M |
| FR-W5 | **Challenge:** the device shall generate a pseudo-random binary stimulus sequence of K slots, each of duration T, and prompt the user for each slot. Prompt variants are (a) "MOVE in front of the area" / "STAY STILL", or (b) "LIGHTS ON" / "LIGHTS OFF". The receiver shall stay on the target's channel. K and T are determined in E2. | M |
| FR-W6 | For each monitored MAC, the detector shall compute a test statistic between the stimulus sequence and the binned uplink byte rate, such as a normalised correlation or a two-sample test of ON versus OFF slots. The decision threshold shall be set from the **null distribution** (permutation test or stimulus-free recordings) for a target false-alarm probability per device of ≤ 1 % [T]. | M |
| FR-W7 | The result per device shall be shown as a likelihood level (for example NONE / LOW / HIGH) together with the statistic and the p-value (the latter in developer mode). | M |
| FR-W8 | **Locate:** for a selected MAC, the device shall display the smoothed RSSI with at least 2 updates/s, as a number and a bar, plus a trend indicator (rising / falling). | M |
| FR-W9 | A conventional AP scan (SSID, BSSID, RSSI, channel) shall be available as a sub-view. | S |
| FR-W10 | A BLE advertisement scan (name, address, RSSI) shall be available as a sub-view. It is informational only and is not used in the decision. [D: BLE is not the video channel of the target camera class.] | C |

#### 2.1.4 User interface

| ID | Requirement | Pri. |
|---|---|---|
| FR-U1 | The UI shall use three buttons (Up/Mode, Select, Back) and a colour display of at least 240×240 pixels, capable of showing a camera preview with markers. | M |
| FR-U2 | Screens: main menu; optical scan (preview + candidates); wireless survey (sortable table); challenge (prompt + progress + result); locate (RSSI meter); history; settings/diagnostics (battery, firmware version, sensor status). | M |
| FR-U3 | The battery level shall be displayed. A low-battery warning shall be shown, and a safe shutdown of scanning functions shall occur below a set voltage. | M |
| FR-U4 | The UI shall remain responsive (button reaction ≤ 200 ms [T]) during scanning. | M |

### 2.2 Reliability requirements

| ID | Requirement | Pri. |
|---|---|---|
| NR-R1 | The device shall operate continuously for ≥ 30 min in a demonstration scenario (mode switching, both scans) without a critical failure or manual reboot. | M |
| NR-R2 | The failure of one subsystem (camera not detected, display error) shall not disable the other mode. Errors shall be shown on screen (or on the USB log if the display fails). The firmware shall not enter a reboot loop. | M |
| NR-R3 | A hardware watchdog shall be enabled. Task stack and heap usage shall be monitored and reported in diagnostics. | S |
| NR-R4 | Repeatability: under unchanged conditions, repeated optical scans of the same scene shall produce the same confirmed candidates in ≥ 90 % of repetitions [T]. | M |

### 2.3 Operating conditions

- Indoor use only; temperature +10 … +35 °C; no condensation or water exposure; no strong shock or vibration.
- Ambient light: from darkness up to typical office lighting (≈ 500 lx [T]). Direct sunlight in the field of view is outside the specified range.
- **Detection limits that are inherent to the physics** (they will be stated in the user manual):
  - *Optical:* the lens must be in line of sight of the device within its field of view. The detection range depends on aperture size, the angle between the device and the hidden camera's optical axis, and the coatings. A lens behind a tinted or IR-only window may not be detectable with red light. [A, quantified in E1]
  - *Wireless:* only 2.4 GHz 802.11 b/g/n transmitters are visible. Cameras on 5 GHz, wired (Ethernet/PoE) cameras, cameras recording only to an SD card, and cameras that do not transmit during the test are not detected by Method 2. [F]
- Use of the device is passive (receiving only). The LED illumination is low-power visible light.

### 2.4 Hardware composition and parameters

#### 2.4.1 Architecture decisions

| # | Decision | Justification |
|---|---|---|
| D1 | **Single main processor: ESP32-S3** with ≥ 8 MB PSRAM and a DVP camera interface. No Raspberry Pi in V1. | The ESP32-S3 has a camera interface, a 2.4 GHz Wi-Fi radio with promiscuous mode, BLE, vector instructions and PSRAM for frame buffers [F, ref. 2–5]. Using one chip reduces power, boot time and inter-processor interfaces. |
| D2 | The developer's existing **ESP32-S3 DevKit** is used immediately for the Wi-Fi experiments (E2) and as a reference sniffer. If E4 shows that sharing one CPU and radio between camera processing and traffic monitoring causes frame or packet loss, it becomes a **dedicated Wi-Fi co-processor connected by UART**. The final choice is made at the Architecture stage. | Lowers risk without new purchases. |
| D3 | **Red LEDs (≈ 620–660 nm)** for V1, not NIR. | Standard OV2640 camera modules usually have an IR-cut filter, so NIR would be largely invisible to our own camera [A, checked on the received module]. The visible red light also lets the user see where the device is pointing. |
| D4 | **On-axis ring:** 4–6 LEDs as close as mechanically possible to the camera lens (target centre offset ≤ 10 mm [T]). **Off-axis group:** 2 LEDs at ≥ 30 mm [T]. The exact geometry follows from E1/E3. | The retroreflected cone is narrow, so the illumination must be close to the observation axis [F, ref. 6, 7]. |
| D5 | LEDs are driven through low-side logic-level N-MOSFETs controlled by GPIO/LEDC, synchronised with the camera VSYNC. | MCU pins cannot supply LED current. Synchronisation is required for clean ON/OFF frames. |
| D6 | The 433 MHz modules owned by the developer (Ra-02 LoRa, CC1101) are **not used in V1**. | The target cameras do not use 433 MHz for video. These modules are possible material for a future sub-GHz survey. |

#### 2.4.2 Component list (V1)

| # | Component | Example / parameters | Interface | Function | Status |
|---|---|---|---|---|---|
| 1 | ESP32-S3 camera board | e.g. Freenove ESP32-S3-WROOM CAM or Seeed XIAO ESP32-S3 Sense; ≥ 8 MB PSRAM; USB-C | — | Main processor, Wi-Fi/BLE | to buy |
| 2 | Camera module | OV2640 (or OV3660) supplied with the board | DVP (8-bit parallel) + SCCB | Image acquisition | with board |
| 3 | ESP32-S3 DevKit | owned | USB / UART | E2 sniffer, possible co-processor (D2) | owned |
| 4 | Red LEDs, high brightness | 5 mm or 3 mm, 620–660 nm, narrow angle; 6–8 pcs | GPIO via MOSFET | On-/off-axis illumination | to buy |
| 5 | N-MOSFETs + resistors | AO3400A (or equivalent, logic level), gate and current-limiting resistors | GPIO | LED drivers | lab / buy |
| 6 | Display | 1.9–2.0" IPS, ST7789, 240×320 | SPI | UI, preview | to buy |
| 7 | Buttons | 3 × tactile 6×6 mm | GPIO (internal pull-ups) | Control | lab |
| 8 | Buzzer or vibration motor | optional | GPIO/PWM via transistor | Feedback (FR-O8) | lab / C |
| 9 | Li-Po battery | 1S 3.7 V, 1500–2500 mAh, with protection | — | Power | to buy |
| 10 | Charger | TP4056-class module, USB-C, with DW01 protection | — | Charging | to buy |
| 11 | Regulator / switch | 3.3 V on the board (check dropout at low battery) or an external buck-boost; slide switch | — | Power path | to buy if needed |
| 12 | Battery-voltage divider | 2 resistors (e.g. 100 k / 100 k) | ADC | Battery monitor | lab |
| 13 | Enclosure | PLA/PETG, lab 3D printer | — | Housing, optical head | lab |
| 14 | **Test targets** | 1–2 mini Wi-Fi cameras (2.4 GHz, client/STA mode support); 1 ESP32-CAM (AI-Thinker) as a controllable reference camera; smartphone camera | — | Experiments and acceptance | to buy |
| 15 | **Distractor set** | screws, glass, mirror, chrome, jewellery, LED indicators, lamp; Wi-Fi devices (router, phones, laptop) | — | False-alarm tests | own / lab |
| 16 | Measuring tools | multimeter, USB power meter, tape measure, angle jig/protractor, lux meter (or a calibrated app) | — | Experiments | lab |

#### 2.4.3 Parameter targets

| ID | Parameter | Target [T] | Verified by |
|---|---|---|---|
| NR-P1 | Optical detection range (reference pinhole camera, on-axis, ≤ 500 lx) | ≥ 1.0 m mandatory; ≥ 2.0 m desired | AT-1 |
| NR-P2 | Angular tolerance (angle between the hidden camera's axis and the line to SpyScan) | ≥ ±30° at 1.0 m | AT-2 |
| NR-P3 | Optical update rate of the candidate overlay | ≥ 5 Hz | AT-9 |
| NR-P4 | Wi-Fi survey of channels 1–13 | complete sweep ≤ 15 s | AT-5 |
| NR-P5 | Challenge duration to a decision | ≤ 60 s | AT-6 |
| NR-P6 | Battery life, continuous mixed scanning | ≥ 1.5 h (computed in the power budget, then measured) | AT-10 |
| NR-P7 | Boot to main menu | ≤ 5 s | AT-9 |
| NR-P8 | Mass / size | ≤ 300 g; hand-held | inspection |

*All targets will be revised once E1–E4 are complete. A changed target will be recorded in the revision history together with the experimental reason.*

### 2.5 Information and software compatibility

- Firmware: **ESP-IDF** (v5.x), language **C**, with **FreeRTOS** (part of ESP-IDF) [D]. Components: `esp32-camera`, `esp_wifi` (promiscuous API), `esp_lcd` or LVGL for the display, NimBLE (only for FR-W10). Arduino may be used for quick experiments only.
- Real-time structure [D, to be detailed in Architecture]: separate FreeRTOS tasks for camera acquisition and LED sync, optical processing, Wi-Fi monitor (the packet callback only queues compact records), the correlation detector, UI and logging. They communicate through queues and event groups.
- Peripheral interfaces: DVP + SCCB (camera), SPI (display), GPIO/LEDC (LEDs, buttons, buzzer), ADC (battery), USB-Serial/JTAG (developer mode, flashing), UART (optional co-processor, D2).
- PC tools: **Python 3** (numpy, pandas, matplotlib) for capturing developer-mode data, analysing experiments and setting thresholds.
- Data formats: CSV for time series and logs; raw/PGM for frames. Each experiment record carries its metadata (distance, angle, lux, target, firmware version).
- Version control: public or instructor-shared **GitHub** repository. Each stage is tagged.

### 2.6 Transportation and storage

- Transport switched off, in a case or soft pouch protecting the display and the optical head.
- Protect from moisture, impact and short circuit of the battery. Battery storage charge ≈ 40–60 % for long-term storage.
- Store indoors, dry, at +5 … +30 °C.

### 2.7 Safety and legal

- The device receives only. Promiscuous capture uses frame headers and metadata. Payloads are encrypted and are neither decrypted nor stored [D]. Captured MAC addresses are kept only in RAM during the session unless developer logging is explicitly enabled for experiments in our own test network.
- LED illumination is low-power visible light. Optical radiation safety will be checked against the LED datasheet values. No lasers are used [D].

---

## 3. Documentation requirements

The following documents shall be prepared and kept in the repository:

1. Technical Specification (this document) with its revision history.
2. **Analysis report:** physical principles, analogues, the fact/assumption/hypothesis register.
3. **Experiment protocols and results** (E1–E4 and the acceptance tests): purpose, set-up photo, procedure, raw data, analysis script and conclusion.
4. **Hardware documentation:** structural (block) diagram, wiring and circuit diagram, interface table, pin map, **power budget calculation**, BOM.
5. **Software documentation:** module/structural diagram (PlantUML), task and timing diagram, data-flow description, algorithm description with parameters and their experimental justification, build/flash instructions.
6. **Enclosure 3D model** (FreeCAD/OpenSCAD source + STL).
7. **User manual** (short): controls, scanning procedure, interpretation of results, limitations.
8. **Source code** (firmware + PC tools) with README.
9. **Work log** in the repository (dated entries and tagged milestones), so the instructor can follow progress.
10. **Final report** in the course structure, plus the defense presentation.

---

## 4. Technical and economic indicators

### 4.1 Preliminary cost estimate (single prototype)

*Prices are pre-purchase estimates. They will be replaced with actual receipts and links once purchased.*

| Item | Qty | Est. price, RUB |
|---|---|---|
| ESP32-S3 camera board (≥ 8 MB PSRAM, OV2640) | 1 | 1 500 – 2 500 |
| ST7789 1.9–2.0" IPS display | 1 | 400 – 700 |
| Li-Po 1S 1500–2500 mAh | 1 | 400 – 800 |
| USB-C TP4056-class charger with protection | 1 | 100 – 250 |
| Red LEDs, MOSFETs, resistors, buttons, switch, wires, perfboard | set | 300 – 600 |
| Test cameras: mini Wi-Fi camera + ESP32-CAM | 2 | 1 300 – 2 900 |
| Enclosure filament (lab printer) | — | lab |
| ESP32-S3 DevKit (already owned) | 1 | 0 |
| **Total** | | **≈ 4 000 – 7 750** |

**Funding:** self-funded by the developer. Lab equipment and the 3D printer are used under lab access. The estimate will be approved with the instructor before purchase.

### 4.2 Serial-production estimate

With a custom PCB, a bare ESP32-S3 module, a camera module, a display and an injection-moulded enclosure, the bill of materials is estimated at **≈ 1 500 – 3 000 RUB per unit** [A]. This excludes certification, tooling, logistics and margin. The estimate is indicative; it will be refined only if time permits.

### 4.3 Comparison with analogues

| Class | Principle | Limitation addressed by SpyScan |
|---|---|---|
| Consumer combined "RF + lens finder" detectors (K18-type and similar) | Broadband RF power meter; red LED ring with a viewing filter | The RF meter cannot tell *which* transmitter is a camera, and phones and routers cause alarms. The lens finder relies on the human eye. |
| Smartphone apps (Wi-Fi/LAN scanners, "camera finder" apps) | List networks or hosts; flashlight + screen | Only see devices on the user's own network or broadcasting APs. No camera-specific evidence. |
| Research systems (DeWiCam, Singh et al., Lumos, LAPD) | Traffic patterns / motion correlation on laptops and phones; phone ToF sensor retroreflection | Demonstrated on phones and laptops with specific hardware. SpyScan studies whether both principles can run together on a low-cost, standalone embedded device. |
| Professional equipment (spectrum analysers, NLJD, professional optical detectors) | Wideband RF analysis; non-linear junction response; professional optics | High cost and a trained operator are needed. |

*Specific domestic and foreign models with verified prices will be added to this table, with sources, during the analysis stage. No unverified figures are included here.*

**Expected advantage of SpyScan:** camera-specific evidence (retroreflection behaviour plus stimulus–traffic response) instead of a generic "signal present" alarm, at a component cost comparable to consumer detectors.

---

## 5. Stages and schedule of development

The developer is responsible for all stages. Dates are planned; the interim demonstration and defense dates will be aligned with the instructor.

| # | Stage | Content | Deliverables | Planned dates |
|---|---|---|---|---|
| 1 | Specification and repository | Revise the ТЗ; set up the repository; draft the estimate | ТЗ v2, repo link sent to the instructor, BOM | 01.10 – 09.10 |
| 2 | Feasibility experiments | E2 on the owned DevKit (immediately); purchase parts; E1, E3 on the camera board; E4 load test | Experiment protocols + data, revised targets | 05.10 – 25.10 |
| 3 | Architecture (graded stage) | HW block diagram, wiring, interfaces, power budget; SW module and task diagrams; enclosure concept | Architecture document, PlantUML sources | 19.10 – 01.11 |
| 4 | Hardware bring-up | Breadboard: camera, LEDs + drivers, display, buttons, battery path | Working bench, bring-up checklist | 26.10 – 08.11 |
| 5 | Firmware | Drivers; synced acquisition; Wi-Fi monitor; UI; developer-mode streaming; logging | Firmware releases (tags) | 02.11 – 29.11 |
| 6 | Algorithms and calibration | Differential detector, persistence, off-axis test; traffic features; correlation detector; thresholds from data | Algorithm document, datasets, analysis scripts | 09.11 – 29.11 |
| — | **Interim demonstration** | Bench device + key function + processed data + plan | Demo | ≈ 16.11 – 23.11 (TBC) |
| 7 | Enclosure and integration | 3D model, printing, assembly, battery integration | Integrated prototype, STL/CAD | 16.11 – 04.12 |
| 8 | Test programme | Acceptance tests AT-1 … AT-11 | Test report with measured data | 30.11 – 08.12 |
| 9 | PIKT-Fest / report / defense | Presentation, demo scenario, final report | Report, slides, demo | first half of Dec (PIKT-Fest); defense on the credit date |

---

## 6. Control and acceptance procedure

### 6.1 General

- Acceptance is based on the **acceptance tests (AT)** below, performed on the integrated battery-powered prototype.
- For every test, the raw data and the analysis script are stored in the repository. Results are reported as measured values with the number of trials. Failures are reported, not hidden.
- A test passes if its criterion is met. If a [T] target is not met, the measured value and the physical reason are documented, and the requirement is revised with the instructor.

### 6.2 Feasibility experiments (before the Architecture stage)

| ID | Question | Method (summary) |
|---|---|---|
| **E1** | How strong is the differential retroreflection signal versus distance, LED offset and ambient light? What threshold k, persistence N and tolerance r are appropriate? | ESP32-CAM / mini-camera lens as the target at 0.5–3 m in steps; LED ring offsets of 5 / 10 / 20 mm; 0 / 200 / 500 lx; ≥ 20 frame pairs per point; record the peak contrast-to-noise ratio. |
| **E2** | Can the ESP32-S3 observe a camera's uplink traffic? Does its rate respond to motion or lighting changes, and how much do non-camera devices respond? | Owned DevKit in promiscuous mode; test camera streaming in STA mode; log per-MAC bytes per 250 ms bin; scripted stimulus slots of 2 / 5 / 10 s; distractors (phone video playback, laptop download, idle phone). |
| **E3** | **[H1]** Do lenses respond much more strongly to on-axis than to off-axis illumination, while screws, glass and metal respond similarly to both? | Measure the on-axis/off-axis response ratio for lenses versus the distractor set at several angles. |
| **E4** | Can one ESP32-S3 run camera acquisition and the Wi-Fi monitor at the same time without unacceptable frame or packet loss? | Load test: frame rate, dropped packets, CPU load, heap. Decides D2. |

### 6.3 Acceptance tests

Standard test room: an ordinary room of about 10–20 m² with indoor lighting ≤ 500 lx. The *reference target* is the ESP32-CAM or mini Wi-Fi camera mounted behind a ~2–3 mm hole in a cardboard or plastic object.

| ID | Test | Procedure | Pass criterion |
|---|---|---|---|
| AT-1 | Optical detection vs. distance | Target on-axis at 0.5 / 1.0 / 1.5 / 2.0 m; 20 trials per distance | Detected in ≥ 90 % of trials at 1.0 m (NR-P1); the curve is reported for all distances |
| AT-2 | Angular tolerance | 1.0 m; target axis at 0 / 15 / 30°; 20 trials each | ≥ 80 % detection at 30° |
| AT-3 | Self-luminous rejection | Lamp, indicator LEDs, phone screen in the field of view; 20 trials | Reported as a candidate in ≤ 5 % of trials (FR-O5) |
| AT-4 | Reflective distractors | Distractor set + target in one scene; 20 scans | The target is among the candidates in ≥ 90 % of scans. The mean number of false candidates per scan is reported, with and without the off-axis test (FR-O6). Target ≤ 3 per scan [T] |
| AT-5 | Wi-Fi survey | Test network with ≥ 5 active 2.4 GHz devices including the camera in client mode | All active devices listed with MAC, role, channel and RSSI within one sweep ≤ 15 s |
| AT-6 | Stimulus–traffic identification | Camera streaming + distractors (phone playing video, laptop downloading, idle phone, router); 10 challenge runs | Camera flagged HIGH in ≥ 8/10 runs. Total false HIGH flags across all distractors ≤ 1 in 10 runs |
| AT-7 | RSSI approach guidance | The user, starting at the door and knowing only the flagged MAC, follows the locate screen | Camera position found within ≈ 1 m in ≤ 3 min in ≥ 4/5 attempts [T] |
| AT-8 | Fault tolerance | Disconnect the camera; corrupt the display init (simulated) | The other mode keeps working; the error is shown or logged; no reboot loop |
| AT-9 | Timing | Measure boot time, overlay update rate, button latency | NR-P7, NR-P3, FR-U4 met |
| AT-10 | Power and battery life | USB power meter per mode; full-battery run-down in mixed mode | Consumption per mode reported; battery life ≥ 1.5 h |
| AT-11 | Stability | 30 min demonstration loop | No critical failure or manual reboot (NR-R1) |

### 6.4 Final demonstration (defense / PIKT-Fest)

1. The test room contains a hidden streaming Wi-Fi camera, a hidden non-transmitting camera (lens visible through a small hole), a router, phones, a charger with an LED, and several shiny objects.
2. **Wireless scan:** the survey shows all devices; the challenge flags the camera's MAC; the locate screen leads to it.
3. **Optical scan:** the sweep marks the lenses of both cameras. The lamp and LEDs are suppressed; shiny objects are shown and explained by the off-axis test.
4. **Developer view:** live difference images and traffic-vs-stimulus plots on a laptop.
5. Statement of the limitations (Section 7).

---

## 7. Known limitations (to be stated in the defense)

1. Optical: no line of sight to the lens means no detection. The range decreases with smaller apertures and larger angles. Some objects (retroreflective tape, certain gems, other optics such as binoculars and phone cameras) are true retroreflectors and will legitimately appear as candidates.
2. Wireless: 2.4 GHz only. Cameras on 5 GHz, Ethernet/PoE or SD-card-only, and cameras that do not upload during the test are invisible to Method 2. Encrypted payloads cannot be inspected; the decision relies on traffic statistics only.
3. Stimulus correlation needs the stimulus to be within the camera's field of view (motion) or to change the room illumination (lights).
4. Neither method proves that a device is recording. The output is evidence for the user to inspect.

## 8. Future development (outside V1)

- NIR illumination (850/940 nm) with a camera without an IR-cut filter; multi-wavelength response. [H: hidden cameras' IR-cut filters reflect NIR strongly]
- Dual-band Wi-Fi monitoring (for example ESP32-C5-class or other dual-band chips) for 5 GHz cameras.
- An active LED stimulus aimed at the suspect area; automatic stimulus without user participation.
- SDR-based spectrum survey and analogue video transmitter detection; directional antenna for direction finding.
- EM side-channel detection of offline (SD-card) cameras correlated with visual stimulation.
- Thermal sensing (MLX90640-class); multi-sensor fusion and confidence scoring; TinyML classification once a labelled dataset exists.
- NLJD-type detection of powered-off electronics (a separate research project).

---

## 9. References

1. ГОСТ 19.201-78. Техническое задание. Требования к содержанию и оформлению.
2. Espressif Systems. *ESP32-S3 Series Datasheet.*
3. Espressif Systems. *ESP-IDF Programming Guide: Wi-Fi Driver* (scan, promiscuous mode, `wifi_promiscuous_pkt_t`, `wifi_pkt_rx_ctrl_t`).
4. Espressif Systems. *esp32-camera* driver (GitHub: espressif/esp32-camera).
5. Espressif Systems. *ESP-IDF Programming Guide: Bluetooth LE (NimBLE).*
6. Sami S., Tan S. R. X., Sun B., Han J. *LAPD: Hidden Spy Camera Detection using Smartphone Time-of-Flight Sensors.* ACM SenSys 2021. https://www.cyphy.kaist.ac.kr/research/lapd
7. Optical retroreflection ("cat's-eye effect") of focused optical systems: a standard optics textbook treatment (to be specified in the analysis report).
8. Cheng Y., Ji X., Lu T., Xu W. *DeWiCam: Detecting Hidden Wireless Cameras via Smartphones.* ACM AsiaCCS 2018; extended as *On Detecting Hidden Wireless Cameras: A Traffic Pattern-based Approach*, IEEE TMC, 2019. https://usslab.org/projects/DeWiCam/dewicam.html
9. Singh A. D., Garcia L., Noor J., Srivastava M. *I Always Feel Like Somebody's Sensing Me! A Framework to Detect, Identify, and Localize Clandestine Wireless Sensors.* USENIX Security 2021. arXiv:2005.03068. https://arxiv.org/abs/2005.03068
10. Sharma R. A., Soltanaghaei E., Rowe A., Sekar V. *Lumos: Identifying and Localizing Diverse Hidden IoT Devices in an Unfamiliar Environment.* USENIX Security 2022. https://www.usenix.org/conference/usenixsecurity22/presentation/sharma-rahul
11. IEEE Std 802.11-2020. *Wireless LAN MAC and PHY Specifications* (frame format, DS bits).
12. Course materials: "Проектный трек — Встроенные системы (ИТМО)", ТЗ template.

*Bibliographic details in refs. 6, 8–10 (author lists, venues) shall be checked against the original papers before the final submission.*

---

## Appendix A. Register of assumptions and hypotheses

| ID | Statement | Type | How it is resolved |
|---|---|---|---|
| A1 | The OV2640 module supplied with the board has an IR-cut filter. | A | Inspect the module; image a 940 nm remote control |
| A2 | The OV2640 allows AEC/AGC to be locked and frames to be synchronised to LED switching at ≥ 10 ON/OFF pairs/s at QVGA. | A | E1 / bring-up |
| A3 | A pinhole camera at 1 m gives a difference-image contrast-to-noise ratio well above the frame noise with ≤ 6 small LEDs. | A | E1 |
| H1 | The on-axis/off-axis response ratio separates lenses from common specular distractors. | H | E3 |
| H2 | The uplink bit rate of the target cameras changes measurably (for example by a factor of ≥ 1.5) between "motion" and "still" slots. | H | E2 |
| H3 | Ordinary devices (phones, laptops, routers) show no correlation with the stimulus beyond the chosen false-alarm rate. | H | E2, AT-6 |
| A4 | One ESP32-S3 can run both modes (not simultaneously) within its RAM and CPU budget. | A | E4 |
| A5 | The target mini Wi-Fi cameras operate on 2.4 GHz and support client (STA) mode. | A | Check on purchase |

## Appendix B. Changes relative to the defended draft and their reasons

| Draft v1 | v2 | Reason |
|---|---|---|
| The goal describes the device | The goal describes the problem being solved and how success is measured | Defense feedback (as recalled by the developer); ГОСТ goal–task logic |
| Optical: brightness threshold on red-lit frames | Differential ON/OFF imaging, an adaptive threshold, an off-axis test | Brightness alone cannot reject lamps, LEDs and glints. Retroreflection physics allows a specific test. |
| Wireless: AP list + BLE list | Passive traffic monitor (APs and clients) + stimulus–traffic correlation; AP/BLE lists kept as sub-views | Most hidden Wi-Fi cameras connect as clients and are invisible to an AP scan, and BLE is not their video channel. Correlation produces camera-specific evidence. |
| Requirements without numbers | Requirements with IDs, priorities, [T] targets and verification | Acceptance must be testable. |
| Schedule without dates | Dated plan aligned with the course stages and PIKT-Fest | |
| Acceptance: qualitative | AT-1 … AT-11 with trial counts and pass criteria | |
| Name CamGuard 360 | SpyScan | Matches the registered project name. |
