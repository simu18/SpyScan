/* spyscan_proto.h — Inter-Board RF Telemetry Protocol
 *
 * Lightweight, framed serial protocol for streaming counter-surveillance
 * telemetry between the ESP32-S3 DevKit (RF Coprocessor) and the
 * ESP32-S3 CAM Board (UI Master) over high-speed UART (921,600 baud).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPYSCAN_PROTO_SYNC1     0x53  /* 'S' */
#define SPYSCAN_PROTO_SYNC2     0x50  /* 'P' */

/* Telemetry Message Types */
typedef enum {
    MSG_TYPE_WIFI_STATUS     = 0x01,
    MSG_TYPE_SUBGHZ_STATUS   = 0x02,
    MSG_TYPE_BLE_STATUS      = 0x03,
    MSG_TYPE_CHALLENGE_PROV  = 0x04,
    MSG_TYPE_CMD_MODE_SET    = 0x10,
    MSG_TYPE_CMD_FREQ_SET    = 0x11,
} spyscan_msg_type_t;

/* 1. Wi-Fi Telemetry Payload */
typedef struct __attribute__((packed)) {
    uint8_t  mac[6];
    uint8_t  ch;
    int8_t   rssi;
    uint16_t tx_kbps;
    uint16_t rx_kbps;
    int8_t   trend_slope;       /* dBm/s (+ = Warmer, - = Colder) */
    uint8_t  is_streaming;      /* 1 = high-uplink video camera */
} spyscan_wifi_telemetry_t;

/* 2. Sub-1 GHz RF Bug Telemetry Payload */
typedef struct __attribute__((packed)) {
    uint32_t freq_khz;
    int8_t   rssi_inst;
    int8_t   rssi_peak;
    uint8_t  duty_pct;          /* 0..100% */
    uint8_t  var_x10;           /* variance * 10 (e.g. 18 = 1.8 dB) */
    uint8_t  threat_code;       /* 0=CLEAR, 1=KEYFOB_BURST, 2=ANALOG_BUG */
} spyscan_subghz_telemetry_t;

/* 3. BLE Covert Bug Telemetry Payload */
typedef struct __attribute__((packed)) {
    uint8_t  tag_count;
    uint8_t  nearest_type;      /* 0=NONE, 1=AIRTAG, 2=SMARTTAG, 3=TUYA */
    int8_t   nearest_rssi;
    uint16_t distance_cm;
} spyscan_ble_telemetry_t;

/* 4. Optical-to-Wi-Fi Challenge Status */
typedef struct __attribute__((packed)) {
    uint8_t  slot_idx;
    uint8_t  slot_total;
    uint8_t  stim_active;       /* 1 = FLASH LIGHT NOW, 0 = STILL */
    uint16_t p_val_x1000;       /* e.g. 30 = p=0.030 */
    uint16_t surge_ratio_x100;  /* e.g. 267 = +2.67x */
    uint8_t  is_proven;         /* 1 = 100% Proven Camera */
} spyscan_challenge_telemetry_t;

/* Unified RF Coprocessor State Cache */
typedef struct {
    uint32_t last_rx_ms;
    bool     coprocessor_online;
    
    /* Wi-Fi */
    spyscan_wifi_telemetry_t wifi;
    
    /* Sub-GHz */
    spyscan_subghz_telemetry_t subghz;
    int8_t subghz_history[8];   /* Rolling 8-epoch waterfall buffer */
    
    /* BLE */
    spyscan_ble_telemetry_t ble;
    
    /* Challenge */
    spyscan_challenge_telemetry_t challenge;
} spyscan_coprocessor_state_t;

#ifdef __cplusplus
}
#endif

