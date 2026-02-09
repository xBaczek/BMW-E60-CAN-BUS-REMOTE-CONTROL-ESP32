# 🚗 BMW E60 AI Smart Controller (ESP32 + CAN + Gemini)

![Project Status](https://img.shields.io/badge/status-development-orange)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C)
![Language](https://img.shields.io/badge/language-HTML-orange)
![AI Engine](https://img.shields.io/badge/AI-Google%20Gemini-8E75B2)
![AI Engine](https://img.shields.io/badge/AI-ElevenLabs-violet)


###  ⚠️  THIS CODE IS NOT FULLY COMPLETED!!! UPDATE SOON!!  ⚠️ ###


This project is an advanced IoT controller for the **BMW 5 Series (E60/E61)**. It bridges the car's **K-CAN** bus with modern AI technology, allowing for conversations with the vehicle, remote control via a Web Dashboard, and intelligent automation features not found in the original modules.

> **Note:** This project is a Work in Progress (WIP). Tested **ONLY** with Pre-LCI E60 530i LIMOUSINE 2003 MANUAL.

## ✨ Key Features

### 🧠 AI Core (Voice Assistant)
* **Natural Conversation**: Powered by **Google Gemini API**. You don't need rigid commands; just say "I'm cold" or "Open the windows a bit".
* **Human-like Voice**: The car replies using **ElevenLabs TTS** (Text-to-Speech) with a distinct personality. 
* **Context Awareness**: The AI understands the context of the car's status.
* ⚠️ **DISCLAMER**: FOR ELEVEN LABS YOU HAVE LIMITED USAGE!!! IN THE FUTURE I WILL CHANGE THE AI OR STAY WITH TTS BUT YOU CAN ALWAYS USE ONLY TTS !!!

### 🎮 CAN-BUS Control
* **Windows & Sunroof**: Full control (Open/Close/Comfort Open).
* **Climate (IHKA)**: Voice control over temperature, fan speed, and MAX AC.
* **Lighting**: Control over fog lights, interior lights, and hazard lights.
* **Police Mode**: Custom strobe light effect (alternating High Beams/Fog Lights) - *for show use only!*
* **Media**: Volume control integration.

### ⚙️ Automation & Logic
* **Needle Sweep**: Modern "needle sweep" animation on engine startup (RPM/Speedometer test).
* **Reverse Assist**: Automatically mutes the radio and dips the passenger mirror when Reverse gear (0x1D2) is engaged.
* **ESS (Emergency Stop Signal)**: Flashes brake lights rapidly during hard braking (based on DSC pressure data).
* **Rain Close**: Automatically closes windows if rain is detected while parked.
* **Welcome Lights**: Custom light sequence when unlocking the car.

### 📱 Web Dashboard
* **Mobile-First Interface**: Control the car from your phone's browser via WiFi.
* **CAN Sniffer**: Live view of CAN bus traffic for debugging and reverse engineering.
* **Telemetry**: Real-time voltage and engine temperature monitoring.

## 📷 Gallery & Screenshots

Here is a preview of the **Web Dashboard** running on a mobile browser. The interface is designed to be responsive and easy to use while driving.

| Main Dashboard | Live CAN Sniffer |
| :---: | :---: |
| ![Dashboard App](images/app_dashboard.png) | ![CAN Sniffer](images/app_sniffer.png) |
| *Control center: AI Voice, Windows, Lights & Climate* | *Real-time CAN-BUS traffic analysis & debugging tool* |


## 🚧 Roadmap & Development Status

| Feature | Status | Progress | Info |
| :--- | :---: | :--- | :--- |
| **CAN Bus Engine** | ![Stable](https://img.shields.io/badge/Stable-green) | ██████████ WIP | Working on receving and sending can bus codes |
| **AI Voice Assistant** | ![Beta](https://img.shields.io/badge/Beta-yellow) | ████████░░ 80% | Gemini works and normal TTS but ElevenLabs is not working in 100%. Improve latency. |
| **Web Dashboard** | ![Beta](https://img.shields.io/badge/Beta-yellow) | ██████░░░░ 60% | The panel works, but Spotify control is missing. Improve sniffer and logs |
| **Spotify Integration** | ![Hold](https://img.shields.io/badge/Hold-red) | ░░░░░░░░░░ 0% | *Suspended by Spotify API changes.* |

### 📅 Upcoming features (To-Do)
* [ ] **Network:** Works like a DNS server not on Hotspot.
* [ ] **Mobile APP:** More car parameters (LIVE!).
* [ ] **Connection** Via bluetooth for mobile APP.
* [ ] **Hardware:** 3D printed housing design.
* [ ] **Docs:** Recording a demonstration video on YouTube.
* [ ] **Full Control** Remote start, open doors and trunk.
* [ ] **Linux radio** Make app on android auto.


---

## 🛠️ Hardware Requirements

To build this project, you need:

1.  **MCU**: ESP32.
2.  **CAN Interface**: MCP2515 Module (with TJA1050 transceiver).
3.  **Power Supply**: USB or DC-DC Buck Converter (12V to 5V), e.g., LM2596.
4.  **Wiring**: Twisted pair cables for CAN-High and CAN-Low.


### Pinout Configuration (ESP32 to MCP2515)

| ESP32 Pin | MCP2515 Pin | Function |
| :--- | :--- | :--- |
| **D18 (GPIO18)** | SCK | SPI Clock |
| **D19 (GPIO19)** | SO (MISO) | SPI MISO |
| **D23 (GPIO23)** | SI (MOSI) | SPI MOSI |
| **D5 (GPIO5)** | CS | Chip Select |
| **VIN / GND** | VCC / GND | 5V Power Source |

> **Wiring Tip:** Connect CAN-H and CAN-L to the K-CAN bus (Green/Orange twisted pair). Good access points are behind the Radio (CCC/MASK) or at the CAS module.


---
## 🌍 Languages & Downloads

This project is available in two fully localized versions. You can choose your preferred language (Polish or English) in the **Releases** section.

| Version | Language | Description | Download |
| :---: | :---: | :--- | :---: |
| **PL** | 🇵🇱 Polish | Polish Web Interface & Polish AI Voice Assistant. SOON!! IN THE NEXT UPDATE!!!  | [**Download**](../../releases) |
| **EN** | 🇬🇧 English | English Web Interface & English AI Voice Assistant. | [**Download**](../../releases) |

> 📥 **How to install?**
> Go to the [**Releases Page**](../../releases) and download the ZIP file matching your language.
---

## 📚 Installation Guide (VS Code + PlatformIO)

This project uses **PlatformIO**, which makes installation much easier than the standard Arduino IDE. PlatformIO automatically downloads all required libraries and manages dependencies.

The project supports only **ESP32**.

### Step 1: Install Visual Studio Code
Download and install [Visual Studio Code](https://code.visualstudio.com/) for your operating system.

### Step 2: Install PlatformIO Extension
1. Open VS Code.
2. Click on the **Extensions** icon in the left sidebar (or press `Ctrl+Shift+X`).
3. Search for `PlatformIO IDE`.
4. Click **Install**.

*(Wait for the installation to finish - it might require a restart).*

### Step 3: Open the Project
1. Download this repository (Code -> Download ZIP) or clone it using Git.
2. Open VS Code.
3. Go to **File** -> **Open Folder...** and select the folder where you extracted the files.
4. PlatformIO will automatically detect the `platformio.ini` file and start initializing the project (downloading libraries, etc.). This may take a minute.


### Step 4: Configure API KEYS and SYSTEM_PROMPT
 API keys you must create manually.

1. Go to the `src/main.cpp`.
2. Press CTRL+F and type `API KEYS`.
3. Then you have to put your API KEYS from this website:
* [GOOGLE AI API](https://aistudio.google.com/api-keys)
* [GEMINI AI MODELS] https://generativelanguage.googleapis.com/v1beta/models?key={TYPE_YOUR_GOOGLE_API_KEY}
* [GEMINI AI MODELS DOCS](https://ai.google.dev/gemini-api/docs/models?hl=en)
* [ELEVENLABS API](https://elevenlabs.io/app/developers/api-keys)
* [ELEVENLABS VOICE ID](https://elevenlabs.io/app/voice-library)
* [SPOTIFY API](https://developer.spotify.com/dashboard)
> **Tip:** FIRST LOG INTO ELEVENLABS AND THEN CLICK THE LINK IF YOU WANT USE THIS

```cpp
const API_KEY = "YOUR_GOOGLE_AI_API"; 
  const MODEL_NAME = "gemini-2.5-flash-lite"; // you can change this if you want 
  const ELEVEN_KEY = "ELEVANLABS_API_KEY"; 
  const VOICE_ID = "ELEVENLABS_VOICE_ID"; 
  const SPOTIFY_CLIENT_ID = "SPOTIFY_API";
---
```
### **SYSTEM_PROMPT**
 * You have to leave commands, dont remove this. Change only this example text.
```cpp
  const SYSTEM_PROMPT = `
    You are the virtual assistant/lover of the BMW E60 530i driver. Be brief and to the point.
    Your job is to help the driver control the car's functions using voice commands.
    Typically, respond in 1-2 sentences, but occasionally, if you feel like it, elaborate.
    Speak in short sentences.

    
   IF you need to do something, add the command in parentheses at the very end:
- Open all windows -> [WIN_DOWN]
- Close all windows -> [WIN_UP]
- Open driver's window -> [WIN_FL_DOWN]
- Close driver's window -> [WIN_FL_UP]
- Open passenger window -> [WIN_FR_DOWN]
- Close passenger window -> [WIN_FR_UP]
- Open left rear window -> [WIN_RL_DOWN]
- Close left rear window -> [WIN_RL_UP]
- Open right rear window -> [WIN_RR_DOWN]
- Close right rear window -> [WIN_RR_UP]
- Open sunroof -> [roof_open]
- Close sunroof -> [roof_close]
- tilt sunroof -> [roof_tilt]
- turn off AC -> [AC_OFF]
- Increase temperature -> [TEMP_UP]
- Decrease temperature -> [TEMP_DOWN]
- Increase air flow -> [FAN_UP]
- Decrease air flow -> [FAN_DOWN]
- Max air flow -> [AC_MAX]
- Fog lights -> [LIGHT_FOG]
- Hazard warning lights -> [LIGHT_HAZARDS]
- Interior lights -> [LIGHT_INTERIOR]
- Welcome lights -> [LIGHT_WELCOME]
- Police/Hide -> [POLICE]
- Needle sweep -> [TEST_CLOCKS]
- Volume up -> [VOL_UP]
- Volume down -> [VOL_DOWN]

If you need to turn something on once, use the command only once. For example, [WIN_DOWN], [AC_MAX].

If you need to change something multiple times, e.g., temperature or airflow, repeat the command as many times as necessary and use the format e.g., [TEMP_UP:X] where X is the number of REPETITIONS. For example, "INCREASE temperature by 3 degrees" -> [TEMP_UP:3]. Do the same with any other command that requires multiple use.
  `;
```
## Step 5: Configure your WIFI
1. Go to the `src/main.cpp`.
2. Press CTRL+F and type `ssid =`.
3. Then you have to put ssid and password to your Hotspot from phone:

```cpp
 const char* ssid = "SSID";     // Your Hotspot Name
const char* password = "PASSWORD";  // Hotspot Password
```
## Step 6: Upload CODE and MONITOR your ESP32
1. Connect your ESP32 via USB.
2. Click button `Platformio: New Terminal` (On the bottom bar with terminal icon).
3. Type: `platformio run --target upload` and click ENTER.
4. After uploading type in terminal: `platformio device monitor --baud 115200` and you can click RESET on ESP to refresh the device.

## Step 7: Enter to Web Panel on Phone
1. Open CHROME!!! and type in search bar: `chrome://flags/`.
2. Search `Insecure origins treated as secure`.

## 🤝 Contributing & Support

**Do you own a BMW E60, E65, E90, or even an E46?**
I am actively looking for contributors and beta testers to help map CAN/K-Bus codes for different BMW models!

If you want to help develop this project, share your findings, or if you simply **need help with the installation**:

* 💬 **Discord (Preferred):** DM me directly at **`@xemuss`**
* 🐛 **Found a bug?** You can also open a [GitHub Issue](../../issues).

Don't hesitate to write to me if you get stuck or have ideas for new features!


## ⚠️ Disclaimer

This project involves connecting custom hardware to your vehicle's CAN bus. 
**Use at your own risk.** The author is not responsible for:
* Drained batteries (ensure proper sleep mode logic is used).
* Electrical damage to car modules.
* Legal issues regarding the use of like "Police Mode" or other lighting effects on public roads.
* Voided warranties.

*Project created for educational and hobby purposes.*

---

