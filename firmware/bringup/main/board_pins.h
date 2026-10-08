/* board_pins.h — SpyScan V1 pin map (single source of truth).
 * Board: ESP32-S3-N16R8 camera board (two USB-C, camera FPC, ESP32-S3-EYE-style
 * camera mapping). Camera pins are fixed by the board; everything else is ours.
 * Keep in sync with sim/wokwi/board_pins.h.
 */
#pragma once

/* ---- Display: ILI9341 240x320, SPI2 ---- */
#define PIN_LCD_SCLK   47
#define PIN_LCD_MOSI   21
#define PIN_LCD_MISO   (-1)
#define PIN_LCD_CS     41
#define PIN_LCD_DC     42
#define PIN_LCD_RST    (-1)   /* module RESET tied to 3V3; software reset is used */

/* ---- Illumination ---- */
#define PIN_LED_AXIS   2      /* on-axis ring (bring-up: 1 LED + 330 ohm direct) */
#define PIN_LED_OFF    14     /* off-axis pair                                  */

/* ---- Buttons (active low, internal pull-up) ---- */
#define PIN_BTN_UP     38
#define PIN_BTN_SEL    39
#define PIN_BTN_BACK   40

/* ---- Analog ---- */
#define PIN_VBAT_SENSE 1

/* ---- Camera (ESP32-S3-EYE style mapping) ---- */
#define PIN_CAM_PWDN   (-1)
#define PIN_CAM_RESET  (-1)
#define PIN_CAM_XCLK   15
#define PIN_CAM_SIOD   4
#define PIN_CAM_SIOC   5
#define PIN_CAM_D0     11
#define PIN_CAM_D1     9
#define PIN_CAM_D2     8
#define PIN_CAM_D3     10
#define PIN_CAM_D4     12
#define PIN_CAM_D5     18
#define PIN_CAM_D6     17
#define PIN_CAM_D7     16
#define PIN_CAM_VSYNC  6
#define PIN_CAM_HREF   7
#define PIN_CAM_PCLK   13
