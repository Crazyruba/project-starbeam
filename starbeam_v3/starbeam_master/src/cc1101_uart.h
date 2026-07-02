// cc1101_uart.h - CC1101 UART Proxy for Starbeam V3
// Replaces direct CC1101 driver with UART commands to Bridge ESP32
// All CC1101 operations are proxied via Serial to the Bridge

#ifndef CC1101_UART_H
#define CC1101_UART_H

#include <Arduino.h>
#include "../types.h"

// ============================================================================
// CC1101 UART Proxy - Sends commands to Bridge ESP32 via UART
// ============================================================================

class CC1101UARTProxy {
public:
    // Initialize UART communication with Bridge
    static void init();
    
    // Check if Bridge is responsive
    static bool ping();
    
    // Initialize CC1101 radios on Bridge
    static bool initRadios();
    
    // Check connections
    static bool checkConnection1();
    static bool checkConnection2();
    
    // Set frequency on Bridge CC1101s
    static void setMhz(float freq);
    
    // Start/stop jamming
    static void startJamming();
    static void startJamming1();
    static void startJamming2();
    static void stopJamming();
    
    // Frequency scan (returns results via callback)
    static void startScan(float beginFreq, float endFreq);
    static void stopScan();
    static bool isScanning();
    
    // Get signal data
    static float getRssi();
    static int getLqi();
    
    // Reset CC1101 radios
    static void reset();
    
    // Get Bridge status
    static BridgeState getBridgeState();
    
    // Process incoming scan results (call in loop)
    static void processIncoming();
    
    // Get last scan results buffer
    static SignalInfo* getScanResults(int* count);
    static void clearScanResults();
    
    // Flush UART buffer
    static void flush();

private:
    // Internal state
    static bool initialized;
    static BridgeState bridgeState;
    static bool scanning;
    static float currentFreq;
    
    // Scan results ring buffer
    static SignalInfo scanResults[64];
    static int scanResultCount;
    static int scanResultHead;
    
    // UART command helpers
    static bool sendCommand(const char* cmd, char* response, size_t respSize, uint32_t timeout = BRIDGE_CMD_TIMEOUT);
    static void sendRaw(const char* cmd);
    static bool waitForResponse(char* buffer, size_t size, uint32_t timeout);
    static void parseScanResult(const char* line);
};

// Legacy compatibility wrapper (matches V2 CC1101Radio API)
class CC1101Radio {
public:
    static void init() { CC1101UARTProxy::init(); CC1101UARTProxy::initRadios(); }
    static void init2() {} // Both radios initialized in initRadios()
    static bool checkConnection() { return CC1101UARTProxy::checkConnection1(); }
    static bool checkConnection2() { return CC1101UARTProxy::checkConnection2(); }
    static void setMhz(float freq) { CC1101UARTProxy::setMhz(freq); }
    static void scan(float beginFreq, float endFreq) { CC1101UARTProxy::startScan(beginFreq, endFreq); }
    static void stopJamming() { CC1101UARTProxy::stopJamming(); }
    static float getRssi() { return CC1101UARTProxy::getRssi(); }
    static int getLqi() { return CC1101UARTProxy::getLqi(); }
    static BridgeState getBridgeState() { return CC1101UARTProxy::getBridgeState(); }
    static void processIncoming() { CC1101UARTProxy::processIncoming(); }
    static SignalInfo* getScanResults(int* count) { return CC1101UARTProxy::getScanResults(count); }
    static void clearScanResults() { CC1101UARTProxy::clearScanResults(); }
    
    // Stub for V2 compatibility - these don't exist on proxy
    static void getCC1() {} 
    static void getCC2() {}
};

#endif // CC1101_UART_H
