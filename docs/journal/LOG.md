# Work log

Dated entries, newest first. Each entry: what was done · decisions · open questions · next step.

## 2026-10-09 (03:46–03:49) — Frame-clock fingerprint CONFIRMED (25 fps); focus capture fixed
Two 60 s focus-records on camera 2 (`cc:b8:…`), P-line capture now working (fix verified: 12 952 P lines).
- **Method 1.2 works, with a correction to my earlier skepticism.** Raw per-packet inter-arrival is dominated by <2 ms within-burst gaps (one encoded frame = a burst of fragments). After collapsing packets into bursts (gap > 8 ms), the **inter-burst interval peaks sharply at 40 ms and 80 ms → 25 fps**; 53 % of intervals fall within ±4 ms of 1× or 2× the 40 ms period. Verified independently on the IAT histogram. See `…/20261009_034658_record/periodicity.png`.
  - So the "guaranteed 25/30 Hz spike" is real **for this camera**, but only after burst-collapse, and it is **25 fps not 30**. The naive per-packet FFT is dominated by burst-internal structure. Analyzer updated to the burst method + standard-fps matcher (15/20/24/25/30).
  - Value: a human, a download or browsing has no tight 25/30 fps burst cadence. This is a camera-specific feature that corroborates the uplink + large-packet + steadiness signature. Novel DSP content for the defense.
- **Closed-app run (03:49):** camera dropped to 0.5 kB/s / 80 frames. It did **not** stand out in the keep-alive table this time (other APs/devices were more regular over 60 s); camera 2's keep-alive is less periodic than camera 1's 15 s beat. **Keep-alive detection is camera-dependent and needs a longer window; it is supporting evidence, not a reliable idle detector.**
- Measured Wi-Fi feature set now: uplink ratio, duty, steadiness (CV 5 s), large-packet fraction, frame-clock fps. All on hardware, two cameras.
- **Next:** optical bring-up test (the primary detector).

## 2026-10-09 (03:38) — New firmware verified: packet-size feature works; focus-capture bug found & fixed
- Reflashed sniffer. 60 s `record` with camera 2 (`cc:b8:…`) streaming (camera 1 idle this session).
- **Packet-size shape (1.3) — strong measured result:** the camera's uplink is **83 % max-size (≥1200 B) packets**; every non-camera client was **100 % tiny** (phone `72:3b`, idle `b2:db`), and the AP relay was mostly mid-size. Clean separation. Refined the rule to **large-packet fraction ≥ 0.5** (the "tiny ACK" half of the textbook bimodal pattern comes from the AP, not the camera's own tx, so pure bimodality does not apply to a one-sided uplink capture).
- **Streaming signature** confirmed on the new data: camera ratio 0.96, duty 1.00, CV 0.08 → STREAMER; no false flags.
- **Bug: 0 `P` (per-packet) lines captured.** The firmware emitted them (focus command acked), but `e2_capture.py` dropped them — its line filter allowed only `I/A/B/M` prefixes, not `P`. **Fixed** (`IABM`→`IABMP`). This run's periodicity data is lost; the fix makes the next focus run work. Firmware unchanged.
- **Next:** re-run `record --focus cc:b8:5e:ad:5c:4a` (camera streaming) with the fixed tool to capture the per-packet data for the 1.2 periodicity test; then optical bring-up.

## 2026-10-09 (methods decision) — Wi-Fi method expanded to a feature set; taxonomy recorded
- Reviewed a full taxonomy of Wi-Fi/RF detection methods (see `docs/analysis/wifi_methods_taxonomy.md`). Decided what to implement vs defer vs exclude, grounded in our E2 measurements. No new hardware needed for any of it.
- **Implemented now (all passive, ESP32-S3):**
  - 1.1 uplink ratio + duty + steadiness (already validated);
  - 1.3 **packet-size shape**: sniffer now emits a per-device tx size histogram (4 buckets, appended to the B line, back-compatible); analyzer flags the bimodal video pattern;
  - 3.2 **keep-alive periodicity**: analyzer coalesces tx frames into events and scores regularity. On the idle-camera run it ranks camera 1 first (gap 15.5 s, regularity 0.95). Finds presence, not identity.
- **One-shot experiments added:** 1.2 frame-clock periodicity — new `focus <mac>` command streams per-packet (t_ms,len) for one device; analyzer does ACF+FFT to 50 Hz and plots it with 25/30 Hz marked. (Note: our cameras show only the keyframe interval, 0.6 s / 3 s, not 30 Hz; the 25/30 Hz claim is tested honestly, not assumed.) 4.1 fast optical strobe deferred to when the optical head is built.
- **Excluded on principle:** 3.3 deauth provocation — it is a denial-of-service attack, illegal to transmit, and not a passive detector. Not in the ТЗ, not in future work.
- Sniffer rebuilds clean on ESP-IDF v5.3.1 (app 0xA4A80). Analyzer + capture self-tested on synthetic and existing data. Not yet run on hardware with the new build.
- **Next:** reflash the sniffer; `record --focus <cam MAC>` on both cameras (streaming) for the size-shape + periodicity data; one idle `record` for keep-alives; then the optical bring-up test.

