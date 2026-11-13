/* ui_manager.c — SpyScan Tactical UI State Machine & Screen Manager
 *
 * Implements 7 tactical counter-surveillance screens, button navigation,
 * and high-contrast HUD graphics on the 2.4" 240x320 ILI9341 display.
 */

#include "ui_manager.h"
#include <stdio.h>
#include <string.h>

static const char *s_menu_labels[UI_SCREEN_COUNT - 1] = {
    "1. OPTICAL RETROREFLECT",
    "2. PASSIVE IR HUNTER",
    "3. WI-FI PROXIMITY RADAR",
    "4. FLASH STIMULUS PROOF",
    "5. SUB-1GHz BUG HUNTER",
    "6. BLE COVERT SNIFFER",
    "7. SYSTEM DIAGNOSTICS"
};

void ui_manager_init(ui_state_t *ui)
{
    if (!ui) return;
    memset(ui, 0, sizeof(ui_state_t));
    ui->screen = UI_SCREEN_MENU;
    ui->menu_cursor = 0;
    ui->vbat = 3.92f;
    ui->bat_pct = 85;
    ui->camera_online = true;
    
    /* Default optical telemetry */
    ui->optical.aec = 600;
    ui->optical.diff_view = true;
    ui->optical.fps = 3.5f;

    /* Default coprocessor telemetry defaults for standalone preview */
    ui->coproc.coprocessor_online = true;
    ui->coproc.wifi.ch = 10;
    ui->coproc.wifi.mac[0] = 0xCC; ui->coproc.wifi.mac[1] = 0xB8;
    ui->coproc.wifi.mac[2] = 0x5E; ui->coproc.wifi.mac[3] = 0xAD;
    ui->coproc.wifi.mac[4] = 0x5C; ui->coproc.wifi.mac[5] = 0x4A;
    ui->coproc.wifi.rssi = -42;
    ui->coproc.wifi.tx_kbps = 290;
    ui->coproc.wifi.trend_slope = 4;
    ui->coproc.wifi.is_streaming = 1;

    ui->coproc.subghz.freq_khz = 433920;
    ui->coproc.subghz.rssi_inst = -44;
    ui->coproc.subghz.rssi_peak = -42;
    ui->coproc.subghz.duty_pct = 95;
    ui->coproc.subghz.var_x10 = 18;
    ui->coproc.subghz.threat_code = 2; /* Analog bug */
    for (int i = 0; i < 8; ++i) ui->coproc.subghz_history[i] = -75 + i * 4;

    ui->coproc.challenge.slot_idx = 4;
    ui->coproc.challenge.slot_total = 6;
    ui->coproc.challenge.surge_ratio_x100 = 267;
    ui->coproc.challenge.p_val_x1000 = 31;
    ui->coproc.challenge.is_proven = 1;
}

