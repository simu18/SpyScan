/* osd_draw.c — SpyScan On-Screen Display (OSD) HUD Graphics Engine
 *
 * Direct RGB565 framebuffer rendering implementation.
 */

#include "osd_draw.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Standard 5x7 ASCII bitmap font (columns 0..4 for characters 0x20 ' ' to 0x7E '~') */
static const uint8_t s_font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* ' ' */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* '!' */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* '"' */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* '#' */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* '$' */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* '%' */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* '&' */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* '\'' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* '(' */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ')' */
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, /* '*' */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* '+' */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* ',' */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* '-' */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* '.' */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* '/' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* '9' */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* ':' */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ';' */
    {0x00, 0x08, 0x14, 0x22, 0x41}, /* '<' */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* '=' */
    {0x41, 0x22, 0x14, 0x08, 0x00}, /* '>' */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* '?' */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* '@' */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 'E' */
    {0x7F, 0x09, 0x09, 0x01, 0x01}, /* 'F' */
    {0x3E, 0x41, 0x41, 0x51, 0x32}, /* 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 'L' */
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, /* 'M' */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 'V' */
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, /* 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* 'X' */
    {0x03, 0x04, 0x78, 0x04, 0x03}, /* 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* 'Z' */
    {0x00, 0x00, 0x7F, 0x41, 0x41}, /* '[' */
    {0x02, 0x04, 0x08, 0x10, 0x20}, /* '\' */
    {0x41, 0x41, 0x7F, 0x00, 0x00}, /* ']' */
    {0x04, 0x02, 0x01, 0x02, 0x04}, /* '^' */
    {0x40, 0x40, 0x40, 0x40, 0x40}, /* '_' */
    {0x00, 0x01, 0x02, 0x04, 0x00}, /* '`' */
    {0x20, 0x54, 0x54, 0x54, 0x78}, /* 'a' */
    {0x7F, 0x48, 0x44, 0x44, 0x38}, /* 'b' */
    {0x38, 0x44, 0x44, 0x44, 0x20}, /* 'c' */
    {0x38, 0x44, 0x44, 0x48, 0x7F}, /* 'd' */
    {0x38, 0x54, 0x54, 0x54, 0x18}, /* 'e' */
    {0x08, 0x7E, 0x09, 0x01, 0x02}, /* 'f' */
    {0x08, 0x14, 0x54, 0x54, 0x3C}, /* 'g' */
    {0x7F, 0x08, 0x04, 0x04, 0x78}, /* 'h' */
    {0x00, 0x44, 0x7D, 0x40, 0x00}, /* 'i' */
    {0x20, 0x40, 0x44, 0x3D, 0x00}, /* 'j' */
    {0x00, 0x7F, 0x10, 0x28, 0x44}, /* 'k' */
    {0x00, 0x41, 0x7F, 0x40, 0x00}, /* 'l' */
    {0x7C, 0x04, 0x18, 0x04, 0x78}, /* 'm' */
    {0x7C, 0x08, 0x04, 0x04, 0x78}, /* 'n' */
    {0x38, 0x44, 0x44, 0x44, 0x38}, /* 'o' */
    {0x7C, 0x14, 0x14, 0x14, 0x08}, /* 'p' */
    {0x08, 0x14, 0x14, 0x18, 0x7C}, /* 'q' */
    {0x7C, 0x08, 0x04, 0x04, 0x08}, /* 'r' */
    {0x48, 0x54, 0x54, 0x54, 0x20}, /* 's' */
    {0x04, 0x3F, 0x44, 0x40, 0x20}, /* 't' */
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, /* 'u' */
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, /* 'v' */
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, /* 'w' */
    {0x44, 0x28, 0x10, 0x28, 0x44}, /* 'x' */
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, /* 'y' */
    {0x44, 0x64, 0x54, 0x4C, 0x44}, /* 'z' */
    {0x00, 0x08, 0x36, 0x41, 0x00}, /* '{' */
    {0x00, 0x00, 0x7F, 0x00, 0x00}, /* '|' */
    {0x00, 0x41, 0x36, 0x08, 0x00}, /* '}' */
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  /* '~' */
};

/* Fast Big-Endian Pixel Writer */
void osd_draw_pixel(uint8_t *fb, int x, int y, uint16_t color_rgb565)
{
    if (!fb || x < 0 || x >= OSD_WIDTH || y < 0 || y >= OSD_HEIGHT) return;
    int idx = 2 * (y * OSD_WIDTH + x);
    fb[idx] = (uint8_t)(color_rgb565 >> 8);
    fb[idx + 1] = (uint8_t)(color_rgb565 & 0xFF);
}

