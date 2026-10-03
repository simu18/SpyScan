/* SpyScan V1 — Wokwi HMI simulation (ESP32-S3 + ILI9341 + 3 buttons + LEDs)
 *
 * REAL code (reused on the device):  ui_fsm.c, challenge.c, board_pins.h
 * SIMULATED here:                    camera frames and Wi-Fi monitor data
 *                                    (sim_sources.c, fake optical candidates)
 *
 * Wokwi parts:
 *   pot "Battery"  (GPIO1)  -> simulated cell voltage 3.0 .. 4.2 V
 *   pot "Distance" (GPIO10) -> distance to the hidden camera 0.3 .. 8 m
 *   red LED "AXIS" (GPIO2)  -> on-axis ring timing (slowed: 4 fps instead of 25)
 *   red LED "OFF"  (GPIO14) -> off-axis pair (lights during the off-axis test)
 */
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "board_pins.h"
#include "ui_fsm.h"
#include "challenge.h"
#include "sim_sources.h"

/* ------------------------------------------------------------------ */
static Adafruit_ILI9341 tft(&SPI, PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_RST);
static ui_t ui;

#define W 320
#define H 240
#define C_BG     ILI9341_BLACK
#define C_FG     ILI9341_WHITE
#define C_DIM    0x7BEF
#define C_ACC    0x07FF   /* cyan */
#define C_WARN   ILI9341_ORANGE
#define C_BAD    ILI9341_RED
#define C_OK     ILI9341_GREEN

/* ---- timing ---- */
static const uint32_t SIM_FRAME_MS  = 250;   /* 4 fps (real: ~40 ms) */
static const uint8_t  K_SLOTS       = 12;    /* challenge slots K    */
static const uint32_t SLOT_MS       = 2000;  /* T (shortened for the sim; E2 decides) */
static const uint32_t TRAFFIC_TICK  = 100;   /* traffic sampling, ms */
static const float    V_CUT         = 3.40f; /* low-battery cut-off  */

/* ---- buttons with debounce ---- */
struct Btn { uint8_t pin; bool stable; bool last; uint32_t t; ui_event_t ev; };
static Btn btns[3] = {
    { PIN_BTN_UP,   true, true, 0, EV_UP   },
    { PIN_BTN_SEL,  true, true, 0, EV_SEL  },
    { PIN_BTN_BACK, true, true, 0, EV_BACK },
};

/* ---- optical (fake scene) ---- */
struct Cand { int16_t x, y; uint8_t kind; };   /* kind: 0 lens, 1 glint, 2 lamp */
static const Cand SCENE[] = {
    { 205, 118, 0 },   /* hidden camera lens (retroreflector)     */
    {  90,  80, 1 },   /* screw head glint (specular)             */
    { 250,  60, 2 },   /* lamp / indicator LED (self-luminous)    */
};
static uint32_t frame_no = 0;
static uint32_t offaxis_t0 = 0;

/* ---- wireless ---- */
static challenge_t chal;
static ch_result_t results[CH_MAX_DEV];
static float up_ema[CH_MAX_DEV], dl_ema[CH_MAX_DEV], rssi_ema[CH_MAX_DEV];
static uint32_t chal_t0 = 0;
static int last_slot = -1;
static bool challenge_running = false;
static float locate_prev = -100;

/* ---- history ---- */
struct HistItem { char text[40]; };
static HistItem hist[6];
static uint8_t hist_n = 0;
static void hist_add(const char *s) {
    if (hist_n == 6) { memmove(&hist[0], &hist[1], sizeof(HistItem) * 5); hist_n = 5; }
    snprintf(hist[hist_n++].text, sizeof(hist[0].text), "%6lus %s", (unsigned long)(millis() / 1000), s);
}

/* ---- battery ---- */
static float vbat = 4.0f;
static float read_vbat_sim() {
    /* SIM: pot 0..3.3 V mapped to 3.0..4.2 V cell voltage.
       Device: v = adc_mV * 2 (100k/100k divider) with ADC calibration. */
    int raw = analogRead(PIN_VBAT_SENSE);
    return 3.0f + 1.2f * raw / 4095.0f;
}

/* ================================================================== */
/*                              DRAWING                               */
/* ================================================================== */
static bool need_full = true;

