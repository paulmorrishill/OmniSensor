# WiFi Temperature Sensor - Refactored

This project has been refactored from a monolithic Arduino sketch into a modular, object-oriented structure for better maintainability and code organization.

## Project Structure

### Original File
- `WifiTempSensor.ino` - Original monolithic implementation

### Refactored Files
- `WifiTempSensor_Refactored.ino` - New main Arduino sketch using modular classes
- **Manager Classes** - Separate responsibilities into focused classes

## Class Architecture

### 1. EEPROMManager (`EEPROMManager.h/.cpp`)
**Responsibility**: Persistent storage management
- Device ID storage and retrieval
- WiFi credentials management
- Device configuration (mode, alias, server URL)
- EEPROM read/write utilities

**Key Methods**:
- `hasDeviceId()`, `getDeviceId()`, `setDeviceId()`
- `hasWiFiCredentials()`, `getSSID()`, `getPassword()`, `saveWiFiCredentials()`
- `getMode()`, `setMode()`, `getAlias()`, `setAlias()`
- `clearAll()` - Factory reset functionality

### 2. WiFiManager (`WiFiManager.h/.cpp`)
**Responsibility**: WiFi connectivity and network management
- WiFi connection using saved credentials
- Access Point (hotspot) mode for configuration
- Network scanning and encryption detection
- WiFi status management

**Key Methods**:
- `connectUsingSavedCredentials()` - Connect to stored WiFi
- `enableHotspotMode()` - Start configuration AP
- `scanNetworks()` - Scan available networks
- `getEncryptionName()` - Decode encryption types

### 3. SensorManager (`SensorManager.h/.cpp`)
**Responsibility**: Hardware sensor management
- Temperature sensor (Dallas DS18B20) operations
- Soil moisture sensor readings
- Sensor power management
- Analog readings

**Key Methods**:
- `readTemperature()` - Get temperature from DS18B20
- `readSoilMoisture()` - Read soil sensor via analog pin
- `powerSensorOn()`, `powerSensorOff()` - Control sensor power

### 4. WebServerManager (`WebServerManager.h/.cpp`)
**Responsibility**: HTTP server and web interface
- HTTP route handling
- Web-based configuration interface
- SSDP (UPnP) discovery
- OTA (Over-The-Air) updates
- API endpoints for device control

**Key Routes**:
- `/` - Main page (config or status)
- `/configure` - WiFi and device configuration
- `/control` - Device control interface
- `/report` - Trigger sensor reporting
- `/output-on`, `/output-off` - Manual sensor control

### 5. DeviceManager (`DeviceManager.h/.cpp`)
**Responsibility**: Device lifecycle and server communication
- Device initialization and identification
- Power management (sleep/wake cycles)
- Server communication and registration
- Button handling and factory reset
- Main device loop coordination

**Key Methods**:
- `init()` - Initialize device components
- `registerWithServer()` - Register device with server
- `reportNow()` - Log sensor data to serial output
- `askServerIfShouldStayUp()` - Check server for wake commands
- `handleButtonPress()` - Process button interactions
- `loop()` - Main device operation loop

## Benefits of Refactoring

### 1. **Separation of Concerns**
Each class has a single, well-defined responsibility, making the code easier to understand and maintain.

### 2. **Modularity**
Components can be modified, tested, or replaced independently without affecting other parts of the system.

### 3. **Reusability**
Classes can be reused in other projects or extended for additional functionality.

### 4. **Testability**
Individual components can be unit tested in isolation.

### 5. **Maintainability**
Bug fixes and feature additions are localized to specific classes, reducing the risk of introducing regressions.

### 6. **Readability**
The main Arduino sketch is now much cleaner and shows the high-level flow clearly.

## Device Modes

The device supports multiple operating modes:
- **0**: Servo control
- **1**: Input switch
- **2**: Thermometer
- **3**: Soil sensor
- **4**: Relay control
- **5**: RGB LED control

## Configuration

### Initial Setup
1. Device creates WiFi hotspot `WiFiSense_[ID]` with password `password`
2. Connect to hotspot and navigate to `192.168.1.1`
3. Configure WiFi credentials, device alias, server URL, and operating mode
4. Device reboots and connects to configured WiFi

### Factory Reset
Hold the button for 10+ seconds to clear all configuration and restart setup process.

## Hardware Connections

- **Button**: GPIO 4 (with internal pullup)
- **Green LED**: GPIO 13
- **Red LED**: GPIO 12
- **Sensor Power**: GPIO 14
- **OneWire (Temperature)**: GPIO 5
- **Auxiliary**: GPIO 5
- **Analog Input**: A0 (soil moisture)

## Dependencies

### Core ESP8266 Libraries (included with ESP8266 core)
- ESP8266WiFi
- ESP8266WebServer
- ESP8266SSDP
- ESP8266HTTPUpdateServer
- EEPROM

### External Libraries
- ArduinoJson (^6.21.3)
- DallasTemperature (^3.11.0)
- OneWire (^2.3.7)

## Setup and Installation

### Option 1: PlatformIO (Recommended)
1. Install [PlatformIO](https://platformio.org/) extension in VS Code
2. Open this project folder in VS Code
3. PlatformIO will automatically install all dependencies listed in `platformio.ini`
4. Select your board environment (nodemcuv2, d1_mini, or esp12e)
5. Build and upload using PlatformIO

### Option 2: Build Scripts (Command Line)
Use the provided build scripts to generate .bin files for manual upload:

#### PowerShell Script (Windows/Linux/macOS)
```powershell
# Build all environments
.\build.ps1

# Build specific environment
.\build.ps1 -Environment nodemcuv2

# Clean and build
.\build.ps1 -Clean

# Build and upload (will prompt for environment if multiple)
.\build.ps1 -Upload

# Build and upload to specific port
.\build.ps1 -Environment nodemcuv2 -Upload -Port COM3
```

#### Batch Script (Windows)
```batch
# Simple build for all environments
build.bat
```

**Generated Files:**
- `WifiTempSensor_nodemcuv2.bin` - For NodeMCU v2 boards
- `WifiTempSensor_wemos_d1_mini.bin` - For Wemos D1 Mini boards
- `WifiTempSensor_esp12e.bin` - For ESP12E modules

### Option 3: Arduino IDE
1. Install the ESP8266 board package in Arduino IDE
2. Install required libraries through Library Manager:
   - Search and install "ArduinoJson" by Benoit Blanchon
   - Search and install "DallasTemperature" by Miles Burton
   - Search and install "OneWire" by Jim Studt, Tom Pollard, Robin James
3. Open `WifiTempSensor.ino` in Arduino IDE
4. Select your ESP8266 board and upload

### Manual Upload Options
Once you have a .bin file, you can upload it using:

1. **ESP Flash Download Tool** (Espressif official tool)
   - Download from Espressif website
   - Load the .bin file at address 0x00000
   - Select your COM port and flash

2. **esptool.py** (Command line)
   ```bash
   esptool.py --port COM3 --baud 921600 write_flash 0x00000 WifiTempSensor_nodemcuv2.bin
   ```

3. **PlatformIO Upload**
   ```bash
   pio run -e nodemcuv2 --target upload --upload-port COM3
   ```

## Usage

1. Upload the code along with all header and implementation files
2. The device will automatically initialize all managers and start operation
3. Use the web interface for configuration and control
4. Monitor sensor data through the device's web interface or serial output

## Future Enhancements

The modular structure makes it easy to add:
- Additional sensor types
- New communication protocols
- Enhanced web interfaces
- Different IoT platform integrations
- Advanced power management features