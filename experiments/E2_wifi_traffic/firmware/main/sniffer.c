/*
 * SpyScan — Experiment E2 sniffer firmware (ESP32-S3, ESP-IDF 6.x)
 *
 * Purpose: measure per-device 802.11 traffic (bytes, frames, RSSI, direction)
 * in fixed time bins and stream it to a PC over USB Serial/JTAG.
 * Passive receive only: no frames are transmitted, payloads are not stored.
 *
 * Output lines (CSV, one record per line, '\n' terminated):
 *   I,<t_ms>,<key>,<value...>                         info / status
 *   A,<t_ms>,<ch>,<bssid>,<rssi>,<ssid>               access point (beacon), once per BSSID
 *   B,<t_ms>,<ch>,<mac>,<tx_B>,<tx_n>,<rx_B>,<rx_n>,<rssi>,<role>,<bssid>
 *                                                     per-MAC activity in the bin that ENDS at t_ms
 *        tx_* = data frames transmitted by <mac> (addr2)   -> uplink for a client
 *        rx_* = unicast data frames addressed to <mac>      -> downlink for a client
 *        role = S (sent ToDS frames => client/station), A (AP), ? (unknown)
 *        RSSI is measured for frames transmitted by <mac>; 0 means no sample.
 *   M,<t_ms>,<label>                                  marker echoed from the PC (stimulus sync)
 *
 * Commands (send a line, '\n' terminated):
 *   ch <1..13>     lock to a channel (stops hopping)
 *   hop <ms>       hop channels 1..13 with the given dwell time
 *   bin <ms>       set bin length (20..5000 ms)
 *   mark <label>   echo a marker line with the device timestamp
 *   stat           print counters
 *   reset          clear the device table
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"

#define MAX_DEV        64       /* tracked MACs; LRU replacement when full */
#define MAX_AP         32
#define PKT_QUEUE_LEN  1024

/* ------------------------------------------------------------------ */
/* Compact record passed from the Wi-Fi callback to the aggregator.   */
typedef struct {
    uint32_t t_ms;
    uint16_t len;          /* sig_len, includes 4-byte FCS              */
    uint8_t  fc0, fc1;     /* frame control bytes                       */
    int8_t   rssi;
    uint8_t  ch;
    uint8_t  a1[6], a2[6], a3[6];
    char     ssid[33];     /* only for beacons, else empty              */
} pkt_rec_t;

typedef struct {
    uint8_t  mac[6];
    uint8_t  bssid[6];
    uint8_t  role;         /* 'S', 'A', '?' */
    uint8_t  ch;
    uint32_t tx_b, tx_n, rx_b, rx_n;   /* current bin */
    int32_t  rssi_sum;
    uint16_t rssi_n;
    uint32_t last_seen_ms;
    bool     used;
} sta_t;

static QueueHandle_t s_pkt_q;
static sta_t   s_dev[MAX_DEV];
static uint8_t s_ap[MAX_AP][6];
static int     s_ap_n;

static volatile uint32_t s_bin_ms   = 100;
static volatile uint32_t s_hop_ms   = 0;     /* 0 = channel locked */
static volatile uint8_t  s_channel  = 1;
static volatile uint32_t s_drops    = 0;
static volatile uint32_t s_rx_total = 0;

static inline uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

/* ------------------------------------------------------------------ */
static void out(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) fwrite(buf, 1, (n < (int)sizeof buf) ? n : (int)sizeof buf - 1, stdout);
}

#define MACFMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACARG(m) (m)[0], (m)[1], (m)[2], (m)[3], (m)[4], (m)[5]

static inline bool is_unicast(const uint8_t *m) { return (m[0] & 0x01) == 0; }

/* ------------------------------------------------------------------ */
/* Wi-Fi driver callback: runs in the Wi-Fi task. Keep it minimal.    */
static void IRAM_ATTR promisc_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t *p = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *h = p->payload;
    uint16_t len = p->rx_ctrl.sig_len;
    if (len < 24) return;

    s_rx_total++;
    pkt_rec_t r;
    r.t_ms = now_ms();
    r.len  = len;
    r.fc0  = h[0];
    r.fc1  = h[1];
    r.rssi = (int8_t)p->rx_ctrl.rssi;
    r.ch   = (uint8_t)p->rx_ctrl.channel;
    memcpy(r.a1, h + 4, 6);
    memcpy(r.a2, h + 10, 6);
    memcpy(r.a3, h + 16, 6);
    r.ssid[0] = 0;

    /* Beacon: type 0 (mgmt), subtype 8 -> fc0 == 0x80. SSID IE at offset 36. */
    if (type == WIFI_PKT_MGMT && h[0] == 0x80 && len > 38 && h[36] == 0) {
        uint8_t sl = h[37];
        if (sl > 32) sl = 32;
        if (38 + sl <= len) { memcpy(r.ssid, h + 38, sl); r.ssid[sl] = 0; }
        for (int i = 0; i < sl; ++i) if (!isprint((unsigned char)r.ssid[i]) || r.ssid[i] == ',') r.ssid[i] = '_';
    } else if (type == WIFI_PKT_MGMT) {
        return;   /* other management frames are not needed for E2 */
    }

    if (xQueueSend(s_pkt_q, &r, 0) != pdTRUE) s_drops++;
}

