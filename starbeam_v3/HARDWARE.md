# Starbeam V3 - Hardware Wiring Guide

## Overview

V3 uses a **dual ESP32 architecture** to support 7 RF modules simultaneously:
- **Master ESP32**: 5× nRF24L01+PA+LNA (2.4 GHz) + OLED + WiFi/BT + Web Server
- **Bridge ESP32**: 2× CC1101 (300-928 MHz) + UART link to Master

---

## Complete Wiring Diagram

### Master ESP32 Pinout

```
ESP32-WROOM-32D (Master)
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║  EN  GPIO23 ────► VSPI_MOSI ─────┬── nRF24 #1 MOSI         ║
║  GPIO36 ────────► BTN_SELECT     ├── nRF24 #2 MOSI         ║
║  GPIO39 ────────► BTN_UP         ├── nRF24 #3 MOSI         ║
║  GPIO34 ────────► BTN_DOWN     ──┴── nRF24 #4 MOSI         ║
║  GPIO35                         ──── nRF24 #5 MOSI         ║
║  GPIO32 ────────► nRF24 #5 CE                              ║
║  GPIO33 ────────► nRF24 #2 CS                              ║
║  GPIO25 ────────► nRF24 #3 CE                              ║
║  GPIO26 ────────► nRF24 #2 CE                              ║
║  GPIO27 ────────► nRF24 #1 CE                              ║
║  GPIO14 ────────► HSPI_SCK  ─────┬── nRF24 #4 SCK          ║
║  GPIO12 ────────► HSPI_MISO ─────┼── nRF24 #4 MISO         ║
║  GPIO13 ────────► HSPI_MOSI ─────┼── nRF24 #5 MOSI         ║
║  GPIO9                          ─┴─  nRF24 #5 SCK           ║
║  GPIO10                                                         ║
║  GPIO11                                                         ║
║  GPIO5  ────────► nRF24 #3 CS  ──── VSPI_SS                 ║
║  GPIO6                                                          ║
║  GPIO7                                                          ║
║  GPIO8                                                          ║
║  GPIO15 ────────► nRF24 #1 CS                              ║
║  GPIO2  ────────► nRF24 #4 CS                              ║
║  GPIO0  ────────► (Boot pin - avoid pulling low at boot)   ║
║  GPIO4  ────────► nRF24 #4 CE                              ║
║  GPIO16 ────────► LED_PIN                                  ║
║  GPIO17 ────────► nRF24 #5 CS                              ║
║  GPIO18 ────────► VSPI_SCK  ─────┬── nRF24 #1 SCK          ║
║  GPIO19 ────────► VSPI_MISO ─────┼── nRF24 #1 MISO         ║
║  GPIO21 ────────► I2C SDA ──────► OLED SDA                 ║
║  GPIO3  ────────► UART RX ◄────── Bridge TX                ║
║  GPIO1  ────────► UART TX ──────► Bridge RX                ║
║  GPIO22 ────────► I2C SCL ──────► OLED SCL                 ║
║  GPIO13*                                                        ║
║                                                              ║
║  3.3V ──────────┬── Power all nRF24 VCC                     ║
║  GND  ──────────┼── Common ground                           ║
║  5V   ──────────┘ (Input only - power ESP32 via USB)        ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝

* GPIO13 is also HSPI_MOSI (shared)
```

### Bridge ESP32 Pinout

```
ESP32-WROOM-32D (Bridge)
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║  EN                                                          ║
║  GPIO36                                                       ║
║  GPIO39                                                       ║
║  GPIO34                                                       ║
║  GPIO35                                                       ║
║  GPIO32 ────────► CC1101 #2 GDO0                           ║
║  GPIO33 ────────► CC1101 #2 CS                             ║
║  GPIO25                                                       ║
║  GPIO26                                                       ║
║  GPIO27                                                       ║
║  GPIO14                                                       ║
║  GPIO12                                                       ║
║  GPIO13                                                       ║
║  GPIO9                                                        ║
║  GPIO10                                                       ║
║  GPIO11                                                       ║
║  GPIO5  ────────► CC1101 #1 CS                             ║
║  GPIO6                                                        ║
║  GPIO7                                                        ║
║  GPIO8                                                        ║
║  GPIO15                                                       ║
║  GPIO2                                                        ║
║  GPIO0  ────────► (Boot pin)                                 ║
║  GPIO4  ────────► CC1101 #1 GDO0                           ║
║  GPIO16                                                       ║
║  GPIO17                                                       ║
║  GPIO18 ────────► SPI SCK  ───────┬── CC1101 #1 SCK         ║
║  GPIO19 ────────► SPI MISO ───────┼── CC1101 #1 MISO       ║
║  GPIO21                                                       ║
║  GPIO3  ────────► UART RX ◄─────── Master TX               ║
║  GPIO1  ────────► UART TX ───────► Master RX               ║
║  GPIO22                                                       ║
║  GPIO13*                                                      ║
║                                                              ║
║  3.3V ──────────┬── Power both CC1101 VCC                   ║
║  GND  ────────◄─┼──► Common ground with Master             ║
║  5V   ──────────┘ (Input only)                               ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
```

