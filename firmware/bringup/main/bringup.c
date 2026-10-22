/*
 * SpyScan — hardware bring-up + first optical test (ESP32-S3-N16R8, ESP-IDF 5.3+/6.x)
 *
 * On boot it checks and prints (serial, USB Serial/JTAG console):
 *   chip / flash / PSRAM size, camera sensor model, camera frame rate,
 *   display, LEDs, buttons.
 * Then it runs, with NO buttons required:
 *   LIVE       ~6 s camera preview on the ILI9341 (auto exposure)
 *   DIFF_AUTO  automatic lens-retroreflection sweep, forever: for each exposure
 *              (50..1200) it runs AXIS then OFF-AXIS diff cycles, pulsing the LED
 *              and printing one D line per cycle. D = Y(LED on) - Y(LED off);
 *              pixels above mean + K*sigma are marked red on the display.
 *              M lines mark each (sweep, aec, led) segment.
 * Buttons (optional, if fitted) are still polled and logged but are not needed.
 *
 * Serial lines in DIFF_AUTO (for E1 data collection):
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
#include <fcntl.h>
#include "esp_camera.h"
#include "board_pins.h"

static const char *TAG = "bringup";

#define W 320
#define H 240
#define STRIP_LINES 40
#define K_SIGMA 6.0f            /* provisional, see E1 */

typedef enum {
    OPMODE_LIVE = 0,         /* Live color preview */
    OPMODE_DIFF = 1,         /* Active optical retroreflection (green reticle [ + ]) */
    OPMODE_IR_HUNTER = 2     /* Passive Infrared Night-Vision Emitter Detector (purple reticle) */
} opmode_t;

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
        .pclk_hz = 10 * 1000 * 1000,           /* 10 MHz: safe over breadboard jumpers (was 40 MHz) */
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

