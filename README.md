# iotPlatform Firmware

**Modular, event-driven firmware for ESP32 IoT edge devices.  
Designed for robust, scalable, and maintainable control of machines, solar trackers, relays, sensors, and cameras.**

---

## 🚀 Overview

`iotPlatform` is a modern, production-grade firmware template for ESP32-based IoT devices, built with ESP-IDF, supporting asynchronous, event-driven architecture.  
It orchestrates complex machines and distributed sensor networks in real-time, with MQTT, HTTP API, dynamic features, and full system observability.

---

## ✨ Key Features

- **Modular EventBus**: Decoupled, plug-and-play architecture via a FreeRTOS-based event bus.
- **Async Boot & Operation**: Each subsystem (filesystem, WiFi, MQTT, sensors, relays, camera, etc.) is initialized and supervised asynchronously, with robust error handling.
- **Dynamic Feature Activation**: Easily enable/disable features at build time (Kconfig) or runtime.
- **Persistent, Safe Configuration**: WiFi/AP, sensors, and system state stored in NVS & LittleFS, with safe fallback.
- **Multi-protocol Communication**: MQTT with async request/response, multi-topic support; HTTP REST API for local management and diagnostics.
- **Device Management**: WiFi manager (AP/STA/Bridge), OTA-ready, robust reconnection.
- **Real-time Diagnostics**: Exposes system status, task health, memory stats, and module states via API/logs.
- **Rich hardware support**: Relays, solar trackers, stepper motors, camera modules, and generic sensors.
- **Safe and robust**: Stack watermark checks, memory pool management, modular tasking (FreeRTOS), async error propagation.

---

## 🏗️ Architecture

```
+-------------------+        +---------------------+
|    EventBus       |<------>|     Subscriber      |  (MQTT, FS, Camera, Sensors, Relays, ...)
+--------^----------+        +---------------------+
         |                             ^
         |                             |
+--------+----------+        +---------+---------+
| Event Producers   |        |  HTTP / MQTT API  |
| (WiFi, Boot, FS)  |------->|     / Status      |
+-------------------+        +-------------------+
```

---

## 🛠️ Main Components

### 1. EventBus

- FreeRTOS queue backbone for async messaging
- Any module can emit/subscribe to status, commands, errors, queries
- Error isolation and non-blocking logic

### 2. Async Boot Sequence

- Subsystems subscribe to "boot events" and start only when ready
- Ensures correct order: filesystem → config → WiFi → MQTT → devices

### 3. Feature Modules

- Filesystem: Async mount, status events, SD/LittleFS
- WiFi Manager: AP/STA/Bridge, dynamic config, fallback
- MQTT Client: Multi-topic, request/response, async, robust reconnection
- HTTP API: RESTful endpoints for features/status
- Device Handlers: Pluggable, event-driven (relays, solar tracker, sensors, camera)
- Persistence: Robust configuration/state management

### 4. Diagnostics/Observability

- System status is always available via logs and API (boot, module states, errors, memory, stack usage)

---

## 🚦 Event-Driven Flow

1. Boot:  
    `app_main` → Essential init (NVS, event loop, eventbus)  
2. Boot Event:  
    Emit `BOOT_CAN_START`  
3. Filesystem:  
    Subscribes, mounts FS, emits `FS_MOUNTED`
4. Config:  
    Loads, emits `CONFIG_READY`
5. WiFi:  
    Connects, emits `WIFI_CONNECTED`
6. MQTT:  
    Connects on `WIFI_CONNECTED`
7. Devices:  
    Subscribe and act on events (relays, sensors, camera, ...)
8. Status API:  
    Exposes state via HTTP/MQTT/status topic

---

## 🔧 Extending/Customizing

- Add a new device: implement a handler, subscribe to events, and emit yours.
- Add new event types: extend `EventType` enum.
- All logic is decoupled; modules interact via the bus.

---

## 📦 Build and Flash

- Configure:
  ```bash
  idf.py menuconfig
  ```
- Build & Flash:
  ```bash
  idf.py build
  idf.py -p <PORT> flash
  idf.py monitor
  ```

---

## 🙌 Contributing

PRs and feature requests welcome!  
Open issues/discussions for bugs, questions, or ideas.

---

## 📄 License

MIT — See LICENSE file.

---
