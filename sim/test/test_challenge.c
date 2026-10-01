#include <stdlib.h>
/* PC test: run many simulated challenges, count HIGH flags per device.
 * Synthetic data only — validates the code path, not the physics. */
#include <stdio.h>
#include "../wokwi/challenge.h"
#include "../wokwi/sim_sources.h"
int main(int argc,char**argv){
  int K = argc>1? atoi(argv[1]):12; int T = argc>2? atoi(argv[2]):5000; int RUNS=200;
  int nd = sim_n_dev(); int high[16]={0}, low[16]={0};
  sim_init(42);
  for(int r=0;r<RUNS;r++){
    challenge_t c; challenge_init(&c,K,T,nd,1000+r*7919);
    for(uint32_t t=0;t<(uint32_t)K*T;t+=100){
      int k=challenge_slot_at(&c,t); int on=c.stim[k];
      /* 300 ms encoder/network lag */
      int klag=challenge_slot_at(&c,t>=300?t-300:0); int on_l=c.stim[klag];
      (void)on;
      for(int d=0;d<nd;d++) challenge_add_bytes(&c,d,t,sim_uplink_bytes(d,100,on_l));
    }
    for(int d=0;d<nd;d++){ ch_result_t x=challenge_evaluate(&c,d); if(x.level==CH_LEVEL_HIGH)high[d]++; else if(x.level==CH_LEVEL_LOW)low[d]++; }
  }
  printf("K=%d T=%dms runs=%d\n",K,T,RUNS);
  for(int d=0;d<nd;d++) printf("  %-11s HIGH %5.1f%%  LOW %5.1f%%\n",sim_dev(d)->name,100.0*high[d]/RUNS,100.0*low[d]/RUNS);
}
