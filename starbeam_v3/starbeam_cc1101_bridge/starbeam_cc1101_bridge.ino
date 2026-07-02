// Project Starbeam V3 - CC1101 Bridge ESP32
// Dedicated ESP32 for 2× CC1101 modules (300-928 MHz)
// Receives commands from Master ESP32 via UART and controls CC1101 radios

#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

// ============================================================================
// Dual CC1101 Driver Setup
// Uses SmartRC-CC1101-Driver-Lib with two instances
// ============================================================================

// Instance 1 - CC1101 #1
#define ELECHOUSE_CC1101_SRC_DRV cc1101_drv1
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#undef ELECHOUSE_CC1101_SRC_DRV

// Instance 2 - CC1101 #2  
#define ELECHOUSE_CC1101_SRC_DRV cc1101_drv2
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#undef ELECHOUSE_CC1101_SRC_DRV

// ============================================================================
// Bridge State Machine
// ============================================================================

enum BridgeState {
    STATE_IDLE,
    STATE_JAMMING_BOTH,
    STATE_JAMMING_1,
    STATE_JAMMING_2,
    STATE_SCANNING
};

volatile BridgeState bridgeState = STATE_IDLE;
bool cc1Ready = false;
bool cc2Ready = false;
float currentFreq = 433.92f;

// Scan parameters
float scanBeginFreq = 433.60f;
float scanEndFreq = 434.20f;
float scanCurrentFreq = 433.60f;
unsigned long lastScanStep = 0;

// Jamming buffer
byte jamBuffer[JAMMING_PACKET_SIZE];

// UART command buffer
char cmdBuffer[UART_BUF_SIZE];
uint8_t cmdIndex = 0;

// ============================================================================
// CC1101 Control Functions
// ============================================================================

bool initCC1101_1() {
    DBG_PRINTLN("[CC1] Initializing...");
    
    cc1101_drv1.setSpiPin(CC1101_1_SCK, CC1101_1_MISO, CC1101_1_MOSI, CC1101_1_CS);
    cc1101_drv1.setGDO(CC1101_1_GDO0, CC1101_1_GDO2);
    
    if (cc1101_drv1.getCC1101()) {
        cc1101_drv1.setMHZ(currentFreq);
        cc1101_drv1.SetRx();
        DBG_PRINTF("[CC1] OK @ %.2f MHz\n", currentFreq);
        return true;
    } else {
        DBG_PRINTLN("[CC1] FAILED!");
        return false;
    }
}

bool initCC1101_2() {
    DBG_PRINTLN("[CC2] Initializing...");
    
    cc1101_drv2.setSpiPin(CC1101_2_SCK, CC1101_2_MISO, CC1101_2_MOSI, CC1101_2_CS);
    cc1101_drv2.setGDO(CC1101_2_GDO0, CC1101_2_GDO2);
    
    if (cc1101_drv2.getCC1101()) {
        cc1101_drv2.setMHZ(currentFreq);
        cc1101_drv2.SetRx();
        DBG_PRINTF("[CC2] OK @ %.2f MHz\n", currentFreq);
        return true;
    } else {
        DBG_PRINTLN("[CC2] FAILED!");
        return false;
    }
}

void setFrequency(float freq) {
    currentFreq = freq;
    if (cc1Ready) {
        cc1101_drv1.setMHZ(freq);
        cc1101_drv1.SetRx();
    }
    if (cc2Ready) {
        cc1101_drv2.setMHZ(freq);
        cc1101_drv2.SetRx();
    }
    DBG_PRINTF("[Freq] Set to %.2f MHz\n", freq);
}

void resetCC1101s() {
    bridgeState = STATE_IDLE;
    cc1Ready = initCC1101_1();
    delay(100);
    cc2Ready = initCC1101_2();
    DBG_PRINTLN("[Reset] CC1101 radios reset");
}

// ============================================================================
// Jamming Functions
// ============================================================================

void startJammingBoth() {
    if (!cc1Ready && !cc2Ready) {
        sendResponse("ERROR:NO_RADIO");
        return;
    }
    bridgeState = STATE_JAMMING_BOTH;
    sendResponse("OK");
    DBG_PRINTLN("[Jam] Started on both radios");
}

void startJamming1() {
    if (!cc1Ready) {
        sendResponse("ERROR:CC1_NOT_READY");
        return;
    }
    bridgeState = STATE_JAMMING_1;
    sendResponse("OK");
    DBG_PRINTLN("[Jam] Started on CC1");
}

void startJamming2() {
    if (!cc2Ready) {
        sendResponse("ERROR:CC2_NOT_READY");
        return;
    }
    bridgeState = STATE_JAMMING_2;
    sendResponse("OK");
    DBG_PRINTLN("[Jam] Started on CC2");
}

