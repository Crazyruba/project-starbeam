# Project Starbeam V3 - Dual ESP32 Architecture

**7 RF Modules: 5× nRF24L01+PA+LNA + 2× CC1101**

---

## Architecture Overview

Starbeam V3 solves the pin-conflict problem of fitting 7 RF modules on a single ESP32 by splitting the workload across two ESP32-WROOM-32D microcontrollers connected via UART.

```
┌─────────────────────────────────────────────────────────────────────┐
│                        STARBEAM V3 SYSTEM                           │
├──────────────────────────────┬──────────────────────────────────────┤
│      MASTER ESP32            │         BRIDGE ESP32                 │
│  (starbeam_master.ino)       │   (starbeam_cc1101_bridge.ino)       │
│                              │                                      │
│  ┌──────────────────────┐    │    ┌──────────────────────┐          │
│  │ 5× nRF24L01+PA+LNA  │    │    │  2× CC1101 Radio     │          │
│  │ 2.4GHz Band          │    │    │  300-928 MHz Band    │          │
│  ├──────────────────────┤    │    ├──────────────────────┤          │
│  │ • BT Jammer          │    │    │ • 433 MHz Jammer     │          │
│  │ • WiFi Jammer        │    │    │ • Signal Scanner     │          │
│  │ • Drone Jammer       │    │    │ • Raw Record/Play    │          │
│  │ • nRF24 Scanner      │    │    │ • RSSI Monitoring    │          │
│  ├──────────────────────┤    │    └──────────────────────┘          │
│  │ OLED Display (I2C)   │◄───UART──►│ TX/RX Crossover    │          │
│  │ 3-Button Input       │ 115200   │ GND Common         │          │
│  │ WiFi / BLE (ESP32)   │          │ 3.3V Shared        │          │
│  │ Web Server           │          │                      │          │
│  │ WiFi Attack Suite    │          └──────────────────────┘          │
│  └──────────────────────┘                                           │
│                                                                    │
│  Optional: HackRF One via UART (shared with Bridge)                │
│  (use USB-UART hub or switch to connect either Bridge or HackRF)   │
└──────────────────────────────┴──────────────────────────────────────┘
```

---

## Hardware Wiring

### Master ESP32 → 5× nRF24L01+PA+LNA

| Signal | GPIO | nRF24 #1 | nRF24 #2 | nRF24 #3 | nRF24 #4 | nRF24 #5 |
|--------|------|----------|----------|----------|----------|----------|
| **VSPI SCK** | 18 | SCK | SCK | SCK | — | — |
| **VSPI MISO** | 19 | MISO | MISO | MISO | — | — |
| **VSPI MOSI** | 23 | MOSI | MOSI | MOSI | — | — |
| **HSPI SCK** | 14 | — | — | — | SCK | SCK |
| **HSPI MISO** | 12 | — | — | — | MISO | MISO |
| **HSPI MOSI** | 13 | — | — | — | MOSI | MOSI |
| **CE** | 27 | CE | — | — | — | — |
| **CE** | 26 | — | CE | — | — | — |
| **CE** | 25 | — | — | CE | — | — |
| **CE** | 4 | — | — | — | CE | — |
| **CE** | 32 | — | — | — | — | CE |
| **CS** | 15 | CSN | — | — | — | — |
| **CS** | 33 | — | CSN | — | — | — |
| **CS** | 5 | — | — | CSN | — | — |
| **CS** | 2 | — | — | — | CSN | — |
| **CS** | 17 | — | — | — | — | CSN |
| **3.3V** | — | VCC | VCC | VCC | VCC | VCC |
| **GND** | — | GND | GND | GND | GND | GND |

**Note:** All 5 nRF24 modules use 3.3V. Do NOT connect to 5V. Use separate 3.3V regulator (AMS1117-3.3) if needed - each nRF24 draws ~300mA peak with PA+LNA.

### Master ESP32 → OLED Display

| Signal | GPIO |
|--------|------|
| SDA | 21 (default I2C) |
| SCL | 22 (default I2C) |
| VCC | 3.3V |
| GND | GND |

### Master ESP32 → Buttons

| Button | GPIO | Mode |
|--------|------|------|
| UP | 39 | INPUT_PULLUP |
| DOWN | 34 | INPUT_PULLUP |
| SELECT | 36 | INPUT_PULLUP |

### Master ESP32 ↔ Bridge ESP32 (UART)

| Signal | Master GPIO | Bridge GPIO |
|--------|-------------|-------------|
| TX → RX | 1 (TX0) | 3 (RX0) |
| RX ← TX | 3 (RX0) | 1 (TX0) |
| GND | GND | GND |