static void header(const char *title) {
    tft.fillRect(0, 0, W, 22, 0x18C3);
    tft.setTextColor(C_FG); tft.setTextSize(2);
    tft.setCursor(6, 3); tft.print(title);
    int pct = (int)((vbat - 3.3f) / 0.9f * 100); pct = constrain(pct, 0, 100);
    tft.setTextSize(1); tft.setCursor(W - 60, 8);
    tft.setTextColor(vbat < 3.5f ? C_WARN : C_FG);
    tft.printf("%.2fV %d%%", vbat, pct);
}
static void footer(const char *s) {
    tft.fillRect(0, H - 14, W, 14, 0x18C3);
    tft.setTextColor(C_DIM); tft.setTextSize(1);
    tft.setCursor(4, H - 11); tft.print(s);
}
static uint16_t lvl_color(ch_level_t l) {
    return l == CH_LEVEL_HIGH ? C_BAD : l == CH_LEVEL_LOW ? C_WARN : C_OK;
}
static const char *lvl_name(ch_level_t l) {
    return l == CH_LEVEL_HIGH ? "HIGH" : l == CH_LEVEL_LOW ? "LOW" : "NONE";
}

static void draw_menu() {
    static const char *items[MENU_N] = { "Optical scan", "Wireless scan", "History", "Diagnostics" };
    tft.fillScreen(C_BG); header("SpyScan");
    for (int i = 0; i < MENU_N; ++i) {
        int y = 40 + i * 36;
        bool sel = (i == ui.menu_idx);
        tft.fillRoundRect(20, y, W - 40, 30, 6, sel ? C_ACC : 0x2104);
        tft.setTextColor(sel ? C_BG : C_FG); tft.setTextSize(2);
        tft.setCursor(34, y + 8); tft.print(items[i]);
    }
    if (ui.err) { tft.setTextColor(C_BAD); tft.setTextSize(1); tft.setCursor(20, 190); tft.print(ui.err); }
    footer("UP: next   SEL: enter   BACK: -");
}

/* Optical: fake preview with candidate markers. */
static void draw_optical(bool full) {
    if (full) { tft.fillScreen(C_BG); header(ui.overlay_diff ? "Optical: ON-OFF" : "Optical: RAW"); }
    /* preview area 0..W, 24..H-16 */
    const int py = 24, ph = H - 40;
    bool led_on = ((frame_no / 2) % 2) == 0;          /* 4-frame cycle: ON,ON,OFF,OFF */
    if (full || !ui.overlay_diff) {
        tft.fillRect(0, py, W, ph, ui.overlay_diff ? C_BG : 0x31A6);
        if (!ui.overlay_diff) {                       /* raw scene: furniture blocks */
            tft.fillRect(170, 95, 80, 50, 0x52AA);    /* "clock" hiding the camera  */
            tft.fillRect(60, 70, 60, 40, 0x4208);     /* "shelf"                    */
            tft.fillRect(235, 45, 30, 30, 0x6B4D);    /* "lamp"                     */
        }
    }
    int shown = 0;
    for (const Cand &c : SCENE) {
        bool visible;
        uint16_t col;
        if (!ui.overlay_diff) {           /* RAW: everything bright is visible, lamp always */
            visible = (c.kind == 2) || led_on;
            col = (c.kind == 2) ? ILI9341_YELLOW : C_FG;
        } else {                          /* DIFF: lamp cancels; lens + glint remain */
            visible = (c.kind != 2);
            col = (c.kind == 0) ? C_BAD : C_WARN;
        }
        if (ui.overlay_diff && full == false) tft.fillCircle(c.x, c.y, 4, C_BG);
        if (visible) {
            tft.fillCircle(c.x, c.y, 3, col);
            if (ui.overlay_diff) { tft.drawRect(c.x - 9, c.y - 9, 19, 19, col); shown++; }
        } else if (!ui.overlay_diff) {
            tft.fillCircle(c.x, c.y, 3, (c.y > 90 && c.x > 170) ? 0x52AA : 0x4208);
        }
    }
    /* centre crosshair */
    tft.drawFastHLine(W / 2 - 10, py + ph / 2, 20, C_DIM);
    tft.drawFastVLine(W / 2, py + ph / 2 - 10, 20, C_DIM);
    tft.fillRect(0, H - 30, W, 14, C_BG);
    tft.setTextSize(1); tft.setTextColor(C_FG); tft.setCursor(4, H - 27);
    if (ui.overlay_diff) tft.printf("Candidates: %d  (lamp suppressed by ON-OFF)  LED %s", shown, led_on ? "ON " : "OFF");
    else                 tft.printf("RAW view: lamp + glints look alike     LED %s", led_on ? "ON " : "OFF");
    if (full) footer("UP: raw/diff  SEL: off-axis test  BACK: menu");
}