void stopJamming() {
    bridgeState = STATE_IDLE;
    // Return to RX mode
    if (cc1Ready) cc1101_drv1.SetRx();
    if (cc2Ready) cc1101_drv2.SetRx();
    sendResponse("OK");
    DBG_PRINTLN("[Jam] Stopped");
}

void doJammingTick() {
    // Fill buffer with random data
    for (int i = 0; i < JAMMING_PACKET_SIZE; i++) {
        jamBuffer[i] = (byte)random(256);
    }
    
    switch (bridgeState) {
        case STATE_JAMMING_BOTH:
            if (cc1Ready) cc1101_drv1.SendData(jamBuffer, JAMMING_PACKET_SIZE);
            if (cc2Ready) cc1101_drv2.SendData(jamBuffer, JAMMING_PACKET_SIZE);
            break;
        case STATE_JAMMING_1:
            if (cc1Ready) cc1101_drv1.SendData(jamBuffer, JAMMING_PACKET_SIZE);
            break;
        case STATE_JAMMING_2:
            if (cc2Ready) cc1101_drv2.SendData(jamBuffer, JAMMING_PACKET_SIZE);
            break;
        default:
            break;
    }
}

// ============================================================================
// Scanning Functions
// ============================================================================

void startScan(float beginFreq, float endFreq) {
    if (!cc1Ready) {
        sendResponse("ERROR:NO_RADIO");
        return;
    }
    scanBeginFreq = beginFreq;
    scanEndFreq = endFreq;
    scanCurrentFreq = beginFreq;
    lastScanStep = 0;
    bridgeState = STATE_SCANNING;
    sendResponse("OK");
    DBG_PRINTF("[Scan] Started: %.2f - %.2f MHz\n", beginFreq, endFreq);
}

void stopScan() {
    bridgeState = STATE_IDLE;
    if (cc1Ready) cc1101_drv1.SetRx();
    if (cc2Ready) cc1101_drv2.SetRx();
    sendResponse("OK");
    DBG_PRINTLN("[Scan] Stopped");
}

void doScanTick() {
    if (bridgeState != STATE_SCANNING) return;
    
    unsigned long now = millis();
    if (now - lastScanStep < SCAN_DWELL_MS) return;
    lastScanStep = now;
    
    if (scanCurrentFreq > scanEndFreq) {
        // Scan complete
        Serial.println("SCAN:DONE");
        bridgeState = STATE_IDLE;
        if (cc1Ready) cc1101_drv1.SetRx();
        return;
    }
    
    // Set frequency and measure RSSI
    if (cc1Ready) {
        cc1101_drv1.setMHZ(scanCurrentFreq);
        cc1101_drv1.SetRx();
        delayMicroseconds(200); // Settling time
        
        byte rssiRaw = cc1101_drv1.getRssi();
        float rssi = (rssiRaw >= 128) ? (rssiRaw - 256) / 2.0 - 74.0 : rssiRaw / 2.0 - 74.0;
        
        // Send result to Master
        Serial.printf("SCAN:%.3f:%.1f\n", scanCurrentFreq, rssi);
    }
    
    scanCurrentFreq += SCAN_STEP_MHZ;
}

// ============================================================================
// Utility Functions
// ============================================================================

float getRSSI() {
    if (!cc1Ready) return -999.0f;
    
    byte rssiRaw = cc1101_drv1.getRssi();
    float rssi = (rssiRaw >= 128) ? (rssiRaw - 256) / 2.0 - 74.0 : rssiRaw / 2.0 - 74.0;
    return rssi;
}

int getLQI() {
    if (!cc1Ready) return 0;
    return cc1101_drv1.getLqi();
}

void sendResponse(const char* response) {
    Serial.println(response);
}

void sendStatus() {
    const char* stateStr = "IDLE";
    switch (bridgeState) {
        case STATE_IDLE:         stateStr = "IDLE"; break;
        case STATE_JAMMING_BOTH:
        case STATE_JAMMING_1:
        case STATE_JAMMING_2:   stateStr = "JAMMING"; break;
        case STATE_SCANNING:     stateStr = "SCANNING"; break;
    }
    Serial.printf("STATUS:%s\n", stateStr);
}

// ============================================================================
// Command Parser
// ============================================================================

