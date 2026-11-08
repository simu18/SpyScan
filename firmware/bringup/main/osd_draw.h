/* osd_draw.h — SpyScan On-Screen Display (OSD) HUD Graphics Engine
 *
 * Ultra-fast, lightweight graphics rendering directly into RGB565 big-endian
 * framebuffers for the 2.4" 320x240 ILI9341 display.
 * Designed for real-time tactical counter-surveillance HUD instrumentation.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSD_WIDTH   320
#define OSD_HEIGHT  240

/* RGB565 Tactical Color Palette */
#define OSD_COLOR_BLACK      0x0000
#define OSD_COLOR_NAVY       0x0841  /* Dark tactical slate background */
#define OSD_COLOR_DARKGRAY   0x18C3  /* Header / panel background */
#define OSD_COLOR_MIDGRAY    0x4A69  /* Frame borders / gridlines */
#define OSD_COLOR_LIGHTGRAY  0xBDD7  /* Secondary text */
#define OSD_COLOR_WHITE      0xFFFF  /* Primary labels */
#define OSD_COLOR_GREEN      0x07E0  /* Normal / Target locked / Safe */
#define OSD_COLOR_RED        0xF800  /* Alert / Bug detected / Low battery */
#define OSD_COLOR_ORANGE     0xFD20  /* Warning / High traffic uplink */
#define OSD_COLOR_YELLOW     0xFFE0  /* Active indicator */
#define OSD_COLOR_CYAN       0x07FF  /* RF spectrum / Info accent */
#define OSD_COLOR_MAGENTA    0xF81F  /* 850/940 nm NIR spectrum / IR locked */
#define OSD_TRANSPARENT      0x0001  /* Key for transparent text background */

/* Basic Primitive Drawing */
void osd_draw_pixel(uint8_t *fb, int x, int y, uint16_t color_rgb565);
void osd_fill_rect(uint8_t *fb, int x, int y, int w, int h, uint16_t color_rgb565);
void osd_draw_rect(uint8_t *fb, int x, int y, int w, int h, uint16_t color_rgb565);
void osd_draw_hline(uint8_t *fb, int x, int y, int len, uint16_t color_rgb565);
void osd_draw_vline(uint8_t *fb, int x, int y, int len, uint16_t color_rgb565);

/* Text Rendering Engine */
void osd_draw_char(uint8_t *fb, int x, int y, char c, uint16_t fg, uint16_t bg, int scale);
void osd_draw_string(uint8_t *fb, int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale);
void osd_printf(uint8_t *fb, int x, int y, uint16_t fg, uint16_t bg, int scale, const char *fmt, ...);

/* HUD High-Level Components */
void osd_draw_header(uint8_t *fb, const char *title, float vbat, int bat_pct, const char *status_badge, uint16_t badge_color);
void osd_draw_footer(uint8_t *fb, const char *up_hint, const char *sel_hint, const char *back_hint);
void osd_draw_progress_bar(uint8_t *fb, int x, int y, int w, int h, float val, float min_v, float max_v, uint16_t fg, uint16_t bg);
void osd_draw_reticle(uint8_t *fb, int cx, int cy, int size, uint16_t color, bool locked);
void osd_draw_badge(uint8_t *fb, int x, int y, const char *text, uint16_t fg, uint16_t bg);

#ifdef __cplusplus
}
#endif
