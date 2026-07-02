// Project Starbeam V3 - Master ESP32 Firmware
// Controls: 5× nRF24L01+PA+LNA, OLED, WiFi/BT, Web Server
// CC1101 radios are on Bridge ESP32, accessed via UART proxy

#include <Wire.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// Project modules
#include "config.h"
#include "types.h"

// Reuse V2 source modules (unchanged for nRF24, display, input, etc.)
// Copy these from starbeam_v2/src/:
#include "src/display.h"
#include "src/display.cpp"
#include "src/input.h"
#include "src/input.cpp"
#include "src/util.h"
#include "src/util.cpp"
#include "src/settings.h"
#include "src/settings.cpp"
#include "src/nrf24.h"
#include "src/nrf24.cpp"
#include "src/recording.h"
#include "src/recording.cpp"
#include "src/analyzer.h"
#include "src/analyzer.cpp"
#include "src/wifi_scanner.h"
#include "src/wifi_scanner.cpp"
#include "src/ble_scanner.h"
#include "src/ble_scanner.cpp"
#include "src/flock_detector.h"
#include "src/flock_detector.cpp"
#include "src/captive_portal.h"
#include "src/captive_portal.cpp"
#include "src/packet_monitor.h"
#include "src/packet_monitor.cpp"
#include "src/webserver.h"
#include "src/webserver.cpp"
#include "src/wifi_attack.h"
#include "src/wifi_attack.cpp"
#include "src/terminal.h"
#include "src/terminal.cpp"

// V3: CC1101 UART Proxy (replaces direct CC1101 driver)
#include "src/cc1101_uart.h"
#include "src/cc1101_uart.cpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ============================================================================
// Global State Variables
// ============================================================================

AppState currentState = STATE_MENU;
MenuItem selectedMenuItem = BT_JAM;
int firstVisibleMenuItem = 0;

// CC1101 jamming state
bool jammingMode = false;
byte ccSendBuffer[64] = {0};

// V3: Bridge status
unsigned long lastBridgePing = 0;
#define BRIDGE_PING_INTERVAL 10000  // Ping bridge every 10 seconds

// ============================================================================
// Helper Functions
// ============================================================================

void nonBlockingDelay(unsigned long ms) {
    Util::coopDelay(ms);
}

static void runFreqPreset(float freqMhz) {
    char l1[20], l2[20];
    snprintf(l1, sizeof(l1), "%.2f MHz set", freqMhz);
    snprintf(l2, sizeof(l2), "[SEL] back");
    CC1101Radio::setMhz(freqMhz);
    Settings::setFreq(freqMhz);
    Serial.printf("[freq] set to %.2f MHz (persisted)\n", freqMhz);
    Display::displayInfo("Frequency", l1, "Saved to flash", l2);
    Util::waitForSelectOrStop();
    Terminal::clearStopFlag();
}

