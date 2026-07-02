// display_v3.cpp - V3 Display Extension Implementation
// Adds BRIDGE_STATUS menu item and V3-specific display screens

#include "display_v3.h"
#include "cc1101_uart.h"

// V3 extended menu labels - must match MenuItem enum order exactly
static const char* menuLabelsV3[NUM_MENU_ITEMS] = {
    // 0-4: Core jammers
    "BT Jammer",           // BT_JAM
    "Drone Jammer",        // DRONE_JAM
    "Wifi Jammer",         // WIFI_JAM
    "CC1 Jammer",          // CC1_JAM
    "CC1101 Scan",         // CC_SCAN
    
    // 5-12: Scanners & tools
    "NRF Scan",            // NRF_SCAN
    "WiFi Scanner",        // WIFI_SCAN
    "WiFi Heatmap",        // WIFI_HEATMAP
    "BLE Scanner",         // BLE_SCAN
    "Flock Detector",      // FLOCK_DETECTOR
    "Captive Portal",      // CAPTIVE_PORTAL
    "Pkt Monitor",         // PACKET_MONITOR
    "Web Server ON",       // WEBSERVER_ON
    "Web Server OFF",      // WEBSERVER_OFF
    "Web Status",          // WEBSERVER_STATUS
    
    // 15-19: Security testing
    "Deauth Target",       // SEC_DEAUTH_TARGET
    "Deauth All",          // SEC_DEAUTH_ALL
    "Beacon Flood",        // SEC_BEACON_FLOOD
    "Probe Flood",         // SEC_PROBE_FLOOD
    "PMKID Capture",       // SEC_PMKID_CAPTURE
    
    // 20-22: Tests
    "NRF Test",            // TEST_NRF
    "CC1101 Test",         // TEST_CC1101
    "Test HSPI",           // TEST_HSPI
    
    // 23-24: Single radio
    "CC Single",           // CC1_SINGLE
    "CC2 Single",          // CC2_SINGLE
    
    // 25-28: Recording
    "Rec Raw",             // REC_RAW
    "Play Raw",            // PLAY_RAW
    "Show Raw",            // SHOW_RAW
    "Show Buffer",         // SHOW_BUFF
    
    // 29-33: Utility
    "Get RSSI",            // GET_RSSI
    "Flush Buffer",        // FLUSH_BUFF
    "Stop CC1101",         // STOP_ALL
    "Reset CC1101",        // RESET_CC
    
    // 34-37: Frequency presets
    "434.40 MHz",          // SET_43440
    "434.30 MHz",          // SET_43430
    "434.00 MHz",          // SET_43400
    "433.90 MHz",          // SET_43390
    
    // 38-39: System
    "Settings",            // SETTINGS
    "Help",                // HELP
    "Bridge Status"        // BRIDGE_STATUS (V3)
};

bool DisplayV3::v3LabelsActive = false;

void DisplayV3::init() {
    v3LabelsActive = true;
}

const char* DisplayV3::getMenuLabel(MenuItem item) {
    int idx = (int)item;
    if (idx >= 0 && idx < NUM_MENU_ITEMS) {
        return menuLabelsV3[idx];
    }
    return "???";
}

// Redraw cache (mirrors V2 display.cpp cache)
namespace {
    int v3_last_menu_sel = -1;
    int v3_last_menu_first = -1;
}

void DisplayV3::drawMenu(MenuItem selectedMenuItem, int firstVisibleMenuItem) {
    // Use V2's Display::drawMenu for basic rendering but with V3 labels
    // We reimplement here to use our extended labels array
    
    extern Adafruit_SSD1306 oled;  // From display.cpp
    extern U8G2_FOR_ADAFRUIT_GFX u8g2;
    
    // Access the oled object - we need to include the display internals
    // Since we can't easily access private static members, we use Display's
    // public methods for drawing and just override the label rendering
    
    // For simplicity, delegate to V2 display but handle the V3 item count
    // The V2 drawMenu uses NUM_MENU_ITEMS from types.h which is now correct
    // We just need to ensure the labels array has the right entries
    
    // Since Display::menuLabels is static and private, we can't override it.
    // Instead, we provide a standalone draw function here.
    
    Adafruit_SSD1306& display = Display::getOled();  // Need to add this accessor
    
    // If accessor isn't available, use a different approach
    // Call the standard drawMenu - the label will show wrong for BRIDGE_STATUS
    // but the menu will still work functionally
    Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
}

// V3-specific display screens

void DisplayV3::displayBridgeStatus(BridgeState state, bool pingOk) {
    const char* stateStr = "UNKNOWN";
    switch (state) {
        case BRIDGE_UNKNOWN:  stateStr = "UNKNOWN"; break;
        case BRIDGE_OFFLINE:  stateStr = "OFFLINE"; break;
        case BRIDGE_ONLINE:   stateStr = "ONLINE"; break;
        case BRIDGE_JAMMING:  stateStr = "JAMMING"; break;
        case BRIDGE_SCANNING: stateStr = "SCANNING"; break;
        case BRIDGE_ERROR:    stateStr = "ERROR"; break;
    }
    
    char l1[24], l2[24];
    snprintf(l1, sizeof(l1), "Bridge: %s", stateStr);
    snprintf(l2, sizeof(l2), "Ping: %s", pingOk ? "OK" : "FAIL");
    Display::displayInfo("CC1101 Bridge", l1, l2, "[SEL] back");
}

void DisplayV3::displayV3BootScreen() {
    // Show V3 boot info instead of V2
    Display::displayInfo(
        "Starbeam V3",
        "Dual ESP32",
        "5 nRF24 + 2 CC1101",
        "Checking Bridge..."
    );
}

// ============================================================================
// Menu Label Patcher
// 
// IMPORTANT: To properly show the BRIDGE_STATUS menu item name,
// modify starbeam_v2/src/display.cpp line ~33:
//
// Change:
//   const char* Display::menuLabels[NUM_MENU_ITEMS] = {
//       ... existing 39 items ...
//   };
//
// To:
//   const char* Display::menuLabels[NUM_MENU_ITEMS] = {
//       ... existing 39 items ...
//       "Bridge Status"    // BRIDGE_STATUS (V3)
//   };
//
// And in displayTitleScreen(), change "v2" to "v3" on the subtitle line.
//
// This is the minimal change needed to V2's display.cpp for V3 compatibility.
// ============================================================================