---

## Step-by-Step Wiring

### Step 1: Master ESP32 + nRF24 Radios

#### nRF24 #1, #2, #3 (VSPI Bus)

| nRF24 Pin | #1 | #2 | #3 | ESP32 GPIO |
|-----------|----|----|----|------------|
| VCC | 3.3V | 3.3V | 3.3V | 3.3V |
| GND | GND | GND | GND | GND |
| CE | 27 | 26 | 25 | GPIO27/26/25 |
| CSN | 15 | 33 | 5 | GPIO15/33/5 |
| SCK | 18 | 18 | 18 | GPIO18 (VSPI_SCK) |
| MOSI | 23 | 23 | 23 | GPIO23 (VSPI_MOSI) |
| MISO | 19 | 19 | 19 | GPIO19 (VSPI_MISO) |
| IRQ | — | — | — | (not connected) |

#### nRF24 #4, #5 (HSPI Bus)

| nRF24 Pin | #4 | #5 | ESP32 GPIO |
|-----------|----|----|------------|
| VCC | 3.3V | 3.3V | 3.3V |
| GND | GND | GND | GND |
| CE | 4 | 32 | GPIO4/32 |
| CSN | 2 | 17 | GPIO2/17 |
| SCK | 14 | 14 | GPIO14 (HSPI_SCK) |
| MOSI | 13 | 13 | GPIO13 (HSPI_MOSI) |
| MISO | 12 | 12 | GPIO12 (HSPI_MISO) |
| IRQ | — | — | (not connected) |

### Step 2: Master ESP32 + OLED Display

| OLED Pin | ESP32 GPIO |
|----------|-----------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO22 (I2C_SCL) |
| SDA | GPIO21 (I2C_SDA) |

**I2C Address:** 0x3C (check your display, some use 0x3D)

### Step 3: Master ESP32 + Buttons

Connect each button between the GPIO and GND (internal pullups used):

| Button | GPIO | Connection |
|--------|------|------------|
| UP | GPIO39 | Button → GND |
| DOWN | GPIO34 | Button → GND |
| SELECT | GPIO36 | Button → GND |

### Step 4: Master ↔ Bridge UART Connection

**IMPORTANT: Cross-connect TX and RX!**

| Master ESP32 | Bridge ESP32 | Wire Color Suggestion |
|-------------|-------------|---------------------|
| GPIO1 (TX0) | GPIO3 (RX0) | Green |
| GPIO3 (RX0) | GPIO1 (TX0) | Yellow |
| GND | GND | Black |

Keep UART wires **under 30cm** for reliable 115200 baud communication.

### Step 5: Bridge ESP32 + CC1101 Radios

Both CC1101s share the SPI bus but have separate CS pins.

| CC1101 Pin | #1 | #2 | ESP32 GPIO |
|------------|----|----|------------|
| VCC | 3.3V | 3.3V | 3.3V |
| GND | GND | GND | GND |
| SCK | 18 | 18 | GPIO18 (SPI_SCK) |
| MISO/SO | 19 | 19 | GPIO19 (SPI_MISO) |
| MOSI/SI | 23 | 23 | GPIO23 (SPI_MOSI) |
| CSN/SS | 5 | 33 | GPIO5/33 |
| GDO0 | 4 | 32 | GPIO4/32 |
| GDO2 | 16 | 17 | GPIO16/17 (optional) |

---

## Power Distribution

### Recommended Power Setup

```
USB Power (5V, 2A+)
    │
    ├──► Master ESP32 (5V pin or USB)
    │      └── 3.3V regulator (built-in)
    │
    ├──► Bridge ESP32 (5V pin or USB)
    │      └── 3.3V regulator (built-in)
    │
    └──► External 3.3V Regulator (AMS1117-3.3 or LD1117-3.3)
           │
           ├──► nRF24 #1 VCC
           ├──► nRF24 #2 VCC
           ├──► nRF24 #3 VCC
           ├──► nRF24 #4 VCC
           ├──► nRF24 #5 VCC
           ├──► CC1101 #1 VCC (Bridge)
           └──► CC1101 #2 VCC (Bridge)
```

### Power Budget

| Component | Qty | Current Each | Total |
|-----------|-----|-------------|-------|
| ESP32 (active WiFi) | 2 | 250mA | 500mA |
| nRF24L01+PA+LNA (TX) | 5 | 300mA | 1500mA |
| CC1101 (TX) | 2 | 50mA | 100mA |
| OLED Display | 1 | 20mA | 20mA |
| **Total Peak** | | | **~2120mA** |
| **Typical (idle)** | | | **~400mA** |