static void runSettingsMode() {
    while (true) {
        char l1[24], l2[24], l3[24];
        snprintf(l1, sizeof(l1), "Freq:%.2f MHz", Settings::freq());
        snprintf(l2, sizeof(l2), "Echo:%s  Verb:%s",
                 Settings::echo() ? "ON" : "off",
                 Settings::verbose() ? "ON" : "off");
        snprintf(l3, sizeof(l3), "Boots:%u", (unsigned)Settings::bootCount());
        Display::displayInfo("Settings", l1, l2, l3);
        if (Input::isButtonPressed(BUTTON_UP)) {
            Settings::setEcho(!Settings::echo());
            Terminal::setEchoEnabled(Settings::echo());
            vTaskDelay(pdMS_TO_TICKS(150));
        } else if (Input::isButtonPressed(BUTTON_DOWN)) {
            Settings::setVerbose(!Settings::verbose());
            Terminal::setVerbose(Settings::verbose());
            vTaskDelay(pdMS_TO_TICKS(150));
        } else if (Input::isButtonPressed(BUTTON_SELECT)) {
            if (Util::isLongPressSelect(LONG_PRESS_MS)) {
                Settings::resetDefaults();
                Terminal::setEchoEnabled(Settings::echo());
                Terminal::setVerbose(Settings::verbose());
                Display::displayInfo("Settings", "Reset to defaults", "", "[SEL] back");
                Util::waitForSelectOrStop();
            }
            Terminal::clearStopFlag();
            return;
        }
        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ============================================================================
// V3: Bridge Status Display
// ============================================================================

static void runBridgeStatusMode() {
    while (true) {
        BridgeState state = CC1101Radio::getBridgeState();
        const char* stateStr = "UNKNOWN";
        switch (state) {
            case BRIDGE_UNKNOWN:  stateStr = "UNKNOWN"; break;
            case BRIDGE_OFFLINE:  stateStr = "OFFLINE"; break;
            case BRIDGE_ONLINE:   stateStr = "ONLINE"; break;
            case BRIDGE_JAMMING:  stateStr = "JAMMING"; break;
            case BRIDGE_SCANNING: stateStr = "SCANNING"; break;
            case BRIDGE_ERROR:    stateStr = "ERROR"; break;
        }
        
        // Try to ping bridge
        bool alive = CC1101UARTProxy::ping();
        
        char l1[24], l2[24], l3[24];
        snprintf(l1, sizeof(l1), "Bridge: %s", stateStr);
        snprintf(l2, sizeof(l2), "Ping: %s", alive ? "OK" : "FAIL");
        snprintf(l3, sizeof(l3), "[SEL] back");
        Display::displayInfo("CC1101 Bridge", l1, l2, l3);
        
        if (Input::isButtonPressed(BUTTON_SELECT) || Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            return;
        }
        
        // Auto-refresh
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============================================================================
// V3: CC1101 Scan with UART Results
// ============================================================================

static void runCCScanMode() {
    Serial.println("CC_SCAN selected - via Bridge");
    Display::displayInfo("CC_SCAN", "Bridge scan...", "Starting....", "");
    
    // Clear previous results
    CC1101Radio::clearScanResults();
    
    // Start scan on bridge
    CC1101Radio::scan(433.60, 434.20);
    
    // Wait and collect results
    unsigned long scanStart = millis();
    int resultCount = 0;
    
    while (true) {
        // Process incoming scan data from bridge
        CC1101Radio::processIncoming();
        
        // Get current results
        int count = 0;
        SignalInfo* results = CC1101Radio::getScanResults(&count);
        
        // Display latest result
        if (count > resultCount) {
            resultCount = count;
            char l1[24], l2[24], l3[24];
            snprintf(l1, sizeof(l1), "Scanning... %d pts", count);
            if (count > 0) {
                snprintf(l2, sizeof(l2), "%.3f MHz", results[count-1].frequency);
                snprintf(l3, sizeof(l3), "RSSI: %.1f dBm", results[count-1].rssi);
            } else {
                strcpy(l2, "");
                strcpy(l3, "");
            }
            Display::displayInfo("CC1101 Scan", l1, l2, l3);
        }
        
        // Timeout or stop
        if ((millis() - scanStart) > 15000) {
            CC1101UARTProxy::stopScan();
            break;
        }
        
        if (Input::isButtonPressed(BUTTON_SELECT) || Terminal::stopRequested()) {
            CC1101UARTProxy::stopScan();
            Terminal::clearStopFlag();
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // Show final results
    int count = 0;
    SignalInfo* results = CC1101Radio::getScanResults(&count);
    char l1[24], l2[24];
    snprintf(l1, sizeof(l1), "Found %d points", count);
    snprintf(l2, sizeof(l2), "[SEL] back");
    Display::displayInfo("Scan Complete", l1, "", l2);
    
    // Print results to serial
    Serial.printf("\n=== CC1101 Scan Results (%d points) ===\n", count);
    for (int i = 0; i < count; i++) {
        Serial.printf("  %.3f MHz: %.1f dBm\n", results[i].frequency, results[i].rssi);
    }
    Serial.println("=====================================\n");
    
    while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    Terminal::clearStopFlag();
}

// ============================================================================
// Web Server, Security Testing, and Other Modes
// (Identical to V2 - unchanged)
// ============================================================================

void runWebServerMode() {
    bool wifiScanActive = false;
    bool bleScanActive = false;
    unsigned long lastWiFiScan = 0;
    unsigned long lastBLEScan = 0;

    if (!StarbeamWebServer::isRunning()) {
        StarbeamWebServer::start();
        delay(2000);
    }

    while (true) {
        if (wifiScanActive && (millis() - lastWiFiScan >= 10000)) {
            if (WiFiScanner::isInitialized()) {
                WiFiScanner::scanNetworks();
                lastWiFiScan = millis();
            } else {
                WiFiScanner::init();
            }
        }

        if (bleScanActive && (millis() - lastBLEScan >= 10000)) {
            if (!BLEScanner::isInitialized()) {
                BLEScanner::init();
            }
            BLEScanner::performScan();
            lastBLEScan = millis();
        }

        String line1 = "WEB SERVER: ON";
        String line2 = "WiFi: " + String(wifiScanActive ? "ON" : "OFF") +
                       " (" + String(WiFiScanner::getNetworkCount()) + ")";
        String line3 = "BLE: " + String(bleScanActive ? "ON" : "OFF") +
                       " (" + String(BLEScanner::getDeviceCount()) + ")";
        String line4 = "[UP]=WiFi [DN]=BLE";
        Display::displayInfo(line1, line2, line3, line4);

        if (Input::isButtonPressed(BUTTON_UP)) {
            wifiScanActive = !wifiScanActive;
            if (!wifiScanActive && WiFiScanner::isInitialized()) {
                WiFiScanner::deinit();
            }
            delay(200);
        }

        if (Input::isButtonPressed(BUTTON_DOWN)) {
            bleScanActive = !bleScanActive;
            if (!bleScanActive && BLEScanner::isInitialized()) {
                BLEScanner::deinit();
            }
            delay(200);
        }

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    StarbeamWebServer::stop();
                    if (WiFiScanner::isInitialized()) WiFiScanner::deinit();
                    if (BLEScanner::isInitialized()) BLEScanner::deinit();
                    return;
                }
                yield();
            }
        }

        StarbeamWebServer::handleClient();
        yield();
    }
}

void runCaptivePortalMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(300);

    int selectedTemplate = 0;
    while (true) {
        Display::displayInfo(
            "Captive Portal",
            String(selectedTemplate + 1) + "/" +
                String(CaptivePortal::getTemplateCount()) + ": " +
                String(CaptivePortal::getTemplateName(selectedTemplate)),
            "[UP/DN]=Template",
            "[SEL]=Start"
        );

        if (Input::isButtonPressed(BUTTON_UP)) {
            selectedTemplate = (selectedTemplate + 1) % CaptivePortal::getTemplateCount();
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            selectedTemplate = (selectedTemplate - 1 + CaptivePortal::getTemplateCount()) %
                               CaptivePortal::getTemplateCount();
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            break;
        }
        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            return;
        }
        yield();
    }

    CaptivePortal::init();
    CaptivePortal::start(selectedTemplate);

    while (CaptivePortal::isRunning()) {
        CaptivePortal::handleClient();

        String line2 = CaptivePortal::getActiveSSID().substring(0, 16);
        String line3 = "Cap:" + String(CaptivePortal::getCaptureCount()) +
                       " Clients:" + String(CaptivePortal::getConnectedClients());
        Display::displayInfo("Portal ACTIVE", line2, line3, "[HOLD SEL]=Stop");

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    Serial.println("Captured credentials:");
                    CaptivePortal::printCapturesToSerial();
                    CaptivePortal::stop();
                    break;
                }
                yield();
            }
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            Serial.println("Captive portal stopped via terminal");
            CaptivePortal::printCapturesToSerial();
            CaptivePortal::stop();
            break;
        }

        yield();
    }
}

