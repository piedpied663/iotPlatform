---

## 🧩 Module Overview

| Module         | Status     | Notes                                             |
|----------------|------------|---------------------------------------------------|
| EventBus       | ✅ Stable  | Central messaging backbone                        |
| MQTT Client    | ✅ Stable  | Multi-topic support with robust reconnection      |
| WiFi Manager   | ✅ OK      | AP/STA modes with fallback handling               |
| HTTP API       | ⚠️ WIP    | Basic server present, endpoints incomplete        |
| OTA Update     | ❌ Missing | Not implemented yet                               |
| Relays         | ❌ Missing | Placeholder or logic to be implemented            |
| Solar Tracker  | ❌ Missing | Mentioned but no current implementation           |
| Camera         | ✅ OK      | LED control and capture working, needs extension |
| Persistence    | ✅ OK      | Binary-safe configuration storage (NVS, FS)       |

---

## 🔜 Work In Progress

The following components are planned or partially implemented. Contributions or suggestions are welcome!

- **OTA Update System**  
  Secure over-the-air updates using partition switching and firmware validation.

- **Extended HTTP API**  
  Additional REST endpoints for device control, system status, and live configuration access.

- **Stepper Motor & Solar Tracking Control**  
  Drivers and logic for solar tracking and axis control using stepper motors.

- **Sensor Framework**  
  Event-driven drivers and integration for analog/digital sensors (temperature, voltage, etc.).

- **WiFi Bridge Mode / NAT**  
  Bridging AP and STA mode with potential NAT/DHCP relay for advanced routing.

- **MQTT Request/Response Framework**  
  Asynchronous command-reply abstraction over MQTT topics for remote control.

- **System Status via HTTP**  
  JSON-based status reporting over HTTP, including memory, task states, and diagnostics.

- **Persistent Config Tooling / UI**  
  A future web interface or CLI tool to read/write configuration safely.

- **I2C/SPI Hardware Modules**  
  Support for popular hardware sensors and modules like BME280, INA219, etc.

---
