// types.h - Type Definitions for Project Starbeam V3 (Master ESP32)
// CC1101 radios are on Bridge ESP32 - this file defines types for nRF24 + UART proxy

#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================================================
// Application State Machine
// ============================================================================

enum AppState {
  STATE_MENU,
  STATE_BT_JAM,
  STATE_DRONE_JAM,
  STATE_WIFI_JAM,
  STATE_CC1_JAM,
  STATE_CC_SCAN,
  STATE_NRF_SCAN,
  STATE_WIFI_SCAN,
  STATE_WIFI_HEATMAP,
  STATE_BLE_SCAN,
  STATE_FLOCK_DETECTOR,
  STATE_CAPTIVE_PORTAL,
  STATE_PACKET_MONITOR,
  STATE_WEBSERVER,
  // Security Testing States
  STATE_SEC_DEAUTH_TARGET,
  STATE_SEC_DEAUTH_ALL,
  STATE_SEC_BEACON_FLOOD,
  STATE_SEC_PROBE_FLOOD,
  STATE_SEC_PMKID_CAPTURE,
  STATE_CC1_SINGLE,
  STATE_CC2_SINGLE,
  STATE_REC_RAW,
  STATE_PLAY_RAW,
  STATE_SHOW_RAW,
  STATE_SHOW_BUFF,
  STATE_FLUSH_BUFF,
  STATE_GET_RSSI,
  STATE_STOP_ALL,
  STATE_RESET_CC,
  STATE_FREQ_PRESET,
  STATE_SETTINGS,
  STATE_TEST_NRF,
  STATE_TEST_NRF_5,
  STATE_TEST_CC1101,
  // V3: Bridge status check
  STATE_BRIDGE_STATUS
};

// ============================================================================
// Menu Items
// ============================================================================

enum MenuItem {
  BT_JAM,
  DRONE_JAM,
  WIFI_JAM,
  CC1_JAM,
  CC_SCAN,
  NRF_SCAN,
  WIFI_SCAN,
  WIFI_HEATMAP,
  BLE_SCAN,
  FLOCK_DETECTOR,
  CAPTIVE_PORTAL,
  PACKET_MONITOR,
  WEBSERVER_ON,
  WEBSERVER_OFF,
  WEBSERVER_STATUS,
  // Security Testing Menu Items
  SEC_DEAUTH_TARGET,
  SEC_DEAUTH_ALL,
  SEC_BEACON_FLOOD,
  SEC_PROBE_FLOOD,
  SEC_PMKID_CAPTURE,
  TEST_NRF,
  TEST_CC1101,
  TEST_HSPI,
  CC1_SINGLE,
  CC2_SINGLE,
  REC_RAW,
  PLAY_RAW,
  SHOW_RAW,
  SHOW_BUFF,
  GET_RSSI,
  FLUSH_BUFF,
  STOP_ALL,
  RESET_CC,
  SET_43440,
  SET_43430,
  SET_43400,
  SET_43390,
  SETTINGS,
  HELP,
  // V3: Bridge status menu item
  BRIDGE_STATUS,
  NUM_MENU_ITEMS
};

// ============================================================================
// Bridge State (V3 - tracks CC1101 Bridge ESP32 status)
// ============================================================================

enum BridgeState {
  BRIDGE_UNKNOWN,      // Haven't communicated yet
  BRIDGE_OFFLINE,      // No response to PING
  BRIDGE_ONLINE,       // Responsive, ready
  BRIDGE_JAMMING,      // Currently jamming
  BRIDGE_SCANNING,     // Currently scanning
  BRIDGE_ERROR         // Error state
};

// ============================================================================
// Signal Information Structure
// ============================================================================

struct SignalInfo {
  float frequency;
  float rssi;
  unsigned long timestamp;
};

// ============================================================================
// V3-Specific Types
// ============================================================================

enum OperationStatus {
  OP_IDLE,
  OP_RUNNING,
  OP_PAUSED,
  OP_COMPLETE,
  OP_ERROR
};

struct RadioConfig {
  uint8_t cePin;
  uint8_t csPin;
  uint8_t spiChannel;
};

struct DisplayRegion {
  uint16_t x, y, width, height;
  bool dirty;
};

struct ButtonState {
  uint8_t pin;
  bool currentState;
  bool lastState;
  uint32_t lastChangeTime;
  bool longPressHandled;
};

enum ButtonEvent {
  BTN_NONE,
  BTN_UP_PRESS,
  BTN_UP_LONG,
  BTN_DOWN_PRESS,
  BTN_DOWN_LONG,
  BTN_SELECT_PRESS,
  BTN_SELECT_LONG
};

enum RadioCommandType {
  RADIO_CMD_NRF24_TX,
  RADIO_CMD_NRF24_RX,
  RADIO_CMD_NRF24_SCAN,
  RADIO_CMD_CC1101_TX,
  RADIO_CMD_CC1101_RX,
  RADIO_CMD_CC1101_SCAN
};

struct RadioCommand {
  RadioCommandType type;
  uint8_t radioId;
  uint8_t channel;
  uint32_t frequency;
  uint8_t data[64];
  uint8_t length;
  int16_t result;
};

// ============================================================================
// Bridge Command/Response Types (V3 UART Protocol)
// ============================================================================

enum BridgeCommandType {
  BRIDGE_CMD_JAM_START,
  BRIDGE_CMD_JAM_STOP,
  BRIDGE_CMD_JAM1_START,
  BRIDGE_CMD_JAM2_START,
  BRIDGE_CMD_SCAN_START,
  BRIDGE_CMD_SCAN_STOP,
  BRIDGE_CMD_SET_FREQ,
  BRIDGE_CMD_GET_RSSI,
  BRIDGE_CMD_INIT,
  BRIDGE_CMD_RESET,
  BRIDGE_CMD_GET_STATUS,
  BRIDGE_CMD_PING
};

enum BridgeResponseType {
  BRIDGE_RESP_OK,
  BRIDGE_RESP_ERROR,
  BRIDGE_RESP_RSSI,
  BRIDGE_RESP_SCAN_RESULT,
  BRIDGE_RESP_SCAN_DONE,
  BRIDGE_RESP_STATUS,
  BRIDGE_RESP_PONG,
  BRIDGE_RESP_TIMEOUT
};

#endif // TYPES_H
