/* ui_manager.h — SpyScan Tactical UI State Machine & Screen Manager
 *
 * Coordinates 7 dedicated counter-surveillance screens, tactile button navigation
 * (UP / SELECT / BACK), optical frame overlays, and RF coprocessor telemetry.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "osd_draw.h"
#include "spyscan_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_SCREEN_MENU = 0,
    UI_SCREEN_OPTICAL,          /* Method 1: Cat's Eye Retroreflection */
    UI_SCREEN_IR_HUNTER,        /* Method 2: Passive NIR Night-Vision */
    UI_SCREEN_WIFI_RADAR,       /* Method 4: Wi-Fi Proximity Radar */
    UI_SCREEN_FLASH_STIMULUS,   /* Method 1.1: Flash Provocation Challenge */
    UI_SCREEN_SUBGHZ,           /* Method 6: Sub-1 GHz RF Bug Hunter */
    UI_SCREEN_BLE_SNIFFER,      /* Method 5: BLE Covert Bug Hunter */
    UI_SCREEN_DIAGNOSTICS,      /* System Telemetry & Battery */
    UI_SCREEN_COUNT
} ui_screen_t;

typedef enum {
    UI_BTN_NONE = 0,
    UI_BTN_UP   = 1,
    UI_BTN_SEL  = 2,
    UI_BTN_BACK = 4,
} ui_button_event_t;

/* Optical Pipeline Telemetry */
typedef struct {
    int   aec;
    bool  diff_view;
    bool  use_off_axis;
    bool  lens_locked;
    int   target_x;
    int   target_y;
    int   peak_val;
    float snr;
    int   hot_px;
    float fps;
} ui_optical_telemetry_t;

/* Global UI Master State */
typedef struct {
    ui_screen_t screen;
    int         menu_cursor;
    
    /* Hardware Telemetry */
    float       vbat;
    int         bat_pct;
    bool        camera_online;
    bool        ir_hw_triggered;
    uint32_t    ir_hw_trigger_time;
    
    /* Subsystems */
    ui_optical_telemetry_t      optical;
    spyscan_coprocessor_state_t coproc;
} ui_state_t;

/* Public API */
void ui_manager_init(ui_state_t *ui);
void ui_manager_handle_button(ui_state_t *ui, int btn_mask);
void ui_manager_render(ui_state_t *ui, uint8_t *fb);

#ifdef __cplusplus
}
#endif

