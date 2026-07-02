// cc1101_uart.cpp - CC1101 UART Proxy Implementation for Starbeam V3
// Handles all UART communication with Bridge ESP32

#include "cc1101_uart.h"

// Static member definitions
bool CC1101UARTProxy::initialized = false;
BridgeState CC1101UARTProxy::bridgeState = BRIDGE_UNKNOWN;
bool CC1101UARTProxy::scanning = false;
float CC1101UARTProxy::currentFreq = 433.92f;
SignalInfo CC1101UARTProxy::scanResults[64];
int CC1101UARTProxy::scanResultCount = 0;
int CC1101UARTProxy::scanResultHead = 0;

void CC1101UARTProxy::init() {
    Serial.begin(BRIDGE_BAUD_RATE);
    initialized = true;
    bridgeState = BRIDGE_UNKNOWN;
    scanning = false;
    scanResultCount = 0;
    scanResultHead = 0;
    
    // Clear any pending data
    flush();
    
    // Try to ping bridge
    if (ping()) {
        Serial.println("[Bridge] CC1101 Bridge online");
        bridgeState = BRIDGE_ONLINE;
    } else {
        Serial.println("[Bridge] WARNING: CC1101 Bridge not responding");
        bridgeState = BRIDGE_OFFLINE;
    }
}

bool CC1101UARTProxy::ping() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("PING", response, sizeof(response), 500)) {
        return (strncmp(response, "PONG", 4) == 0);
    }
    return false;
}

bool CC1101UARTProxy::initRadios() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("INIT", response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            bridgeState = BRIDGE_ONLINE;
            return true;
        }
    }
    return false;
}

bool CC1101UARTProxy::checkConnection1() {
    // After INIT, Bridge returns connection status
    // For simplicity, we check via STATUS command
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("STATUS", response, sizeof(response), 500)) {
        return (bridgeState != BRIDGE_OFFLINE && bridgeState != BRIDGE_UNKNOWN);
    }
    return false;
}

bool CC1101UARTProxy::checkConnection2() {
    // Same as checkConnection1 for V3 - both radios on same bridge
    return checkConnection1();
}

void CC1101UARTProxy::setMhz(float freq) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "FREQ:%.2f", freq);
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand(cmd, response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            currentFreq = freq;
        }
    }
}

void CC1101UARTProxy::startJamming() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("JAM:START", response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            bridgeState = BRIDGE_JAMMING;
        }
    }
}

void CC1101UARTProxy::startJamming1() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("JAM1:START", response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            bridgeState = BRIDGE_JAMMING;
        }
    }
}

void CC1101UARTProxy::startJamming2() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("JAM2:START", response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            bridgeState = BRIDGE_JAMMING;
        }
    }
}

void CC1101UARTProxy::stopJamming() {
    char response[BRIDGE_BUF_SIZE];
    // Send multiple times to ensure it gets through
    for (int i = 0; i < 2; i++) {
        if (sendCommand("JAM:STOP", response, sizeof(response), 300)) {
            break;
        }
    }
    bridgeState = BRIDGE_ONLINE;
}

void CC1101UARTProxy::startScan(float beginFreq, float endFreq) {
    clearScanResults();
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "SCAN:START:%.2f:%.2f", beginFreq, endFreq);
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand(cmd, response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            scanning = true;
            bridgeState = BRIDGE_SCANNING;
        }
    }
}

void CC1101UARTProxy::stopScan() {
    char response[BRIDGE_BUF_SIZE];
    sendCommand("SCAN:STOP", response, sizeof(response), 300);
    scanning = false;
    bridgeState = BRIDGE_ONLINE;
}

bool CC1101UARTProxy::isScanning() {
    return scanning;
}

float CC1101UARTProxy::getRssi() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("RSSI", response, sizeof(response), 500)) {
        float rssi = -999.0f;
        int lqi = 0;
        if (sscanf(response, "RSSI:%f:%d", &rssi, &lqi) >= 1) {
            return rssi;
        }
    }
    return -999.0f;
}

