# Work log

Dated entries, newest first. Each entry: what was done · decisions · open questions · next step.

## 2026-10-07 (late) — E2 pilot with Google Meet as stand-in camera (run 20:20)
- **Stream visible on 2.4 GHz:** the laptop (`b2:db:…`, −30 dBm) uploaded 48.9 kB/s on average and ≈ 70–90 kB/s (≈ 0.6 Mbit/s) once stable. 11 667 frames, 0 drops.
- **Result: NONE (S = −0.44, p = 0.94).** No positive response to motion. Per-slot uplink, `*` = MOVE: `18, 0*, 29*, 59*, 74, 79*, 91, 14*, 71*, 77, 76, 74`.
- **Why (from the data):**
  1. **Ramp-up.** Uplink rose from ≈ 0 to ≈ 75 kB/s over the first ~25 s (WebRTC bandwidth estimation). The first MOVE slots fell into this ramp, which pushed the correlation negative.
  2. **Two dropouts** (≈ 12–20 s and ≈ 47–50 s, uplink ≈ 0–3 kB/s).
  3. **In steady state the bit rate is flat whatever the motion.** Meet's rate control holds the bit rate and spends the bits on quality instead. This matches the warning in PROTOCOL §2: a video call is a poor stand-in for a camera that encodes at a constant quality level.
  4. The laptop's downlink (receiving the phone's video) disappeared after slot 1. Possible cause: AP→laptop frames sent with 2-stream MIMO, which the single-antenna ESP32-S3 cannot decode (limitation Q1, to verify).
- **Method lesson (important for the device):** the permutation test assumes the slots are exchangeable. A stream that is ramping up or dropping out violates that. Added:
  - a default warm-up of 20 s;
  - a live display of the target's uplink during warm-up and slots (`--target`), with a warning if it is below 5 kB/s;
  - a per-slot table in RESULTS.md with trend (Spearman ρ) and dropout warnings.
  The pre-registered statistic is unchanged; the diagnostics only flag runs to repeat.
- **Next:** wait for the real cameras (ESP32-CAM, A9). Optionally try `--stimulus light` with Meet.

## 2026-10-07 (night) — E2 first challenge pilots (ch 10, K=12, T=5 s, motion)
- 4 challenge runs: 19:11, 19:15, 19:19, 19:23. **No continuous video stream was visible in any run.** All devices on channel 10 stayed at ≤ 3 kB/s uplink, except one burst. A real video stream should be ≥ 20 kB/s, sustained.
  - Likely causes: the call ran on 5 GHz (invisible to the ESP32-S3), or the video was not active.
  - Result: all devices NONE. This is the correct output when nothing is streaming.
- Run 19:23:
  - Device `b2:db:…` (−28 dBm) uploaded ~55–60 kB/s only in the last 2 slots (60–71 s). Both were MOVE slots, but the other 4 MOVE slots showed nothing.
  - ON/OFF ratio 13×, yet p = 0.17 → NONE. The permutation test correctly refuses a response that does not repeat; a single burst cannot produce HIGH.
- **Firmware validation (measured):** the device's tx matches the AP's rx slot by slot (60.9/62.1 and 53.1/53.8 kB/s), and the device's rx matches the AP's tx (38.6/38.6 and 20.4/20.4). The ToDS/FromDS direction accounting works.
- Observation: each capture start resets the board (`boot` at t ≈ 80 ms). This is harmless and gives a clean state.
- **Next pilot:**
  - force the stream source onto 2.4 GHz;
  - confirm sustained uplink with `record` before running `challenge`;
  - pass `--target` and `--note`.

## 2026-10-07 (evening) — E2 sniffer first run on hardware
- Firmware flashed on the owned ESP32-S3 DevKit with ESP-IDF 6.1. Output moved to the **native USB Serial/JTAG** port (`/dev/cu.usbmodem*`) instead of UART0.
- Firmware fix: installed the interrupt-driven USB Serial/JTAG driver (buffered TX, blocking RX) so bursts of output do not stall the aggregator. Builds on ESP-IDF v5.3.1; to confirm on 6.1.
- **First measured result (survey, 40 s, hop 500 ms, dorm room):** 28 APs, 18 active devices, **0 dropped frames**, free heap ≈ 190 KB. APs on 10 of the 13 channels (channel 11 busiest: 10 APs), so the RF environment is crowded and realistic for false-alarm tests.
- Tool fixes:
  - survey rates are now normalised by on-channel time (÷13);
  - the capture tool sends `reset` at start, so APs and devices seen before the capture are re-reported in the run;
  - the serial port is opened before the run folder is created, with a clear error if it is busy;
  - DTR/RTS are kept low on open, so the USB-JTAG port does not reset the chip.