**Minimum Power Supply:** 5V 2.5A (12.5W) for USB-powered operation
**Recommended:** 5V 3A (15W) or separate 3.3V 3A rail for RF modules

### Decoupling Capacitors

Add 100nF ceramic + 10µF electrolytic capacitors near:
- Each nRF24 VCC/GND pins
- Each CC1101 VCC/GND pins
- ESP32 3.3V input

---

## Breadboard Layout (Prototype)

```
┌─────────────────────────────────────────────────────────────┐
│                         BREADBOARD                           │
│                                                              │
│  ┌──────────────┐              ┌──────────────┐             │
│  │ Master ESP32 │              │ Bridge ESP32 │             │
│  │              │              │              │             │
│  │          TX ─┼──────────────┼─ RX          │             │
│  │          RX ─┼──────────────┼─ TX          │             │
│  │         GND ─┼──────────────┼─ GND         │             │
│  └──────────────┘              └──────────────┘             │
│        │                              │                      │
│   ┌────┴────┐                    ┌────┴────┐                │
│   │  nRF24  │                    │ CC1101  │                │
│   │  #1-3   │                    │  #1-2   │                │
│   │ (VSPI)  │                    │  (SPI)  │                │
│   └─────────┘                    └─────────┘                │
│        │                              │                      │
│   ┌────┴────┐                                                 │
│   │  nRF24  │                                                 │
│   │  #4-5   │                                                 │
│   │ (HSPI)  │                                                 │
│   └─────────┘                                                 │
│                                                              │
│   ┌─────────┐                                                │
│   │  OLED   │                                                │
│   │ Display │                                                │
│   └─────────┘                                                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## PCB Design Notes

If designing a custom PCB for V3:

### Layer Stackup
- **4-layer board recommended**
- Layer 1: Signal + RF component placement
- Layer 2: Ground plane
- Layer 3: Power plane (3.3V split)
- Layer 4: Signal routing

### RF Layout Considerations
- Keep nRF24 antenna traces short (<2cm)
- Place ground vias around RF module footprints
- Separate 2.4GHz and 433MHz antenna areas (minimum 5cm apart)
- Use coplanar waveguide or microstrip for RF traces if possible

### Power Planes
- Split 3.3V plane: one for ESP32s, one for RF modules
- Use ferrite beads or 0-ohm resistors to isolate noisy digital from clean RF power
- Place bulk capacitors (100µF) at power entry points

---

## Enclosure & Antennas

### Antenna Placement
- **nRF24 antennas (×5)**: 2.4GHz dipole or PCB antennas, spaced at least 2.5cm apart
- **CC1101 antennas (×2)**: 433MHz helical or 17.3cm wire dipole
- Keep antennas away from ESP32 WiFi antenna area
- Consider SMA connectors for external antennas

### Enclosure Ideas
- Hammond 1590-series aluminum enclosure (RF shielding)
- 3D printed case with ventilation slots
- Separate compartments for digital (ESP32) and RF sections

---

## Testing Sequence

### 1. Test Master ESP32 Alone
1. Upload `starbeam_master.ino`
2. Open Serial Monitor (115200 baud)
3. Verify boot message: "Project Starbeam V3"
4. Check Bridge ping shows OFFLINE (expected without bridge)
5. Test nRF24 menu items (BT Jam, nRF Scan, etc.)

### 2. Test Bridge ESP32 Alone
1. Disconnect from Master
2. Upload `starbeam_cc1101_bridge.ino`
3. Open Serial Monitor (115200 baud)
4. Verify boot message: "Starbeam V3 - CC1101 Bridge"
5. Check CC1101 init messages
6. Send test commands via Serial Monitor:
   - Type `PING` → should respond `PONG`
   - Type `RSSI` → should respond with signal level

### 3. Test Together
1. Connect UART between Master and Bridge
2. Power on Bridge first, then Master
3. Verify Master shows "CC1101 Bridge: ONLINE"
4. Test CC1101 menu items (433MHz Jam, CC Scan, RSSI)
5. Verify both systems work simultaneously

---

## Troubleshooting

### Bridge shows OFFLINE
- Check UART TX/RX are cross-connected (not straight-through)
- Verify common GND between both ESP32s
- Check baud rate matches (115200 on both)
- Try swapping TX/RX wires
- Verify Bridge is powered and running

### nRF24 not working
- Check 3.3V power with multimeter (should be 3.2-3.4V)
- Verify SPI wiring with continuity test
- Check CE/CS pins match config.h
- Try one nRF24 at a time to isolate

### CC1101 not responding
- Check SPI wiring on Bridge ESP32
- Verify SmartRC library is installed
- Check GDO0 pin connections
- Try `INIT` command via Serial Monitor

### UART communication errors
- Keep wires short (<30cm)
- Add 100Ω series resistors on TX/RX lines
- Ensure common ground reference
- Check for noise sources near UART lines