int CC1101UARTProxy::getLqi() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("RSSI", response, sizeof(response), 500)) {
        float rssi = -999.0f;
        int lqi = 0;
        if (sscanf(response, "RSSI:%f:%d", &rssi, &lqi) >= 2) {
            return lqi;
        }
    }
    return 0;
}

void CC1101UARTProxy::reset() {
    char response[BRIDGE_BUF_SIZE];
    if (sendCommand("RESET", response, sizeof(response))) {
        if (strncmp(response, "OK", 2) == 0) {
            bridgeState = BRIDGE_ONLINE;
        }
    }
}

BridgeState CC1101UARTProxy::getBridgeState() {
    return bridgeState;
}

void CC1101UARTProxy::processIncoming() {
    // Process unsolicited scan results from Bridge
    while (Serial.available()) {
        char line[BRIDGE_BUF_SIZE];
        size_t len = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
        if (len > 0) {
            line[len] = '\0';
            // Remove trailing \r
            if (len > 0 && line[len - 1] == '\r') {
                line[len - 1] = '\0';
            }
            
            if (strncmp(line, "SCAN:", 5) == 0) {
                if (strcmp(line + 5, "DONE") == 0) {
                    scanning = false;
                    bridgeState = BRIDGE_ONLINE;
                } else {
                    parseScanResult(line);
                }
            } else if (strncmp(line, "STATUS:", 7) == 0) {
                const char* state = line + 7;
                if (strcmp(state, "IDLE") == 0) bridgeState = BRIDGE_ONLINE;
                else if (strcmp(state, "JAMMING") == 0) bridgeState = BRIDGE_JAMMING;
                else if (strcmp(state, "SCANNING") == 0) bridgeState = BRIDGE_SCANNING;
                else if (strcmp(state, "ERROR") == 0) bridgeState = BRIDGE_ERROR;
            }
        }
    }
}

SignalInfo* CC1101UARTProxy::getScanResults(int* count) {
    if (count) *count = scanResultCount;
    return scanResults;
}

void CC1101UARTProxy::clearScanResults() {
    scanResultCount = 0;
    scanResultHead = 0;
}

void CC1101UARTProxy::flush() {
    while (Serial.available()) {
        Serial.read();
    }
}

// Private helpers

bool CC1101UARTProxy::sendCommand(const char* cmd, char* response, size_t respSize, uint32_t timeout) {
    if (!initialized) {
        if (response && respSize > 0) {
            strncpy(response, "ERROR:NOT_INIT", respSize);
        }
        return false;
    }
    
    // Drain any pending input
    flush();
    
    // Send command with newline
    Serial.print(cmd);
    Serial.print('\n');
    Serial.flush();
    
    // Wait for response
    return waitForResponse(response, respSize, timeout);
}

void CC1101UARTProxy::sendRaw(const char* cmd) {
    Serial.print(cmd);
    Serial.print('\n');
    Serial.flush();
}

bool CC1101UARTProxy::waitForResponse(char* buffer, size_t size, uint32_t timeout) {
    if (!buffer || size == 0) return false;
    
    unsigned long start = millis();
    size_t idx = 0;
    
    while ((millis() - start) < timeout) {
        while (Serial.available() && idx < size - 1) {
            char c = Serial.read();
            if (c == '\n') {
                buffer[idx] = '\0';
                // Remove trailing \r
                if (idx > 0 && buffer[idx - 1] == '\r') {
                    buffer[idx - 1] = '\0';
                }
                return true;
            }
            if (c != '\r') {
                buffer[idx++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    buffer[idx] = '\0';
    return (idx > 0); // Return partial data if we got anything
}

void CC1101UARTProxy::parseScanResult(const char* line) {
    float freq = 0;
    float rssi = -999;
    
    // Format: SCAN:<freq>:<rssi>
    if (sscanf(line, "SCAN:%f:%f", &freq, &rssi) == 2) {
        if (scanResultCount < 64) {
            int idx = scanResultHead;
            scanResults[idx].frequency = freq;
            scanResults[idx].rssi = rssi;
            scanResults[idx].timestamp = millis();
            scanResultHead = (scanResultHead + 1) % 64;
            if (scanResultCount < 64) {
                scanResultCount++;
            }
        }
    }
}