// Security testing modes (identical to V2)
void runDeauthTargetMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    Display::displayInfo("DEAUTH TARGET", "Scanning...", "", "");
    int networkCount = WiFi.scanNetworks();

    if (networkCount == 0) {
        Display::displayInfo("DEAUTH TARGET", "No networks found", "", "[SELECT] Exit");
        while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
        return;
    }

    int selectedNetwork = 0;
    while (true) {
        String line1 = "Select Target:";
        String line2 = WiFi.SSID(selectedNetwork);
        String line3 = "Ch:" + String(WiFi.channel(selectedNetwork)) +
                       " RSSI:" + String(WiFi.RSSI(selectedNetwork));
        String line4 = "[UP/DN][SEL]Start";
        Display::displayInfo(line1, line2, line3, line4);

        if (Input::isButtonPressed(BUTTON_UP)) {
            selectedNetwork = (selectedNetwork + 1) % networkCount;
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            selectedNetwork = (selectedNetwork - 1 + networkCount) % networkCount;
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) break;
        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            return;
        }
        yield();
    }

    WiFiAttack::init();
    WiFiAttack::startDeauthTargeted(selectedNetwork, 7);

    while (WiFiAttack::isAttacking()) {
        String line1 = "DEAUTH ACTIVE";
        String line2 = WiFi.SSID(selectedNetwork);
        String line3 = "Frames: " + String(WiFiAttack::getFramesSent());
        String line4 = "Clients: " + String(WiFiAttack::getStationsEliminated());
        Display::displayInfo(line1, line2, line3, line4);

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    WiFiAttack::stopAttack();
                    break;
                }
                yield();
            }
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            WiFiAttack::stopAttack();
            break;
        }
        yield();
    }

    WiFiAttack::deinit();
}

void runDeauthBroadcastMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    Display::displayInfo("WARNING!", "This will disrupt", "ALL nearby WiFi", "[SEL] Confirm");
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    WiFiAttack::init();
    WiFiAttack::startDeauthBroadcast(7);

    while (WiFiAttack::isAttacking()) {
        String line1 = "DEAUTH ALL";
        String line2 = "Channel: " + String(WiFiAttack::getCurrentChannel());
        String line3 = "Frames: " + String(WiFiAttack::getFramesSent());
        String line4 = "[HOLD SEL] Stop";
        Display::displayInfo(line1, line2, line3, line4);

        static unsigned long lastHop = 0;
        if (millis() - lastHop > 10) {
            WiFiAttack::channelHop();
            lastHop = millis();
        }

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    WiFiAttack::stopAttack();
                    break;
                }
                yield();
            }
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            WiFiAttack::stopAttack();
            break;
        }
        yield();
    }

    WiFiAttack::deinit();
}

void runBeaconFloodMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    const char* fake_ssids[20] = {
        "Free WiFi", "Airport WiFi", "Guest Network", "Starbucks WiFi",
        "xfinitywifi", "ATT WiFi", "Verizon WiFi", "Public WiFi",
        "Hotel Guest", "Conference WiFi", "Open Network", "Guest",
        "Visitor WiFi", "Free Internet", "WiFi Access", "Network",
        "Internet", "WiFi", "Wireless", "Public"
    };

    WiFiAttack::init();
    WiFiAttack::startBeaconFlood(fake_ssids, 20);

    while (WiFiAttack::isAttacking()) {
        String line1 = "BEACON FLOOD";
        String line2 = "APs: 20 | Ch: 6";
        String line3 = "Sent: " + String(WiFiAttack::getFramesSent());
        String line4 = "[HOLD SEL] Stop";
        Display::displayInfo(line1, line2, line3, line4);

        for (int i = 0; i < 20 && WiFiAttack::isAttacking(); i++) {
            WiFiAttack::buildBeaconFrame(i);
            delay(10);
            if (i % 10 == 0) yield();
        }

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    WiFiAttack::stopAttack();
                    break;
                }
                yield();
            }
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            WiFiAttack::stopAttack();
            break;
        }
    }

    WiFiAttack::deinit();
}

void runProbeFloodMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    WiFiAttack::init();
    WiFiAttack::startProbeFlood("");

    while (WiFiAttack::isAttacking()) {
        String line1 = "PROBE FLOOD";
        String line2 = "Mode: Wildcard";
        String line3 = "Sent: " + String(WiFiAttack::getFramesSent());
        String line4 = "[HOLD SEL] Stop";
        Display::displayInfo(line1, line2, line3, line4);

        for (int i = 0; i < PROBE_BURST_COUNT && WiFiAttack::isAttacking(); i++) {
            WiFiAttack::buildProbeFrame();
        }
        delay(2);

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            unsigned long pressStart = millis();
            while (Input::isButtonPressed(BUTTON_SELECT)) {
                if (millis() - pressStart > 1000) {
                    WiFiAttack::stopAttack();
                    break;
                }
                yield();
            }
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            WiFiAttack::stopAttack();
            break;
        }
        yield();
    }

    WiFiAttack::deinit();
}

void runPMKIDCaptureMode() {
    Display::displayLegalWarning();
    while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
    delay(500);

    Display::displayInfo("PMKID CAPTURE", "Scanning...", "", "");
    int networkCount = WiFi.scanNetworks();

    int wpa2Indices[20];
    int wpa2Count = 0;
    for (int i = 0; i < networkCount && wpa2Count < 20; i++) {
        wifi_auth_mode_t auth = WiFi.encryptionType(i);
        if (auth >= WIFI_AUTH_WPA2_PSK) {
            wpa2Indices[wpa2Count++] = i;
        }
    }

    if (wpa2Count == 0) {
        Display::displayInfo("PMKID CAPTURE", "No WPA2/3 APs", "", "[SEL] Exit");
        while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
        return;
    }

    int selectedIdx = 0;
    while (true) {
        int netIdx = wpa2Indices[selectedIdx];
        String line1 = "Select Target:";
        String line2 = WiFi.SSID(netIdx);
        String line3 = "Ch:" + String(WiFi.channel(netIdx));
        String line4 = "[UP/DN][SEL]";
        Display::displayInfo(line1, line2, line3, line4);

        if (Input::isButtonPressed(BUTTON_UP)) {
            selectedIdx = (selectedIdx + 1) % wpa2Count;
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            selectedIdx = (selectedIdx - 1 + wpa2Count) % wpa2Count;
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) break;
        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            return;
        }
        yield();
    }

    WiFiAttack::init();
    WiFiAttack::startPMKIDCapture(wpa2Indices[selectedIdx]);

    unsigned long startTime = millis();
    while (WiFiAttack::isAttacking()) {
        const pmkid_capture_t* data = WiFiAttack::getPMKIDData();

        if (data->valid) {
            Display::displayInfo("PMKID CAPTURED!", data->ssid, "Check Serial", "[SEL] Exit");
            while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
            break;
        }

        unsigned long elapsed = (millis() - startTime) / 1000;
        String line1 = "PMKID CAPTURE";
        String line2 = "Listening...";
        String line3 = "Time: " + String(elapsed) + "s";
        String line4 = "[SEL] Cancel";
        Display::displayInfo(line1, line2, line3, line4);

        if (elapsed > 120) {
            Display::displayInfo("TIMEOUT", "No handshake", "", "[SEL] Exit");
            while (!Input::isButtonPressed(BUTTON_SELECT)) yield();
            break;
        }

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            WiFiAttack::stopAttack();
            break;
        }

        if (Terminal::stopRequested()) {
            Terminal::clearStopFlag();
            WiFiAttack::stopAttack();
            break;
        }
        yield();
    }

    WiFiAttack::deinit();
}

// ============================================================================
// Menu Execution (Modified for V3 - CC1101 via UART proxy)
// ============================================================================