**IMPORTANT:** Cross-connect TX↔RX between the two ESP32s. Connect GND together. Use 3.3V logic level.

For stable operation at 115200 baud, keep wires short (<30cm). If longer distance needed, use a level shifter or differential UART (RS-485).

### Bridge ESP32 → 2× CC1101

| Signal | GPIO | CC1101 #1 | CC1101 #2 |
|--------|------|-----------|-----------|
| SCK | 18 | SCK | SCK |
| MISO | 19 | MISO (SO) | MISO (SO) |
| MOSI | 23 | MOSI (SI) | MOSI (SI) |
| CS/SS | 5 | CSN | — |
| CS/SS | 33 | — | CSN |
| GDO0 | 4 | GDO0 | — |
| GDO0 | 32 | — | GDO0 |
| GND | — | GND | GND |
| 3.3V | — | VCC | VCC |

**Note:** GDO2 pins are optional (not used in V3 software).

---

## Pinout Comparison: V2 vs V3

### V2 (Single ESP32, 5 radio slots)
- Radio slots 4-5 shared between nRF24 and CC1101 (mutually exclusive)
- Could only have 5 total radios, not 7

### V3 (Dual ESP32, 7 radio slots)
- Master: 5× nRF24 (dedicated, no sharing)
- Bridge: 2× CC1101 (dedicated, on separate ESP32)
- Zero pin conflicts
- Both SPI buses on Master used exclusively for nRF24
- Bridge has its own dedicated SPI for CC1101s

---

## UART Protocol

The Master and Bridge communicate via a simple text-based UART protocol at 115200 baud.

### Master → Bridge Commands

| Command | Description | Example |
|---------|-------------|---------|
| `JAM:START` | Start jamming on both CC1101s | `JAM:START\n` |
| `JAM:STOP` | Stop all jamming | `JAM:STOP\n` |
| `JAM1:START` | Start jamming on CC1101 #1 | `JAM1:START\n` |
| `JAM2:START` | Start jamming on CC1101 #2 | `JAM2:START\n` |
| `SCAN:START:<begin>:<end>` | Start frequency scan | `SCAN:START:433.60:434.20\n` |
| `SCAN:STOP` | Stop scanning | `SCAN:STOP\n` |
| `FREQ:<mhz>` | Set frequency | `FREQ:434.40\n` |
| `RSSI` | Get RSSI and LQI | `RSSI\n` |
| `INIT` | Initialize CC1101 radios | `INIT\n` |
| `RESET` | Reset CC1101 radios | `RESET\n` |
| `STATUS` | Get bridge status | `STATUS\n` |
| `PING` | Health check | `PING\n` |

### Bridge → Master Responses

| Response | Description | Example |
|----------|-------------|---------|
| `OK` | Command successful | `OK\n` |
| `ERROR:<msg>` | Error occurred | `ERROR:NOT_INITIALIZED\n` |
| `RSSI:<rssi>:<lqi>` | RSSI and LQI values | `RSSI:-72.5:85\n` |
| `SCAN:<freq>:<rssi>` | Scan result point | `SCAN:433.920:-45.2\n` |
| `SCAN:DONE` | Scan complete | `SCAN:DONE\n` |
| `STATUS:<state>` | Bridge state | `STATUS:IDLE\n` |
| `PONG` | Health check response | `PONG\n` |

### State Machine

```
Bridge States:
  IDLE → JAMMING (on JAM:START/JAM1:START/JAM2:START)
  IDLE → SCANNING (on SCAN:START)
  JAMMING → IDLE (on JAM:STOP)
  SCANNING → IDLE (on SCAN:STOP or SCAN:DONE)
```

---

## Required Libraries

### Master ESP32
Same as Starbeam V2:
- Adafruit GFX Library
- Adafruit SSD1306
- RF24 by TMRh20
- ESP32 BLE Arduino (built-in)

### Bridge ESP32
- SmartRC-CC1101-Driver-Lib by LSatan
  - Extract `SmartRC-CC1101-Driver-Lib2 2.zip` to `~/Documents/Arduino/libraries/`

---

## Upload Instructions

### Master ESP32
1. Open `starbeam_v3/starbeam_master/starbeam_master.ino` in Arduino IDE
2. Select Board: **ESP32 Dev Module**
3. Upload Speed: **921600**
4. CPU Frequency: **240 MHz**
5. Flash Size: **4MB (32Mb)**
6. Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**
7. Select correct COM port
8. Click Upload

