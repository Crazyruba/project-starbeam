# Starbeam V2 → V3 Migration Guide

## Overview

V3 builds on top of V2. You need to copy V2's source files and apply minimal patches. This guide walks you through the process.

---

## Project Structure

```
starbeam_v3/
├── README.md                           # V3 documentation
├── HARDWARE.md                         # V3 wiring guide
├── MIGRATION.md                        # This file
├── starbeam_master/                    # Master ESP32 firmware
│   ├── starbeam_master.ino             # Main sketch (NEW)
│   ├── config.h                        # V3 config (NEW)
│   ├── types.h                         # V3 types (NEW)
│   └── src/
│       ├── cc1101_uart.h               # UART proxy for CC1101 (NEW)
│       ├── cc1101_uart.cpp             # UART proxy implementation (NEW)
│       ├── display.h                   # COPY from starbeam_v2/src/
│       ├── display.cpp                 # COPY + PATCH from starbeam_v2/src/
│       ├── input.h                     # COPY from starbeam_v2/src/
│       ├── input.cpp                   # COPY from starbeam_v2/src/
│       ├── util.h                      # COPY from starbeam_v2/src/
│       ├── util.cpp                    # COPY from starbeam_v2/src/
│       ├── settings.h                  # COPY from starbeam_v2/src/
│       ├── settings.cpp                # COPY from starbeam_v2/src/
│       ├── nrf24.h                     # COPY from starbeam_v2/src/
│       ├── nrf24.cpp                   # COPY from starbeam_v2/src/
│       ├── recording.h                 # COPY from starbeam_v2/src/
│       ├── recording.cpp               # COPY from starbeam_v2/src/
│       ├── analyzer.h                  # COPY from starbeam_v2/src/
│       ├── analyzer.cpp                # COPY from starbeam_v2/src/
│       ├── wifi_scanner.h              # COPY from starbeam_v2/src/
│       ├── wifi_scanner.cpp            # COPY from starbeam_v2/src/
│       ├── ble_scanner.h               # COPY from starbeam_v2/src/
│       ├── ble_scanner.cpp             # COPY from starbeam_v2/src/
│       ├── flock_detector.h            # COPY from starbeam_v2/src/
│       ├── flock_detector.cpp          # COPY from starbeam_v2/src/
│       ├── captive_portal.h            # COPY from starbeam_v2/src/
│       ├── captive_portal.cpp          # COPY from starbeam_v2/src/
│       ├── packet_monitor.h            # COPY from starbeam_v2/src/
│       ├── packet_monitor.cpp          # COPY from starbeam_v2/src/
│       ├── webserver.h                 # COPY from starbeam_v2/src/
│       ├── webserver.cpp               # COPY from starbeam_v2/src/
│       ├── wifi_attack.h               # COPY from starbeam_v2/src/
│       ├── wifi_attack.cpp             # COPY from starbeam_v2/src/
│       ├── terminal.h                  # COPY from starbeam_v2/src/
│       └── terminal.cpp                # COPY from starbeam_v2/src/
└── starbeam_cc1101_bridge/             # Bridge ESP32 firmware
    ├── starbeam_cc1101_bridge.ino      # Bridge sketch (NEW)
    └── config.h                        # Bridge config (NEW)
```

---

## Step-by-Step Migration

### Step 1: Copy V2 Source Files

Copy all files from `starbeam_v2/src/` into `starbeam_v3/starbeam_master/src/`, **except** `cc1101.h` and `cc1101.cpp` which are replaced by the UART proxy.

```bash
# From repo root:
cp starbeam_v2/src/display.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/input.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/util.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/settings.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/nrf24.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/recording.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/analyzer.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/wifi_scanner.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/ble_scanner.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/flock_detector.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/captive_portal.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/packet_monitor.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/webserver.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/wifi_attack.* starbeam_v3/starbeam_master/src/
cp starbeam_v2/src/terminal.* starbeam_v3/starbeam_master/src/
```

### Step 2: Apply Display Patch

**File:** `starbeam_v3/starbeam_master/src/display.cpp`

#### Patch 1: Add menu label

Find the `menuLabels` array (~line 33) and add one entry at the end:

```cpp
// BEFORE (V2):
const char* Display::menuLabels[NUM_MENU_ITEMS] = {
    "BT Jammer",
    // ... all existing items ...
    "Settings",
    "Help"
};

// AFTER (V3):
const char* Display::menuLabels[NUM_MENU_ITEMS] = {
    "BT Jammer",
    // ... all existing items ...
    "Settings",
    "Help",
    "Bridge Status"     // <-- ADD THIS LINE
};
```

#### Patch 2: Update version string in boot animation

Find `displayTitleScreen()` (~line 130) and update the subtitle:

```cpp
// BEFORE:
const char* sub = "v2  RF intelligence";

// AFTER:
const char* sub = "v3  7 RF modules";
```

#### Patch 3: Update header in drawMenu

Find `drawHeaderBar(oled, "Starbeam V2")` in `drawMenu()` and change to:

```cpp
drawHeaderBar(oled, "Starbeam V3");
```

That's it! Only 3 small changes to display.cpp.

### Step 3: Verify File List

Your `starbeam_v3/starbeam_master/src/` should now contain:

```
src/
├── cc1101_uart.h          (NEW - V3)
├── cc1101_uart.cpp        (NEW - V3)
├── display.h              (from V2 - UNCHANGED)
├── display.cpp            (from V2 - PATCHED per above)
├── input.h                (from V2 - UNCHANGED)
├── input.cpp              (from V2 - UNCHANGED)
├── util.h                 (from V2 - UNCHANGED)
├── util.cpp               (from V2 - UNCHANGED)
├── settings.h             (from V2 - UNCHANGED)
├── settings.cpp           (from V2 - UNCHANGED)
├── nrf24.h                (from V2 - UNCHANGED)
├── nrf24.cpp              (from V2 - UNCHANGED)
├── recording.h            (from V2 - UNCHANGED)
├── recording.cpp          (from V2 - UNCHANGED)
├── analyzer.h             (from V2 - UNCHANGED)
├── analyzer.cpp           (from V2 - UNCHANGED)
├── wifi_scanner.h         (from V2 - UNCHANGED)
├── wifi_scanner.cpp       (from V2 - UNCHANGED)
├── ble_scanner.h          (from V2 - UNCHANGED)
├── ble_scanner.cpp        (from V2 - UNCHANGED)
├── flock_detector.h       (from V2 - UNCHANGED)
├── flock_detector.cpp     (from V2 - UNCHANGED)
├── captive_portal.h       (from V2 - UNCHANGED)
├── captive_portal.cpp     (from V2 - UNCHANGED)
├── packet_monitor.h       (from V2 - UNCHANGED)
├── packet_monitor.cpp     (from V2 - UNCHANGED)
├── webserver.h            (from V2 - UNCHANGED)
├── webserver.cpp          (from V2 - UNCHANGED)
├── wifi_attack.h          (from V2 - UNCHANGED)
├── wifi_attack.cpp        (from V2 - UNCHANGED)
├── terminal.h             (from V2 - UNCHANGED)
└── terminal.cpp           (from V2 - UNCHANGED)
```

### Step 4: Install Bridge Library

The Bridge ESP32 needs the SmartRC-CC1101-Driver-Lib:

1. Extract `SmartRC-CC1101-Driver-Lib2 2.zip` from the repo root
2. Copy the extracted folder to your Arduino libraries directory:
   - Windows: `Documents\Arduino\libraries\`
   - macOS: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

### Step 5: Compile and Upload

1. **Master ESP32**: Open `starbeam_v3/starbeam_master/starbeam_master.ino`
2. **Bridge ESP32**: Open `starbeam_v3/starbeam_cc1101_bridge/starbeam_cc1101_bridge.ino`
3. Upload both sketches to their respective ESP32s

---

## What Changed from V2 → V3

### New Files (V3 only)
| File | Purpose |
|------|---------|
| `starbeam_master.ino` | Main firmware with CC1101 UART proxy integration |
| `config.h` (master) | Pin config for 5× nRF24, UART bridge settings |
| `types.h` (master) | Added BridgeState enum, bridge command types |
| `cc1101_uart.h/cpp` | CC1101 proxy that sends commands over UART |
| `starbeam_cc1101_bridge.ino` | Bridge ESP32 firmware for 2× CC1101 |
| `config.h` (bridge) | Bridge pin config, CC1101 pins |

### Modified from V2
| File | Change |
|------|--------|
| `display.cpp` | Added "Bridge Status" menu label, updated version strings |

### Removed from V2
| File | Reason |
|------|--------|
| `cc1101.h` | Replaced by `cc1101_uart.h` (UART proxy) |
| `cc1101.cpp` | CC1101 driver now runs on Bridge ESP32 |

### Unchanged from V2
All other `src/` files work without modification:
- `nrf24.h/cpp` — 5× nRF24 control (unchanged)
- `wifi_scanner.cpp` — WiFi scanning (unchanged)
- `ble_scanner.cpp` — BLE scanning (unchanged)
- `wifi_attack.cpp` — WiFi attack suite (unchanged)
- `webserver.cpp` — Web interface (unchanged)
- `display.cpp` core logic — OLED rendering (unchanged)
- All other modules

---

## Quick Test After Migration

1. Power on both ESP32s
2. Master Serial Monitor should show:
   ```
   Project Starbeam V3
     Dual ESP32 - 7 RF Modules
   [Boot] CC1101 Bridge: ONLINE
   ```
3. Navigate menu to **"Bridge Status"** item
4. Should show: `Bridge: ONLINE` with `Ping: OK`
5. Test **"CC1 Jammer"** — should activate via Bridge
6. Test **"CC1101 Scan"** — should show scan results from Bridge

If Bridge shows OFFLINE:
- Check UART wiring (TX↔RX crossover)
- Verify common GND
- Check both ESP32s are at 115200 baud