void osd_fill_rect(uint8_t *fb, int x, int y, int w, int h, uint16_t color_rgb565)
{
    if (!fb || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = (x + w > OSD_WIDTH) ? OSD_WIDTH : x + w;
    int y1 = (y + h > OSD_HEIGHT) ? OSD_HEIGHT : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    uint8_t hi = (uint8_t)(color_rgb565 >> 8);
    uint8_t lo = (uint8_t)(color_rgb565 & 0xFF);

    for (int cy = y0; cy < y1; ++cy) {
        int row_idx = 2 * (cy * OSD_WIDTH + x0);
        for (int cx = x0; cx < x1; ++cx) {
            fb[row_idx++] = hi;
            fb[row_idx++] = lo;
        }
    }
}

void osd_draw_rect(uint8_t *fb, int x, int y, int w, int h, uint16_t color_rgb565)
{
    osd_draw_hline(fb, x, y, w, color_rgb565);
    osd_draw_hline(fb, x, y + h - 1, w, color_rgb565);
    osd_draw_vline(fb, x, y, h, color_rgb565);
    osd_draw_vline(fb, x + w - 1, y, h, color_rgb565);
}

void osd_draw_hline(uint8_t *fb, int x, int y, int len, uint16_t color_rgb565)
{
    if (!fb || y < 0 || y >= OSD_HEIGHT || len <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int x1 = (x + len > OSD_WIDTH) ? OSD_WIDTH : x + len;
    if (x0 >= x1) return;

    uint8_t hi = (uint8_t)(color_rgb565 >> 8);
    uint8_t lo = (uint8_t)(color_rgb565 & 0xFF);
    int idx = 2 * (y * OSD_WIDTH + x0);
    for (int cx = x0; cx < x1; ++cx) {
        fb[idx++] = hi;
        fb[idx++] = lo;
    }
}

void osd_draw_vline(uint8_t *fb, int x, int y, int len, uint16_t color_rgb565)
{
    if (!fb || x < 0 || x >= OSD_WIDTH || len <= 0) return;
    int y0 = y < 0 ? 0 : y;
    int y1 = (y + len > OSD_HEIGHT) ? OSD_HEIGHT : y + len;
    if (y0 >= y1) return;

    uint8_t hi = (uint8_t)(color_rgb565 >> 8);
    uint8_t lo = (uint8_t)(color_rgb565 & 0xFF);
    for (int cy = y0; cy < y1; ++cy) {
        int idx = 2 * (cy * OSD_WIDTH + x);
        fb[idx] = hi;
        fb[idx + 1] = lo;
    }
}

void osd_draw_char(uint8_t *fb, int x, int y, char c, uint16_t fg, uint16_t bg, int scale)
{
    if (c < ' ' || c > '~') c = '?';
    int font_idx = c - ' ';
    if (scale < 1) scale = 1;

    for (int col = 0; col < 5; ++col) {
        uint8_t line = s_font5x7[font_idx][col];
        for (int row = 0; row < 7; ++row) {
            bool pixel_on = (line & (1 << row)) != 0;
            if (pixel_on) {
                if (scale == 1) osd_draw_pixel(fb, x + col, y + row, fg);
                else osd_fill_rect(fb, x + col * scale, y + row * scale, scale, scale, fg);
            } else if (bg != OSD_TRANSPARENT) {
                if (scale == 1) osd_draw_pixel(fb, x + col, y + row, bg);
                else osd_fill_rect(fb, x + col * scale, y + row * scale, scale, scale, bg);
            }
        }
    }
    /* 1-pixel right spacing between characters */
    if (bg != OSD_TRANSPARENT) {
        if (scale == 1) osd_draw_vline(fb, x + 5, y, 7, bg);
        else osd_fill_rect(fb, x + 5 * scale, y, scale, 7 * scale, bg);
    }
}

void osd_draw_string(uint8_t *fb, int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale)
{
    if (!str) return;
    int cur_x = x;
    int char_w = (scale < 1 ? 1 : scale) * 6;
    while (*str) {
        if (*str == '\n') {
            y += (scale < 1 ? 1 : scale) * 9;
            cur_x = x;
        } else {
            osd_draw_char(fb, cur_x, y, *str, fg, bg, scale);
            cur_x += char_w;
        }
        str++;
    }
}

void osd_printf(uint8_t *fb, int x, int y, uint16_t fg, uint16_t bg, int scale, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    osd_draw_string(fb, x, y, buf, fg, bg, scale);
}

void osd_draw_header(uint8_t *fb, const char *title, float vbat, int bat_pct, const char *status_badge, uint16_t badge_color)
{
    /* Dark Slate Header Banner */
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, 20, OSD_COLOR_DARKGRAY);
    osd_draw_hline(fb, 0, 20, OSD_WIDTH, OSD_COLOR_MIDGRAY);

    /* Title */
    osd_printf(fb, 6, 6, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1, "%s", title ? title : "SPYSCAN");

    /* Status Badge if present */
    if (status_badge) {
        int badge_len = (int)strlen(status_badge) * 6 + 8;
        int bx = 135;
        osd_fill_rect(fb, bx, 3, badge_len, 14, badge_color);
        osd_printf(fb, bx + 4, 6, OSD_COLOR_BLACK, OSD_TRANSPARENT, 1, "%s", status_badge);
    }

    /* Battery Telemetry [■■■□] 3.92V 85% */
    uint16_t bat_col = (bat_pct < 20) ? OSD_COLOR_RED : (bat_pct < 40) ? OSD_COLOR_ORANGE : OSD_COLOR_GREEN;
    osd_printf(fb, OSD_WIDTH - 85, 6, bat_col, OSD_TRANSPARENT, 1, "%.2fV %d%%", vbat, bat_pct);

    /* Battery Icon */
    int ix = OSD_WIDTH - 20, iy = 6;
    osd_draw_rect(fb, ix, iy, 14, 8, bat_col);
    osd_draw_vline(fb, ix + 14, iy + 2, 4, bat_col);
    int fill_w = (bat_pct * 10) / 100;
    if (fill_w > 0) osd_fill_rect(fb, ix + 2, iy + 2, fill_w, 4, bat_col);
}

void osd_draw_footer(uint8_t *fb, const char *up_hint, const char *sel_hint, const char *back_hint)
{
    osd_fill_rect(fb, 0, OSD_HEIGHT - 16, OSD_WIDTH, 16, OSD_COLOR_DARKGRAY);
    osd_draw_hline(fb, 0, OSD_HEIGHT - 17, OSD_WIDTH, OSD_COLOR_MIDGRAY);

    char footer_txt[80];
    snprintf(footer_txt, sizeof(footer_txt), "^:%-10s  *:%-10s  <:%-10s",
             up_hint ? up_hint : "-", sel_hint ? sel_hint : "-", back_hint ? back_hint : "-");
    osd_draw_string(fb, 8, OSD_HEIGHT - 12, footer_txt, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1);
}

void osd_draw_progress_bar(uint8_t *fb, int x, int y, int w, int h, float val, float min_v, float max_v, uint16_t fg, uint16_t bg)
{
    osd_fill_rect(fb, x, y, w, h, bg);
    osd_draw_rect(fb, x, y, w, h, OSD_COLOR_MIDGRAY);

    if (max_v > min_v) {
        float ratio = (val - min_v) / (max_v - min_v);
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        int fill_w = (int)(ratio * (w - 4));
        if (fill_w > 0) {
            osd_fill_rect(fb, x + 2, y + 2, fill_w, h - 4, fg);
        }
    }
}

void osd_draw_reticle(uint8_t *fb, int cx, int cy, int size, uint16_t color, bool locked)
{
    if (size < 6) size = 10;
    int b = size;

    /* Corner Bracket Crosshairs */
    /* Top-Left */
    osd_draw_hline(fb, cx - b, cy - b, b / 2, color);
    osd_draw_vline(fb, cx - b, cy - b, b / 2, color);
    /* Top-Right */
    osd_draw_hline(fb, cx + b / 2, cy - b, b / 2, color);
    osd_draw_vline(fb, cx + b, cy - b, b / 2, color);
    /* Bottom-Left */
    osd_draw_hline(fb, cx - b, cy + b, b / 2, color);
    osd_draw_vline(fb, cx - b, cy + b / 2, b / 2, color);
    /* Bottom-Right */
    osd_draw_hline(fb, cx + b / 2, cy + b, b / 2, color);
    osd_draw_vline(fb, cx + b, cy + b / 2, b / 2, color);

    /* Center Crosshair or Dot */
    osd_draw_pixel(fb, cx, cy, color);
    if (locked) {
        /* Full bounding box and center dot */
        osd_draw_rect(fb, cx - b, cy - b, 2 * b + 1, 2 * b + 1, color);
        osd_draw_hline(fb, cx - 2, cy, 5, color);
        osd_draw_vline(fb, cx, cy - 2, 5, color);
    }
}

void osd_draw_badge(uint8_t *fb, int x, int y, const char *text, uint16_t fg, uint16_t bg)
{
    if (!text) return;
    int len = (int)strlen(text) * 6 + 6;
    osd_fill_rect(fb, x, y, len, 12, bg);
    osd_draw_rect(fb, x, y, len, 12, fg);
    osd_draw_string(fb, x + 3, y + 3, text, fg, OSD_TRANSPARENT, 1);
}