/* ------------------------------------------------------------------ */
static sta_t *dev_get(const uint8_t *mac, uint32_t t)
{
    sta_t *free_slot = NULL, *oldest = NULL;
    for (int i = 0; i < MAX_DEV; ++i) {
        sta_t *d = &s_dev[i];
        if (!d->used) { if (!free_slot) free_slot = d; continue; }
        if (memcmp(d->mac, mac, 6) == 0) return d;
        if (!oldest || d->last_seen_ms < oldest->last_seen_ms) oldest = d;
    }
    sta_t *d = free_slot ? free_slot : oldest;   /* LRU replacement when full */
    memset(d, 0, sizeof *d);
    memcpy(d->mac, mac, 6);
    d->used = true;
    d->role = '?';
    d->last_seen_ms = t;
    return d;
}

static void ap_seen(const pkt_rec_t *r)
{
    for (int i = 0; i < s_ap_n; ++i) if (memcmp(s_ap[i], r->a2, 6) == 0) return;
    if (s_ap_n < MAX_AP) memcpy(s_ap[s_ap_n++], r->a2, 6);
    out("A,%lu,%u," MACFMT ",%d,%s\n", (unsigned long)r->t_ms, r->ch, MACARG(r->a2), r->rssi, r->ssid);
}

static void handle(const pkt_rec_t *r)
{
    uint8_t type = (r->fc0 >> 2) & 0x3;
    if (type == 0) {                         /* beacon */
        ap_seen(r);
        sta_t *d = dev_get(r->a2, r->t_ms);
        d->role = 'A';
        memcpy(d->bssid, r->a2, 6);
        d->ch = r->ch;
        d->rssi_sum += r->rssi; d->rssi_n++;
        d->last_seen_ms = r->t_ms;
        return;
    }
    if (type != 2) return;                   /* data frames only */

    bool to_ds   = r->fc1 & 0x01;
    bool from_ds = r->fc1 & 0x02;

    /* Transmitter = addr2 */
    sta_t *tx = dev_get(r->a2, r->t_ms);
    tx->tx_b += r->len; tx->tx_n++;
    tx->rssi_sum += r->rssi; tx->rssi_n++;
    tx->last_seen_ms = r->t_ms;
    tx->ch = r->ch;
    if (to_ds && !from_ds) { tx->role = 'S'; memcpy(tx->bssid, r->a1, 6); }
    else if (from_ds && !to_ds) { tx->role = 'A'; memcpy(tx->bssid, r->a2, 6); }

    /* Receiver = addr1 (unicast only) */
    if (is_unicast(r->a1)) {
        sta_t *rx = dev_get(r->a1, r->t_ms);
        rx->rx_b += r->len; rx->rx_n++;
        rx->last_seen_ms = r->t_ms;
        rx->ch = r->ch;
        if (to_ds && !from_ds) {
            rx->role = 'A';
            memcpy(rx->bssid, r->a1, 6);
        } else if (from_ds && !to_ds) {
            if (rx->role == '?') rx->role = 'S';
            memcpy(rx->bssid, r->a2, 6);
        }
    }
}

static void flush_bin(uint32_t t_end)
{
    for (int i = 0; i < MAX_DEV; ++i) {
        sta_t *d = &s_dev[i];
        if (!d->used || (d->tx_n == 0 && d->rx_n == 0)) continue;
        int rssi = d->rssi_n ? (int)(d->rssi_sum / d->rssi_n) : 0;
        out("B,%lu,%u," MACFMT ",%lu,%lu,%lu,%lu,%d,%c," MACFMT "\n",
            (unsigned long)t_end, d->ch, MACARG(d->mac),
            (unsigned long)d->tx_b, (unsigned long)d->tx_n,
            (unsigned long)d->rx_b, (unsigned long)d->rx_n,
            rssi, d->role, MACARG(d->bssid));
        d->tx_b = d->tx_n = d->rx_b = d->rx_n = 0;
        d->rssi_sum = 0; d->rssi_n = 0;
    }
}

