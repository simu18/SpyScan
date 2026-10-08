# Bring-up firmware: board check + first lens-reflection test

Target: **ESP32-S3-N16R8** camera board. ESP-IDF **5.3+ / 6.x**.

The component manager downloads `espressif/esp32-camera` and `espressif/esp_lcd_ili9341` on the first build.

Verified: builds without warnings on ESP-IDF v5.3.1. App size 0x54FE0 (77 % of the partition free). Not yet run on hardware.

## 1. Wiring (breadboard, powered from USB)

| Module pin | Goes to | Note |
|---|---|---|
| LCD **VCC** | 3V3 | |
| LCD **GND** | GND | |
| LCD **CS** | GPIO **41** | |
| LCD **RESET** | **3V3** | software reset is used |
| LCD **DC** (D/C, RS) | GPIO **42** | |
| LCD **SDI** (MOSI) | GPIO **21** | |
| LCD **SCK** | GPIO **47** | |
| LCD **LED** (backlight) | 3V3 | |
| LCD SDO, T_CLK, T_CS, T_DIN, T_DO, T_IRQ | not connected | touch is not used |
| Button UP | GPIO **38** ↔ GND | internal pull-up |
| Button SELECT | GPIO **39** ↔ GND | |
| Button BACK | GPIO **40** ↔ GND | |
| LED "AXIS" | GPIO **2** → 100–150 Ω → LED anode (long leg); cathode → GND | put it **right next to the camera lens (≤ 1 cm)** |
| LED "OFF-AXIS" | GPIO **14** → 100–150 Ω → LED → GND | **≥ 3 cm** away from the lens |
| Camera | FPC connector, contacts facing the board, latch closed | |

- With 100–150 Ω the LED current is ≈ 9–13 mA, within the GPIO limit. The AO3400 drivers come later.
- **Never connect an LED without a resistor.**

## 2. Build and flash

```sh
source /Users/af/.espressif/tools/activate_idf_v6.1.sh
cd firmware/bringup
idf.py set-target esp32s3        # first time only
idf.py build
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

## 3. What you should see

Serial output:
```
I,…,boot,spyscan-bringup,v0.1
I,…,flash,16 MB
I,…,psram,8192 KB          <- must be 8192; if 0 / DISABLED, the PSRAM config is wrong
I,…,leds,blinked …         <- both LEDs blink 3 times
I,…,lcd,OK                 <- screen shows RED, GREEN, BLUE, WHITE
I,…,camera,OV2640,PID=0x0026   (or another model)
I,…,camera_init,OK
I,…,camera_fps,…           <- record this number (it affects NR-P3)
```

Then the screen shows the **LIVE** camera preview.

| Problem | Try |
|---|---|
| Colours wrong (blue ↔ red) | tell me, it is a one-line change (`LCD_RGB_ELEMENT_ORDER_…`) |
| Image mirrored or rotated | tell me (`esp_lcd_panel_mirror`) |
| Garbage on the screen | lower `pclk_hz` to 20 MHz |
| `camera_init` fails | check the FPC cable. If it still fails, the board uses a different camera pin map → send me a clear photo of the board's back side |

## 4. First lens-retroreflection test (preview of E1)

1. Press **SELECT** to go to **DIFF** mode. Exposure is now locked. The AXIS LED is toggled ON/OFF and the screen shows the **difference image**: grey = small difference, **red = above threshold**.
2. Point the board at the **A9 mini camera** from **0.5 m**, lens facing the board, in normal room light. Its lens should appear as a **red dot**.
3. Check the principle:
   - a **lamp or the LED of a charger** in view should **not** turn red, because it is self-luminous and cancels out;
   - shiny objects (spoon, screw, glass) may turn red; note which ones.
4. **UP** cycles the exposure (50 → 1200). Find the value where the lens dot is clearest.
5. **BACK** switches to the **OFF-AXIS** LED (hypothesis H1): the lens dot should get much weaker, while a flat shiny object may not.
6. Repeat at 1.0 m and 1.5 m.

Every ON/OFF pair prints one line:
```
D,t_ms,led(A/O),aec,mean,sigma,thr,n_hot,peak,peak_x,peak_y,snr
```
Copy the monitor output (or log it) and send it to me. `snr` at the lens position vs distance is the first E1 data point. The threshold `K = 6σ` is provisional; E1 data will set it.