static void draw_offaxis(bool full) {
    if (full) { tft.fillScreen(C_BG); header("Off-axis test"); footer("BACK: cancel"); }
    uint32_t dt = millis() - offaxis_t0;
    tft.setTextSize(2); tft.setTextColor(C_FG, C_BG);
    tft.setCursor(10, 40); tft.print("Measuring...");
    tft.setTextSize(1);
    tft.setCursor(10, 80);  tft.print("Candidate      on-axis  off-axis  ratio  class");
    /* Illustrative values for the sim; real ratios come from E3. */
    tft.setCursor(10, 100); tft.setTextColor(C_BAD, C_BG);  tft.print("#1 (205,118)     high      ~0      >10  RETROREFLECTOR-like");
    tft.setCursor(10, 116); tft.setTextColor(C_WARN, C_BG); tft.print("#2 ( 90, 80)     high     high     ~1   specular glint");
    int w = (int)min<uint32_t>(dt * (W - 20) / 1500, W - 20);
    tft.fillRect(10, 150, w, 10, C_ACC);
}

static void draw_survey(bool full) {
    if (full) { tft.fillScreen(C_BG); header("Wireless: survey"); footer("UP: select  SEL: challenge  BACK: menu"); }
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_BG); tft.setCursor(4, 28);
    tft.print("   MAC(last3) vendor     role ch  RSSI  up kB/s  dn kB/s");
    for (int i = 0; i < ui.n_dev; ++i) {
        const sim_dev_t *d = sim_dev(i);
        int y = 44 + i * 16;
        bool sel = (i == ui.dev_sel);
        bool streaming = up_ema[i] > 20000;   /* placeholder criterion (FR-W4, set in E2) */
        tft.fillRect(0, y - 2, W, 14, sel ? 0x2945 : C_BG);
        tft.setTextColor(streaming ? C_WARN : C_FG, sel ? 0x2945 : C_BG);
        tft.setCursor(4, y);
        tft.printf("%c %02X:%02X:%02X  %-10s %-4s %2d  %4.0f  %7.1f  %7.1f",
                   sel ? '>' : ' ', d->mac[3], d->mac[4], d->mac[5], d->vendor,
                   d->is_ap ? "AP" : "STA", d->ch, rssi_ema[i], up_ema[i] / 1000, dl_ema[i] / 1000);
    }
    tft.setTextColor(C_WARN, C_BG); tft.setCursor(4, 200);
    tft.print("orange = sustained uplink (streaming candidate)");
}

static void draw_challenge(bool full) {
    if (full) { tft.fillScreen(C_BG); header("Challenge"); footer("BACK: abort"); }
    uint32_t t = millis() - chal_t0;
    int k = challenge_slot_at(&chal, t);
    if (k < 0) return;
    bool move = chal.stim[k];
    tft.fillRect(20, 50, W - 40, 90, move ? C_BAD : 0x03EF);
    tft.setTextColor(C_FG); tft.setTextSize(4);
    tft.setCursor(move ? 110 : 40, 80); tft.print(move ? "MOVE" : "STAY STILL");
    tft.setTextSize(1); tft.setTextColor(C_FG, C_BG);
    tft.setCursor(20, 150);
    tft.printf("slot %2d/%d   %4.1f s left in slot   ", k + 1, K_SLOTS,
               (SLOT_MS - (t % SLOT_MS)) / 1000.0f);
    /* sequence bar */
    for (int i = 0; i < K_SLOTS; ++i) {
        uint16_t c = (i < k) ? (chal.stim[i] ? C_BAD : 0x03EF) : (i == k ? C_FG : 0x2104);
        tft.fillRect(20 + i * 23, 170, 20, 12, c);
    }
}