void executeSelectedMenuItem() {
    switch (selectedMenuItem) {
        case BT_JAM:
            currentState = STATE_BT_JAM;
            Serial.println("BT JAM selected");
            Display::displayInfo("BT JAMMER", "TURN ON LEFT SWITCH", "Starting....", "");
            NRF24Radio::initVSPI();
            NRF24Radio::initHSPI();
            nonBlockingDelay(2000);
            Display::displayInfo("BT JAMMER", "RADIOS ACTIVE", "Running....", "");
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                NRF24Radio::btJam();
            }
            if (Terminal::stopRequested()) {
                Terminal::clearStopFlag();
                Serial.println("BT JAM stopped via terminal");
            }
            break;

        case WIFI_JAM:
            currentState = STATE_WIFI_JAM;
            Serial.println("WIFI JAM selected");
            Display::displayInfo("WIFI JAMMER", "TURN ON LEFT SWITCH", "Starting....", "");
            NRF24Radio::initVSPI();
            NRF24Radio::initHSPI();
            nonBlockingDelay(2000);
            Display::displayInfo("WIFI JAMMER", "RADIOS ACTIVE", "Running....", "");
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                NRF24Radio::wifiJam();
            }
            if (Terminal::stopRequested()) {
                Terminal::clearStopFlag();
                Serial.println("WIFI JAM stopped via terminal");
            }
            break;

        case DRONE_JAM:
            currentState = STATE_DRONE_JAM;
            Serial.println("DRONE JAM selected");
            Display::displayInfo("DRONE JAMMER", "TURN ON LEFT SWITCH", "Starting....", "");
            NRF24Radio::initVSPI();
            NRF24Radio::initHSPI();
            nonBlockingDelay(2000);
            Display::displayInfo("DRONE JAMMER", "RADIOS ACTIVE", "Running....", "");
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                NRF24Radio::droneJam();
            }
            if (Terminal::stopRequested()) {
                Terminal::clearStopFlag();
                Serial.println("DRONE JAM stopped via terminal");
            }
            break;

        case NRF_SCAN:
            currentState = STATE_NRF_SCAN;
            Serial.println("NRF_SCAN selected");
            Display::displayInfo("NRF24 SCAN", "TURN ON LEFT SWITCH", "Scanning....", "");
            analyzerSetup();
            nonBlockingDelay(4000);
            break;

        case WIFI_SCAN:
            currentState = STATE_WIFI_SCAN;
            Serial.println("WIFI_SCAN selected");
            WiFiScanner::runScanner();
            break;

        case WIFI_HEATMAP:
            currentState = STATE_WIFI_HEATMAP;
            Serial.println("WIFI_HEATMAP selected");
            WiFiScanner::runHeatmap();
            break;

        case BLE_SCAN:
            currentState = STATE_BLE_SCAN;
            Serial.println("BLE_SCAN selected");
            BLEScanner::init();
            BLEScanner::runScanner();
            BLEScanner::deinit();
            break;

        case FLOCK_DETECTOR:
            currentState = STATE_FLOCK_DETECTOR;
            Serial.println("FLOCK_DETECTOR selected");
            FlockDetector::runDetector();
            break;

        case CAPTIVE_PORTAL:
            currentState = STATE_CAPTIVE_PORTAL;
            Serial.println("CAPTIVE_PORTAL selected");
            runCaptivePortalMode();
            break;

        case PACKET_MONITOR:
            currentState = STATE_PACKET_MONITOR;
            Serial.println("PACKET_MONITOR selected");
            PacketMonitor::runMonitor();
            break;

        case WEBSERVER_ON:
            currentState = STATE_WEBSERVER;
            Serial.println("WEBSERVER_ON selected");
            if (!StarbeamWebServer::isRunning()) {
                StarbeamWebServer::start();
                Display::displayInfo("Web Server", "Starting...", "", "");
                delay(1000);
                Display::displayInfo("Web Server", "STARTED", "IP: " + StarbeamWebServer::getIP(), "[SELECT] Back");
                while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) yield();
            } else {
                Display::displayInfo("Web Server", "Already running", "IP: " + StarbeamWebServer::getIP(), "[SELECT] Back");
                while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) yield();
            }
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case WEBSERVER_OFF:
            currentState = STATE_WEBSERVER;
            Serial.println("WEBSERVER_OFF selected");
            if (StarbeamWebServer::isRunning()) {
                StarbeamWebServer::stop();
                Display::displayInfo("Web Server", "Stopping...", "", "");
                delay(500);
                Display::displayInfo("Web Server", "STOPPED", "", "[SELECT] Back");
                while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) yield();
            } else {
                Display::displayInfo("Web Server", "Not running", "", "[SELECT] Back");
                while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) yield();
            }
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case WEBSERVER_STATUS:
            currentState = STATE_WEBSERVER;
            Serial.println("WEBSERVER_STATUS selected");
            if (StarbeamWebServer::isRunning()) {
                Display::displayInfo("Web Server", "Status: RUNNING", "IP: " + StarbeamWebServer::getIP(), "[SELECT] Back");
            } else {
                Display::displayInfo("Web Server", "Status: STOPPED", "", "[SELECT] Back");
            }
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) yield();
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case SEC_DEAUTH_TARGET:
            currentState = STATE_SEC_DEAUTH_TARGET;
            Serial.println("SEC_DEAUTH_TARGET selected");
            runDeauthTargetMode();
            break;

        case SEC_DEAUTH_ALL:
            currentState = STATE_SEC_DEAUTH_ALL;
            Serial.println("SEC_DEAUTH_ALL selected");
            runDeauthBroadcastMode();
            break;

        case SEC_BEACON_FLOOD:
            currentState = STATE_SEC_BEACON_FLOOD;
            Serial.println("SEC_BEACON_FLOOD selected");
            runBeaconFloodMode();
            break;

        case SEC_PROBE_FLOOD:
            currentState = STATE_SEC_PROBE_FLOOD;
            Serial.println("SEC_PROBE_FLOOD selected");
            runProbeFloodMode();
            break;

        case SEC_PMKID_CAPTURE:
            currentState = STATE_SEC_PMKID_CAPTURE;
            Serial.println("SEC_PMKID_CAPTURE selected");
            runPMKIDCaptureMode();
            break;

        case TEST_NRF:
            currentState = STATE_TEST_NRF;
            Serial.println("TEST_NRF selected");
            Display::displayInfo("NRF24 TEST", "TURN ON LEFT SWITCH", "Starting....", "");
            NRF24Radio::initVSPI();
            nonBlockingDelay(2000);
            break;

        case TEST_CC1101:
            currentState = STATE_TEST_CC1101;
            Serial.println("TEST_CC1101 selected");
            Display::displayInfo("CC1101 TEST", "Check Bridge...", "", "");
            if (CC1101UARTProxy::ping()) {
                Display::displayInfo("CC1101 TEST", "Bridge ONLINE!", "", "");
            } else {
                Display::displayInfo("CC1101 TEST", "Bridge OFFLINE!", "Check wiring", "");
            }
            nonBlockingDelay(2000);
            break;

        case TEST_HSPI:
            currentState = STATE_TEST_NRF_5;
            Serial.println("TEST_HSPI selected");
            Display::displayInfo("HSPI TEST", "TURN ON LEFT SWITCH", "Starting....", "");
            NRF24Radio::initHSPI();
            nonBlockingDelay(2000);
            break;

        // V3: CC1101 jamming via Bridge UART
        case CC1_JAM:
            currentState = STATE_CC1_JAM;
            Serial.println("CC1 JAM selected (Bridge)");
            Display::displayInfo("433MHz JAMMER", "Bridge: Starting", "", "");
            jammingMode = true;
            CC1101Radio::init();
            CC1101Radio::startJamming();
            while (jammingMode) {
                if (Input::isButtonPressed(BUTTON_SELECT) || Terminal::stopRequested()) {
                    Serial.println("Exiting Jamming Mode");
                    jammingMode = false;
                    CC1101Radio::stopJamming();
                    if (Terminal::stopRequested()) {
                        Terminal::clearStopFlag();
                        Serial.println("CC1 JAM stopped via terminal");
                    }
                    break;
                }
                Display::displayInfo("CC1101 JAMMER", "BRIDGE ACTIVE", "Running....", "");
                CC1101Radio::processIncoming();
                nonBlockingDelay(100);
            }
            break;

        case CC1_SINGLE:
            currentState = STATE_CC1_SINGLE;
            Serial.println("CC1 SINGLE selected (Bridge)");
            Display::displayInfo("CC#1 JAMMER", "Bridge: Starting", "", "");
            jammingMode = true;
            CC1101Radio::init();
            CC1101Radio::startJamming1();
            while (jammingMode) {
                if (Input::isButtonPressed(BUTTON_SELECT) || Terminal::stopRequested()) {
                    jammingMode = false;
                    CC1101Radio::stopJamming();
                    if (Terminal::stopRequested()) {
                        Terminal::clearStopFlag();
                        Serial.println("CC1 SINGLE stopped via terminal");
                    }
                    break;
                }
                Display::displayInfo("CC1101 JAMMER", "CC#1 ACTIVE", "Running....", "");
                CC1101Radio::processIncoming();
                nonBlockingDelay(100);
            }
            break;

        case CC2_SINGLE:
            currentState = STATE_CC2_SINGLE;
            Serial.println("CC2 SINGLE selected (Bridge)");
            Display::displayInfo("CC#2 JAMMER", "Bridge: Starting", "", "");
            jammingMode = true;
            CC1101Radio::init();
            CC1101Radio::startJamming2();
            while (jammingMode) {
                if (Input::isButtonPressed(BUTTON_SELECT) || Terminal::stopRequested()) {
                    jammingMode = false;
                    CC1101Radio::stopJamming();
                    if (Terminal::stopRequested()) {
                        Terminal::clearStopFlag();
                        Serial.println("CC2 SINGLE stopped via terminal");
                    }
                    break;
                }
                Display::displayInfo("CC1101 JAMMER", "CC#2 ACTIVE", "Running....", "");
                CC1101Radio::processIncoming();
                nonBlockingDelay(100);
            }
            break;

        // V3: CC1101 scan via Bridge UART
        case CC_SCAN:
            currentState = STATE_CC_SCAN;
            runCCScanMode();
            break;

        case REC_RAW:
            currentState = STATE_REC_RAW;
            Serial.println("REC_RAW selected");
            Display::displayInfo("REC_RAW", "Recording raw data", "Recording....", "");
            Recording::recordRawData(100);
            break;

        case PLAY_RAW:
            currentState = STATE_PLAY_RAW;
            Serial.println("PLAY_RAW selected");
            Display::displayInfo("PLAY_RAW", "Playing raw data", "Playing....", "");
            Recording::playRawData(100);
            break;

        case SHOW_RAW:
            currentState = STATE_SHOW_RAW;
            Serial.println("SHOW_RAW selected");
            Display::displayInfo("SHOW_RAW", "Showing raw data", "Raw data....", "");
            Recording::showRawData();
            nonBlockingDelay(3000);
            break;

        case SHOW_BUFF:
            currentState = STATE_SHOW_BUFF;
            Serial.println("SHOW_BUFF selected");
            Display::displayInfo("SHOW_BUFF", "Showing buffer", "Bit data....", "");
            Recording::showBitData();
            nonBlockingDelay(3000);
            break;

        case FLUSH_BUFF:
            currentState = STATE_FLUSH_BUFF;
            Serial.println("FLUSH_BUFF selected");
            Display::displayInfo("FLUSH_BUFF", "Clearing buffer", "Clearing....", "");
            Recording::flushBuffer();
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                Display::displayInfo("FLUSH_BUFF", "Buffer cleared!", "Press SELECT", "to return");
            }
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case GET_RSSI:
            currentState = STATE_GET_RSSI;
            Serial.println("GET_RSSI selected (Bridge)");
            {
                float rssi = CC1101Radio::getRssi();
                int lqi = CC1101Radio::getLqi();
                char rssiStr[16];
                char lqiStr[16];
                snprintf(rssiStr, 16, "RSSI: %.1f dBm", rssi);
                snprintf(lqiStr, 16, "LQI: %d", lqi);
                Display::displayInfo("GET_RSSI (Bridge)", rssiStr, lqiStr, "");
                Serial.printf("[Bridge] RSSI: %.1f dBm, LQI: %d\n", rssi, lqi);
            }
            nonBlockingDelay(3000);
            break;

        case STOP_ALL:
            currentState = STATE_STOP_ALL;
            Serial.println("STOP_ALL selected");
            Display::displayInfo("STOP_ALL", "Stopping all modes....", "", "");
            jammingMode = false;
            CC1101Radio::stopJamming();
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                Display::displayInfo("STOP_ALL", "All stopped!", "Press SELECT", "to return");
            }
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case RESET_CC:
            currentState = STATE_RESET_CC;
            Serial.println("RESET_CC selected (Bridge)");
            Display::displayInfo("RESET_CC", "Resetting Bridge...", "", "");
            CC1101Radio::reset();
            while (!Input::isButtonPressed(BUTTON_SELECT) && !Terminal::stopRequested()) {
                Display::displayInfo("RESET_CC", "Bridge reset done!", "Press SELECT", "to return");
            }
            if (Terminal::stopRequested()) Terminal::clearStopFlag();
            break;

        case SET_43440:
            currentState = STATE_FREQ_PRESET;
            runFreqPreset(434.40f);
            break;

        case SET_43430:
            currentState = STATE_FREQ_PRESET;
            runFreqPreset(434.30f);
            break;

        case SET_43400:
            currentState = STATE_FREQ_PRESET;
            runFreqPreset(434.00f);
            break;

        case SET_43390:
            currentState = STATE_FREQ_PRESET;
            runFreqPreset(433.90f);
            break;

        case SETTINGS:
            currentState = STATE_SETTINGS;
            runSettingsMode();
            break;

        // V3: Bridge status check
        case BRIDGE_STATUS:
            currentState = STATE_BRIDGE_STATUS;
            Serial.println("BRIDGE_STATUS selected");
            runBridgeStatusMode();
            break;

        case HELP:
            Display::displayInfo("Help", "Starbeam V3", "Dual ESP32", "7 RF Modules");
            nonBlockingDelay(3000);
            break;

        default:
            Display::displayInfo("Unknown", "Menu item", "not found", "");
            nonBlockingDelay(2000);
            break;
    }
}

