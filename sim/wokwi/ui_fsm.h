/* ui_fsm.h — SpyScan UI state machine (pure C, hardware-independent).
 * Same code runs in the Wokwi simulation and on the device.
 * Input: events. Output: next state + an action for the application layer.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ST_BOOT = 0, ST_MENU,
    ST_OPT_LIVE, ST_OPT_OFFAXIS,
    ST_W_SURVEY, ST_W_CHALLENGE, ST_W_RESULT, ST_W_LOCATE,
    ST_HISTORY, ST_DIAG, ST_LOWBAT,
    ST__COUNT
} ui_state_t;

typedef enum {
    EV_NONE = 0, EV_UP, EV_SEL, EV_BACK,
    EV_INIT_DONE, EV_OFFAXIS_DONE, EV_CHALLENGE_DONE,
    EV_LOWBAT, EV_BAT_OK
} ui_event_t;

typedef enum {
    ACT_NONE = 0,
    ACT_START_OPTICAL, ACT_STOP_OPTICAL, ACT_START_OFFAXIS,
    ACT_START_SURVEY, ACT_STOP_WIRELESS,
    ACT_START_CHALLENGE, ACT_ABORT_CHALLENGE,
    ACT_START_LOCATE, ACT_STOP_LOCATE,
    ACT_STOP_ALL, ACT_SHOW_ERROR
} ui_action_t;

enum { MENU_OPTICAL = 0, MENU_WIRELESS, MENU_HISTORY, MENU_DIAG, MENU_N };

typedef struct {
    ui_state_t st;
    ui_state_t prev;        /* state before LOWBAT */
    uint8_t menu_idx;
    uint8_t dev_sel;        /* selected device in survey/result */
    uint8_t n_dev;          /* number of devices in the table   */
    bool    cam_ok;
    bool    wifi_ok;
    bool    overlay_diff;   /* optical view: true = ON-OFF diff, false = raw */
    const char *err;        /* last error message (static string) */
} ui_t;

void        ui_init(ui_t *u, bool cam_ok, bool wifi_ok);
ui_action_t ui_handle(ui_t *u, ui_event_t ev);
const char *ui_state_name(ui_state_t s);

#ifdef __cplusplus
}
#endif
