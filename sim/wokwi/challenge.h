/* challenge.h — stimulus–traffic correlation detector (pure C, no HW).
 *
 * Method (ТЗ FR-W5/FR-W6):
 *   1. A balanced pseudo-random stimulus s[k] in {0,1}, k = 0..K-1
 *      (K/2 "MOVE" slots, K/2 "STILL" slots) is generated.
 *   2. For each monitored device, uplink bytes are accumulated per slot.
 *   3. Statistic S = Pearson correlation between per-slot rate x[k] and s[k].
 *   4. Null distribution: S recomputed for N random permutations of s.
 *      p = (1 + #{S_perm >= S}) / (1 + N)   (one-sided: camera rate rises
 *      when the scene changes).
 *   5. Level: HIGH if p < P_HIGH, LOW if p < P_LOW, else NONE.
 * The false-alarm probability is fixed by construction (p threshold),
 * not by a hand-picked byte-rate threshold.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CH_MAX_SLOTS   32
#define CH_MAX_DEV     16
#define CH_N_PERM      1000
#define CH_P_HIGH      0.01f
#define CH_P_LOW       0.05f

typedef enum { CH_LEVEL_NONE = 0, CH_LEVEL_LOW, CH_LEVEL_HIGH } ch_level_t;

typedef struct {
    float      stat;      /* correlation coefficient S          */
    float      p;         /* permutation p-value                */
    ch_level_t level;
} ch_result_t;

typedef struct {
    uint8_t  k_slots;                       /* K                       */
    uint32_t slot_ms;                       /* T                       */
    uint8_t  stim[CH_MAX_SLOTS];            /* s[k] in {0,1}           */
    uint32_t bytes[CH_MAX_DEV][CH_MAX_SLOTS];
    uint8_t  n_dev;
    uint32_t rng;                           /* xorshift32 state        */
} challenge_t;

/* Generate a balanced PRBS; k_slots must be even and <= CH_MAX_SLOTS. */
void challenge_init(challenge_t *c, uint8_t k_slots, uint32_t slot_ms,
                    uint8_t n_dev, uint32_t seed);

/* Slot index for a time since challenge start, or -1 when finished. */
int  challenge_slot_at(const challenge_t *c, uint32_t t_ms);

/* Add observed uplink bytes of device `dev` at time t_ms since start. */
void challenge_add_bytes(challenge_t *c, uint8_t dev, uint32_t t_ms,
                         uint32_t bytes);

/* Evaluate one device. */
ch_result_t challenge_evaluate(challenge_t *c, uint8_t dev);

/* Small helper RNG shared with the simulation sources. */
uint32_t ch_xorshift32(uint32_t *state);

#ifdef __cplusplus
}
#endif
