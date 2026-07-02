// config.h - Hardware Configuration for Project Starbeam V3
// Master ESP32 - Controls 5× nRF24L01+PA+LNA + OLED + WiFi/BT/Web
// CC1101 radios are on a separate Bridge ESP32, accessed via UART proxy

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// VERSION
// ============================================================================

#define STARBEAM_VERSION_MAJOR 3
#define STARBEAM_VERSION_MINOR 0
#define STARBEAM_VERSION_STRING "V3.0-DUAL"

// ============================================================================
// NRF24L01 Radio Pin Definitions (Master ESP32)
// ============================================================================
// All 5 nRF24 radios use dedicated pins - no sharing with CC1101

// nRF24 Radio 1-3: VSPI bus
#define NRF24_1_CE  27
#define NRF24_1_CS  15
#define NRF24_2_CE  26
#define NRF24_2_CS  33
#define NRF24_3_CE  25
#define NRF24_3_CS  5

// nRF24 Radio 4-5: HSPI bus
#define NRF24_4_CE  4
#define NRF24_4_CS  2
#define NRF24_5_CE  32
#define NRF24_5_CS  17

// ============================================================================
// SPI Bus Definitions
// ============================================================================

// VSPI (nRF24 radios 1-3)
#define VSPI_SCK   18
#define VSPI_MISO  19
#define VSPI_MOSI  23

// HSPI (nRF24 radios 4-5)
#define HSPI_SCK   14
#define HSPI_MISO  12
#define HSPI_MOSI  13

// ============================================================================
// UART Bridge Configuration (Master ↔ CC1101 Bridge ESP32)
// ============================================================================

#define BRIDGE_UART_NUM     UART_NUM_0  // Uses Serial (TX=GPIO1, RX=GPIO3)
#define BRIDGE_BAUD_RATE    115200
#define BRIDGE_CMD_TIMEOUT  2000    // ms to wait for bridge response
#define BRIDGE_MAX_RETRIES  3       // command retry attempts
#define BRIDGE_BUF_SIZE     128

// ============================================================================
// Display & UI Pin Definitions
// ============================================================================

// SSD1306 OLED Display (I2C)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_I2C_ADDR 0x3C

// LED
#define LED_PIN 16

// Buttons
#define BUTTON_UP     39
#define BUTTON_DOWN   34
#define BUTTON_SELECT 36

// ============================================================================
// Buffer & Memory Sizes
// ============================================================================

#define CCBUFFERSIZE         64
#define RECORDINGBUFFERSIZE  4096
#define EEPROM_SIZE          512
#define EEPROM_TOTAL_SIZE    1024
#define SETTINGS_EEPROM_OFFSET 512
#define SETTINGS_EEPROM_SIZE 64
#define BUF_LENGTH           128
#define MAX_SIGNALS          4

// ============================================================================
// Timing Constants
// ============================================================================

#define DEBOUNCE_MS      50
#define LONG_PRESS_MS    1000
#define DISPLAY_REFRESH_MS  50
#define BUTTON_POLL_MS   10

// ============================================================================
// FreeRTOS Task Configuration
// ============================================================================

#define TASK_STACK_SIZE_UI      4096
#define TASK_STACK_SIZE_INPUT   2048
#define TASK_STACK_SIZE_RADIO   8192
#define TASK_STACK_SIZE_OPS     8192

#define PRIORITY_UI       2
#define PRIORITY_INPUT    3
#define PRIORITY_RADIO    2
#define PRIORITY_OPS      1

#define CORE_RADIO  0
#define CORE_UI     1

// ============================================================================
// Radio Configuration
// ============================================================================

#define RF24_SPI_SPEED  16000000  // 16 MHz
#define RADIO_COUNT_NRF24  5      // 5 nRF24 radios on Master
#define RADIO_COUNT_CC1101 2      // 2 CC1101 radios on Bridge (via UART)

// ============================================================================
// Web Server Configuration
// ============================================================================

#define WEB_SERVER_PORT      80
#define AP_SSID              "StarbeamV3"
#define AP_PASSWORD          "starbeam2024"
#define AP_CHANNEL           1
#define AP_HIDDEN            false
#define AP_MAX_CONNECTIONS   4

#endif // CONFIG_H
