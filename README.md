# OPENDOM - Embedded ESP32 Smart Home System

OPENDOM is an autonomous, open-source embedded home automation system powered by the ESP32 microcontroller. Designed to operate 100% offline without any cloud dependencies, OPENDOM integrates a Progressive Web App (PWA) with a mobile-first interface, an automated rule evaluation engine, and a multi-sensor monitoring framework.

## Key Features

### Hardware Integration
- Microcontroller: ESP32 DevKit or compatible boards with embedded HTTP server and standalone WiFi Access Point.
- Status Signaling: RGB LED indicator for visual feedback (Red for alarms, Green for active actuators, Blue for idle state).
- Sensor Support: DHT11 (Temperature and Humidity), MQ2 (Gas / Smoke), ACS712 (Current), LDR (Light level), PIR (Motion detection), and Digital Push Button.
- Actuator Support: Relays (Solenoid / Appliance control) and Active Buzzers (Audible alarms and alert patterns).
- Sensor Diagnostics: Fault detection and automatic disconnection handling.

### Software Architecture
- Offline-First PWA: Modern Progressive Web App with responsive, mobile-first interface designed for desktop and mobile devices.
- Automation Engine: Configurable rule engine supporting multi-condition triggers, scheduled events, hysteresis, and action timeouts.
- Security and Access Control: Token-based API authentication and multi-tier user role validation (standard and administrator access).
- REST API: Endpoints for real-time sensor polling, actuator control, rule execution, and system configuration.
- Storage: SPIFFS (Serial Peripheral Interface Flash File System) central JSON configuration storage.

### Security and Reliability
- Multi-sample Sensor Filtering: Noise suppression, triple-reading validation, and adaptive thresholds to ensure measurement stability.
- Memory Management: Heap tracking, static allocations, and connection state checks.
- Network Isolation: Standalone WiFi access point with optional captive portal support.

## RGB LED Status Indicators

- Red: Active Alarm Mode (Critical gas threshold or emergency button triggered)
- Green: Active Actuator Mode (Relay or load active)
- Blue: Idle Mode (System operational and ready)
- Off: System error or boot sequence initialization

## Default System Configuration

- WiFi Access Point SSID: OPENDOM
- WiFi Password: opendom2025
- Standard User Credentials: astron / astron
- Root Administrator Credentials: astron / astronome
- Web Interface Access URL: http://192.168.4.1

## Project File Structure

```
OPENDOM/
├── platformio.ini              # PlatformIO build configuration
├── src/                        # ESP32 C++ source code
│   ├── main.cpp               # Main application and web server handlers
│   ├── Config.cpp             # System configuration parser
│   ├── Sensor.cpp             # Sensor abstraction layer with validation
│   ├── Actuator.cpp           # Actuator controller implementations
│   └── StatusLED.cpp          # Visual RGB signaling controller
├── include/                    # C++ header declarations
│   ├── Config.h
│   ├── Sensor.h
│   ├── Actuator.h
│   └── StatusLED.h
├── data/                       # Offline PWA web interface
│   ├── index.html             # Application markup
│   ├── style.css              # Responsive styling
│   ├── app.js                 # PWA logic and API communication
│   ├── manifest.json          # Web application manifest
│   ├── sw.js                  # Service Worker offline caching controller
│   └── configuration.json     # Default hardware and automation setup
└── README.md                   # System documentation
```

## Quick Start Installation Guide

### Prerequisites
- Installed PlatformIO IDE or PlatformIO CLI.
- ESP32 Development Board (ESP32-WROOM-32 or similar).
- Required sensors and actuators wired according to GPIO configuration.

### Setup Steps

1. Clone the repository:
```bash
git clone https://github.com/palukuba/opendom-esp32.git
cd opendom-esp32
```

2. Build and flash firmware:
```bash
# Compile project firmware
pio run

# Upload SPIFFS web interface files
pio run -t uploadfs

# Upload compiled firmware to ESP32 board
pio run -t upload

# Monitor serial output logs
pio device monitor
```

3. Connect to OPENDOM Access Point:
- Connect your device to WiFi network: `OPENDOM` (Password: `opendom2025`).
- Open a browser and navigate to: `http://192.168.4.1`.
- Authenticate using default credentials: `astron` / `astron`.

## REST API Reference

| Endpoint | Method | Description |
|---|---|---|
| `/login` | POST | Authenticates user credentials and returns session token |
| `/api/sensors` | GET | Returns real-time sensor measurements |
| `/api/actuators` | POST | Toggles or updates actuator state |
| `/api/config` | GET / POST | Retrieves or updates system config (Admin authorization required for POST) |
| `/api/system` | GET | Retrieves system diagnostics, heap memory, and status metrics |
| `/api/time` | POST | Synchronizes system timestamp with client time |

## License and Attribution

This project was created by Paluku B as an embedded systems and IoT engineering implementation.

Source code is released under the MIT License. You are free to modify, distribute, and integrate this software in academic, commercial, or private environments provided the original attribution is retained.