static void draw_result(bool full) {
    if (full) { tft.fillScreen(C_BG); header("Challenge result"); footer("UP: select  SEL: locate  BACK: survey"); }
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_BG); tft.setCursor(4, 28);
    tft.print("   device        vendor     corr    p      level  role");
    for (int i = 0; i < ui.n_dev; ++i) {
        const sim_dev_t *d = sim_dev(i);
        const ch_result_t &r = results[i];
        int y = 44 + i * 16;
        bool sel = (i == ui.dev_sel);
        /* Direction feature: a correlated source is uplink-dominant;
           a correlated viewer/relay is downlink-dominant (hypothesis H4). */
        const char *role = "-";
        if (r.level != CH_LEVEL_NONE) role = (up_ema[i] > dl_ema[i]) ? "SOURCE" : "viewer";
        tft.fillRect(0, y - 2, W, 14, sel ? 0x2945 : C_BG);
        tft.setTextColor(lvl_color(r.level), sel ? 0x2945 : C_BG);
        tft.setCursor(4, y);
        tft.printf("%c %-12s %-10s %+5.2f  %5.3f  %-5s  %s", sel ? '>' : ' ',
                   d->name, d->vendor, r.stat, r.p, lvl_name(r.level), role);
    }
    tft.setTextColor(C_DIM, C_BG); tft.setCursor(4, 200);
    tft.print("HIGH: p<0.01  LOW: p<0.05  (permutation test, 1000 perms)");
}

static void draw_locate(bool full) {
    const sim_dev_t *d = sim_dev(ui.dev_sel);
    if (full) {
        tft.fillScreen(C_BG); header("Locate");
        tft.setTextSize(1); tft.setTextColor(C_FG); tft.setCursor(10, 30);
        tft.printf("%s  %02X:%02X:%02X:%02X:%02X:%02X  ch %d", d->name,
                   d->mac[0], d->mac[1], d->mac[2], d->mac[3], d->mac[4], d->mac[5], d->ch);
        footer("turn the DISTANCE pot to walk   BACK: result");
    }
    float r = rssi_ema[ui.dev_sel];
    int bar = constrain((int)((r + 90) / 60.0f * (W - 40)), 0, W - 40);
    tft.fillRect(20, 70, W - 40, 30, 0x2104);
    tft.fillRect(20, 70, bar, 30, r > -50 ? C_BAD : r > -65 ? C_WARN : C_OK);
    tft.setTextSize(3); tft.setTextColor(C_FG, C_BG);
    tft.setCursor(20, 120); tft.printf("%4.0f dBm ", r);
    tft.setTextSize(2); tft.setCursor(20, 160);
    float d_r = r - locate_prev;
    tft.setTextColor(d_r > 0.5f ? C_BAD : d_r < -0.5f ? C_ACC : C_FG, C_BG);
    tft.print(d_r > 0.5f ? "WARMER  ^   " : d_r < -0.5f ? "COLDER  v   " : "steady  =   ");
}

static void draw_history() {
    tft.fillScreen(C_BG); header("History");
    tft.setTextSize(1); tft.setTextColor(C_FG);
    if (!hist_n) { tft.setCursor(10, 40); tft.print("No results in this session."); }
    for (int i = 0; i < hist_n; ++i) { tft.setCursor(10, 40 + i * 16); tft.print(hist[i].text); }
    footer("BACK: menu");
}

static void draw_diag() {
    tft.fillScreen(C_BG); header("Diagnostics");
    tft.setTextSize(1); tft.setTextColor(C_FG);
    int y = 40;
    tft.setCursor(10, y); tft.printf("Firmware      : sim-0.1 (%s)", __DATE__); y += 16;
    tft.setCursor(10, y); tft.printf("Battery       : %.2f V", vbat); y += 16;
    tft.setCursor(10, y); tft.printf("Camera        : %s", ui.cam_ok ? "OK (simulated)" : "FAIL"); y += 16;
    tft.setCursor(10, y); tft.printf("Wi-Fi monitor : %s", ui.wifi_ok ? "OK (simulated)" : "FAIL"); y += 16;
    tft.setCursor(10, y); tft.printf("Free heap     : %u B", (unsigned)ESP.getFreeHeap()); y += 16;
    tft.setCursor(10, y); tft.printf("Uptime        : %lu s", (unsigned long)(millis() / 1000));
    footer("BACK: menu");
}

static void draw_lowbat() {
    tft.fillScreen(C_BAD);
    tft.setTextColor(C_FG); tft.setTextSize(3);
    tft.setCursor(30, 90); tft.print("LOW BATTERY");
    tft.setTextSize(1); tft.setCursor(30, 130);
    tft.printf("%.2f V < %.2f V - scanning stopped", vbat, V_CUT);
}

