/* sim_sources.c — SIMULATION ONLY (see header). */
#include "sim_sources.h"
#include "challenge.h"   /* ch_xorshift32 */
#include <math.h>

/* Device 1 is the hidden camera. Device 2 watches its live stream
 * (its uplink = TCP ACKs, so it ALSO follows the stimulus, but it is
 * downlink-dominant -> labelled "viewer", not "source"). */
static sim_dev_t DEV[] = {
    /* name          vendor      mac                                ap ch  up      dl      gain  burst rssi1m dist */
    { "Router",      "TP-Link",  {0x50,0xC7,0xBF,0x12,0x34,0x56},   1, 6,  0,      0,      1.0f, 0.0f, -35,  4.0f },
    { "Hidden cam",  "Espressif",{0x24,0x6F,0x28,0xAA,0xBB,0xCC},   0, 6,  60000,  1500,   1.8f, 0.25f,-40,  3.0f },
    { "Phone A",     "(random)", {0x5A,0x11,0x22,0x33,0x44,0x55},   0, 6,  2500,   60000,  1.8f, 0.30f,-38,  2.0f },
    { "Laptop",      "Intel",    {0x3C,0xA9,0xF4,0x01,0x02,0x03},   0, 6,  15000,  200000, 1.0f, 1.20f,-42,  3.5f },
    { "Smart plug",  "Espressif",{0x24,0x6F,0x28,0x10,0x20,0x30},   0, 6,  300,    300,    1.0f, 0.50f,-45,  5.0f },
    { "Phone B",     "(random)", {0x7E,0x99,0x88,0x77,0x66,0x55},   0, 1,  400,    800,    1.0f, 1.00f,-40,  6.0f },
};
#define N_DEV ((int)(sizeof(DEV) / sizeof(DEV[0])))

static uint32_t rng = 0xC0FFEEu;

/* Approximate N(0,1): sum of 4 uniforms, scaled (Irwin–Hall). */
static float gauss(void)
{
    float s = 0;
    for (int i = 0; i < 4; ++i) s += (float)(ch_xorshift32(&rng) & 0xFFFF) / 65535.0f;
    return (s - 2.0f) * 1.7320508f;   /* var(sum of 4 U) = 1/3 */
}

void sim_init(uint32_t seed) { rng = seed ? seed : 0xC0FFEEu; }
int  sim_n_dev(void) { return N_DEV; }
const sim_dev_t *sim_dev(int i) { return (i >= 0 && i < N_DEV) ? &DEV[i] : 0; }
void sim_set_camera_distance(float m) { DEV[1].dist_m = (m < 0.3f) ? 0.3f : m; }

uint32_t sim_uplink_bytes(int i, uint32_t dt_ms, int stim_on)
{
    const sim_dev_t *d = sim_dev(i);
    if (!d || d->up_Bps <= 0) return 0;
    float mean = d->up_Bps * (float)dt_ms / 1000.0f;
    if (stim_on) mean *= d->gain;
    float v = mean * (1.0f + d->burst * gauss());
    return (v > 0) ? (uint32_t)v : 0;
}

float sim_rssi(int i)
{
    const sim_dev_t *d = sim_dev(i);
    if (!d) return -100;
    return d->rssi_1m - 25.0f * log10f(d->dist_m) + 2.0f * gauss();
}
