/* PC unit test for ui_fsm.c: checks every transition in 06_ui_fsm.puml */
#include <assert.h>
#include <stdio.h>
#include "../wokwi/ui_fsm.h"
#define EXPECT(ev, st_, act_) do{ ui_action_t a=ui_handle(&u,ev); \
  if(u.st!=(st_)||a!=(act_)){printf("FAIL line %d: state=%s act=%d\n",__LINE__,ui_state_name(u.st),a);return 1;} n++; }while(0)
int main(void){ ui_t u; int n=0; ui_init(&u,true,true); u.n_dev=3;
  EXPECT(EV_UP, ST_BOOT, ACT_NONE);
  EXPECT(EV_INIT_DONE, ST_MENU, ACT_NONE);
  EXPECT(EV_SEL, ST_OPT_LIVE, ACT_START_OPTICAL);
  EXPECT(EV_UP, ST_OPT_LIVE, ACT_NONE); assert(!u.overlay_diff);
  EXPECT(EV_SEL, ST_OPT_OFFAXIS, ACT_START_OFFAXIS);
  EXPECT(EV_OFFAXIS_DONE, ST_OPT_LIVE, ACT_NONE);
  EXPECT(EV_BACK, ST_MENU, ACT_STOP_OPTICAL);
  EXPECT(EV_UP, ST_MENU, ACT_NONE); assert(u.menu_idx==MENU_WIRELESS);
  EXPECT(EV_SEL, ST_W_SURVEY, ACT_START_SURVEY);
  EXPECT(EV_UP, ST_W_SURVEY, ACT_NONE); assert(u.dev_sel==1);
  EXPECT(EV_SEL, ST_W_CHALLENGE, ACT_START_CHALLENGE);
  EXPECT(EV_BACK, ST_W_SURVEY, ACT_ABORT_CHALLENGE);
  EXPECT(EV_SEL, ST_W_CHALLENGE, ACT_START_CHALLENGE);
  EXPECT(EV_CHALLENGE_DONE, ST_W_RESULT, ACT_NONE);
  EXPECT(EV_SEL, ST_W_LOCATE, ACT_START_LOCATE);
  EXPECT(EV_LOWBAT, ST_LOWBAT, ACT_STOP_ALL);
  EXPECT(EV_SEL, ST_LOWBAT, ACT_NONE);
  EXPECT(EV_BAT_OK, ST_MENU, ACT_NONE);
  /* camera failure: optical refused, wireless still works (NR-R2) */
  ui_init(&u,false,true); EXPECT(EV_INIT_DONE, ST_MENU, ACT_NONE);
  EXPECT(EV_SEL, ST_MENU, ACT_SHOW_ERROR); assert(u.err);
  EXPECT(EV_UP, ST_MENU, ACT_NONE);
  EXPECT(EV_SEL, ST_W_SURVEY, ACT_START_SURVEY);
  printf("ui_fsm: %d checks passed\n", n); return 0; }