static void redraw(bool full) {
    switch (ui.st) {
    case ST_MENU:        if (full) draw_menu(); break;
    case ST_OPT_LIVE:    draw_optical(full); break;
    case ST_OPT_OFFAXIS: draw_offaxis(full); break;
    case ST_W_SURVEY:    draw_survey(full); break;
    case ST_W_CHALLENGE: draw_challenge(full); break;
    case ST_W_RESULT:    if (full) draw_result(true); break;
    case ST_W_LOCATE:    draw_locate(full); break;
    case ST_HISTORY:     if (full) draw_history(); break;
    case ST_DIAG:        if (full) draw_diag(); break;
    case ST_LOWBAT:      if (full) draw_lowbat(); break;
    default: break;
    }
}

/* ================================================================== */
/*                     ACTIONS (application layer)                    */
/* ================================================================== */
static void leds_off() { digitalWrite(PIN_LED_AXIS, LOW); digitalWrite(PIN_LED_OFF, LOW); }

static void do_action(ui_action_t a) {
    switch (a) {
    case ACT_START_OPTICAL:  frame_no = 0; break;
    case ACT_STOP_OPTICAL:   leds_off(); hist_add("Optical: 2 candidates (1 retro-like)"); break;
    case ACT_START_OFFAXIS:  offaxis_t0 = millis(); digitalWrite(PIN_LED_AXIS, LOW); break;
    case ACT_START_SURVEY:   ui.n_dev = (uint8_t)sim_n_dev(); break;
    case ACT_STOP_WIRELESS:  break;
    case ACT_START_CHALLENGE:
        challenge_init(&chal, K_SLOTS, SLOT_MS, ui.n_dev, millis() | 1u);
        chal_t0 = millis(); last_slot = -1; challenge_running = true;
        break;
    case ACT_ABORT_CHALLENGE: challenge_running = false; noTone(PIN_BUZZER); break;
    case ACT_START_LOCATE:   locate_prev = rssi_ema[ui.dev_sel]; break;
    case ACT_STOP_ALL:       leds_off(); challenge_running = false; noTone(PIN_BUZZER); break;
    default: break;
    }
}

static void post(ui_event_t ev) {
    ui_state_t before = ui.st;
    ui_action_t a = ui_handle(&ui, ev);
    do_action(a);
    Serial.printf("[ui] %-12s --%d--> %-12s act=%d\n", ui_state_name(before), (int)ev,
                  ui_state_name(ui.st), (int)a);
    need_full = true;   /* redraw after any handled event (cheap enough for the sim) */
}

/* ================================================================== */
void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED_AXIS, OUTPUT); pinMode(PIN_LED_OFF, OUTPUT); leds_off();
    for (auto &b : btns) pinMode(b.pin, INPUT_PULLUP);
    analogReadResolution(12);

    SPI.begin(PIN_LCD_SCLK, PIN_LCD_MISO, PIN_LCD_MOSI, PIN_LCD_CS);
    tft.begin(40000000);
    tft.setRotation(1);   /* landscape 320x240 */

    tft.fillScreen(C_BG);
    tft.setTextColor(C_ACC); tft.setTextSize(3); tft.setCursor(70, 90); tft.print("SpyScan");
    tft.setTextSize(1); tft.setTextColor(C_DIM); tft.setCursor(70, 125);
    tft.print("V1 HMI simulation - booting...");

    sim_init(12345);
    for (int i = 0; i < CH_MAX_DEV; ++i) { up_ema[i] = dl_ema[i] = 0; rssi_ema[i] = -90; }

    ui_init(&ui, /*cam_ok=*/true, /*wifi_ok=*/true);
    delay(800);
    post(EV_INIT_DONE);
}

