/* ui_fsm.c — transitions follow docs/architecture/diagrams/06_ui_fsm.puml */
#include "ui_fsm.h"
#include <stddef.h>

static const char *NAMES[ST__COUNT] = {
    "BOOT", "MENU", "OPT_LIVE", "OPT_OFFAXIS", "W_SURVEY", "W_CHALLENGE",
    "W_RESULT", "W_LOCATE", "HISTORY", "DIAG", "LOWBAT"
};

const char *ui_state_name(ui_state_t s)
{
    return (s < ST__COUNT) ? NAMES[s] : "?";
}

void ui_init(ui_t *u, bool cam_ok, bool wifi_ok)
{
    u->st = ST_BOOT; u->prev = ST_BOOT;
    u->menu_idx = 0; u->dev_sel = 0; u->n_dev = 0;
    u->cam_ok = cam_ok; u->wifi_ok = wifi_ok;
    u->overlay_diff = true; u->err = NULL;
}

static ui_action_t go(ui_t *u, ui_state_t s, ui_action_t a)
{
    u->st = s;
    return a;
}

ui_action_t ui_handle(ui_t *u, ui_event_t ev)
{
    /* Global: low battery pre-empts every state except BOOT/LOWBAT. */
    if (ev == EV_LOWBAT && u->st != ST_LOWBAT && u->st != ST_BOOT) {
        u->prev = u->st;
        return go(u, ST_LOWBAT, ACT_STOP_ALL);
    }

    switch (u->st) {
    case ST_BOOT:
        if (ev == EV_INIT_DONE) return go(u, ST_MENU, ACT_NONE);
        break;

    case ST_MENU:
        if (ev == EV_UP) { u->menu_idx = (uint8_t)((u->menu_idx + 1) % MENU_N); u->err = NULL; }
        else if (ev == EV_SEL) {
            u->err = NULL;
            switch (u->menu_idx) {
            case MENU_OPTICAL:
                if (!u->cam_ok) { u->err = "Camera not available"; return ACT_SHOW_ERROR; }
                return go(u, ST_OPT_LIVE, ACT_START_OPTICAL);
            case MENU_WIRELESS:
                if (!u->wifi_ok) { u->err = "Wi-Fi not available"; return ACT_SHOW_ERROR; }
                u->dev_sel = 0;
                return go(u, ST_W_SURVEY, ACT_START_SURVEY);
            case MENU_HISTORY: return go(u, ST_HISTORY, ACT_NONE);
            case MENU_DIAG:    return go(u, ST_DIAG, ACT_NONE);
            default: break;
            }
        }
        break;

    case ST_OPT_LIVE:
        if (ev == EV_UP)   { u->overlay_diff = !u->overlay_diff; }
        if (ev == EV_SEL)  return go(u, ST_OPT_OFFAXIS, ACT_START_OFFAXIS);
        if (ev == EV_BACK) return go(u, ST_MENU, ACT_STOP_OPTICAL);
        break;

    case ST_OPT_OFFAXIS:
        if (ev == EV_OFFAXIS_DONE || ev == EV_BACK) return go(u, ST_OPT_LIVE, ACT_NONE);
        break;

    case ST_W_SURVEY:
        if (ev == EV_UP && u->n_dev) u->dev_sel = (uint8_t)((u->dev_sel + 1) % u->n_dev);
        if (ev == EV_SEL)  return go(u, ST_W_CHALLENGE, ACT_START_CHALLENGE);
        if (ev == EV_BACK) return go(u, ST_MENU, ACT_STOP_WIRELESS);
        break;

    case ST_W_CHALLENGE:
        if (ev == EV_CHALLENGE_DONE) return go(u, ST_W_RESULT, ACT_NONE);
        if (ev == EV_BACK)           return go(u, ST_W_SURVEY, ACT_ABORT_CHALLENGE);
        break;

    case ST_W_RESULT:
        if (ev == EV_UP && u->n_dev) u->dev_sel = (uint8_t)((u->dev_sel + 1) % u->n_dev);
        if (ev == EV_SEL)  return go(u, ST_W_LOCATE, ACT_START_LOCATE);
        if (ev == EV_BACK) return go(u, ST_W_SURVEY, ACT_NONE);
        break;

    case ST_W_LOCATE:
        if (ev == EV_BACK) return go(u, ST_W_RESULT, ACT_STOP_LOCATE);
        break;

    case ST_HISTORY:
    case ST_DIAG:
        if (ev == EV_BACK) return go(u, ST_MENU, ACT_NONE);
        break;

    case ST_LOWBAT:
        if (ev == EV_BAT_OK) return go(u, ST_MENU, ACT_NONE);
        break;

    default:
        break;
    }
    return ACT_NONE;
}