// ============================================================================
// Setup & Main Loop
// ============================================================================

static TaskHandle_t s_serialTaskHandle = nullptr;

static void serialBridgeTask(void* /*pv*/) {
    for (;;) {
        Terminal::processInput();
        // V3: Also process incoming data from CC1101 Bridge
        CC1101Radio::processIncoming();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup() {
    Serial.begin(115200);
    Terminal::init();

    Serial.println("\n\n=================================");
    Serial.println("  Project Starbeam V3");
    Serial.println("  Dual ESP32 - 7 RF Modules");
    Serial.println("  Master ESP32 (5x nRF24)");
    Serial.println("=================================");

    delay(1000);

    EEPROM.begin(EEPROM_TOTAL_SIZE);
    Settings::init();
    Terminal::setEchoEnabled(Settings::echo());
    Terminal::setVerbose(Settings::verbose());

    Display::init();

    esp_bt_controller_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    Input::init();
    StarbeamWebServer::init();

    // V3: Initialize CC1101 UART proxy (talks to Bridge ESP32)
    CC1101UARTProxy::init();

    Display::playBootAnimation();

    // V3: Check Bridge status during boot
    Display::displayInfo("V3 Boot", "Checking Bridge...", "", "");
    delay(500);
    
    if (CC1101UARTProxy::ping()) {
        Serial.println("[Boot] CC1101 Bridge: ONLINE");
        Display::displayInfo("CC1101 Bridge", "ONLINE", "2 radios ready", "");
        CC1101UARTProxy::initRadios();
    } else {
        Serial.println("[Boot] CC1101 Bridge: OFFLINE");
        Display::displayInfo("CC1101 Bridge", "OFFLINE", "Check UART wiring", "");
    }
    delay(1500);

    // Apply persisted frequency via proxy
    CC1101Radio::setMhz(Settings::freq());
    Serial.printf("[Boot] Frequency: %.2f MHz\n", Settings::freq());

    Recording::init();

    // Spawn serial + bridge processing task on Core 0
    xTaskCreatePinnedToCore(
        serialBridgeTask, "serial_bridge",
        TASK_STACK_SIZE_INPUT, nullptr,
        PRIORITY_INPUT, &s_serialTaskHandle, CORE_RADIO);

    Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);

    Serial.println("[Boot] Setup complete!");
    Serial.printf("[Boot] Free heap: %lu bytes\n", ESP.getFreeHeap());
}

void loop() {
    // V3: Periodic bridge health check
    if (millis() - lastBridgePing > BRIDGE_PING_INTERVAL) {
        lastBridgePing = millis();
        // Quick ping in background (don't block UI)
        // Actual ping happens via menu or on-demand
    }

    if (Terminal::hasCommand()) {
        MenuItem cmd = Terminal::getCommand();
        Terminal::clearCommand();

        if (currentState != STATE_MENU) {
            Serial.println("Serial command - exiting current operation");
            currentState = STATE_MENU;
            delay(100);
        }

        selectedMenuItem = cmd;
        executeSelectedMenuItem();
        currentState = STATE_MENU;
        Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
        Input::setButtonPressedState(false);
    }

    if (Terminal::stopRequested() && currentState != STATE_MENU) {
        Serial.println("Stop command received");
        Terminal::clearStopFlag();
        jammingMode = false;
        // V3: Also stop bridge jamming
        CC1101Radio::stopJamming();
        currentState = STATE_MENU;
        Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
        return;
    }

    // V3: Process any incoming bridge data in main loop too
    CC1101Radio::processIncoming();

    switch (currentState) {
        case STATE_MENU:
            Input::handleMenuSelection(selectedMenuItem, firstVisibleMenuItem);
            if (Input::getButtonPressedState() && Input::isButtonPressed(BUTTON_SELECT)) {
                Serial.printf("Selected menu item: %d\n", selectedMenuItem);
                executeSelectedMenuItem();
                currentState = STATE_MENU;
                Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
                Input::setButtonPressedState(false);
            }
            break;

        case STATE_NRF_SCAN:
            analyzerLoop();
            if (Input::isButtonPressed(BUTTON_SELECT)) {
                currentState = STATE_MENU;
                Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
                nonBlockingDelay(500);
            }
            break;

        case STATE_BT_JAM:
        case STATE_DRONE_JAM:
        case STATE_WIFI_JAM:
        case STATE_CC1_JAM:
        case STATE_WIFI_SCAN:
        case STATE_WIFI_HEATMAP:
        case STATE_FLOCK_DETECTOR:
        case STATE_CAPTIVE_PORTAL:
        case STATE_PACKET_MONITOR:
        case STATE_CC1_SINGLE:
        case STATE_CC2_SINGLE:
        case STATE_REC_RAW:
        case STATE_PLAY_RAW:
        case STATE_SHOW_RAW:
        case STATE_SHOW_BUFF:
        case STATE_FLUSH_BUFF:
        case STATE_GET_RSSI:
        case STATE_STOP_ALL:
        case STATE_RESET_CC:
        case STATE_FREQ_PRESET:
        case STATE_SETTINGS:
        case STATE_TEST_NRF:
        case STATE_TEST_NRF_5:
        case STATE_TEST_CC1101:
        case STATE_BRIDGE_STATUS:
            if (Input::isButtonPressed(BUTTON_SELECT)) {
                currentState = STATE_MENU;
                Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
                nonBlockingDelay(500);
            }
            break;
    }

    yield();
}