void ui_manager_handle_button(ui_state_t *ui, int btn_mask)
{
    if (!ui || !btn_mask) return;

    if (ui->screen == UI_SCREEN_MENU) {
        if (btn_mask & UI_BTN_UP) {
            ui->menu_cursor = (ui->menu_cursor + 1) % (UI_SCREEN_COUNT - 1);
        } else if (btn_mask & UI_BTN_SEL) {
            ui->screen = (ui_screen_t)(ui->menu_cursor + 1);
        }
    } else {
        /* In any active tool screen, BACK returns to main menu */
        if (btn_mask & UI_BTN_BACK) {
            ui->screen = UI_SCREEN_MENU;
            return;
        }

        /* Screen-specific button actions */
        switch (ui->screen) {
            case UI_SCREEN_OPTICAL:
                if (btn_mask & UI_BTN_UP) {
                    ui->optical.diff_view = !ui->optical.diff_view;
                } else if (btn_mask & UI_BTN_SEL) {
                    static const int aec_steps[] = { 300, 600, 900, 1200 };
                    static int aec_i = 1;
                    aec_i = (aec_i + 1) % 4;
                    ui->optical.aec = aec_steps[aec_i];
                }
                break;
            case UI_SCREEN_IR_HUNTER:
                if (btn_mask & UI_BTN_SEL) {
                    ui->ir_hw_triggered = false;
                }
                break;
            case UI_SCREEN_SUBGHZ:
                if (btn_mask & UI_BTN_UP) {
                    if (ui->coproc.subghz.freq_khz == 433920) ui->coproc.subghz.freq_khz = 868300;
                    else if (ui->coproc.subghz.freq_khz == 868300) ui->coproc.subghz.freq_khz = 315000;
                    else ui->coproc.subghz.freq_khz = 433920;
                }
                break;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------- SCREEN RENDERERS */

static void render_menu(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "SPYSCAN TSCM", ui->vbat, ui->bat_pct, "STANDBY", OSD_COLOR_GREEN);

    osd_printf(fb, 10, 24, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "SELECT INSTRUMENT (1-7):");

    int num_items = UI_SCREEN_COUNT - 1;
    for (int i = 0; i < num_items; ++i) {
        int y = 38 + i * 36;
        bool is_sel = (i == ui->menu_cursor);

        if (is_sel) {
            osd_fill_rect(fb, 8, y, OSD_WIDTH - 16, 28, OSD_COLOR_CYAN);
            osd_draw_rect(fb, 8, y, OSD_WIDTH - 16, 28, OSD_COLOR_WHITE);
            osd_printf(fb, 14, y + 10, OSD_COLOR_BLACK, OSD_TRANSPARENT, 1, "> %s", s_menu_labels[i]);
        } else {
            osd_fill_rect(fb, 8, y, OSD_WIDTH - 16, 28, OSD_COLOR_DARKGRAY);
            osd_draw_rect(fb, 8, y, OSD_WIDTH - 16, 28, OSD_COLOR_MIDGRAY);
            osd_printf(fb, 14, y + 10, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "  %s", s_menu_labels[i]);
        }
    }

    osd_draw_footer(fb, "NEXT", "SELECT", "HOME");
}

static void render_optical(ui_state_t *ui, uint8_t *fb)
{
    /* Background contains live camera/diff image already */
    osd_draw_header(fb, "1. OPTICAL RETRO", ui->vbat, ui->bat_pct,
                    ui->optical.lens_locked ? "LOCKED" : "SCANNING",
                    ui->optical.lens_locked ? OSD_COLOR_GREEN : OSD_COLOR_DARKGRAY);

    if (ui->optical.lens_locked) {
        int cx = ui->optical.target_x > 0 ? ui->optical.target_x : (OSD_WIDTH / 2);
        int cy = ui->optical.target_y > 0 ? ui->optical.target_y : (OSD_HEIGHT / 2);
        osd_draw_reticle(fb, cx, cy, 14, OSD_COLOR_GREEN, true);
        osd_printf(fb, cx + 18, cy - 5, OSD_COLOR_GREEN, OSD_COLOR_BLACK, 1, "LENS SNR:%.1f", ui->optical.snr);
    }

    /* Live Telemetry Overlay */
    osd_fill_rect(fb, 0, OSD_HEIGHT - 32, OSD_WIDTH, 16, OSD_COLOR_DARKGRAY);
    osd_printf(fb, 4, OSD_HEIGHT - 28, OSD_COLOR_YELLOW, OSD_TRANSPARENT, 1,
               "AEC:%-4d %-4s %-6s %.1fFPS",
               ui->optical.aec,
               ui->optical.diff_view ? "DIFF" : "RAW",
               ui->optical.use_off_axis ? "OFF-AX" : "ON-AX",
               ui->optical.fps);

    osd_draw_footer(fb, "DIFF", "AEC", "MENU");
}

static void render_ir_hunter(ui_state_t *ui, uint8_t *fb)
{
    osd_draw_header(fb, "2. PASSIVE IR", ui->vbat, ui->bat_pct, "NIR 940nm", OSD_COLOR_MAGENTA);

    if (ui->optical.lens_locked) {
        int cx = ui->optical.target_x > 0 ? ui->optical.target_x : (OSD_WIDTH / 2);
        int cy = ui->optical.target_y > 0 ? ui->optical.target_y : (OSD_HEIGHT / 2);
        osd_draw_reticle(fb, cx, cy, 16, OSD_COLOR_MAGENTA, true);
        osd_printf(fb, cx + 18, cy - 5, OSD_COLOR_MAGENTA, OSD_COLOR_BLACK, 1, "NIR EMITTER");
    }

    if (ui->ir_hw_triggered) {
        osd_draw_badge(fb, 8, 24, "38kHz BEACON DETECTED", OSD_COLOR_BLACK, OSD_COLOR_MAGENTA);
    }

    osd_fill_rect(fb, 0, OSD_HEIGHT - 32, OSD_WIDTH, 16, OSD_COLOR_DARKGRAY);
    osd_printf(fb, 4, OSD_HEIGHT - 28, OSD_COLOR_MAGENTA, OSD_TRANSPARENT, 1,
               "BAND:850/940nm  HW:%s", ui->ir_hw_triggered ? "ALERT" : "IDLE");

    osd_draw_footer(fb, "SENS", "CLEAR", "MENU");
}

static void render_wifi_radar(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "3. WI-FI RADAR", ui->vbat, ui->bat_pct, "2.4 GHz", OSD_COLOR_CYAN);

    /* Target Box */
    osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 68, OSD_COLOR_DARKGRAY);
    osd_draw_rect(fb, 8, 24, OSD_WIDTH - 16, 68, OSD_COLOR_MIDGRAY);

    osd_printf(fb, 14, 28, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1,
               "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
               ui->coproc.wifi.mac[0], ui->coproc.wifi.mac[1], ui->coproc.wifi.mac[2],
               ui->coproc.wifi.mac[3], ui->coproc.wifi.mac[4], ui->coproc.wifi.mac[5]);

    osd_printf(fb, 14, 42, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1,
               "CH: %-2d | ACTIVE STREAMING", ui->coproc.wifi.ch);

    osd_printf(fb, 14, 56, OSD_COLOR_ORANGE, OSD_TRANSPARENT, 1,
               "UP THROUGHPUT: %d kB/s", ui->coproc.wifi.tx_kbps);

    osd_printf(fb, 14, 70, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1,
               "SIGNAL POWER : %d dBm", ui->coproc.wifi.rssi);

    /* Huge Signal Bar */
    uint16_t bar_color = (ui->coproc.wifi.rssi >= -50) ? OSD_COLOR_RED :
                         (ui->coproc.wifi.rssi >= -65) ? OSD_COLOR_ORANGE : OSD_COLOR_GREEN;
    osd_draw_progress_bar(fb, 8, 100, OSD_WIDTH - 16, 32, (float)ui->coproc.wifi.rssi, -90.0f, -30.0f, bar_color, OSD_COLOR_DARKGRAY);
    osd_printf(fb, 90, 108, OSD_COLOR_WHITE, OSD_TRANSPARENT, 2, "%d dBm", ui->coproc.wifi.rssi);

    /* Direction / Proximity Trend */
    int slope = ui->coproc.wifi.trend_slope;
    if (slope > 1) {
        osd_fill_rect(fb, 8, 142, OSD_WIDTH - 16, 52, OSD_COLOR_RED);
        osd_printf(fb, 36, 154, OSD_COLOR_WHITE, OSD_TRANSPARENT, 2, "^^ WARMER ^^");
        osd_printf(fb, 24, 174, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1, "PROXIMITY: < 35 cm (HOTSPOT)");
    } else if (slope < -1) {
        osd_fill_rect(fb, 8, 142, OSD_WIDTH - 16, 52, OSD_COLOR_CYAN);
        osd_printf(fb, 36, 154, OSD_COLOR_BLACK, OSD_TRANSPARENT, 2, "vv COLDER vv");
        osd_printf(fb, 24, 174, OSD_COLOR_BLACK, OSD_TRANSPARENT, 1, "PROXIMITY: > 2.0 m (FADING)");
    } else {
        osd_fill_rect(fb, 8, 142, OSD_WIDTH - 16, 52, OSD_COLOR_DARKGRAY);
        osd_printf(fb, 36, 154, OSD_COLOR_WHITE, OSD_TRANSPARENT, 2, "== STEADY ==");
        osd_printf(fb, 36, 174, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "DISTANCE: ~80 cm");
    }

    osd_draw_footer(fb, "SCAN", "LOCK", "MENU");
}

