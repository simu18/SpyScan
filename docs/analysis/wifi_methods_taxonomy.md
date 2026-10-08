# SpyScan — Wireless / signal detection methods: survey and decision

Status: 2026-10-09. This records which detection methods SpyScan implements, which are deferred, and which are excluded, with the reasons grounded in our own measurements (experiment E2, two real spy cameras). It is defence material: it shows the method space was surveyed and the choice was deliberate.

Labels: **[F]** fact/measured, **[A]** assumption, **[H]** hypothesis, **[D]** decision.

## 0. What our measurements forced us to accept

Two cheap Wi-Fi spy cameras (A9-type `46:88:…`, random MAC; and a second `cc:b8:…`, real OUI) on 2.4 GHz:

1. **[F] Both stream only while someone watches the live view.** With the app closed, each dropped to a keep-alive of a few frames every ~15 s and no video.
2. **[F] Neither camera's bitrate follows the scene** on a 5 s timescale: motion, room-light on/off, and fully covering the lens all gave ON/OFF ≈ 1.0 (p ≥ 0.3). The stimulus–response challenge does **not** work on these constant-bitrate cameras.
3. **[F] While being watched, each camera is a near-constant, upload-only stream** that no normal device matched (uplink ratio ≈ 1.0, duty ≈ 1.0). This is detectable passively.

**[D] Consequence:** the **optical method is the primary detector** (it works with the camera off). The **Wi-Fi method is a secondary detector** that adds strong evidence when a camera is being watched live, and can show the *presence* of an idle camera via keep-alives. This is the opposite of cheap "RF bug detectors", which rely on transmission alone and miss exactly these cases.

## 1. Methods we implement this semester (no new hardware)

All run on the ESP32-S3 in passive promiscuous mode. Together they make **one** method (Wi-Fi), as a small feature set rather than a single number.

| ID | Feature | Mechanism | Status |
|---|---|---|---|
| 1.1 | **Uplink ratio + duty + steadiness** | A live video stream is continuous, steady and upload-only | **[F] implemented, validated** (flags camera in 9/9 watched runs, 0/… non-camera so far; thresholds still need independent validation) |
| 1.3 | **Packet-size shape** | Video fragments into max-size packets + tiny ACKs → bimodal size histogram, empty middle | implemented in sniffer (tx histogram) + analyzer; to measure |
| 3.2 | **Keep-alive periodicity** | An idle camera still sends NAT keep-alives at a fixed interval (~15 s here) | implemented in analyzer; on our idle run it ranked the camera first (gap 15.5 s, regularity 0.95). Finds **presence**, not identity (other IoT devices heartbeat too) |
| 7 | **RSSI locate ("warmer/colder")** | Inverse-square path loss; signal rises near the device | planned (UI exists in the sim); needs the device built |

Decision logic (fusion with the optical method), **[D]**:
1. passive screen → "a continuous video uplink is present, strength N" (1.1 + 1.3);
2. if nothing is streaming, keep-alive scan → "a quiet periodic device is here" (3.2);
3. RSSI locate → narrow to a corner/object (7);
4. optical retroreflection scan there → find the lens (Method 1, primary);
5. final alarm needs **two** independent kinds of evidence (radio + optical), which is what keeps false alarms down.

## 2. Methods we test as single experiments (keep if they pay off)

| ID | Idea | Why uncertain | Test |
|---|---|---|---|
| 1.2 | **Frame-clock periodicity (FFT/ACF)** | The widely-quoted "sharp 25/30 Hz line" is **not guaranteed**: encoders coalesce frames, so the sensor cadence often does not survive to the MAC layer. Our cameras showed only the **keyframe** interval (0.6 s and 3 s), not 30 Hz. Our 100 ms bins also cannot see 30 Hz at all. | New `focus <mac>` firmware captures per-packet (t_ms, len) for one device; analyzer does ACF+FFT up to 50 Hz and plots it with 25/30 Hz marked. Honest outcome either way. |
| 4.1 | **Fast optical strobe (10 Hz PRBS)** to stress the camera's auto-exposure | Our slower stimuli (light switch, cover) already failed; a 10 Hz LED burst is more aggressive and *might* force I-frames, but may still not move a CBR encoder | Optical LED PRBS + sniffer, once the optical head is built |
| 4.2 | **Acoustic stimulus** (camera microphone → audio packets) | Needs the camera to stream audio with silence-suppression; many don't by default; our buzzer is weak | Low priority; only if 4.1 and 1.2 disappoint |

## 3. Deferred to future work (out of semester scope)

| ID | Idea | Why deferred |
|---|---|---|
| 1.4 | 802.11 capability / IE fingerprint (SISO, no 5 GHz, chip vendor) | Needs to catch association/probe frames, which are intermittent; opportunistic only. Good report paragraph, not a reliable detector yet |
| 3.1 | Active SSDP / ONVIF discovery | Requires joining the network (needs the Wi-Fi password) and a station mode; useless on a hidden SSID or a camera on cellular. Strong for the hotel/Airbnb case as a later mode |
| 2.1 | Wi-Fi CSI Doppler / localization | ESP32 CSI is noisy; room localization is research-grade, not a semester result |
| 5.1 | Carrier-frequency-offset thermal drift | CFO is not cleanly exposed on the ESP32; sensitive, uncalibrated |
| 5.2 | Image-sensor pixel-clock RF leakage | This is the EM side-channel already on the roadmap; needs an analog front end and shielding |

## 4. Excluded on principle

| ID | Idea | Why excluded |
|---|---|---|
| 3.3 | **802.11 deauthentication provocation** | Transmitting deauth frames is a denial-of-service attack: it knocks other people's devices off the network, is illegal to transmit in Russia and most jurisdictions regardless of intent, and does not belong in a passive *detector*. **Not implemented, not in the ТЗ, not in future work.** |

## 5. Hardware impact

**None.** Every item in sections 1 and 2 (except the optical strobe, which uses the LEDs we already planned) runs on the ESP32-S3 DevKit we already have, in passive receive. No purchase is required to add the Wi-Fi features.

## References (to verify against originals before the report)
DeWiCam (AsiaCCS 2018 / IEEE TMC 2019); Singh et al., USENIX Security 2021; Lumos, USENIX Security 2022; LAPD, SenSys 2021; IEEE 802.11-2020.
