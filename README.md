# project starbeam v2 2026
**The Ultimate ESP32 WiFi + BT + Drone Jammer w/ 5 Radios.**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Status](https://img.shields.io/badge/Status-Active-success.svg)]()

> **🆕 Starbeam V3 Now Available!** — Check out [`starbeam_v3/`](starbeam_v3/) for the **dual-ESP32 architecture** supporting **7 RF modules** (5× nRF24 + 2× CC1101) simultaneously. V3 splits the workload across two ESP32s connected via UART, eliminating the pin-conflict limitations of the single-ESP32 design.

 ![Project Starbeam Logo](img/starbeam_logo.PNG)

> **⚠️ LEGAL DISCLAIMER**: This tool is designed for **AUTHORIZED SECURITY TESTING ONLY**. Unauthorized use of RF jamming, WiFi deauthentication, or network analysis tools may violate federal, state, and local laws including but not limited to the Computer Fraud and Abuse Act (CFAA), FCC regulations, and similar laws in other countries. Users are solely responsible for compliance with all applicable laws and regulations. By using this software, you agree to use it only on networks and systems you own or have explicit written authorization to test.

## Project Starbeam: An Affordable Approach to Signal Intelligence for Security Applications

Project Starbeam is an innovative signal intelligence platform leveraging cost-effective, open-source hardware to deliver advanced capabilities in signal analysis, generation, and manipulation. Central to its design is a custom PCB incorporating the ESP32-WROOM-32D microcontroller alongside multiple radio frequency modules. The system's modularity and compatibility with devices like HackRF One extend its frequency coverage up to 6 GHz, making it a versatile tool for various applications in security testing and operations.

**Intended Use Cases:**
- Authorized penetration testing on owned networks
- Educational research in controlled environments
- RF security auditing with written authorization
- Capture-the-Flag (CTF) competitions
- Wireless protocol analysis and research

## Starbeam V3 — Dual ESP32, 7 RF Modules

The new V3 architecture ([`starbeam_v3/`](starbeam_v3/)) solves the hardware limitation of fitting 7 RF modules on a single ESP32:

| | V2 (Single ESP32) | V3 (Dual ESP32) |
|---|---|---|
| **nRF24 Radios** | Up to 5 (shared slots) | 5 dedicated |
| **CC1101 Radios** | Up to 2 (shared slots) | 2 dedicated |
| **Total Modules** | 5 max | **7 simultaneous** |
| **Architecture** | 1× ESP32 | 2× ESP32 via UART |
| **Pin Conflicts** | Slots 4-5 shared | None |
| **Code Changes** | Baseline | +3 lines display.cpp |

**V3 Documentation:**
- [`starbeam_v3/README.md`](starbeam_v3/README.md) — V3 architecture overview
- [`starbeam_v3/HARDWARE.md`](starbeam_v3/HARDWARE.md) — Complete wiring guide
- [`starbeam_v3/MIGRATION.md`](starbeam_v3/MIGRATION.md) — V2→V3 migration steps

## Starbeam PCB Assembly Guide
This guide provides instructions for completing the Starbeam PCB assembly after the components have been manufactured and attached. You can source the necessary parts from various suppliers, but the provided links offer decent price points. It is crucial to obtain the exact specified parts if using this PCB, especially for the display and USB-C module.

This is what you will get after PCB order:
 ![Project Starbeam PCBA](img/starbeam2.JPG)

### PCB-A Notes:
**This is a 4-layer PCB, there are notes for the required specifications in /hardware**
**This is an advanced project, so please do not waste your time & money if you do not understand PCB ordering, & if you need any assistance you can book a call & i'll be more than happy to walk you through the whole build process:
[Book a 30 Minute Consultation](https://book.stripe.com/cN2eWneWf77F4gM5kq)**

Link to PCB to place order:
https://pcbway.com/g/87Pi52

**Required Components After PCB Assembly:**
- **ESP32-Wroom-32D**:
    - https://amzn.to/3YR1noR
- **SSD1306 128x64 0.96-inch Display**:
    - https://amzn.to/4lONdP9
- **NRF24 Radios (x5) for 2.4GHz**:
    - https://amzn.to/4iBofjl
- **CC1101 Radios (x2) for 433MHz**:
    - https://amzn.to/4iuopZH
- **USB-C Module**:
    - https://amzn.to/4lKZ5BL

**Software Setup & Code Upload:**
1. Download Arduino IDE:
    - Get it here: https://www.arduino.cc/en/software/
2. Install Arduino IDE:
    - Refer to the installation guide:
    - https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/
3. Open the Code:
    - Extract the starbeam code .zip file.
    - **Extract SmartRC-CC1101-Driver-Lib2 2.zip & add it to your Documents/Arduino/libraries/ folder. This must be added to use the 2nd CC1101 radio module!**
    - If you are creative, you can also add up to 5 CC1101's for advanced testing.
    - Open the starbeam_v2.ino file in starbeam_v2/ folder. This will automatically open the code in Arduino IDE.
4. Upload Code to ESP32:
    - Upload the code to your ESP32 microcontroller using Arduino IDE.
    -   Note: Videos on uploading code and using Arduino IDE are available in the Hakr Hardware Club (https://whop.com/little-hakr).
5. Final Steps:
    - Attach the antennas that correspond to your desired setup.
    - Good luck...

## Hardware Specifications

### Core Components

- **Microcontroller**: ESP32-WROOM-32D (240 MHz dual-core, 4MB flash, 520KB SRAM)
- **Display**: SSD1306 OLED (128×64, I²C)
- **Radio Modules**:
  - Up to 5× NRF24L01+ (2.4 GHz) with PA+LNA
  - Up to 2× CC1101 (300-928 MHz, dual-chip configuration)
  - Optional HackRF One integration (1 MHz - 6 GHz)
- **Controls**: 3-button interface (Up, Down, Select)

### Pin Configuration

| Component | Pin(s) | Interface |
|-----------|--------|-----------|
| SSD1306 OLED | SDA/SCL (default I²C) | I²C @ 0x3C |
| NRF24 (VSPI) | CE=27/26/25, CS=15/33/5 | VSPI |
| NRF24 (HSPI) | CE=4/32, CS=2/17 | HSPI |
| CC1101 #1 | SS=2, GDO0=4, GDO2=16 | Shared SPI |
| CC1101 #2 | SS=32, GDO0=35, GDO2=17 | Shared SPI |
| Buttons | UP=39, DOWN=34, SEL=36 | GPIO |

## Features

### Core Capabilities

#### 🔊 RF Jamming (Authorized Use Only)
- **Bluetooth Jammer**: Disrupts 2.4 GHz Bluetooth connections
- **WiFi Jammer**: Interferes with 2.4 GHz WiFi networks
- **Drone Jammer**: Targets common drone control frequencies
- **CC1101 Jammer**: Broadband noise on 433 MHz ISM band

#### 📡 RF Analysis
- **NRF24 Spectrum Scanner**: Real-time 2.4 GHz band visualization
- **CC1101 Frequency Scanner**: 300-928 MHz signal detection
- **RSSI Monitoring**: Signal strength analysis across bands
- **Signal Recording**: Capture raw RF data for later analysis
- **Signal Playback**: Replay captured RF signals

#### 📶 WiFi & Bluetooth Scanning
- **WiFi Network Scanner**: Enumerate access points with RSSI, channel, security
- **WiFi Channel Heatmap**: Visualize 2.4 GHz channel utilization
- **BLE Device Scanner**: Discover Bluetooth Low Energy devices
- **Independent Web Server**: Remote access with captive portal

### 🔐 Security Testing Features (V2)

> **CRITICAL**: These features require explicit authorization. See Legal Compliance section below.

#### WiFi Security Testing

**1. Deauthentication Attacks**
- **Targeted Deauth**: Disconnect specific clients from an access point
- **Broadcast Deauth**: Disrupt all nearby WiFi networks

**2. Beacon Flooding**
- Generate up to 20 fake access points with random BSSIDs

**3. Probe Request Flooding**
- Simulate multiple client devices searching for networks

**4. PMKID Capture**
- Extract PMKID from WPA2/WPA3 EAPOL handshakes

### 🌐 Web Interface

**Features:**
- **WiFi Scanner Dashboard**: Live network list with refresh
- **BLE Scanner Dashboard**: Discovered BLE devices
- **Security Testing Dashboard**: Attack control interface with legal warnings
- **Background Scanning**: Independent WiFi/BLE scan toggles
- **JSON APIs**: `/api/wifi`, `/api/ble`, `/api/attack/status`
- **Captive Portal**: Automatic redirect on connection

## Technical Specifications

* **Microcontroller:** ESP32-WROOM-32D
* **RF Modules:**
    * Configuration A: 5x NRF24L01+PA+LNA
    * Configuration B: 3x NRF24L01+PA+LNA and 2x CC1101
* **Extended Frequency Range:** Up to 6 GHz (with HackRF One)
* **Power Specifications:** micro-USB & USB-C for 5V
* **Connectivity:** Wi-Fi, Bluetooth, Serial

## License

Apache License 2.0 - See [LICENSE](LICENSE) file for details.

## Support & Documentation

- **V3 Documentation**: [`starbeam_v3/`](starbeam_v3/) — New dual-ESP32 architecture
- **GitHub Issues**: Report bugs or request features
- **Serial Monitor**: Enable at 115200 baud for verbose debugging
- **Hardware Schematics**: `hardware/starbeam_V1_design/`
- **CLAUDE.md**: Development guide for contributors

## Projekt StarBeam 3.0
**Version 3 — Dual ESP32 architecture with 7 simultaneous RF modules.**
- **cypher**
- **little hakr**
