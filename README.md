# Wireless Dual-Channel WiFi Oscilloscope (Seeed XIAO ESP32-C3)

This project implements a **dual-channel wireless oscilloscope** and **waveform generator** using the **Seeed Studio XIAO ESP32-C3**.  
Data is transmitted in **real-time via WebSockets** to a browser or desktop client for visualization.  

---

## ✨ Features
- 📡 **WiFi-based high-speed data transfer** (WebSocket server on port `81`)  
- 📊 **Dual ADC input channels** for real-time sampling  
- ⚡ **Configurable sample rate** (`10 Hz – 10 kHz`)  
- 📝 **JSON data format** for easy client parsing  
- 🔄 **Button-controlled waveform generator** (SINE, SQUARE, SAWTOOTH, TRIANGLE)  
- 🌐 **WebSocket commands** for remote control of acquisition and waveform  
- 💡 **Status LED** for connection and sampling indication  

---

## 🔌 Hardware Setup

| Function            | Pin (XIAO ESP32-C3) | Notes                                |
|---------------------|---------------------|--------------------------------------|
| Channel 1 (ADC1)    | D0 → GPIO1 (A0)     | Connect to input amplifier           |
| Channel 2 (ADC2)    | D2 → GPIO3 (A1)     | Connect to input amplifier           |
| DAC / Waveform Out  | D4                  | PWM output (use RC filter)           |
| Push Button         | D5                  | Pulls to GND (internal pull-up used) |
| Status LED          | D6 (built-in)       | Blinks when active                   |

⚠️ **Voltage Input Range:** `0–3.3V`. Use a resistor divider for higher voltages.  

---

## 📡 WiFi Configuration

Edit the following in the sketch before uploading:

```cpp
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
```

When connected, the ESP32-C3 prints its **local IP address**:  
```
ws://<device-ip>:81
```

---

## 🧩 WebSocket API

All messages are **plain text commands** or **JSON objects**.

### Commands (client → ESP32-C3)
- `START` → Begin sampling  
- `STOP` → Stop sampling  
- `RATE:<Hz>` → Change sample rate (10–10000 Hz)  
- `WAVE:SINE` → Set waveform to **SINE**  
- `WAVE:SQUARE` → Set waveform to **SQUARE**  
- `WAVE:SAW` or `WAVE:SAWTOOTH` → Set waveform to **SAWTOOTH**  
- `WAVE:TRIANGLE` → Set waveform to **TRIANGLE**  

### Events (ESP32-C3 → client)

**Configuration:**
```json
{
  "type": "config",
  "channels": 2,
  "sampleRate": 1000,
  "bufferSize": 500,
  "voltageRef": 3.3,
  "adcResolution": 4095,
  "waveform": "SINE"
}
```

**Data Packet:**
```json
{
  "type": "data",
  "timestamp": 123456,
  "channel1": [0.12, 0.25, ...],
  "channel2": [0.15, 0.22, ...]
}
```

**Waveform Update:**
```json
{
  "type": "waveform",
  "value": "SQUARE"
}
```

---

## 🛠️ Software Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/)  
- **ESP32 board support** installed in Arduino IDE  
- Libraries:
  - `WiFi.h`  
  - `WebSocketsServer.h`  
  - `ArduinoJson.h`  

---

## 🚀 Usage
1. Upload the code to your **Seeed XIAO ESP32-C3**.  
2. Open Serial Monitor → Note the device’s IP.  
3. Connect your client app (browser or desktop) to:  
   ```
   ws://<device-ip>:81
   ```
4. Send commands (`START`, `STOP`, etc.) and stream real-time oscilloscope data.  

---

## 📷 Example Workflow
1. Power up the device.  
2. Press the **button** to cycle through waveform outputs.  
3. Use a browser-based oscilloscope client (or your own WebSocket app) to visualize signals in real-time.  

---

## 🚀 Oscilloscope emulator 
=> : https://osc.jlcodelabs.com/esp32-oscilloscope

## 📌 Notes
- This code is tailored for the **Seeed Studio XIAO ESP32-C3** (⚠️ not ESP32-S3).  
- Only specific pins are ADC-capable (A0–A5).  
- DAC is simulated via **PWM + RC filter**, since ESP32-C3 lacks a true DAC.  

---
