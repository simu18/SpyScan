/* cc1101.c — SpyScan Sub-1 GHz RF Bug Hunter Driver
 *
 * Implements low-level SPI communication, RF synthesizer calibration,
 * wideband carrier detection, and TSCM statistical bug classification.
 */

#include "cc1101.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "cc1101";

static spi_device_handle_t s_spi = NULL;
static bool s_connected = false;
static subghz_mode_t s_mode = SUBGHZ_MODE_MONITOR;
static uint32_t s_current_freq_hz = 433920000; /* Default 433.92 MHz */
static uint8_t s_current_ch = 0;

/* Circular window for threat analysis */
#define WINDOW_SIZE 30
static int8_t s_rssi_history[WINDOW_SIZE];
static int s_hist_idx = 0;
static int s_hist_count = 0;

#define CARRIER_THRESHOLD_DBM  (-78)   /* Above this is considered active signal */

/* ------------------------------------------------------------------ SPI Low-Level */
static inline void cs_select(void)
{
    gpio_set_level(PIN_CC1101_CSN, 0);
}

static inline void cs_deselect(void)
{
    gpio_set_level(PIN_CC1101_CSN, 1);
}

static inline void wait_miso_low(void)
{
    int timeout = 5000;
    while (gpio_get_level(PIN_CC1101_MISO) != 0 && timeout > 0) {
        esp_rom_delay_us(1);
        timeout--;
    }
}

static uint8_t spi_transfer_byte(uint8_t data)
{
    uint8_t rx_data = 0;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = &rx_data
    };
    spi_device_polling_transmit(s_spi, &t);
    return rx_data;
}

uint8_t cc1101_strobe(uint8_t cmd)
{
    cs_select();
    wait_miso_low();
    uint8_t status = spi_transfer_byte(cmd);
    cs_deselect();
    return status;
}

void cc1101_write_reg(uint8_t addr, uint8_t val)
{
    cs_select();
    wait_miso_low();
    spi_transfer_byte(addr & 0x3F);
    spi_transfer_byte(val);
    cs_deselect();
}

uint8_t cc1101_read_reg(uint8_t addr)
{
    cs_select();
    wait_miso_low();
    spi_transfer_byte(addr | 0x80);
    uint8_t val = spi_transfer_byte(0x00);
    cs_deselect();
    return val;
}

uint8_t cc1101_read_status(uint8_t addr)
{
    cs_select();
    wait_miso_low();
    spi_transfer_byte(addr | 0xC0); /* Burst bit must be set to read status regs */
    uint8_t val = spi_transfer_byte(0x00);
    cs_deselect();
    return val;
}

/* ------------------------------------------------------------------ RF Config */
static void cc1101_rf_defaults(void)
{
    cc1101_strobe(CC1101_SIDLE);

    /* Reset / initial modem config for wideband analog/digital bug detection
     * IF freq 152 kHz, 26 MHz crystal, Channel BW ~203 kHz */
    cc1101_write_reg(CC1101_IOCFG2,   0x29); /* Chip ready */
    cc1101_write_reg(CC1101_IOCFG0,   0x06); /* Sync word / Carrier Sense */
    cc1101_write_reg(CC1101_FIFOTHR,  0x47);
    cc1101_write_reg(CC1101_PKTLEN,   0xFF);
    cc1101_write_reg(CC1101_PKTCTRL1, 0x04);
    cc1101_write_reg(CC1101_PKTCTRL0, 0x00); /* Continuous RX mode */
    cc1101_write_reg(CC1101_FSCTRL1,  0x06); /* 152.34 kHz IF */
    cc1101_write_reg(CC1101_FSCTRL0,  0x00);

    /* MDMCFG4: 203 kHz filter bandwidth, MDMCFG3: 250 kBaud */
    cc1101_write_reg(CC1101_MDMCFG4,  0x88);
    cc1101_write_reg(CC1101_MDMCFG3,  0x93);
    cc1101_write_reg(CC1101_MDMCFG2,  0x30); /* 2-FSK / carrier sense, no sync word */
    cc1101_write_reg(CC1101_MDMCFG1,  0x22); /* Channel spacing ~100 kHz */
    cc1101_write_reg(CC1101_MDMCFG0,  0xF8);

    cc1101_write_reg(CC1101_DEVIATN,  0x47); /* 47 kHz deviation */
    cc1101_write_reg(CC1101_MCSM1,    0x30); /* Always stay in RX */
    cc1101_write_reg(CC1101_MCSM0,    0x18); /* Auto-calibrate when going from IDLE to RX */
    cc1101_write_reg(CC1101_FOCCFG,   0x16);
    cc1101_write_reg(CC1101_AGCCTRL2, 0x43); /* Target amplitude */
    cc1101_write_reg(CC1101_AGCCTRL1, 0x40);
    cc1101_write_reg(CC1101_AGCCTRL0, 0x91);

    cc1101_write_reg(CC1101_FREND1,   0x56);
    cc1101_write_reg(CC1101_FREND0,   0x10);
    cc1101_write_reg(CC1101_FSCAL3,   0xE9);
    cc1101_write_reg(CC1101_FSCAL2,   0x2A);
    cc1101_write_reg(CC1101_FSCAL1,   0x00);
    cc1101_write_reg(CC1101_FSCAL0,   0x1F);
    cc1101_write_reg(CC1101_TEST2,    0x81);
    cc1101_write_reg(CC1101_TEST1,    0x35);
    cc1101_write_reg(CC1101_TEST0,    0x09);

    /* Set default frequency (433.92 MHz) */
    cc1101_set_frequency(s_current_freq_hz);
}

