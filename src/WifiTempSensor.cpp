#include <ArduinoJson.hpp>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "platform_config.h"

#ifdef ESP8266_PLATFORM
    #include <Servo.h>
#elif defined(ESP32_PLATFORM)
    #include <ESP32Servo.h>
#endif

// Include our custom classes
#include "EEPROMManager.h"
#include "WiFiManager.h"
#include "SensorManager.h"
#include "WebServerManager.h"
#include "DeviceManager.h"

#ifdef ESP8266_PLATFORM
// ESP8266 specific includes
extern "C" {
#include "user_interface.h"
#include "WifiTempSensor.h"
}
#endif

// Pin definitions
const int oneWireBus = 5;
const int SENSE_POWER_PIN = 14;
const int BUTTON_PIN = 4;
const int GREEN_PIN = 13;

// Manager instances
EEPROMManager* eepromManager;
WiFiManager* wifiManager;
SensorManager* sensorManager;
WebServerManager* webServerManager;
DeviceManager* deviceManager;

void connectToWiFi();

void setup() {
    Serial.begin(115200);
    Serial.println("\nOmnisensor Refactored\n");
    
    // Initialize managers
    eepromManager = new EEPROMManager();
    eepromManager->init();
    
    // Initialize WiFi first (required for MAC address access)
    WiFi.mode(WIFI_STA);
    
    wifiManager = new WiFiManager();
    sensorManager = new SensorManager(oneWireBus, SENSE_POWER_PIN);
    deviceManager = new DeviceManager(eepromManager, sensorManager);
    webServerManager = new WebServerManager(eepromManager, wifiManager, sensorManager, deviceManager);
    
    // Initialize device and pins
    deviceManager->init();
    sensorManager->init();
    
    // Configure A0 for analog reading and seed random number generator
    #ifdef ESP32_PLATFORM
        // ESP32 doesn't need pinMode for analog pins, but ensure it's available
        pinMode(A0, INPUT);
    #endif
    randomSeed(analogRead(A0));
    
    // Initialize WiFi with device ID and EEPROM manager
    wifiManager->init(deviceManager->getDeviceId(), eepromManager);
    
    // Check for button pressed
    bool buttonPressed = !digitalRead(BUTTON_PIN);
    if (buttonPressed) {
        deviceManager->handleButtonPress();
    }
    
    // Initialize web server
    webServerManager->init();
    
    // Setup SSDP
    webServerManager->setupSSDP(deviceManager->getSerialNumber(), deviceManager->getDeviceId());
    
    // Connect to WiFi
    connectToWiFi();
    
    // If WiFi is connected, send failure log and register with server
    if (WiFi.status() == WL_CONNECTED) {
        deviceManager->sendFailureLogToServer();
        deviceManager->registerWithServer();
    }
}

void connectToWiFi() {
    if (!eepromManager->hasWiFiCredentials()) {
        Serial.println("Wifi creds not stored entering wifi config mode.");
        eepromManager->clearAll();
        wifiManager->enableHotspotMode();
        digitalWrite(GREEN_PIN, HIGH);
        deviceManager->setStayAwake(true);
        return;
    }
    
    String ssid = eepromManager->getSSID();
    String password = eepromManager->getPassword();
    
    if (!wifiManager->connectUsingSavedCredentials(ssid, password)) {
        Serial.println("Failed to connect, entering config mode");
        wifiManager->enableHotspotMode();
        digitalWrite(GREEN_PIN, HIGH);
        deviceManager->setStayAwake(true);
    }
}

void loop() {
    if (deviceManager->shouldStayAwake()) {
        webServerManager->handleClient();
    }
    
    deviceManager->loop();
}