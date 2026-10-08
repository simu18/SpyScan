/*
 * SpyScan — hardware bring-up + first optical test (ESP32-S3-N16R8, ESP-IDF 5.3+/6.x)
 *
 * On boot it checks and prints (serial, USB Serial/JTAG console):
 *   chip / flash / PSRAM size, camera sensor model, camera frame rate,
 *   display, LEDs, buttons.
 * Then it runs two screens:
 *   LIVE  camera preview on the ILI9341 (auto exposure)
 *   DIFF  first lens-retroreflection test: exposure locked, on-axis LED toggled,
 *         D = Y(LED on) - Y(LED off); pixels above mean + K*sigma are marked red.
 *
 * Buttons:  SEL  = LIVE <-> DIFF
 *           UP   = (DIFF) next exposure value   (LIVE) blink both LEDs once
 *           BACK = (DIFF) use the OFF-AXIS LED instead of the on-axis LED (H1 preview)
 *
 * Serial lines in DIFF mode (for E1 data collection):
 *   D,<t_ms>,<led:A|O>,<aec>,<mean>,<sigma>,<thr>,<n_hot>,<peak>,<peak_x>,<peak_y>,<snr>
 * K = 6 is PROVISIONAL — the real threshold comes from E1 data.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_camera.h"
#include "board_pins.h"

static const char *TAG = "bringup";

#define W 320
#define H 240
#define STRIP_LINES 40
#define K_SIGMA 6.0f            /* provisional, see E1 */

static esp_lcd_panel_handle_t s_panel = NULL;
static SemaphoreHandle_t s_lcd_done;
static uint16_t *s_strip[2];    /* DMA-capable strips in internal RAM */
static bool s_cam_ok = false, s_lcd_ok = false;

static inline uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

/* ------------------------------------------------------------------ LCD */
static bool lcd_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t hp = pdFALSE;
    xSemaphoreGiveFromISR(s_lcd_done, &hp);
    return hp == pdTRUE;
}

static esp_err_t lcd_init(void)
{
    spi_bus_config_t bus = ILI9341_PANEL_BUS_SPI_CONFIG(PIN_LCD_SCLK, PIN_LCD_MOSI, W * STRIP_LINES * 2);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "spi bus");

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,           /* drop to 20 MHz if the image is corrupted */
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 4,
        .on_color_trans_done = lcd_trans_done,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io), TAG, "io");

    esp_lcd_panel_dev_config_t dev = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9341(io, &dev, &s_panel), TAG, "panel");
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_swap_xy(s_panel, true);          /* landscape 320x240 */
    esp_lcd_panel_mirror(s_panel, false, false);   /* flip here if the image is mirrored */
    esp_lcd_panel_disp_on_off(s_panel, true);

    s_lcd_done = xSemaphoreCreateCounting(2, 2);
    for (int i = 0; i < 2; ++i) {
        s_strip[i] = heap_caps_malloc(W * STRIP_LINES * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!s_strip[i]) return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* Draw a full 320x240 frame whose pixels are big-endian RGB565 bytes (LCD byte order). */
static void lcd_draw_be(const uint8_t *frame_be)
{
    if (!s_lcd_ok) return;
    for (int y = 0, i = 0; y < H; y += STRIP_LINES, i ^= 1) {
        xSemaphoreTake(s_lcd_done, portMAX_DELAY);
        memcpy(s_strip[i], frame_be + (size_t)y * W * 2, W * STRIP_LINES * 2);
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, W, y + STRIP_LINES, s_strip[i]);
    }
}

static void lcd_fill(uint16_t rgb565)
{
    if (!s_lcd_ok) return;
    uint16_t be = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
    for (int y = 0, i = 0; y < H; y += STRIP_LINES, i ^= 1) {
        xSemaphoreTake(s_lcd_done, portMAX_DELAY);
        for (int k = 0; k < W * STRIP_LINES; ++k) s_strip[i][k] = be;
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, W, y + STRIP_LINES, s_strip[i]);
    }
}

/* --------------------------------------------------------------- Camera */
static esp_err_t cam_init(void)
{
    camera_config_t c = {
        .pin_pwdn = PIN_CAM_PWDN, .pin_reset = PIN_CAM_RESET, .pin_xclk = PIN_CAM_XCLK,
        .pin_sccb_sda = PIN_CAM_SIOD, .pin_sccb_scl = PIN_CAM_SIOC,
        .pin_d7 = PIN_CAM_D7, .pin_d6 = PIN_CAM_D6, .pin_d5 = PIN_CAM_D5, .pin_d4 = PIN_CAM_D4,
        .pin_d3 = PIN_CAM_D3, .pin_d2 = PIN_CAM_D2, .pin_d1 = PIN_CAM_D1, .pin_d0 = PIN_CAM_D0,
        .pin_vsync = PIN_CAM_VSYNC, .pin_href = PIN_CAM_HREF, .pin_pclk = PIN_CAM_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,          /* 320x240 = display size */
        .jpeg_quality = 12,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
    };
    esp_err_t err = esp_camera_init(&c);
    if (err != ESP_OK) return err;
    sensor_t *s = esp_camera_sensor_get();
    const char *name = "unknown";
    switch (s->id.PID) {
        case OV2640_PID: name = "OV2640"; break;
        case OV3660_PID: name = "OV3660"; break;
        case OV5640_PID: name = "OV5640"; break;
        case OV7725_PID: name = "OV7725"; break;
        default: break;
    }
    printf("I,%lu,camera,%s,PID=0x%04x\n", (unsigned long)now_ms(), name, s->id.PID);
    return ESP_OK;
}