void parseAndExecute(char* cmd) {
    // Trim whitespace
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    if (strlen(cmd) == 0) {
        sendResponse("ERROR:EMPTY_CMD");
        return;
    }
    
    DBG_PRINTF("[CMD] %s\n", cmd);
    
    // Parse command and arguments
    char* args[CMD_MAX_ARGS];
    int argCount = 0;
    
    char* token = strtok(cmd, ":");
    while (token != NULL && argCount < CMD_MAX_ARGS) {
        args[argCount++] = token;
        token = strtok(NULL, ":");
    }
    
    if (argCount == 0) {
        sendResponse("ERROR:PARSE_FAIL");
        return;
    }
    
    // Execute command
    if (strcmp(args[0], "PING") == 0) {
        sendResponse("PONG");
    }
    else if (strcmp(args[0], "INIT") == 0) {
        resetCC1101s();
        if (cc1Ready || cc2Ready) {
            sendResponse("OK");
        } else {
            sendResponse("ERROR:INIT_FAILED");
        }
    }
    else if (strcmp(args[0], "RESET") == 0) {
        resetCC1101s();
        sendResponse("OK");
    }
    else if (strcmp(args[0], "JAM") == 0 && argCount >= 2) {
        if (strcmp(args[1], "START") == 0) {
            startJammingBoth();
        } else if (strcmp(args[1], "STOP") == 0) {
            stopJamming();
        } else {
            sendResponse("ERROR:UNKNOWN_JAM_CMD");
        }
    }
    else if (strcmp(args[0], "JAM1") == 0 && argCount >= 2) {
        if (strcmp(args[1], "START") == 0) {
            startJamming1();
        } else {
            sendResponse("ERROR:UNKNOWN_JAM1_CMD");
        }
    }
    else if (strcmp(args[0], "JAM2") == 0 && argCount >= 2) {
        if (strcmp(args[1], "START") == 0) {
            startJamming2();
        } else {
            sendResponse("ERROR:UNKNOWN_JAM2_CMD");
        }
    }
    else if (strcmp(args[0], "SCAN") == 0 && argCount >= 2) {
        if (strcmp(args[1], "START") == 0 && argCount >= 4) {
            float beginFreq = atof(args[2]);
            float endFreq = atof(args[3]);
            startScan(beginFreq, endFreq);
        } else if (strcmp(args[1], "STOP") == 0) {
            stopScan();
        } else {
            sendResponse("ERROR:UNKNOWN_SCAN_CMD");
        }
    }
    else if (strcmp(args[0], "FREQ") == 0 && argCount >= 2) {
        float freq = atof(args[1]);
        setFrequency(freq);
        sendResponse("OK");
    }
    else if (strcmp(args[0], "RSSI") == 0) {
        float rssi = getRSSI();
        int lqi = getLQI();
        Serial.printf("RSSI:%.1f:%d\n", rssi, lqi);
    }
    else if (strcmp(args[0], "STATUS") == 0) {
        sendStatus();
    }
    else {
        sendResponse("ERROR:UNKNOWN_CMD");
    }
}

// ============================================================================
// Setup & Main Loop
// ============================================================================

void setup() {
    Serial.begin(UART_BAUD_RATE);
    
    DBG_PRINTLN("\n\n=================================");
    DBG_PRINTLN("  Starbeam V3 - CC1101 Bridge");
    DBG_PRINTLN("  2× CC1101 Controller");
    DBG_PRINTLN("=================================");
    
    randomSeed(analogRead(34)); // Use floating pin for entropy
    
    // Initialize CC1101 radios
    delay(500); // Allow power to stabilize
    cc1Ready = initCC1101_1();
    delay(200);
    cc2Ready = initCC1101_2();
    
    DBG_PRINTLN("\n[Bridge] Ready for commands");
    DBG_PRINTF("[Bridge] CC1: %s  CC2: %s\n", 
               cc1Ready ? "OK" : "FAIL",
               cc2Ready ? "OK" : "FAIL");
    
    // Signal ready
    Serial.println("STATUS:IDLE");
}

void loop() {
    // Process incoming UART commands
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n') {
            cmdBuffer[cmdIndex] = '\0';
            if (cmdIndex > 0) {
                parseAndExecute(cmdBuffer);
            }
            cmdIndex = 0;
        } else if (c != '\r' && cmdIndex < UART_BUF_SIZE - 1) {
            cmdBuffer[cmdIndex++] = c;
        }
        
        // Prevent buffer overflow
        if (cmdIndex >= UART_BUF_SIZE - 1) {
            cmdBuffer[cmdIndex] = '\0';
            parseAndExecute(cmdBuffer);
            cmdIndex = 0;
        }
    }
    
    // Handle active operations
    switch (bridgeState) {
        case STATE_JAMMING_BOTH:
        case STATE_JAMMING_1:
        case STATE_JAMMING_2:
            doJammingTick();
            delayMicroseconds(JAMMING_INTERVAL_MS * 1000);
            break;
            
        case STATE_SCANNING:
            doScanTick();
            break;
            
        default:
            // Idle - just yield
            vTaskDelay(pdMS_TO_TICKS(5));
            break;
    }
    
    yield();
}
