// config.h - Bridge ESP32 Configuration for Project Starbeam V3
// Dedicated ESP32 controlling 2× CC1101 modules
// Receives commands from Master ESP32 via UART

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// VERSION
// ============================================================================

#define BRIDGE_VERSION_MAJOR 3
#define BRIDGE_VERSION_MINOR 0
#define BRIDGE_VERSION_STRING "V3.0-BRIDGE"

// ============================================================================
// CC1101 Radio Pin Definitions (Bridge ESP32)
// ============================================================================

// CC1101 #1 - SPI
#define CC1101_1_SCK   18
#define CC1101_1_MISO  19
#define CC1101_1_MOSI  23
#define CC1101_1_CS    5
#define CC1101_1_GDO0  4
#define CC1101_1_GDO2  16  // Optional

// CC1101 #2 - SPI (shared bus, different CS/GDO0)
#define CC1101_2_SCK   18
#define CC1101_2_MISO  19
#define CC1101_2_MOSI  23
#define CC1101_2_CS    33
#define CC1101_2_GDO0  32
#define CC1101_2_GDO2  17  // Optional

// ============================================================================
// SPI Bus (dedicated on Bridge ESP32)
// ============================================================================

#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23

// ============================================================================
// UART Configuration (Bridge ↔ Master ESP32)
// ============================================================================

#define UART_BAUD_RATE    115200
#define UART_BUF_SIZE     128
#define CMD_MAX_ARGS      4

// ============================================================================
// Timing
// ============================================================================

#define JAMMING_PACKET_SIZE   60
#define JAMMING_INTERVAL_MS   10
#define SCAN_STEP_MHZ         0.05
#define SCAN_DWELL_MS         50
#define WATCHDOG_TIMEOUT_MS   5000

// ============================================================================
// Debug
// ============================================================================

// Set to 0 to disable debug output (recommended for production)
// Set to 1 to enable debug messages on Serial
#define BRIDGE_DEBUG 1

#if BRIDGE_DEBUG
  #define DBG_PRINT(x) Serial.print(x)
  #define DBG_PRINTLN(x) Serial.println(x)
  #define DBG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#else
  #define DBG_PRINT(x)
  #define DBG_PRINTLN(x)
  #define DBG_PRINTF(fmt, ...)
#endif

#endif // CONFIG_H