void loop() {
    const uint32_t now = millis();

    /* ---- buttons: 20 ms debounce ---- */
    for (auto &b : btns) {
        bool lvl = digitalRead(b.pin);
        if (lvl != b.last) { b.last = lvl; b.t = now; }
        if (now - b.t > 20 && lvl != b.stable) {
            b.stable = lvl;
            if (!lvl) post(b.ev);           /* pressed (active low) */
        }
    }

    /* ---- battery monitor, 4 Hz, with hysteresis ---- */
    static uint32_t t_bat = 0;
    if (now - t_bat > 250) {
        t_bat = now;
        vbat = read_vbat_sim();
        /* Debounce: require 8 consecutive samples (2 s) below the cut-off so a
           load transient (Wi-Fi TX burst, LED pulse) cannot trigger LOWBAT. */
        static uint8_t low_cnt = 0;
        low_cnt = (vbat < V_CUT) ? (uint8_t)min(low_cnt + 1, 255) : 0;
        if (low_cnt >= 8 && ui.st != ST_LOWBAT && ui.st != ST_BOOT) post(EV_LOWBAT);
        else if (ui.st == ST_LOWBAT && vbat > V_CUT + 0.1f) post(EV_BAT_OK);   /* 0.1 V hysteresis */
    }

    /* ---- optical: frame clock + LED sync (4-frame cycle) ---- */
    static uint32_t t_frame = 0;
    if (ui.st == ST_OPT_LIVE && now - t_frame >= SIM_FRAME_MS) {
        t_frame = now; frame_no++;
        bool led_on = ((frame_no / 2) % 2) == 0;
        digitalWrite(PIN_LED_AXIS, led_on);
        if (!need_full) redraw(false);
    }
    if (ui.st == ST_OPT_OFFAXIS) {
        digitalWrite(PIN_LED_OFF, ((now - offaxis_t0) / SIM_FRAME_MS) % 2 == 0);
        static uint32_t t_oa = 0;
        if (now - t_oa > 100) { t_oa = now; if (!need_full) redraw(false); }
        if (now - offaxis_t0 > 1500) { digitalWrite(PIN_LED_OFF, LOW); post(EV_OFFAXIS_DONE); }
    }

    /* ---- wireless: synthetic traffic, 100 ms ticks ---- */
    static uint32_t t_trf = 0;
    bool wireless = (ui.st == ST_W_SURVEY || ui.st == ST_W_CHALLENGE ||
                     ui.st == ST_W_RESULT || ui.st == ST_W_LOCATE);
    if (wireless && now - t_trf >= TRAFFIC_TICK) {
        t_trf = now;
        sim_set_camera_distance(0.3f + 7.7f * analogRead(PIN_SIM_DISTANCE) / 4095.0f);
        int stim = 0;
        uint32_t tc = now - chal_t0;
        if (challenge_running) {
            int k = challenge_slot_at(&chal, tc > 300 ? tc - 300 : 0);  /* 300 ms lag */
            stim = (k >= 0) ? chal.stim[k] : 0;
        }
        for (int i = 0; i < ui.n_dev; ++i) {
            uint32_t b = sim_uplink_bytes(i, TRAFFIC_TICK, stim);
            float dl = sim_dev(i)->dl_Bps * (stim && i == 2 ? sim_dev(2)->gain : 1.0f);
            up_ema[i]   = 0.9f * up_ema[i] + 0.1f * (b * 1000.0f / TRAFFIC_TICK);
            dl_ema[i]   = 0.9f * dl_ema[i] + 0.1f * dl;
            rssi_ema[i] = 0.8f * rssi_ema[i] + 0.2f * sim_rssi(i);
            if (challenge_running) challenge_add_bytes(&chal, (uint8_t)i, tc, b);
        }
        if (challenge_running) {
            int k = challenge_slot_at(&chal, tc);
            if (k != last_slot && k >= 0) { last_slot = k; tone(PIN_BUZZER, chal.stim[k] ? 2000 : 1000, 80); }
            if (k < 0) {
                challenge_running = false;
                int best = -1;
                for (int i = 0; i < ui.n_dev; ++i) {
                    results[i] = challenge_evaluate(&chal, (uint8_t)i);
                    Serial.printf("[chal] dev %d %-11s S=%+.3f p=%.4f level=%d\n", i,
                                  sim_dev(i)->name, results[i].stat, results[i].p, results[i].level);
                    if (results[i].level == CH_LEVEL_HIGH && up_ema[i] > dl_ema[i]) best = i;
                }
                char msg[40];
                if (best >= 0) snprintf(msg, sizeof msg, "Wi-Fi: camera-like %02X:%02X:%02X",
                                        sim_dev(best)->mac[3], sim_dev(best)->mac[4], sim_dev(best)->mac[5]);
                else           snprintf(msg, sizeof msg, "Wi-Fi: no camera-like source");
                hist_add(msg);
                post(EV_CHALLENGE_DONE);
            }
        }
        static uint32_t t_ui = 0;
        if (now - t_ui > 500 && !need_full && ui.st != ST_W_RESULT) {
            t_ui = now;
            if (ui.st == ST_W_LOCATE) { redraw(false); locate_prev = 0.7f * locate_prev + 0.3f * rssi_ema[ui.dev_sel]; }
            else redraw(false);
        }
    }

    if (need_full) { need_full = false; redraw(true); }
    delay(5);
}
