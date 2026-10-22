# SpyScan — bring-up & first optical test: full step-by-step

Board: ESP32-S3-N16R8 + OV2640. Powered from USB the whole time. No MOSFET, no battery needed.

## 0. Safety rules (read once)
- **Wire everything with the USB cable unplugged.** Only plug in after a full re-check.
- **Every LED must have its resistor in series.** Never LED straight to a pin.
- 3V3 is the 3.3 V pin. **Never connect 5V to any GPIO or to the display.**
- Double-check GND is common (board GND ↔ breadboard GND rail).

## 1. Parts for this test
- ESP32-S3-N16R8 board with the OV2640 camera on its FPC connector (ribbon contacts facing the board, latch closed)
- ILI9341 2.4" display
- 2 × red LED (3 mm is best; the lens-close one especially)
- 2 × **150 Ω** resistor (from the kit — strip labelled "150", or color bands brown-green-brown)
- 3 × push button (optional but recommended)
- breadboard + jumper wires (M-M for board-to-rail, M-F if the display has male pins)
- USB-C data cable

**LED polarity:** the **long leg = +(anode)**; the short leg / flat side of the rim = **−(cathode)**.

## 2. Wiring map (what connects where)

### Power rails (do first)
| Board pin | Breadboard |
|---|---|
| **3V3** | red (+) rail |
| **GND** | blue (−) rail |

### Display — ILI9341 (SPI)
| Display pin | Connect to |
|---|---|
| VCC | 3V3 rail |
| GND | GND rail |
| CS  | GPIO **41** |
| RESET | 3V3 rail (we use software reset) |
| DC (also marked D/C or RS) | GPIO **42** |
| SDI (MOSI) | GPIO **21** |
| SCK | GPIO **47** |
| LED (backlight) | 3V3 rail |
| SDO (MISO), T_CLK, T_CS, T_DIN, T_DO, T_IRQ | leave **unconnected** (touch unused) |

### LEDs (each through its own 150 Ω)
| LED | Drive pin | Wiring | Placement |
|---|---|---|---|
| **AXIS** (main) | GPIO **2** | GPIO2 → 150 Ω → LED long leg; LED short leg → GND rail | **touching the camera lens rim, ≤ 1 cm** |
| **OFF-AXIS** | GPIO **14** | GPIO14 → 150 Ω → LED long leg; LED short leg → GND rail | **3–5 cm to the side** of the lens |

### Buttons (optional; internal pull-ups, so no resistor)
| Button | One leg → | Other leg → |
|---|---|---|
| UP | GPIO **38** | GND rail |
| SELECT | GPIO **39** | GND rail |
| BACK | GPIO **40** | GND rail |

Without buttons you still get boot checks + LIVE preview; you need SELECT to reach the DIFF test, so fit at least that one.

## 3. Final check before power (30 seconds)
- No 5V anywhere on the breadboard.
- Each LED has a 150 Ω in series, long leg toward the resistor/GPIO.
- Display VCC/LED/RESET on 3V3, GND on GND.
- Camera ribbon latch closed, contacts the right way.
- Then plug in the USB cable.

## 4. Build and flash
```sh
source /Users/af/.espressif/tools/activate_idf_v6.1.sh
cd "/Users/af/ITMO/5th_sem/Embeded_System/Project/SpyScan/firmware/bringup"
idf.py set-target esp32s3      # first time only; downloads camera + LCD drivers
idf.py build
ls /dev/cu.*                   # find the port, e.g. cu.usbmodem1101
idf.py -p /dev/cu.usbmodem1101 flash monitor
```
Stop the monitor with **Ctrl + ]**.

## 5. What the boot should print (copy this for me)
```
I,..,boot,spyscan-bringup,v0.1
I,..,flash,16 MB
I,..,psram,8192 KB          <- MUST be ~8192. If 0/DISABLED, stop and tell me.
I,..,leds,blinked ...        <- both LEDs blink 3×
I,..,lcd,OK
I,..,camera,OV2640,PID=0x..  <- camera model
I,..,camera_init,OK
I,..,camera_fps,..           <- write this number down
```
At the same time the screen shows: **RED → GREEN → BLUE → WHITE** bars, then the **live camera picture**.

## 6. Optical test (the actual experiment)
1. Press **SELECT** → screen title changes to **Optical: ON-OFF (DIFF)**. Exposure is now locked and the AXIS LED pulses.
   - Grey = little change between LED-on and LED-off; **red = a spot that reflects our LED back** (a lens candidate).
2. Point the board at a **spy-camera lens from ~0.5 m**, lens facing you, normal room light.
   - **Expected:** the camera lens shows up as a **red dot**.
3. Put a **lamp or a charger's LED** in view.
   - **Expected:** it stays grey/dark (it glows by itself, so ON−OFF cancels it). This proves the method rejects light sources.
4. Try shiny objects (spoon, screw, glass). Note which turn red — these are the false candidates we'll study.
5. Press **UP** to step the exposure (50 → 150 → 300 → 600 → 1200). Find the value where the lens dot is cleanest.
6. Press **BACK** → switches to the **OFF-AXIS** LED. The lens dot should get **much weaker**; a flat shiny object may not. (This is the lens-vs-glint test, H1.)
7. Repeat the lens at **1.0 m** and **1.5 m**.

Each ON/OFF pair prints one line:
```
D,t_ms,led(A/O),aec,mean,sigma,thr,n_hot,peak,peak_x,peak_y,snr
```
`snr` (last number) at the lens spot is our first E1 data point.

## 7. Record for me
1. All the boot lines from §5 (PSRAM, camera model, fps).
2. Does the lens show as a red dot? At what distances?
3. Does the lamp stay dark (not red)?
4. 5–10 `D,...` lines: lens at 0.5/1.0/1.5 m, and the AXIS-vs-OFF-AXIS comparison.
5. A photo of the screen pointing at a lens, if you can.

## 8. Troubleshooting (all are quick fixes — tell me which)
| Symptom | Likely cause / fix |
|---|---|
| `psram` 0 or DISABLED | sdkconfig PSRAM option; I fix in one line |
| screen white/blank or garbage | lower SPI clock (one line); check CS/DC/SCK/MOSI |
| colors swapped (red↔blue) | RGB/BGR order — one line |
| image mirrored/upside down | mirror flag — one line |
| `camera_init` FAIL | re-seat the FPC ribbon; if still failing the board may use a different camera pin map — send a sharp photo of the board back |
| nothing on serial | wrong port, or close any other monitor; use the native-USB `cu.usbmodem*` |
| LED never lights | polarity (long leg to resistor) or missing GND |

Do the wiring and the boot first; send me §5 before the optical test so we confirm the hardware is healthy.