## 2026-10-09 (01:13–01:16) — Second spy camera (camera 2) on the same network
Channel 10. Camera 1 (`46:88:44:6e:4f:e4`) stayed powered on but unwatched in both runs: 12–13 keep-alive frames, ≈ 0 kB/s.
- **Run 01:13, camera 2 app open:** camera 2 = `cc:b8:5e:ad:5c:4a`. It has a **globally unique (non-random) MAC**, unlike camera 1. Uplink **196 kB/s (≈ 1.6 Mbit/s)**, downlink 7.7, RSSI −46.
  - The viewing phone (`72:3b…`) was on the same Wi-Fi, so the AP relayed the stream locally: AP rx 210 kB/s, AP tx 165, phone downlink 157.
  - Motion challenge: **NONE** (ON/OFF = 1.08, p = 0.35). Camera 2 does not follow motion either.
  - **New observation:** its 1 s uplink has a clear **3 s periodic burst** (≈ 300–480 kB in every third second), probably the encoder's keyframe (GOP) interval. Camera 1 shows a 0.59 s periodicity in all three runs checked. **Hypothesis H6:** keyframe periodicity in the uplink is a camera-specific feature. To be tested; the phone and the AP relaying the stream inherit the same periodicity.
- **Run 01:16, camera 2 app closed:** camera 2 at 0.38 kB/s (76 frames). It **also streams only when watched**, the same as camera 1.
- **Signature rule:** v1 (CV over 1 s windows) **missed camera 2** (CV 0.51 > 0.50) because of the keyframe bursts. **v2 uses CV over 5 s windows.**
  Re-run over all runs ≥ 30 s:
  - **camera flagged in 9/9 watched runs** (camera 1 ×8, camera 2 ×1);
  - **0 false flags:** the AP relay is rejected by the up ratio 0.44, the phone viewer by 0.08, the Meet laptop by duty 0.71 and CV 0.64;
  - 0 flags in 3 unwatched runs. This is a **miss by design**: nothing is streaming.
  - Caveat: v2 was tuned on the same data → **independent validation is still required** (new runs plus distractors: cloud upload, YouTube, video call).

## 2026-10-09 — Wi-Fi phase paused; optical bring-up firmware
- The A9 camera has no motion detection, so the "event upload" path does not exist for it. **E2 conclusion for this camera:**
  - visible on Wi-Fi only while someone watches live;
  - the stimulus challenge fails (constant bit rate);
  - the passive streaming signature identifies it while it is being watched.
- **Decision (developer + assistant):** the optical method becomes the primary detector; Wi-Fi is secondary/confirming. The ТЗ must be updated (FR-W*, limitations).
- `firmware/bringup/`:
  - board self-test: flash, PSRAM, camera sensor ID and fps, LCD colour bars, LEDs, buttons;
  - LIVE camera preview on the ILI9341;
  - **DIFF mode**: the first live lens-retroreflection test (exposure locked, LED ON/OFF difference, red = above mean + 6σ, provisional), on-axis vs off-axis LED, and serial `D,` lines for E1.
  - Builds without warnings on ESP-IDF v5.3.1 (vendored components, local check). Not yet run on hardware.

## 2026-10-08 (23:37) — A9 camera with the app closed: it does not stream (important limitation)
- Run 23:37, channel 10, note "app off": the camera app and the YouTube video were closed.
- **Camera `46:88:44:6e:4f:e4`: only 12 tiny frames (≈ 1 KB in total) in 82 s**, about one every 5–6 s. These look like keep-alive / heartbeat traffic. It was still connected to the router (RSSI −29 dBm), but **no video was uploaded**.
  No other device uploaded ≥ 1 kB/s either, which crashed `e2_analyze.py` (empty signature table). Fixed: the analyser now handles "nobody streaming".
- **Meaning:** this camera streams **only while someone watches live** (or, possibly, when it uploads a motion event — not observed here). While nobody watches, both Wi-Fi traffic methods (challenge and passive signature) are blind to it, and only its presence (keep-alives, RSSI) is visible.
- **Consequence for the design:**
  - the **optical method is the primary detector**, because it does not depend on the camera transmitting;
  - the Wi-Fi method detects a camera that is **being watched live** (streaming signature), and otherwise can only list it as a quiet device near the user (keep-alives plus RSSI);
  - this must be stated as a limitation in the ТЗ and the report.
- **Open:** does the camera upload when its motion detection triggers (alarm clips or snapshots)? Test with the app closed but motion alerts enabled, and walk in front of it.