/* ------------------------------------------------------------------ */
static void agg_task(void *arg)
{
    pkt_rec_t r;
    uint32_t next_bin = now_ms() + s_bin_ms;
    uint32_t next_hop = now_ms() + (s_hop_ms ? s_hop_ms : 1000);
    uint32_t next_stat = now_ms() + 5000;
    uint8_t  hop_ch = 1;

    for (;;) {
        if (xQueueReceive(s_pkt_q, &r, pdMS_TO_TICKS(5)) == pdTRUE) handle(&r);

        uint32_t t = now_ms();
        if ((int32_t)(t - next_bin) >= 0) {
            flush_bin(next_bin);
            next_bin += s_bin_ms;
            if ((int32_t)(t - next_bin) > 0) next_bin = t + s_bin_ms;   /* fell behind */
        }
        if (s_hop_ms && (int32_t)(t - next_hop) >= 0) {
            hop_ch = (hop_ch % 13) + 1;
            esp_wifi_set_channel(hop_ch, WIFI_SECOND_CHAN_NONE);
            s_channel = hop_ch;
            next_hop = t + s_hop_ms;
        }
        if ((int32_t)(t - next_stat) >= 0) {
            out("I,%lu,stat,rx=%lu,drops=%lu,ch=%u,bin=%lu,hop=%lu,heap=%lu\n",
                (unsigned long)t, (unsigned long)s_rx_total, (unsigned long)s_drops,
                s_channel, (unsigned long)s_bin_ms, (unsigned long)s_hop_ms,
                (unsigned long)esp_get_free_heap_size());
            next_stat = t + 5000;
        }
    }
}

/* ------------------------------------------------------------------ */
static void cmd_task(void *arg)
{
    char line[96];
    int  n = 0;
    uint8_t c;
    for (;;) {
        int input = getchar();
        if (input == EOF) { vTaskDelay(1); continue; }
        c = (uint8_t)input;
        if (c == '\r') continue;
        if (c != '\n' && n < (int)sizeof line - 1) { line[n++] = (char)c; continue; }
        line[n] = 0; n = 0;

        uint32_t t = now_ms();
        if (strncmp(line, "ch ", 3) == 0) {
            int ch = atoi(line + 3);
            if (ch >= 1 && ch <= 13) {
                s_hop_ms = 0; s_channel = (uint8_t)ch;
                esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
                out("I,%lu,channel,%d\n", (unsigned long)t, ch);
            }
        } else if (strncmp(line, "hop ", 4) == 0) {
            int ms = atoi(line + 4);
            s_hop_ms = (ms >= 50) ? (uint32_t)ms : 0;
            out("I,%lu,hop,%lu\n", (unsigned long)t, (unsigned long)s_hop_ms);
        } else if (strncmp(line, "bin ", 4) == 0) {
            int ms = atoi(line + 4);
            if (ms >= 20 && ms <= 5000) s_bin_ms = (uint32_t)ms;
            out("I,%lu,bin,%lu\n", (unsigned long)t, (unsigned long)s_bin_ms);
        } else if (strncmp(line, "mark ", 5) == 0) {
            out("M,%lu,%s\n", (unsigned long)t, line + 5);
        } else if (strcmp(line, "stat") == 0) {
            out("I,%lu,stat,rx=%lu,drops=%lu,ch=%u\n", (unsigned long)t,
                (unsigned long)s_rx_total, (unsigned long)s_drops, s_channel);
        } else if (strcmp(line, "reset") == 0) {
            memset(s_dev, 0, sizeof s_dev); s_ap_n = 0;
            out("I,%lu,reset\n", (unsigned long)t);
        } else if (line[0]) {
            out("I,%lu,unknown_cmd,%s\n", (unsigned long)t, line);
        }
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);

    /* Use the interrupt-driven USB Serial/JTAG driver instead of the default
       polling console: buffered TX (bursts of B lines do not stall the
       aggregator) and blocking RX for the command task. */
    usb_serial_jtag_driver_config_t usj = { .tx_buffer_size = 8192, .rx_buffer_size = 512 };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usj));
    usb_serial_jtag_vfs_use_driver();
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_pkt_q = xQueueCreate(PKT_QUEUE_LEN, sizeof(pkt_rec_t));
    configASSERT(s_pkt_q);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_promiscuous_filter_t f = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&f));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promisc_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE));

    xTaskCreatePinnedToCore(agg_task, "agg", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(cmd_task, "cmd", 3072, NULL, 4, NULL, 1);

    out("I,%lu,boot,spyscan-e2-sniffer,v0.1,transport=usb-serial-jtag\n", (unsigned long)now_ms());
}
