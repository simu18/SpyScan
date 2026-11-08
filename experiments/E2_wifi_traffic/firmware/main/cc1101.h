/* cc1101.h — SpyScan Sub-1 GHz RF Bug Hunter Driver
 *
 * Interfaces Texas Instruments CC1101 transceiver via SPI to hunt for
 * analog audio bugs, covert transmitters, and wireless bugs on 315 / 433 / 868 MHz.
 *
 * Default Pin Mapping for ESP32-S3 DevKit:
 *   CC1101 VCC   -> ESP32-S3 3V3 (3.3V ONLY - DO NOT USE 5V!)
 *   CC1101 GND   -> ESP32-S3 GND
 *   CC1101 CSN   -> ESP32-S3 GPIO 10
 *   CC1101 MOSI  -> ESP32-S3 GPIO 11
 *   CC1101 SCK   -> ESP32-S3 GPIO 12
 *   CC1101 MISO  -> ESP32-S3 GPIO 13
 *   CC1101 GDO0  -> ESP32-S3 GPIO 9 (optional carrier / interrupt)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pin definitions for ESP32-S3 DevKit */
#define PIN_CC1101_CSN     10
#define PIN_CC1101_MOSI    11
#define PIN_CC1101_SCK     12
#define PIN_CC1101_MISO    13
#define PIN_CC1101_GDO0    9

/* CC1101 Command Strobes */
#define CC1101_SRES         0x30        /* Reset chip */
#define CC1101_SFSTXON      0x31        /* Enable and calibrate frequency synthesizer */
#define CC1101_SXOFF        0x32        /* Turn off crystal oscillator */
#define CC1101_SCAL         0x33        /* Calibrate frequency synthesizer and turn it off */
#define CC1101_SRX          0x34        /* Enable RX */
#define CC1101_STX          0x35        /* Enable TX */
#define CC1101_SIDLE        0x36        /* Exit RX / TX, turn off frequency synthesizer */
#define CC1101_SWOR         0x38        /* Start automatic RX polling sequence */
#define CC1101_SPWD         0x39        /* Enter power down mode when CSn goes high */
#define CC1101_SFRX         0x3A        /* Flush RX FIFO buffer */
#define CC1101_SFTX         0x3B        /* Flush TX FIFO buffer */
#define CC1101_SNOP         0x3D        /* No operation */

/* CC1101 Configuration Registers */
#define CC1101_IOCFG2       0x00        /* GDO2 output pin configuration */
#define CC1101_IOCFG1       0x01        /* GDO1 output pin configuration */
#define CC1101_IOCFG0       0x02        /* GDO0 output pin configuration */
#define CC1101_FIFOTHR      0x03        /* RX FIFO and TX FIFO thresholds */
#define CC1101_SYNC1        0x04        /* Sync word, high byte */
#define CC1101_SYNC0        0x05        /* Sync word, low byte */
#define CC1101_PKTLEN       0x06        /* Packet length */
#define CC1101_PKTCTRL1     0x07        /* Packet automation control */
#define CC1101_PKTCTRL0     0x08        /* Packet automation control */
#define CC1101_ADDR         0x09        /* Device address */
#define CC1101_CHANNR       0x0A        /* Channel number */
#define CC1101_FSCTRL1      0x0B        /* Frequency synthesizer control */
#define CC1101_FSCTRL0      0x0C        /* Frequency synthesizer control */
#define CC1101_FREQ2        0x0D        /* Frequency control word, high byte */
#define CC1101_FREQ1        0x0E        /* Frequency control word, middle byte */
#define CC1101_FREQ0        0x0F        /* Frequency control word, low byte */
#define CC1101_MDMCFG4      0x10        /* Modem configuration (BW, data rate) */
#define CC1101_MDMCFG3      0x11        /* Modem configuration (data rate) */
#define CC1101_MDMCFG2      0x12        /* Modem configuration (modulation) */
#define CC1101_MDMCFG1      0x13        /* Modem configuration (channel spacing) */
#define CC1101_MDMCFG0      0x14        /* Modem configuration (channel spacing) */
#define CC1101_DEVIATN      0x15        /* Modem deviation setting */
#define CC1101_MCSM2        0x16        /* Main Radio Control State Machine config */
#define CC1101_MCSM1        0x17        /* Main Radio Control State Machine config */
#define CC1101_MCSM0        0x18        /* Main Radio Control State Machine config */
#define CC1101_FOCCFG       0x19        /* Frequency Offset Compensation config */
#define CC1101_BSCFG        0x1A        /* Bit Synchronization config */
#define CC1101_AGCCTRL2     0x1B        /* AGC control */
#define CC1101_AGCCTRL1     0x1C        /* AGC control */
#define CC1101_AGCCTRL0     0x1D        /* AGC control */
#define CC1101_FREND1       0x21        /* Front end RX configuration */
#define CC1101_FREND0       0x22        /* Front end TX configuration */
#define CC1101_FSCAL3       0x23        /* Frequency synthesizer calibration */
#define CC1101_FSCAL2       0x24        /* Frequency synthesizer calibration */
#define CC1101_FSCAL1       0x25        /* Frequency synthesizer calibration */
#define CC1101_FSCAL0       0x26        /* Frequency synthesizer calibration */
#define CC1101_TEST2        0x2C        /* Various test settings */
#define CC1101_TEST1        0x2D        /* Various test settings */
#define CC1101_TEST0        0x2E        /* Various test settings */

/* CC1101 Status Registers (Burst bit set: reg | 0xC0) */
#define CC1101_PARTNUM      0x30        /* Part number (0x00) */
#define CC1101_VERSION      0x31        /* Chip version (0x14 or 0x04) */
#define CC1101_RSSI         0x34        /* Received signal strength indicator */
#define CC1101_MARCSTATE    0x35        /* Main Radio Control State Machine state */
#define CC1101_PKTSTATUS    0x38        /* Current GDOx status and sync word */

/* TSCM Threat Classification */
typedef enum {
    THREAT_NONE = 0,            /* Ambient RF background (< -85 dBm) */
    THREAT_BURST = 1,           /* Digital burst: car keyfob, weather sensor, doorbell */
    THREAT_ANALOG_BUG = 2       /* Continuous carrier: covert analog FM/AM audio bug */
} subghz_threat_t;

typedef enum {
    SUBGHZ_MODE_OFF = 0,
    SUBGHZ_MODE_MONITOR = 1,    /* Fixed frequency continuous monitoring */
    SUBGHZ_MODE_SWEEP = 2       /* Channel sweep across ISM band */
} subghz_mode_t;

typedef struct {
    uint32_t freq_khz;
    int8_t   rssi_instant;
    int8_t   rssi_mean;
    int8_t   rssi_peak;
    float    variance;
    uint8_t  duty_percent;
    subghz_threat_t threat;
    const char *threat_name;
} subghz_sample_t;

/* Public API */
bool cc1101_init(void);
bool cc1101_is_connected(void);
void cc1101_set_frequency(uint32_t freq_hz);
void cc1101_set_channel(uint8_t ch);
void cc1101_set_rx(void);
void cc1101_idle(void);
int8_t cc1101_read_rssi(void);
uint8_t cc1101_read_marcstate(void);
uint8_t cc1101_read_version(void);

/* Threat Analysis Engine */
void cc1101_engine_init(void);
void cc1101_set_mode(subghz_mode_t mode);
void cc1101_process_step(subghz_sample_t *out_sample);

#ifdef __cplusplus
}
#endif