static void cam_auto(bool on, int aec)
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_exposure_ctrl(s, on);
    s->set_gain_ctrl(s, on);
    s->set_whitebal(s, on);
    s->set_awb_gain(s, on);
    if (!on) {
        s->set_aec2(s, 0);
        s->set_aec_value(s, aec);      /* 0..1200 on OV2640 */
        s->set_agc_gain(s, 0);         /* minimum analogue gain -> lowest noise */
    }
}

/* Discard frames so the returned one was fully exposed after a lighting change
 * (rolling shutter + double buffering). */
static camera_fb_t *grab_fresh(int discard)
{
    for (int i = 0; i < discard; ++i) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
    }
    return esp_camera_fb_get();
}

/* RGB565 big-endian bytes -> luminance 0..255 */
static inline uint8_t lum(const uint8_t *p)
{
    uint16_t v = (uint16_t)(p[0] << 8 | p[1]);
    int r = (v >> 11) << 3, g = ((v >> 5) & 0x3F) << 2, b = (v & 0x1F) << 3;
    return (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
}

/* -------------------------------------------------------------- Buttons */
typedef struct { int pin; int stable, last; uint32_t t; } btn_t;
static btn_t s_btn[3] = { { PIN_BTN_UP, 1, 1, 0 }, { PIN_BTN_SEL, 1, 1, 0 }, { PIN_BTN_BACK, 1, 1, 0 } };

/* returns bitmask of buttons pressed since last call (bit0 UP, bit1 SEL, bit2 BACK) */
static int buttons_poll(void)
{
    int ev = 0;
    uint32_t t = now_ms();
    for (int i = 0; i < 3; ++i) {
        int l = gpio_get_level(s_btn[i].pin);
        if (l != s_btn[i].last) { s_btn[i].last = l; s_btn[i].t = t; }
        if (t - s_btn[i].t > 20 && l != s_btn[i].stable) {
            s_btn[i].stable = l;
            if (!l) { ev |= 1 << i; printf("I,%lu,button,%s\n", (unsigned long)t, i == 0 ? "UP" : i == 1 ? "SEL" : "BACK"); }
        }
    }
    return ev;
}

/* ------------------------------------------------------------ Self-test */
static void report_system(void)
{
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    uint32_t flash = 0;
    esp_flash_get_size(NULL, &flash);
    printf("I,%lu,chip,cores=%d,rev=%d\n", (unsigned long)now_ms(), ci.cores, ci.revision);
    printf("I,%lu,flash,%lu MB\n", (unsigned long)now_ms(), (unsigned long)(flash >> 20));
#if CONFIG_SPIRAM
    printf("I,%lu,psram,%u KB\n", (unsigned long)now_ms(), (unsigned)(esp_psram_get_size() >> 10));
#else
    printf("I,%lu,psram,DISABLED in sdkconfig\n", (unsigned long)now_ms());
#endif
    printf("I,%lu,heap,internal=%u KB,psram_free=%u KB\n", (unsigned long)now_ms(),
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >> 10),
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >> 10));
}

static void leds_blink(void)
{
    for (int i = 0; i < 3; ++i) {
        gpio_set_level(PIN_LED_AXIS, 1); vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(PIN_LED_AXIS, 0); gpio_set_level(PIN_LED_OFF, 1); vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(PIN_LED_OFF, 0);
    }
}

static void measure_fps(void)
{
    if (!s_cam_ok) return;
    uint32_t t0 = now_ms();
    int n = 0;
    while (now_ms() - t0 < 2000) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) { n++; esp_camera_fb_return(fb); }
    }
    printf("I,%lu,camera_fps,%.1f (QVGA RGB565, no display)\n", (unsigned long)now_ms(), n * 1000.0 / (now_ms() - t0));
}