/* ------------------------------------------------------------------ Hardware Reset */
static void cc1101_manual_reset(void)
{
    cs_deselect();
    esp_rom_delay_us(5);
    cs_select();
    esp_rom_delay_us(10);
    cs_deselect();
    esp_rom_delay_us(45);
    cs_select();
    wait_miso_low();
    spi_transfer_byte(CC1101_SRES);
    wait_miso_low();
    cs_deselect();
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* ------------------------------------------------------------------ Public API */
bool cc1101_init(void)
{
    /* 1. Configure CSN as GPIO output */
    gpio_config_t csn_cfg = {
        .pin_bit_mask = 1ULL << PIN_CC1101_CSN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&csn_cfg);
    cs_deselect();

    /* 2. Configure MISO as input with pullup to detect ready state */
    gpio_config_t miso_cfg = {
        .pin_bit_mask = 1ULL << PIN_CC1101_MISO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&miso_cfg);

    /* 3. Initialize SPI bus */
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_CC1101_MISO,
        .mosi_io_num = PIN_CC1101_MOSI,
        .sclk_io_num = PIN_CC1101_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }

    /* 4. Add SPI device */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000, /* 4 MHz: safe and noise-immune over jumpers */
        .mode = 0,                         /* CPOL=0, CPHA=0 */
        .spics_io_num = -1,                /* CS managed manually for CC1101 strobe timing */
        .queue_size = 1
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return false;
    }

    /* 5. Perform manual strobe reset */
    cc1101_manual_reset();

    /* 6. Verify presence: read PARTNUM and VERSION */
    uint8_t part = cc1101_read_status(CC1101_PARTNUM);
    uint8_t ver  = cc1101_read_status(CC1101_VERSION);

    /* CC1101 returns PARTNUM=0x00 and VERSION=0x14 (or 0x04) */
    if (part == 0x00 && (ver == 0x14 || ver == 0x04 || ver == 0x13)) {
        s_connected = true;
        ESP_LOGI(TAG, "CC1101 detected: Part=0x%02x, Version=0x%02x", part, ver);
        cc1101_rf_defaults();
        cc1101_set_rx();
        return true;
    }

    ESP_LOGW(TAG, "CC1101 not detected (Part=0x%02x, Ver=0x%02x). Check wiring.", part, ver);
    s_connected = false;
    return false;
}

bool cc1101_is_connected(void)
{
    return s_connected;
}

void cc1101_set_frequency(uint32_t freq_hz)
{
    s_current_freq_hz = freq_hz;
    uint64_t val = ((uint64_t)freq_hz << 16) / 26000000ULL;
    uint8_t freq2 = (val >> 16) & 0xFF;
    uint8_t freq1 = (val >> 8)  & 0xFF;
    uint8_t freq0 = val & 0xFF;

    cc1101_strobe(CC1101_SIDLE);
    cc1101_write_reg(CC1101_FREQ2, freq2);
    cc1101_write_reg(CC1101_FREQ1, freq1);
    cc1101_write_reg(CC1101_FREQ0, freq0);
    cc1101_set_rx();
}

void cc1101_set_channel(uint8_t ch)
{
    s_current_ch = ch;
    cc1101_strobe(CC1101_SIDLE);
    cc1101_write_reg(CC1101_CHANNR, ch);
    cc1101_set_rx();
}