static void cam_auto(bool on, int aec, int gain_override)
{
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return;
    s->set_exposure_ctrl(s, on);
    s->set_gain_ctrl(s, on);
    s->set_whitebal(s, on);
    s->set_awb_gain(s, on);
    if (!on) {
        s->set_aec2(s, 0);
        s->set_aec_value(s, aec);      /* 0..1200 */
        int gain = (gain_override > 0) ? gain_override : ((s->id.PID == OV3660_PID) ? 4 : 2);
        s->set_agc_gain(s, gain);
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

/* ----------------------------------------------- serial frame streaming
 * Sends a downsampled 80x60 image to the laptop so the physical LCD is
 * optional. One F line per LIVE frame (throttled) and per DIFF cycle:
 *   F,<aec>,<led L|A|O>,<w>,<h>,<thr>,<peak_x>,<peak_y>,<base64 of w*h bytes>
 * The laptop viewer (tools/e1/e1_view.py) decodes and displays it. */
#define DSW 80
#define DSH 60
#define DSBX (W / DSW)              /* 4 */
#define DSBY (H / DSH)              /* 4 */
static uint8_t s_small[DSW * DSH];
static char    s_b64[DSW * DSH * 4 / 3 + 8];

static const char B64TAB[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void b64_encode(const uint8_t *in, int n, char *out)
{
    int o = 0;
    for (int i = 0; i < n; i += 3) {
        int b0 = in[i], b1 = (i + 1 < n) ? in[i + 1] : 0, b2 = (i + 2 < n) ? in[i + 2] : 0;
        out[o++] = B64TAB[b0 >> 2];
        out[o++] = B64TAB[((b0 & 3) << 4) | (b1 >> 4)];
        out[o++] = (i + 1 < n) ? B64TAB[((b1 & 15) << 2) | (b2 >> 6)] : '=';
        out[o++] = (i + 2 < n) ? B64TAB[b2 & 63] : '=';
    }
    out[o] = 0;
}
static void emit_small(int aec, char led, float thr, int px, int py)
{
    b64_encode(s_small, DSW * DSH, s_b64);
    printf("F,%d,%c,%d,%d,%.1f,%d,%d,%s\n", aec, led, DSW, DSH, thr, px, py, s_b64);
}

/* Draws a green targeting box around the detected lens */
static void draw_reticle(uint8_t *img, int cx, int cy, uint16_t color_rgb565)
{
    uint8_t hi = color_rgb565 >> 8;
    uint8_t lo = color_rgb565 & 0xFF;
    int b = 10;
    for (int k = -b; k <= b; ++k) {
        if (cy - b >= 0 && cx + k >= 0 && cx + k < W) { img[2 * ((cy - b) * W + (cx + k))] = hi; img[2 * ((cy - b) * W + (cx + k)) + 1] = lo; }
        if (cy + b < H  && cx + k >= 0 && cx + k < W) { img[2 * ((cy + b) * W + (cx + k))] = hi; img[2 * ((cy + b) * W + (cx + k)) + 1] = lo; }
        if (cx - b >= 0 && cy + k >= 0 && cy + k < H) { img[2 * ((cy + k) * W + (cx - b))] = hi; img[2 * ((cy + k) * W + (cx - b)) + 1] = lo; }
        if (cx + b < W  && cy + k >= 0 && cy + k < H) { img[2 * ((cy + k) * W + (cx + b))] = hi; img[2 * ((cy + k) * W + (cx + b)) + 1] = lo; }
    }
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

#ifdef PIN_IR_RX
    if (PIN_IR_RX >= 0) {
        gpio_config_t ir_in = {
            .pin_bit_mask = 1ULL << PIN_IR_RX,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&ir_in);
        printf("I,%lu,ir_rx,GPIO%d ready for 5V IR module\n", (unsigned long)now_ms(), PIN_IR_RX);
    }
#endif

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

    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    uint8_t *diff_img = heap_caps_malloc(W * H * 2, MALLOC_CAP_SPIRAM);
    uint8_t *y_on     = heap_caps_malloc(W * H, MALLOC_CAP_SPIRAM);
    uint8_t *y_off    = heap_caps_malloc(W * H, MALLOC_CAP_SPIRAM);
    configASSERT(diff_img && y_on && y_off);

    static const int AEC_PRESETS[] = { 300, 600, 900, 1200 };
    const int N_AEC = 4;
    int aec_idx = 1;                    /* default AEC = 600 */
    opmode_t mode = OPMODE_DIFF;        /* default mode */
    bool use_off_axis = false;
    uint32_t t_emit = 0;

    /* ---- Phase 1: LIVE preview for 3 s (camera + LCD sanity) ---- */
    cam_auto(true, 0, 0);
    printf("I,%lu,mode,LIVE (3 s preview)\n", (unsigned long)now_ms());
    uint32_t t_live = now_ms();
    while (now_ms() - t_live < 3000) {
        buttons_poll();
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            if (fb->len >= W * H * 2) {
                lcd_draw_be(fb->buf);
                if (now_ms() - t_emit > 300) {          /* stream luma to laptop */
                    for (int by = 0; by < DSH; ++by)
                        for (int bx = 0; bx < DSW; ++bx) {
                            int mx = 0;
                            for (int yy = 0; yy < DSBY; ++yy)
                                for (int xx = 0; xx < DSBX; ++xx) {
                                    int v = lum(fb->buf + 2 * ((by * DSBY + yy) * W + (bx * DSBX + xx)));
                                    if (v > mx) mx = v;
                                }
                            s_small[by * DSW + bx] = (uint8_t)mx;
                        }
                    emit_small(0, 'L', 0, 0, 0);
                    t_emit = now_ms();
                }
            }
            esp_camera_fb_return(fb);
        }
    }

    /* ---- Phase 2: Multi-Modal Detection Engine ---- */
    cam_auto(false, AEC_PRESETS[aec_idx], 8);
    printf("I,%lu,mode,DIFF_REALTIME,aec=%d,dual_led=A+O\n", (unsigned long)now_ms(), AEC_PRESETS[aec_idx]);

    for (;;) {
        int btn = buttons_poll();
        int input = getchar();

        /* Hardware 5V IR Module Interrupt / Polling */
#ifdef PIN_IR_RX
        static int s_last_ir_hw = 1;
        static uint32_t s_ir_hw_debounce = 0;
        if (PIN_IR_RX >= 0) {
            int ir_lvl = gpio_get_level(PIN_IR_RX);
            if (ir_lvl == 0 && s_last_ir_hw == 1 && (now_ms() - s_ir_hw_debounce > 100)) {
                s_ir_hw_debounce = now_ms();
                printf("I,%lu,IR_HW_TRIGGER,level=0\n", (unsigned long)now_ms());
            }
            s_last_ir_hw = ir_lvl;
        }
#endif

        /* Serial keyboard shortcuts or physical buttons */
        if (input == '1' || input == 'l') {
            mode = OPMODE_LIVE;
            cam_auto(true, 0, 0);
            printf("I,%lu,mode,LIVE\n", (unsigned long)now_ms());
        } else if (input == '2' || input == 'd') {
            mode = OPMODE_DIFF;
            cam_auto(false, AEC_PRESETS[aec_idx], 0);
            printf("I,%lu,mode,DIFF_REALTIME,aec=%d\n", (unsigned long)now_ms(), AEC_PRESETS[aec_idx]);
        } else if (input == '3' || input == 'i') {
            mode = OPMODE_IR_HUNTER;
            cam_auto(false, 900, 7);
            printf("I,%lu,mode,IR_HUNTER (Passive Night-Vision Hunter)\n", (unsigned long)now_ms());
        } else if (input == 'm' || input == ' ' || (btn & 2)) {
            /* Cycle modes: LIVE -> DIFF -> IR_HUNTER */
            mode = (mode + 1) % 3;
            if (mode == OPMODE_LIVE) {
                cam_auto(true, 0, 0);
                printf("I,%lu,mode,LIVE\n", (unsigned long)now_ms());
            } else if (mode == OPMODE_DIFF) {
                cam_auto(false, AEC_PRESETS[aec_idx], 0);
                printf("I,%lu,mode,DIFF_REALTIME\n", (unsigned long)now_ms());
            } else {
                cam_auto(false, 900, 7);
                printf("I,%lu,mode,IR_HUNTER\n", (unsigned long)now_ms());
            }
        }

        if (input == 'a') { use_off_axis = false; printf("I,%lu,led,AXIS\n", (unsigned long)now_ms()); }
        if (input == 'o' || (btn & 4)) {
            use_off_axis = !use_off_axis;
            printf("I,%lu,led,%s\n", (unsigned long)now_ms(), use_off_axis ? "OFF_AXIS" : "AXIS");
        }
        if (input == '+' || (btn & 1)) {
            aec_idx = (aec_idx + 1) % N_AEC;
            if (mode == OPMODE_DIFF) cam_auto(false, AEC_PRESETS[aec_idx], 0);
            printf("I,%lu,aec,%d\n", (unsigned long)now_ms(), AEC_PRESETS[aec_idx]);
        }
        if (input == '-') {
            aec_idx = (aec_idx - 1 + N_AEC) % N_AEC;
            if (mode == OPMODE_DIFF) cam_auto(false, AEC_PRESETS[aec_idx], 0);
            printf("I,%lu,aec,%d\n", (unsigned long)now_ms(), AEC_PRESETS[aec_idx]);
        }
        if (input == 's') {
            printf("I,%lu,status,mode=%d,aec=%d,led=%c\n", (unsigned long)now_ms(),
                   mode, AEC_PRESETS[aec_idx], use_off_axis ? 'O' : 'A');
        }
        if (input == 'h' || input == '?') {
            printf("I,%lu,cmd_help,1=LIVE,2=DIFF,3=IR_HUNTER,m=cycle,+=aec_up,-=aec_down,a=axis,o=off_axis\n", (unsigned long)now_ms());
        }

        /* ----------------------------------------------- MODE 1: LIVE VIEW */
        if (mode == OPMODE_LIVE) {
            gpio_set_level(PIN_LED_AXIS, 0);
            gpio_set_level(PIN_LED_OFF, 0);
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                if (fb->len >= W * H * 2) lcd_draw_be(fb->buf);
                esp_camera_fb_return(fb);
            }
            continue;
        }

        /* ----------------------------------- MODE 3: PASSIVE IR NIGHT-VISION HUNTER */
        if (mode == OPMODE_IR_HUNTER) {
            gpio_set_level(PIN_LED_AXIS, 0);
            gpio_set_level(PIN_LED_OFF, 0);

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) continue;

            double sum = 0, sum2 = 0;
            int peak = 0, px = 0, py = 0;
            for (int i = 0; i < W * H; ++i) {
                uint8_t v = lum(fb->buf + 2 * i);
                y_on[i] = v;
                sum += v;
                sum2 += (double)v * v;
                if (v > peak) { peak = v; px = i % W; py = i / W; }
            }
            esp_camera_fb_return(fb);

            float mean = (float)(sum / (W * H));
            float sigma = sqrtf(fmaxf((float)(sum2 / (W * H)) - mean * mean, 1e-3f));
            float thr = mean + 4.5f * sigma;
            if (thr < 110.0f) thr = 110.0f;

            int hot = 0;
            for (int i = 0; i < W * H; ++i) {
                if (y_on[i] > thr) hot++;
            }

            float contrast = (float)peak - mean;
            float snr = contrast / (sigma > 0.1f ? sigma : 1.0f);

            /* Active IR Night-Vision Emitter criterion:
             * 1) Peak intensity >= 110 DN
             * 2) Contrast >= 35.0 DN above ambient background
             * 3) SNR >= 3.8 standard deviations
             * 4) Compact optical bloom (hot pixel count 1..80 px) */
            bool is_ir = (peak >= 110 && contrast >= 35.0f && snr >= 3.8f && hot >= 1 && hot <= 80);

            for (int i = 0; i < W * H; ++i) {
                uint8_t v = y_on[i];
                uint16_t cc;
                if (is_ir && v > thr) {
                    cc = 0xF81F; /* Bright Magenta / Purple for IR */
                } else {
                    cc = (uint16_t)(((v >> 3) << 11) | ((v >> 2) << 5) | (v >> 3));
                }
                diff_img[2 * i] = cc >> 8;
                diff_img[2 * i + 1] = cc & 0xFF;
            }

            if (is_ir) {
                draw_reticle(diff_img, px, py, 0xF81F); /* Purple targeting reticle */
                printf("I,%lu,IR_ALERT,peak=%d,loc=(%d,%d),snr=%.1f,hot=%d\n", (unsigned long)now_ms(), peak, px, py, snr, hot);
            }

            lcd_draw_be(diff_img);

            if (now_ms() - t_emit > 150) {
                for (int by = 0; by < DSH; ++by)
                    for (int bx = 0; bx < DSW; ++bx) {
                        int mx = 0;
                        for (int yy = 0; yy < DSBY; ++yy)
                            for (int xx = 0; xx < DSBX; ++xx) {
                                int i = (by * DSBY + yy) * W + (bx * DSBX + xx);
                                int v = y_on[i];
                                if (v > mx) mx = v;
                            }
                        s_small[by * DSW + bx] = (uint8_t)mx;
                    }
                emit_small(900, 'I', thr, px, py);
                t_emit = now_ms();
            }

            printf("D,%lu,I,900,%.2f,%.2f,%.1f,%d,%d,%d,%d,%.1f\n",
                   (unsigned long)now_ms(), mean, sigma, thr, hot, peak, px, py, snr);
            continue;
        }

        /* ----------------------------------- MODE 2: ACTIVE RETROREFLECTION DIFF */
        /* Alternating On-Axis vs Off-Axis Strobing:
         * Step 1: Pulse ON-AXIS LED (GPIO 2) -> Capture Retroreflective Frame
         * Step 2: Pulse OFF-AXIS LED (GPIO 14) -> Capture Ambient/Diffuse Frame
         * Diff = max(0, on_axis - off_axis): isolates optical cat-eye retroreflection! */

        /* Step 1: ON-AXIS frame */
        gpio_set_level(PIN_LED_OFF, 0);
        gpio_set_level(PIN_LED_AXIS, 1);
        { camera_fb_t *f = grab_fresh(1); if (f) esp_camera_fb_return(f); }
        camera_fb_t *fb_on = esp_camera_fb_get();
        if (!fb_on) { gpio_set_level(PIN_LED_AXIS, 0); continue; }
        for (int i = 0; i < W * H; ++i) y_on[i] = lum(fb_on->buf + 2 * i);
        esp_camera_fb_return(fb_on);

        /* Step 2: OFF-AXIS frame */
        gpio_set_level(PIN_LED_AXIS, 0);
        gpio_set_level(PIN_LED_OFF, 1);
        { camera_fb_t *f = grab_fresh(1); if (f) esp_camera_fb_return(f); }
        camera_fb_t *fb_off = esp_camera_fb_get();
        gpio_set_level(PIN_LED_OFF, 0);
        if (!fb_off) continue;
        for (int i = 0; i < W * H; ++i) y_off[i] = lum(fb_off->buf + 2 * i);
        esp_camera_fb_return(fb_off);

        /* Step 3: Compute Difference d = max(0, on_axis - off_axis) */
        double sum = 0, sum2 = 0;
        int peak = 0, px = 0, py = 0;
        for (int i = 0; i < W * H; ++i) {
            int d = (int)y_on[i] - (int)y_off[i];
            if (d < 0) d = 0;
            y_on[i] = (uint8_t)d; /* reuse y_on as diff buffer */
            sum += d;
            sum2 += (double)d * d;
            if (d > peak) { peak = d; px = i % W; py = i / W; }
        }

        float mean = (float)(sum / (W * H));
        float sigma = sqrtf(fmaxf((float)(sum2 / (W * H)) - mean * mean, 1e-3f));
        float thr = mean + 3.8f * sigma;
        if (thr < 10.0f) thr = 10.0f;

        /* Count hot pixels */
        int hot = 0;
        for (int i = 0; i < W * H; ++i) {
            if (y_on[i] > thr) hot++;
        }

        float contrast = (float)peak - mean;
        float snr = contrast / (sigma > 0.1f ? sigma : 1.0f);

        /* Optical Cat's Eye Lens Return Criterion:
         * 1) Peak intensity >= 12 DN
         * 2) Contrast above noise floor >= 8.0 DN
         * 3) SNR >= 3.0 standard deviations
         * 4) Optical aperture halo (hot cluster 1..250 pixels) */
        bool is_lens = (peak >= 12 && contrast >= 8.0f && snr >= 3.0f && hot >= 1 && hot <= 250);

        /* Render difference image on display */
        for (int i = 0; i < W * H; ++i) {
            uint16_t cc;
            uint8_t d = y_on[i];
            if (d > thr) {
                cc = is_lens ? 0x07E0 : 0xF800; /* Green if locked, Red if ambient high */
            } else {
                int g = (int)(d * 5); /* amplified greyscale */
                if (g > 255) g = 255;
                cc = (uint16_t)(((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3));
            }
            diff_img[2 * i] = cc >> 8;
            diff_img[2 * i + 1] = cc & 0xFF;
        }

        /* If lens detected, draw green targeting reticle box around it */
        if (is_lens) {
            draw_reticle(diff_img, px, py, 0x07E0); /* Green crosshair/box */
            printf("I,%lu,LENS_LOCKED,peak=%d,loc=(%d,%d),snr=%.1f,hot=%d\n",
                   (unsigned long)now_ms(), peak, px, py, snr, hot);
        }

        lcd_draw_be(diff_img);

        /* Stream downsampled frame to laptop e1_view.py */
        if (now_ms() - t_emit > 200) {
            for (int by = 0; by < DSH; ++by)
                for (int bx = 0; bx < DSW; ++bx) {
                    int mx = 0;
                    for (int yy = 0; yy < DSBY; ++yy)
                        for (int xx = 0; xx < DSBX; ++xx) {
                            int i = (by * DSBY + yy) * W + (bx * DSBX + xx);
                            int d = y_on[i];
                            if (d > mx) mx = d;
                        }
                    s_small[by * DSW + bx] = (uint8_t)mx;
                }
            emit_small(AEC_PRESETS[aec_idx], use_off_axis ? 'O' : 'A', thr, px, py);
            t_emit = now_ms();
        }

        printf("D,%lu,%c,%d,%.2f,%.2f,%.1f,%d,%d,%d,%d,%.1f\n",
               (unsigned long)now_ms(), use_off_axis ? 'O' : 'A', AEC_PRESETS[aec_idx],
               mean, sigma, thr, hot, peak, px, py, (peak - mean) / sigma);
    }
}
