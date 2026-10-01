/* sim_sources.h — SIMULATION ONLY: synthetic Wi-Fi devices.
 * Replaces wifi_mon in Wokwi, where monitor mode does not exist.
 * The numbers are illustrative, NOT measurements; E2 will replace them.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;       /* label shown in the sim (a real device has no name) */
    const char *vendor;     /* from OUI; "(random)" = locally administered MAC */
    uint8_t     mac[6];
    uint8_t     is_ap;
    uint8_t     ch;
    float       up_Bps;     /* mean ToDS (uplink) bytes/s                   */
    float       dl_Bps;     /* mean bytes/s received (FromDS addressed to it) */
    float       gain;       /* rate multiplier while the scene changes      */
    float       burst;      /* relative std-dev of per-100 ms traffic        */
    float       rssi_1m;    /* dBm at 1 m                                    */
    float       dist_m;     /* fixed distance (camera: from potentiometer)   */
} sim_dev_t;

void             sim_init(uint32_t seed);
int              sim_n_dev(void);
const sim_dev_t *sim_dev(int i);
void             sim_set_camera_distance(float m);

/* Uplink bytes produced by device i during dt_ms; stim_on = scene changing. */
uint32_t sim_uplink_bytes(int i, uint32_t dt_ms, int stim_on);
/* Noisy RSSI sample (log-distance model, n = 2.5). */
float    sim_rssi(int i);

#ifdef __cplusplus
}
#endif
