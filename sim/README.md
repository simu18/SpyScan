# Simulation

## 1. Wokwi: HMI simulation (`wokwi/`)

ESP32-S3 DevKitC-1 + ILI9341 + 3 buttons + LED channels + buzzer. It uses **the same GPIO numbers as the real device** (`board_pins.h`).

| Real code (reused in the firmware) | Simulated (replaced by drivers later) |
|---|---|
| `ui_fsm.c`: UI state machine | camera frames → fake scene with a lens, a glint and a lamp |
| `challenge.c`: PRBS stimulus + correlation + permutation test | Wi-Fi monitor → `sim_sources.c` synthetic devices |
| `board_pins.h`: pin map | battery → potentiometer; distance → potentiometer |

**Run it in the browser:**
1. Go to wokwi.com → *New project* → **ESP32-S3** (Arduino).
2. Replace `sketch.ino` and `diagram.json` with the files from this folder.
3. Use the **"+" → Upload file(s)** button for `board_pins.h`, `ui_fsm.h/.c`, `challenge.h/.c`, `sim_sources.h/.c`, `libraries.txt`.
4. Press ▶. The Serial Monitor shows every state transition and the challenge statistics.

**Run it from VS Code with PlatformIO:** Open `sim/wokwi/` as the workspace, run `pio run` once to build the firmware, then start the Wokwi extension. The `platformio.ini` and `wokwi.toml` in that folder point the extension at the generated ESP32-S3 firmware.

**Start values:** the *Battery* pot starts at full (4.2 V) and the *Distance* pot at about 3 m. Turning *Battery* below 3.4 V for 2 s triggers LOW BATTERY; turning it back above 3.5 V returns to the menu.

**Demo script (≈ 2 min):**
- Menu → *Optical scan*. The AXIS LED blinks in the 4-frame ON,ON,OFF,OFF cycle, slowed to 4 fps.
- UP switches RAW / ON−OFF views. In the diff view the lamp disappears; this is the principle of FR-O5.
- SEL starts the off-axis test (the OFF LED blinks) and shows the lens versus glint classification.
- BACK → *Wireless scan*. The survey table shows six devices; streaming devices are orange.
- SEL starts the challenge: follow the MOVE / STAY STILL prompts (in the sim the camera "reacts" automatically) for 12 slots × 2 s.
- The result shows the hidden camera as **HIGH / SOURCE** and the viewing phone as **HIGH / viewer**. The others show NONE in most runs.
- SEL opens the locate screen: turn the *Distance* pot and watch the RSSI and WARMER/COLDER indicator.
- Turn the *Battery* pot down below 3.4 V to trigger the LOW BATTERY screen.

**Limits:** Wokwi cannot simulate the OV2640 or Wi-Fi monitor mode. Those are covered by `test/` (algorithm level) and by the real experiments E1–E4.

## 2. PC tests (`test/`)

```
sh sim/test/run_tests.sh
```

- `test_ui_fsm.c`: 22 checks covering every transition of `docs/architecture/diagrams/06_ui_fsm.puml`, including the case "camera failed → wireless still works" (NR-R2).
- `test_challenge.c`: 200 simulated challenges per setting, giving the HIGH/LOW rate per device.

### First synthetic results (2026-10-02, **simulated data, not measurements**)

| K slots × T | Hidden cam HIGH | Viewer phone HIGH | Non-reacting devices HIGH (false alarm) |
|---|---|---|---|
| 12 × 5 s | 100 % | 100 % | 0–2 % (expected ≈ 1 % by construction) |
| 8 × 5 s | 4.5 % | 11 % | 0–0.5 % |
| 12 × 2 s | 100 % | 100 % | 0–1 % |

What this shows (design consequences, independent of the synthetic numbers):
1. **K ≥ 10 is mandatory.** With K balanced slots there are only C(K, K/2) distinct stimulus patterns, so the smallest possible p is ≈ 1/C(K, K/2). For K = 8 that is 1/70 ≈ 0.014, which is above the 0.01 HIGH threshold. K = 10 gives 1/252 and K = 12 gives 1/924.
2. **The non-camera false-alarm rate matches the chosen p threshold** (≈ 1 % HIGH, ≈ 5 % LOW). The permutation test is calibrated, as designed.
3. **A device watching the stream also correlates** (its TCP ACK uplink follows the stream). Correlation alone therefore cannot separate the *source* from a *viewer*. Added feature: **traffic direction** (uplink ≫ downlink means source). New hypothesis **H4**, to be tested in E2.
