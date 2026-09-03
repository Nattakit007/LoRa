<h1 align="center">LoRaWAN Water Level Monitor</h1>

<h3 align="center">Heltec WiFi LoRa 32 V3 • ESP32-S3 • LoRaWAN AS923 • Ultrasonic Telemetry</h3>

<p align="center">
  <img src="https://readme-typing-svg.demolab.com?font=Fira+Code&size=20&pause=1000&color=2F80ED&center=true&vCenter=true&width=650&lines=LoRaWAN+Water+Level+Telemetry;ESP32-S3+%26+Heltec+V3+Architecture;JSN-SR04T+Ultrasonic+Acquisition;Onboard+OLED+Menu+%26+Field+Tuning" alt="Typing SVG" />[cite: 1]
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32--S3-E7352C?style=for-the-badge&logo=espressif&logoColor=white" />
  <img src="https://img.shields.io/badge/Hardware-Heltec%20V3-03234B?style=for-the-badge" />
  <img src="https://img.shields.io/badge/LoRaWAN-AS923-00AEEF?style=for-the-badge&logo=lorawan&logoColor=white" />
  <img src="https://img.shields.io/badge/IDE-PlatformIO-F38B00?style=for-the-badge&logo=platformio&logoColor=white" />
</p>

---

### Overview

An autonomous, field-configurable water level telemetry system built around the **Heltec WiFi LoRa 32 V3 (ESP32-S3)**. The system measures water depth via a weatherproof ultrasonic sensor, outputs diagnostics to an onboard OLED display, and transmits JSON payloads over **LoRaWAN (AS923)** to The Things Network (TTN) or ChirpStack. Parameter tuning is supported directly in the field via a rotary encoder interface without reflashing firmware.

---

### Key Features

* **Acoustic Ranging:** JSN-SR04T waterproof ultrasonic transducer for real-time liquid level measurement.
* **On-Device UI:** Built-in OLED menu displaying real-time telemetry, signal status, and system settings.
* **Field Configuration:** Rotary encoder menu allows runtime adjustments to drought/flood limits, tank height, and sensor offsets without code modification.
* **Non-Volatile Storage (NVS):** Retains runtime calibration values across power cycles.
* **Dual Boot Logic:** 10-second startup window to enter configuration mode or automatically initiate LoRaWAN uplinks.
* **Network Interoperability:** Transmits structured JSON payloads over regional AS923 channel plans to TTN or ChirpStack.

---

### System Architecture

```mermaid
flowchart LR
    subgraph Edge [Acquisition & Edge Control]
        Sensor[JSN-SR04T Ultrasonic] -->|Echo / Trig| MCU[Heltec WiFi LoRa 32 V3<br>ESP32-S3]
        Encoder[Rotary Encoder EC11] -->|Menu Navigation| MCU
        MCU -->|Live Metrics / UI| OLED[Onboard OLED Display]
    end

    subgraph RF [LoRaWAN Telemetry]
        MCU -->|AS923 Uplink| Gateway[LoRaWAN Gateway]
        Gateway --> LNS[Network Server<br>TTN / ChirpStack]
    end

    style MCU stroke:#2F80ED,stroke-width:2px
    style Sensor stroke:#2F80ED,stroke-width:2px
    style Encoder stroke:#2F80ED,stroke-width:2px
    style OLED stroke:#2F80ED,stroke-width:2px
    style Gateway stroke:#2F80ED,stroke-width:2px
    style LNS stroke:#2F80ED,stroke-width:2px