## 2026-10-08 (late night) — E2 runs v0.2 on the A9 camera: stimulus method fails, passive signature works
Runs on channel 10, K = 12, T = 5 s, warm-up 20 s, target `46:88:44:6e:4f:e4`: cover #1 (23:11), cover #2 (23:14), light #1 (23:17), NULL (23:19).
- **cover → NONE in both runs** (two-sided p = 0.71, 0.40). Covered slots 29–32 kB/s, uncovered 29–44 kB/s. **H5 rejected for this camera.**
- **light → NONE** (p = 0.32). The 5–7 s bursts (+15–20 kB/s) appear at times that do not line up with the stimulus.
- **NULL → NONE** (p = 0.37), as expected.
- **Rate level varies between sessions:** ≈ 30 kB/s in the cover/light runs vs ≈ 55 kB/s earlier and in the NULL run. Most likely an app quality setting (SD/HD) or network adaptation.
- **Conclusion (8 runs, 4 stimuli: motion, light, cover, null):** this A9-type camera streams at a rate that does **not** follow the scene on a 5 s time scale. The stimulus–response challenge cannot identify CBR-like cameras of this type. It remains to be tested on the ESP32-CAM (MJPEG, expected to be content-dependent).
- **Passive signature (exploratory, computed on all A9 runs plus 2 non-camera runs):** the camera had uplink ratio 1.00, duty (> 10 kB/s) 1.00 and CV 0.09–0.30 in all 8 runs. No other device met all of these; the closest, a Google Meet laptop, had ratio 0.89, duty 0.71, CV 0.74.
  Provisional rule: up ≥ 10 kB/s, duty ≥ 0.9, CV ≤ 0.5, ratio ≥ 0.9, ≥ 30 s.
  **The thresholds were chosen after seeing the data, so they must be validated on new runs with distractors** (YouTube upload/streaming, video call, cloud backup, smart-home devices). Added to `e2_analyze.py` (`signature.csv` and a section in RESULTS.md).
- **Proposed change to method 2 (decision pending with the developer):**
  - primary: passive streaming-signature screening, no user action needed;
  - secondary: the stimulus challenge as confirmation (works only on content-dependent encoders);
  - then RSSI locate.

## 2026-10-08 (night) — E2 with the real mini Wi-Fi camera (A9 type): first real negative result
Runs on channel 10, K = 12, T = 5 s: 22:52 motion, 22:56 record, 22:57 light, 22:59 motion. No `--target`/`--note` given; the camera was identified from the data.
- **Camera identified passively:** `46:88:44:6e:4f:e4`, a **locally administered (random) MAC**, so OUI vendor lookup is useless for this camera. It sits at −28 dBm (near the sniffer). Uplink 50–63 kB/s (≈ 0.45 Mbit/s) and downlink ≈ 0. It was the **only** device with sustained, uplink-dominant traffic in every run. 0 dropped frames.
- **Motion → NONE in both runs.** Per-slot uplink was flat: ON/OFF = 1.00 and 1.03. Two-sided p = 0.86 and 0.48. Exploratory features (frames/s, burstiness, peak 100 ms load, bytes/frame) were also flat.
  → **This camera encodes at a constant bit rate (CBR).** H2a ("H.264 cameras respond to motion") is **rejected for this camera**.
- **Light → p = 0.04 two-sided (rate higher in the dark), but INVALID:** uplink climbed steadily 31 → 68 kB/s (trend ρ = +0.92). Most likely a one-time switch to night mode at the first OFF slot, without switching back. This is not a repeated response.
- **What still works:** the passive traffic signature (sustained, near-constant, uplink-only stream) singled out the camera among about 6 active devices in all 4 runs. This is DeWiCam-style traffic-pattern evidence (FR-W4).
- **Protocol amendment v0.2 (made after these runs, applies to all future runs):**
  - the permutation test is **two-sided**, because the direction of the response depends on the camera and the stimulus;
  - new stimuli **`cover`** (cover / uncover the lens) and **`flash`** (phone flashlight into the lens).
- **New hypothesis H5:** even a CBR encoder cannot keep its bit rate up on a black (covered) frame, so covering the lens lowers the uplink. This enables a fused workflow: the optical scan finds a lens candidate → the user covers it → the Wi-Fi challenge confirms that it is a live camera and gives its MAC.
- **Next:** 3× `cover`, 3× `flash` and 3× null runs on the mini camera; a 60 s `record` with the scene static vs. with motion, to confirm CBR.

## 2026-10-08 — Parts received (first batch)
- **ESP32-S3 camera board: module ESP32-S3-N16R8** (16 MB flash, 8 MB octal PSRAM), as required. It has a camera FPC connector, two USB-C ports and BOOT/RST buttons. The camera module came with it (sensor model to be read out at bring-up).
- ILI9341-type 2.4" SPI TFT (red PCB, 14-pin header; the touch pins are unused).
- Schottky diodes (1N5819 type), SOT-23→DIP adapters, red LEDs (3 mm and 5 mm, diffused).
- Mini Wi-Fi camera (A9 type): connects to the home Wi-Fi via its app (tested by the developer).
- Not in the photo yet: AO3400, Li-Po, IP5306 module, ESP32-CAM, resistors.
- **Next:** E2 runs with the mini camera (motion, light, null); then board bring-up firmware (PSRAM, camera sensor ID, display, LEDs).

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
