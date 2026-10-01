/* challenge.c — see challenge.h for the method description. */
#include "challenge.h"
#include <math.h>
#include <string.h>

uint32_t ch_xorshift32(uint32_t *s)
{
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}

static void shuffle_u8(uint8_t *a, int n, uint32_t *rng)
{
    for (int i = n - 1; i > 0; --i) {
        int j = (int)(ch_xorshift32(rng) % (uint32_t)(i + 1));
        uint8_t t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

void challenge_init(challenge_t *c, uint8_t k_slots, uint32_t slot_ms,
                    uint8_t n_dev, uint32_t seed)
{
    memset(c, 0, sizeof(*c));
    if (k_slots > CH_MAX_SLOTS) k_slots = CH_MAX_SLOTS;
    if (k_slots & 1u) k_slots--;               /* must be even (balanced) */
    if (n_dev > CH_MAX_DEV) n_dev = CH_MAX_DEV;
    c->k_slots = k_slots;
    c->slot_ms = slot_ms;
    c->n_dev   = n_dev;
    c->rng     = seed ? seed : 0x1234567u;
    for (int k = 0; k < k_slots; ++k) c->stim[k] = (k < k_slots / 2) ? 1 : 0;
    shuffle_u8(c->stim, k_slots, &c->rng);
}

int challenge_slot_at(const challenge_t *c, uint32_t t_ms)
{
    uint32_t k = t_ms / c->slot_ms;
    return (k < c->k_slots) ? (int)k : -1;
}

void challenge_add_bytes(challenge_t *c, uint8_t dev, uint32_t t_ms,
                         uint32_t bytes)
{
    int k = challenge_slot_at(c, t_ms);
    if (k < 0 || dev >= c->n_dev) return;
    c->bytes[dev][k] += bytes;
}

/* Pearson correlation between x[] and s[] (s in {0,1}). */
static float corr(const float *x, const uint8_t *s, int n)
{
    float mx = 0, ms = 0;
    for (int i = 0; i < n; ++i) { mx += x[i]; ms += s[i]; }
    mx /= n; ms /= n;
    float sxy = 0, sxx = 0, syy = 0;
    for (int i = 0; i < n; ++i) {
        float dx = x[i] - mx, ds = (float)s[i] - ms;
        sxy += dx * ds; sxx += dx * dx; syy += ds * ds;
    }
    if (sxx <= 0.0f || syy <= 0.0f) return 0.0f;
    return sxy / sqrtf(sxx * syy);
}

ch_result_t challenge_evaluate(challenge_t *c, uint8_t dev)
{
    ch_result_t r = { 0.0f, 1.0f, CH_LEVEL_NONE };
    const int n = c->k_slots;
    if (dev >= c->n_dev || n < 4) return r;

    float x[CH_MAX_SLOTS];
    for (int k = 0; k < n; ++k) x[k] = (float)c->bytes[dev][k];

    r.stat = corr(x, c->stim, n);

    uint8_t perm[CH_MAX_SLOTS];
    memcpy(perm, c->stim, (size_t)n);
    uint32_t ge = 0;
    for (int i = 0; i < CH_N_PERM; ++i) {
        shuffle_u8(perm, n, &c->rng);
        if (corr(x, perm, n) >= r.stat) ge++;
    }
    r.p = (1.0f + (float)ge) / (1.0f + (float)CH_N_PERM);
    r.level = (r.p < CH_P_HIGH) ? CH_LEVEL_HIGH
            : (r.p < CH_P_LOW)  ? CH_LEVEL_LOW : CH_LEVEL_NONE;
    return r;
}
