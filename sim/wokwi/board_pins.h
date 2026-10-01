/* board_pins.h — SpyScan V1 pin map (single source of truth).
 * Draft for an ESP32-S3-EYE-style camera board (Freenove-type).
 * VERIFY against the purchased board's schematic before wiring (risk R1).
 */
#pragma once

/* ---- Display: ILI9341 240x320, SPI2 ---- */
#define PIN_LCD_SCLK   47
#define PIN_LCD_MOSI   21
#define PIN_LCD_MISO   (-1)   /* not used */
#define PIN_LCD_CS     41
#define PIN_LCD_DC     42
#define PIN_LCD_RST    (-1)   /* tied high, software reset */

/* ---- Illumination (MOSFET gates) ---- */
#define PIN_LED_AXIS   2      /* on-axis ring around the camera lens */
#define PIN_LED_OFF    14     /* off-axis pair (>= 30 mm offset)     */

/* ---- Buttons (active low, pull-up) ---- */
#define PIN_BTN_UP     38
#define PIN_BTN_SEL    39
#define PIN_BTN_BACK   40

/* ---- Analog ---- */
#define PIN_VBAT_SENSE 1      /* ADC1_CH0, 100k/100k divider */

/* ---- Optional ---- */
#define PIN_BUZZER     3      /* LEDC PWM */

/* ---- Camera (fixed by board, ESP32-S3-EYE style mapping) ---- */
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

/* ---- SIMULATION ONLY ----
 * In Wokwi there is no camera, so camera pin 10 is reused for a
 * potentiometer that simulates "distance to the hidden camera" (RSSI).
 */
#define PIN_SIM_DISTANCE 10