static void render_flash_stimulus(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "4. FLASH PROOF", ui->vbat, ui->bat_pct, "PROOF", OSD_COLOR_RED);

    /* Stimulus Prompt Banner */
    if (ui->coproc.challenge.stim_active) {
        osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 44, OSD_COLOR_RED);
        osd_printf(fb, 20, 36, OSD_COLOR_WHITE, OSD_TRANSPARENT, 2, "FLASH LIGHT NOW!");
    } else {
        osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 44, OSD_COLOR_DARKGRAY);
        osd_printf(fb, 20, 36, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 2, "HOLD CAMERA STILL");
    }

    /* Statistical Card */
    osd_fill_rect(fb, 8, 76, OSD_WIDTH - 16, 130, OSD_COLOR_DARKGRAY);
    osd_draw_rect(fb, 8, 76, OSD_WIDTH - 16, 130, OSD_COLOR_MIDGRAY);

    osd_printf(fb, 14, 84, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1,
               "TRIAL : %d OF %d (PERMUTATION)",
               ui->coproc.challenge.slot_idx, ui->coproc.challenge.slot_total);

    osd_printf(fb, 14, 104, OSD_COLOR_ORANGE, OSD_TRANSPARENT, 1,
               "SURGE : +%.2fx POST-FLASH PEAK",
               (float)ui->coproc.challenge.surge_ratio_x100 / 100.0f);

    osd_printf(fb, 14, 124, OSD_COLOR_CYAN, OSD_TRANSPARENT, 1,
               "P-VAL : p = %.4f (p < 0.05)",
               (float)ui->coproc.challenge.p_val_x1000 / 1000.0f);

    osd_printf(fb, 14, 144, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1,
               "DELAY : 50 ms (H.264 I-FRAME)");

    if (ui->coproc.challenge.is_proven) {
        osd_draw_badge(fb, 12, 172, " 100% PROVEN: LENS=WIFI STREAM ", OSD_COLOR_BLACK, OSD_COLOR_GREEN);
    }

    osd_draw_footer(fb, "RESTART", "TRIGGER", "MENU");
}

