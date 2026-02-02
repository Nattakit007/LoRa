

###THIS IS PRE-RELEASE (NOT TEST ON THE REAL-BOARD YET)




# LoRaWAN Water Level Monitor

A water level monitoring system based on the **Heltec WiFi LoRa 32 V3 (ESP32-S3)**.  
This project uses a waterproof ultrasonic sensor to measure water level, displays status and configuration menus on the onboard OLED, and transmits telemetry data via **LoRaWAN (AS923)**.  
A rotary encoder allows full field configuration without reflashing firmware, making it suitable for lab demonstrations and real-world deployments.

---

## 🌟 Features
* **Real-Time Water Level Measurement:** Uses the JSN-SR04T ultrasonic sensor to measure distance and calculate water depth.
* **OLED Menu Interface:** Built-in screen for live readings, system status, and configuration menus.
* **Field Configurable (No Reprogramming):** Adjust drought threshold, flood threshold, tank depth, and sensor height using a rotary encoder.
* **LoRaWAN Telemetry:** Sends JSON-formatted sensor data to TTN or ChirpStack (AS923).
* **Auto / Setup Boot Logic:** 10-second startup window to enter setup mode or automatically start LoRaWAN transmission.
* **Non-Volatile Storage:** Configuration values are retained across power cycles.

---

## 🛠 Hardware Requirements
1. **MCU:** Heltec WiFi LoRa 32 V3 (ESP32-S3)
2. **Sensor:** JSN-SR04T Waterproof Ultrasonic Sensor
3. **Input:** Rotary Encoder (EC11 or equivalent)
4. **Display:** Built-in Heltec OLED
5. **Misc:** Breadboard, jumper wires, power source

---

## 🔌 Pin Map
| Component | Function | ESP32 Pin |
| :--- | :--- | :--- |
| **JSN-SR04T** | Trigger | `GPIO 46` |
| | Echo | `GPIO 45` |
| **Rotary Encoder** | A (CLK) | `GPIO 36` |
| | B (DT) | `GPIO 37` |
| | Switch | `GPIO 38` |
| **OLED (Heltec)** | SDA | `GPIO 17` |
| | SCL | `GPIO 18` |

---

## 📦 Software Dependencies
Built using **PlatformIO**. Required libraries are managed via `platformio.ini`.

* `Heltec ESP32 Dev-Boards`
* `ESP32_LoRaWAN`
* `AsyncDelay`
* `Adafruit GFX`
* `Adafruit BusIO`
* `NewPing` (Optional / Alternative)

---

## ⚙️ Installation & Build
1. Clone this repository.
2. Open the project in **VS Code** with the **PlatformIO** extension installed.
3. **Important Build Fix (ESP32 Core v3.x):**

   To prevent `GPIO_PIN_COUNT` compilation errors, ensure your `platformio.ini` contains:
   ```ini
   build_flags =
       -DGPIO_PIN_COUNT=SOC_GPIO_PIN_COUNT if not working just replace it in the libdep's src