void cc1101_set_rx(void)
{
    cc1101_strobe(CC1101_SRX);
}

void cc1101_idle(void)
{
    cc1101_strobe(CC1101_SIDLE);
}

uint8_t cc1101_read_marcstate(void)
{
    return cc1101_read_status(CC1101_MARCSTATE) & 0x1F;
}

uint8_t cc1101_read_version(void)
{
    return cc1101_read_status(CC1101_VERSION);
}

int8_t cc1101_read_rssi(void)
{
    if (!s_connected) return -120;

    /* Verify radio is still in RX (MARCSTATE 0x0D = RX, 0x11 = RX_OVERFLOW) */
    uint8_t state = cc1101_read_marcstate();
    if (state == 0x11) { /* Overflow */
        cc1101_strobe(CC1101_SIDLE);
        cc1101_strobe(CC1101_SFRX);
        cc1101_strobe(CC1101_SRX);
    } else if (state != 0x0D) {
        cc1101_strobe(CC1101_SRX);
    }

    uint8_t raw = cc1101_read_status(CC1101_RSSI);
    int8_t rssi_dbm;
    if (raw >= 128) {
        rssi_dbm = (int8_t)((raw - 256) / 2) - 74;
    } else {
        rssi_dbm = (int8_t)(raw / 2) - 74;
    }
    return rssi_dbm;
}

/* ------------------------------------------------------------------ Threat Engine */
void cc1101_engine_init(void)
{
    memset(s_rssi_history, -110, sizeof(s_rssi_history));
    s_hist_idx = 0;
    s_hist_count = 0;
}

void cc1101_set_mode(subghz_mode_t mode)
{
    s_mode = mode;
    if (mode == SUBGHZ_MODE_OFF) {
        cc1101_idle();
    } else {
        cc1101_set_rx();
    }
}

void cc1101_process_step(subghz_sample_t *out)
{
    if (!out) return;

    if (!s_connected) {
        out->freq_khz = s_current_freq_hz / 1000;
        out->rssi_instant = -120;
        out->rssi_mean = -120;
        out->rssi_peak = -120;
        out->variance = 0.0f;
        out->duty_percent = 0;
        out->threat = THREAT_NONE;
        out->threat_name = "DISCONNECTED";
        return;
    }

    int8_t rssi = cc1101_read_rssi();

    /* Store into rolling buffer */
    s_rssi_history[s_hist_idx] = rssi;
    s_hist_idx = (s_hist_idx + 1) % WINDOW_SIZE;
    if (s_hist_count < WINDOW_SIZE) s_hist_count++;

    /* Compute stats over window */
    int sum = 0;
    int peak = -130;
    int carrier_hits = 0;

    for (int i = 0; i < s_hist_count; ++i) {
        int v = s_rssi_history[i];
        sum += v;
        if (v > peak) peak = v;
        if (v >= CARRIER_THRESHOLD_DBM) carrier_hits++;
    }

    float mean = (float)sum / s_hist_count;
    float sum_sq = 0.0f;
    for (int i = 0; i < s_hist_count; ++i) {
        float diff = s_rssi_history[i] - mean;
        sum_sq += diff * diff;
    }
    float variance = sqrtf(sum_sq / s_hist_count);
    uint8_t duty = (uint8_t)((carrier_hits * 100) / s_hist_count);

    /* TSCM Threat Decision:
     * 1) Analog Bug (FM/AM covert mic): Continuous carrier, 100% duty, low variance, strong RSSI
     * 2) Digital Burst (Car remote, keyfob, doorbell): Short pulse, duty 5..40%, high peak
     * 3) Clear: Ambient noise floor */
    subghz_threat_t threat = THREAT_NONE;
    const char *name = "CLEAR";

    if (duty >= 80 && mean >= -78.0f && variance <= 3.8f) {
        threat = THREAT_ANALOG_BUG;
        name = "ANALOG_BUG";
    } else if (peak >= -74 && duty >= 3 && duty <= 45) {
        threat = THREAT_BURST;
        name = "BURST_PACKET";
    }

    out->freq_khz = (s_current_freq_hz + (s_current_ch * 100000)) / 1000;
    out->rssi_instant = rssi;
    out->rssi_mean = (int8_t)mean;
    out->rssi_peak = (int8_t)peak;
    out->variance = variance;
    out->duty_percent = duty;
    out->threat = threat;
    out->threat_name = name;
}