/* ----------------------------------------------------------------- Main */
void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    printf("I,%lu,boot,spyscan-bringup,v0.1\n", (unsigned long)now_ms());
    report_system();

    gpio_config_t out = { .pin_bit_mask = (1ULL << PIN_LED_AXIS) | (1ULL << PIN_LED_OFF), .mode = GPIO_MODE_OUTPUT };
    gpio_config(&out);
    gpio_config_t in = { .pin_bit_mask = (1ULL << PIN_BTN_UP) | (1ULL << PIN_BTN_SEL) | (1ULL << PIN_BTN_BACK),
                         .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&in);
    leds_blink();
    printf("I,%lu,leds,blinked AXIS(GPIO%d) and OFF(GPIO%d) 3x\n", (unsigned long)now_ms(), PIN_LED_AXIS, PIN_LED_OFF);

    s_lcd_ok = (lcd_init() == ESP_OK);
    printf("I,%lu,lcd,%s\n", (unsigned long)now_ms(), s_lcd_ok ? "OK" : "FAIL");
    if (s_lcd_ok) {   /* colour bars: you should see RED, GREEN, BLUE, WHITE */
        const uint16_t c[4] = { 0xF800, 0x07E0, 0x001F, 0xFFFF };
        for (int i = 0; i < 4; ++i) { lcd_fill(c[i]); vTaskDelay(pdMS_TO_TICKS(400)); }
    }

    esp_err_t ce = cam_init();
    s_cam_ok = (ce == ESP_OK);
    printf("I,%lu,camera_init,%s\n", (unsigned long)now_ms(), s_cam_ok ? "OK" : esp_err_to_name(ce));
    measure_fps();

    if (!s_cam_ok) {
        printf("I,%lu,halt,camera missing - check the FPC cable orientation and the pin map\n", (unsigned long)now_ms());
        for (;;) { buttons_poll(); vTaskDelay(pdMS_TO_TICKS(10)); }
    }

    uint8_t *diff_img = heap_caps_malloc(W * H * 2, MALLOC_CAP_SPIRAM);
    uint8_t *y_on = heap_caps_malloc(W * H, MALLOC_CAP_SPIRAM);
    configASSERT(diff_img && y_on);

    static const int AEC[] = { 50, 150, 300, 600, 1200 };
    int aec_i = 1;
    bool diff_mode = false, use_off_axis = false;
    uint32_t frames = 0, t_fps = now_ms();

    for (;;) {
        int ev = buttons_poll();
        if (ev & 2) {                                   /* SEL: switch mode */
            diff_mode = !diff_mode;
            cam_auto(!diff_mode, AEC[aec_i]);
            printf("I,%lu,mode,%s\n", (unsigned long)now_ms(), diff_mode ? "DIFF" : "LIVE");
        }
        if (ev & 1) {
            if (diff_mode) { aec_i = (aec_i + 1) % 5; cam_auto(false, AEC[aec_i]);
                             printf("I,%lu,aec,%d\n", (unsigned long)now_ms(), AEC[aec_i]); }
            else leds_blink();
        }
        if ((ev & 4) && diff_mode) {
            use_off_axis = !use_off_axis;
            printf("I,%lu,led_group,%s\n", (unsigned long)now_ms(), use_off_axis ? "OFF_AXIS" : "AXIS");
        }

        if (!diff_mode) {                               /* ---------- LIVE */
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                if (fb->len >= W * H * 2) lcd_draw_be(fb->buf);
                esp_camera_fb_return(fb);
                frames++;
            }
            if (now_ms() - t_fps > 5000) {
                printf("I,%lu,live_fps,%.1f\n", (unsigned long)now_ms(), frames * 1000.0 / (now_ms() - t_fps));
                frames = 0; t_fps = now_ms();
            }
            continue;
        }

        /* ---------------------------------------------------------- DIFF */
        int led = use_off_axis ? PIN_LED_OFF : PIN_LED_AXIS;
        gpio_set_level(led, 1);
        camera_fb_t *fb = grab_fresh(2);
        if (!fb) continue;
        for (int i = 0; i < W * H; ++i) y_on[i] = lum(fb->buf + 2 * i);
        esp_camera_fb_return(fb);

        gpio_set_level(led, 0);
        fb = grab_fresh(2);
        if (!fb) continue;

        /* D = max(0, on - off); statistics over the whole frame */
        double sum = 0, sum2 = 0;
        int peak = 0, px = 0, py = 0;
        for (int i = 0; i < W * H; ++i) {
            int d = (int)y_on[i] - (int)lum(fb->buf + 2 * i);
            if (d < 0) d = 0;
            y_on[i] = (uint8_t)d;                       /* reuse buffer for D */
            sum += d; sum2 += (double)d * d;
            if (d > peak) { peak = d; px = i % W; py = i / W; }
        }
        esp_camera_fb_return(fb);
        float mean = (float)(sum / (W * H));
        float sigma = sqrtf(fmaxf((float)(sum2 / (W * H)) - mean * mean, 1e-3f));
        float thr = mean + K_SIGMA * sigma;
        int hot = 0;
        for (int i = 0; i < W * H; ++i) {
            uint16_t c;
            if (y_on[i] > thr) { c = 0xF800; hot++; }        /* red = above threshold */
            else { int g = y_on[i] * 4; if (g > 255) g = 255;  /* amplified grey */
                   c = (uint16_t)(((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3)); }
            diff_img[2 * i] = c >> 8; diff_img[2 * i + 1] = c & 0xFF;
        }
        lcd_draw_be(diff_img);
        printf("D,%lu,%c,%d,%.2f,%.2f,%.1f,%d,%d,%d,%d,%.1f\n", (unsigned long)now_ms(),
               use_off_axis ? 'O' : 'A', AEC[aec_i], mean, sigma, thr, hot, peak, px, py,
               (peak - mean) / sigma);
    }
}