static void render_subghz(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "5. SUB-1 GHz BUG", ui->vbat, ui->bat_pct, "CC1101", OSD_COLOR_CYAN);

    /* Frequency Card */
    osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 42, OSD_COLOR_DARKGRAY);
    osd_draw_rect(fb, 8, 24, OSD_WIDTH - 16, 42, OSD_COLOR_MIDGRAY);
    osd_printf(fb, 14, 28, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1,
               "FREQ: %.2f MHz (BW 203k)", (float)ui->coproc.subghz.freq_khz / 1000.0f);
    osd_printf(fb, 14, 44, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1,
               "INST:%ddBm PEAK:%ddBm DUTY:%d%%",
               ui->coproc.subghz.rssi_inst, ui->coproc.subghz.rssi_peak,
               ui->coproc.subghz.duty_pct);

    /* Threat Status Banner */
    if (ui->coproc.subghz.threat_code == 2) {
        osd_fill_rect(fb, 8, 72, OSD_WIDTH - 16, 26, OSD_COLOR_RED);
        osd_printf(fb, 14, 80, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1, "! ALERT: CONTINUOUS BUG DETECTED !");
    } else if (ui->coproc.subghz.threat_code == 1) {
        osd_fill_rect(fb, 8, 72, OSD_WIDTH - 16, 26, OSD_COLOR_ORANGE);
        osd_printf(fb, 14, 80, OSD_COLOR_BLACK, OSD_TRANSPARENT, 1, "BURST: KEYFOB / REMOTE DETECTED");
    } else {
        osd_fill_rect(fb, 8, 72, OSD_WIDTH - 16, 26, OSD_COLOR_GREEN);
        osd_printf(fb, 14, 80, OSD_COLOR_BLACK, OSD_TRANSPARENT, 1, "AMBIENT RF FLOOR: CLEAR");
    }

    /* Rolling Mini-Waterfall */
    osd_printf(fb, 8, 106, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "ROLLING RF WATERFALL (LAST 8):");
    for (int i = 0; i < 8; ++i) {
        int wy = 120 + i * 18;
        int r_val = ui->coproc.subghz_history[i];
        uint16_t wcol = (r_val >= -50) ? OSD_COLOR_RED : (r_val >= -70) ? OSD_COLOR_ORANGE : OSD_COLOR_GREEN;
        osd_draw_progress_bar(fb, 8, wy, 150, 14, (float)r_val, -100.0f, -30.0f, wcol, OSD_COLOR_DARKGRAY);
        osd_printf(fb, 164, wy + 3, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "%3d dBm", r_val);
    }

    osd_draw_footer(fb, "BAND", "SWEEP", "MENU");
}