### Bridge ESP32
1. Open `starbeam_v3/starbeam_cc1101_bridge/starbeam_cc1101_bridge.ino` in Arduino IDE
2. Same board settings as Master
3. Select correct COM port (may need to switch USB cable)
4. Click Upload

**Note:** Both ESP32s use the same board settings. The only difference is the sketch file and the connected RF modules during testing.

---

## Power Considerations

### Current Draw Estimate

| Component | Peak Current |
|-----------|-------------|
| ESP32-WROOM-32D (×2) | ~500mA total (WiFi TX peak) |
| nRF24L01+PA+LNA (×5) | ~300mA each = 1500mA |
| CC1101 (×2) | ~50mA each = 100mA |
| OLED Display | ~20mA |
| **Total Peak** | **~2120mA (2.1A)** |

**Power Supply Recommendations:**
- Use a 3.3V buck converter rated for at least 3A
- Or power ESP32s via 5V USB and use separate 3.3V regulators for RF modules
- Add 100µF+ decoupling capacitors near each radio module
- Use thick power rails (at least 22 AWG for main power distribution)

---

## Bill of Materials

| Qty | Component | Notes |
|-----|-----------|-------|
| 2 | ESP32-WROOM-32D | DevKit boards recommended for prototyping |
| 5 | nRF24L01+PA+LNA | 2.4GHz with power amplifier + antenna |
| 2 | CC1101 module | 433MHz (or 868/915 MHz variants) |
| 1 | SSD1306 OLED | 128×64, I2C, 0.96" |
| 3 | Tactile buttons | 6×6mm or similar |
| 1 | AMS1117-3.3 3A | 3.3V regulator for RF modules (or 2× 1A) |
| 5 | nRF24 antennas | 2.4GHz dipole, SMA or PCB antenna |
| 2 | CC1101 antennas | 433MHz helical or dipole |
| — | Breadboard/PCB | For prototyping |
| — | Jumper wires | M-M and F-M as needed |
| — | USB cables | For programming and power |

---

## Changes from V2

### What Changed
- Split from single ESP32 to dual ESP32 architecture
- CC1101 radios moved to dedicated Bridge ESP32
- All 5 nRF24 radios available simultaneously (no longer shared with CC1101 slots)
- UART protocol for Master↔Bridge communication
- CC1101 operations now proxied through UART

### What Stayed the Same
- All V2 features preserved (BT jam, WiFi jam, drone jam, nRF24 scan, etc.)
- Web server and WiFi attack suite unchanged on Master
- OLED menu system identical
- Same libraries and dependencies
- Backward-compatible pinout for nRF24 radios 1-3 on VSPI

### New Features in V3
- **7 simultaneous RF modules** (vs 5 in V2)
- **No radio slot conflicts** - all 7 modules can be active at once
- **Modular design** - Bridge can be omitted for 5-radio-only operation
- **HackRF compatibility preserved** - Master UART pins still available

---

## Troubleshooting

### Bridge not responding
1. Check UART wiring: TX↔RX crossover, common GND
2. Verify both ESP32s share the same baud rate (115200)
3. Check Bridge Serial Monitor for boot messages
4. Try `PING` command from Master Serial Monitor

### CC1101 not initializing
1. Check SPI wiring on Bridge ESP32
2. Verify CS pins match config.h definitions
3. Ensure SmartRC-CC1101-Driver-Lib is installed
4. Check 3.3V power supply stability

### nRF24 not responding
1. Check VSPI/HSPI wiring on Master ESP32
2. Verify CE/CS pin assignments in config.h
3. Ensure all 5 radios have adequate 3.3V power
4. Check antenna connections

### Serial command echo issues
The Bridge uses Serial for both debug output and UART communication. In production, disable Bridge debug output by setting `BRIDGE_DEBUG 0` in config.h.

---

## Version History

- **V1** (2024): Initial release, single ESP32, 3× nRF24 + 2× CC1101
- **V2** (2025): Added WiFi attack suite, web server, BLE scanning, 5× nRF24 support
- **V3** (2026): Dual ESP32 architecture, 7 RF modules (5× nRF24 + 2× CC1101)

---

## License

Apache License 2.0 - See [LICENSE](../LICENSE) file for details.

**⚠️ LEGAL DISCLAIMER:** This tool is designed for **AUTHORIZED SECURITY TESTING ONLY**. Unauthorized use of RF jamming, WiFi deauthentication, or network analysis tools may violate federal, state, and local laws. Users are solely responsible for compliance with all applicable laws and regulations.