- The challenge run of 18:44 left an empty folder: the port could not be opened (most likely another serial monitor was still open).
- **Next:** challenge pilot using a laptop video call as a stand-in camera.

## 2026-10-07 — E2 sniffer firmware and PC tools
- Parts reviewed before ordering. Added: ILI9341 display, 1N5819 Schottky diodes (prevents the USB and battery 5 V supplies from back-feeding each other), SOT-23 adapters. Two seller checks: board PSRAM (N16R8/N8R8) and mini camera 2.4 GHz + router mode. Found at home: an IR module (useful for the IR-cut check, A1) and AMS1117 modules (not needed).
- `experiments/E2_wifi_traffic/firmware`: ESP32-S3 passive sniffer (promiscuous mode, per-MAC tx/rx bytes, frames, RSSI and role in 100 ms bins, markers for stimulus sync, channel lock/hop). **Builds with no warnings on ESP-IDF v5.3.1.** Not yet run on hardware.
- `tools/e2`: `e2_capture.py` (survey / record / challenge with on-screen MOVE/STILL prompts), `e2_analyze.py` (permutation test, direction feature, plots), `fake_device.py` (synthetic device for testing the tools). End-to-end self-test on synthetic data passed; this is not a measurement.
- New hypothesis split: an MJPEG camera (ESP32-CAM) is expected to respond to the light stimulus more than to motion; H.264 cameras are expected to respond to motion. Both stimuli are tested on both cameras (PROTOCOL §1).
- **Next:** flash the DevKit, run a survey, then pilot challenge runs with a laptop video call as a stand-in camera.

## 2026-10-02 — Parts list, architecture draft, simulation
- Parts list finalised for ordering (see the BOM in the ТЗ §4.1). The display changed from ST7789 to **ILI9341 2.4"** because the Wokwi simulator supports it, so the simulated UI is the real UI. Added test targets: an ESP32-CAM and a 2.4 GHz mini Wi-Fi camera.
- Architecture draft 0.1 (`docs/architecture/ARCHITECTURE.md`): HW block diagram, interface table, draft pin map (to verify on the real board), LED driver calculation, power budget (`tools/power_budget.py`: ≈ 5.3 h estimated vs the 1.5 h target), FreeRTOS task design, timing analysis, UI state machine, risks. All diagrams in PlantUML.
- Wokwi HMI simulation (`sim/wokwi/`): ESP32-S3 + ILI9341 + 3 buttons + LEDs + buzzer + battery/distance pots; real `ui_fsm.c` and `challenge.c` running on synthetic data.
- PC tests (`sim/test/`): 22 UI transition checks pass; Monte-Carlo test of the challenge detector.
- **Findings from the simulation (synthetic data, design-level):**
  - K ≥ 10 slots is mandatory, because the minimum achievable p is 1/C(K, K/2).
  - A phone viewing the stream also correlates with the stimulus, so the **direction feature** (uplink vs downlink) is added to separate source from viewer (hypothesis H4).
- Found: the ТЗ edit removed the requirement IDs and the [F]/[A]/[T] legend, but sections 2.4.1, 6.3 and Appendix B still reference them. To fix in ТЗ v2.1.
- **Next:** E2 sniffer firmware for the owned DevKit; Python capture and analysis; ESP-IDF install on the Mac.

## 2026-10-01 — Project restart, ТЗ v2
- Studied the course requirements, the ТЗ template, the original idea (CamGuard 360) and the defended draft ТЗ.
- Defense feedback (as recalled by the developer): the goal must state the **problem**, not describe the device. The goal was rewritten in ТЗ v2 §1.3.
- **Decision D-M1:** method 1 changed from a brightness threshold to *differential ON/OFF retroreflection imaging + off-axis test*. Reason: a brightness threshold cannot reject lamps, indicator LEDs or glints.
- **Decision D-M2:** method 2 changed from an AP/BLE list to *passive Wi-Fi traffic monitoring + stimulus–traffic correlation*. Reason: client-mode cameras are invisible to an AP scan, and published work (DeWiCam, Singh et al., Lumos) shows that traffic responds to the scene.
- Language: English. Repository: GitHub. Funding: self-funded; lab access for equipment and the 3D printer.
- Hardware owned: ESP32-S3 DevKit, Ra-02 LoRa 433, CC1101 433 (the 433 MHz modules are not used in V1).
- **Next:** send ТЗ v2 + repo link to the instructor; start E2 (Wi-Fi traffic) on the owned DevKit; order the camera board, display, LEDs and test cameras.