static void render_ble_sniffer(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "6. BLE TRACKERS", ui->vbat, ui->bat_pct, "BLE 2.4G", OSD_COLOR_CYAN);

    osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 220, OSD_COLOR_DARKGRAY);
    osd_draw_rect(fb, 8, 24, OSD_WIDTH - 16, 220, OSD_COLOR_MIDGRAY);

    osd_printf(fb, 14, 30, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1, "SURVEILLANCE BEACONS:");

    osd_printf(fb, 14, 48, OSD_COLOR_RED, OSD_TRANSPARENT, 1, "[!] APPLE AIRTAG / FINDMY");
    osd_printf(fb, 26, 62, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "RSSI: -58 dBm  (~1.2 m)");

    osd_printf(fb, 14, 82, OSD_COLOR_YELLOW, OSD_TRANSPARENT, 1, "[*] SAMSUNG SMARTTAG");
    osd_printf(fb, 26, 96, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "RSSI: -72 dBm  (~3.4 m)");

    osd_printf(fb, 14, 116, OSD_COLOR_ORANGE, OSD_TRANSPARENT, 1, "[*] TUYA SPY CAM BEACON");
    osd_printf(fb, 26, 130, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "RSSI: -45 dBm  (~0.4 m)");

    osd_draw_hline(fb, 14, 154, OSD_WIDTH - 28, OSD_COLOR_MIDGRAY);
    osd_printf(fb, 14, 164, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "CLOSEST THREAT DISTANCE:");
    osd_printf(fb, 14, 180, OSD_COLOR_RED, OSD_TRANSPARENT, 2, "40 cm ALERT!");

    osd_draw_footer(fb, "NEXT", "BEEP", "MENU");
}

static void render_diagnostics(ui_state_t *ui, uint8_t *fb)
{
    osd_fill_rect(fb, 0, 0, OSD_WIDTH, OSD_HEIGHT, OSD_COLOR_NAVY);
    osd_draw_header(fb, "7. DIAGNOSTICS", ui->vbat, ui->bat_pct, "OK", OSD_COLOR_GREEN);

    osd_fill_rect(fb, 8, 24, OSD_WIDTH - 16, 240, OSD_COLOR_DARKGRAY);
    osd_draw_rect(fb, 8, 24, OSD_WIDTH - 16, 240, OSD_COLOR_MIDGRAY);

    osd_printf(fb, 14, 32, OSD_COLOR_WHITE, OSD_TRANSPARENT, 1, "HARDWARE TELEMETRY:");
    osd_printf(fb, 14, 52, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "FW: SPYSCAN V1.0-RELEASE");
    osd_printf(fb, 14, 72, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "VBAT: %.2f V (%d%%)", ui->vbat, ui->bat_pct);
    osd_printf(fb, 14, 92, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "CAM: OV3660 (QVGA RGB)");
    osd_printf(fb, 14, 112, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "FPS: %.1f FPS", ui->optical.fps);
    osd_printf(fb, 14, 132, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "COPROC: %s", ui->coproc.coprocessor_online ? "ONLINE (UART 921k)" : "STANDALONE");
    osd_printf(fb, 14, 152, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "PSRAM: 8192 KB OCTAL");
    osd_printf(fb, 14, 172, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "IR RX: GPIO 3 (TSOP)");
    osd_printf(fb, 14, 192, OSD_COLOR_LIGHTGRAY, OSD_TRANSPARENT, 1, "LCD: 240x320 PORTRAIT");

    osd_draw_footer(fb, "CAL", "TEST", "MENU");
}

void ui_manager_render(ui_state_t *ui, uint8_t *fb)
{
    if (!ui || !fb) return;

    switch (ui->screen) {
        case UI_SCREEN_MENU:
            render_menu(ui, fb);
            break;
        case UI_SCREEN_OPTICAL:
            render_optical(ui, fb);
            break;
        case UI_SCREEN_IR_HUNTER:
            render_ir_hunter(ui, fb);
            break;
        case UI_SCREEN_WIFI_RADAR:
            render_wifi_radar(ui, fb);
            break;
        case UI_SCREEN_FLASH_STIMULUS:
            render_flash_stimulus(ui, fb);
            break;
        case UI_SCREEN_SUBGHZ:
            render_subghz(ui, fb);
            break;
        case UI_SCREEN_BLE_SNIFFER:
            render_ble_sniffer(ui, fb);
            break;
        case UI_SCREEN_DIAGNOSTICS:
            render_diagnostics(ui, fb);
            break;
        default:
            render_menu(ui, fb);
            break;
    }
}
