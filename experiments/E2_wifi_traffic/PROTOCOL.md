# E2: Wi-Fi traffic response to a stimulus

**Status:** protocol v0.1 (2026-10-07). Firmware and PC tools are ready. No measurements yet.

## 1. Questions

| ID | Question | Feeds |
|---|---|---|
| Q1 | Can an ESP32-S3 in promiscuous mode see a camera's uplink, and which frames does it miss (drops, MIMO / HE frames)? | FR-W1/W2, R5 |
| Q2 | **[H2]** Does the camera's uplink rate change measurably between stimulus ON and OFF slots? Target: ON/OFF ratio ≥ 1.5. | FR-W5 |
| Q3 | **[H3]** Do non-camera devices stay below the chosen false-alarm rate (HIGH in ≤ 1 % of runs)? | FR-W6, AT-6 |
| Q4 | **[H4]** Does the traffic direction (tx vs rx) separate the camera (SOURCE) from a phone viewing its stream (viewer)? | result screen |
| Q5 | What slot length T and lag give reliable detection? This sets K and T in the ТЗ (NR-P5 ≤ 60 s). | FR-W5 |

### A hypothesis split to test (new)

The ESP32-CAM's CameraWebServer streams **MJPEG**: every frame is compressed on its own. Its size depends on the *content* of the scene (texture, brightness, noise), **not on motion**.

Most commercial mini cameras are expected to use **H.264/H.265**, where inter-frame compression makes the bit rate jump with *motion* [A: to be confirmed for our A9-type camera].

- **H2a:** H.264 cameras respond to the **motion** stimulus.
- **H2b:** MJPEG cameras respond weakly to motion but strongly to the **light** stimulus (a dark scene gives small, low-detail JPEGs).

So both stimuli are tested on both cameras.

## 2. Setup

- **Sniffer:** the owned ESP32-S3 DevKit running `firmware/` (passive, native USB Serial/JTAG port, `/dev/cu.usbmodem*`), connected by USB to the laptop, placed 1–3 m from the camera.
- **Network:** your own home router or phone hotspot, 2.4 GHz. Write down the channel; `survey` mode finds it.
- **Cameras** (ground truth: write down each camera's MAC from the router's client list or the app):
  - **C0 (pilot, available now):** laptop webcam in a video call to your phone (Telegram/Zoom/Meet) on the 2.4 GHz network. *Stand-in only: video-call codecs use rate control, so the response may be weaker than a real IP camera.*
  - **C1:** ESP32-CAM, CameraWebServer example, stream opened in the phone browser (MJPEG).
  - **C2:** A9-type mini Wi-Fi camera in router mode, live view open in its phone app.
- **Distractors on the same network:** a phone playing YouTube, a laptop downloading a large file, an idle phone, the router itself.
- Record in `--note`: distances, room size, lighting (lux, from a phone app), router model and channel, and which devices are active.

**Ethics / privacy:** use only your own network and devices. A survey also sees neighbours' MAC addresses, so the `data/` folder is git-ignored. Commit only the `RESULTS.md` files you choose, and remove foreign MACs first.

## 3. Procedure

1. **Flash** `firmware/` (see `firmware/README.md`), then check that `I,…,boot,spyscan-e2-sniffer` appears.
2. **Survey** for 40 s:
   ```
   python tools/e2/e2_capture.py --port <PORT> --note "home survey" survey --seconds 40
   ```
   This tells you the router channel and the camera's MAC (role S, high tx kB/s while streaming).
3. **Null runs** (control for Q3): run `challenge` with the camera streaming, but **do NOT follow the prompts**. Keep the scene static the whole time. Do 10 runs. The camera should come out NONE/LOW, and HIGH should appear in about 1 % of device-runs.
4. **Challenge runs:** follow the prompts.
   ```
   python tools/e2/e2_capture.py --port <PORT> --note "<setup>" challenge --channel <CH> \
          --k 12 --t 5 --stimulus motion --target <CAMERA_MAC>
   ```
   - **motion**: during MOVE, walk or wave in front of the camera (within about 1–2 m of its field of view). During STILL, stand still out of view.
   - **light**: during LIGHTS ON / OFF, switch the room light.
5. **Design** (about 1 h per camera):

   | Factor | Levels |
   |---|---|
   | camera | C0, C1, C2 |
   | stimulus | motion, light |
   | T (slot) | 2 s, 5 s |
   | repetitions | 5 per cell |

   K = 12 is fixed (minimum p ≈ 1/924, see `sim/README.md`). Pre-registered lag: 0 s.

## 4. Analysis (automatic: `tools/e2/e2_analyze.py`)

- Per device, the tx (uplink) bytes per slot are tested against the stimulus with a Pearson correlation and a 10 000-permutation p-value. Levels: HIGH p < 0.01, LOW p < 0.05.
- Reported per run: S, p, level, ON/OFF ratio and direction (SOURCE / viewer).
- `traffic.png` plots the time series with the stimulus slots shaded. `lag.png` is an exploratory lag sweep.
- **Aggregate across runs (in `RESULTS.md`):**
  - detection rate (camera HIGH) per cell;
  - false-HIGH rate for the distractors and in the null runs;
  - median ON/OFF ratio;
  - frame-drop counter (`I,stat,drops=`).

## 5. Expected / pass criteria (from the ТЗ; to be confirmed or revised by the data)

| Item | Target [T] |
|---|---|
| Camera HIGH, best stimulus | ≥ 8/10 runs (AT-6) |
| Distractor false HIGH | ≤ 1 per 10 runs, all distractors together |
| ON/OFF ratio of the camera | ≥ 1.5 (H2) |
| Drops | ≈ 0 at the camera's traffic level |

## 6. Known limitations of the sniffer (to state in the report)

- The ESP32-S3 has one antenna and is 802.11b/g/n only. It cannot decode 2-stream MIMO (MCS 8–15) or 802.11ax (HE) frames, so phones and laptops can be **partly invisible**, while 1×1 802.11n cameras should be fully visible [A, checked in Q1].
- Only one channel is monitored at a time.
- The length reported for aggregated (A-MPDU) frames is to be verified against a known traffic source.
- Timing: markers are timestamped by the device clock, so stimulus and traffic share one time base. The person following the prompts adds a reaction delay of about 0.3–0.5 s, which is why T ≥ 2 s.
