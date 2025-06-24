#include "DeviceManager.h"
#include "EEPROMManager.h"
#include "SensorManager.h"
#include "WiFiManager.h"
#include <WiFiClient.h>

#ifdef ESP8266_PLATFORM
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort(struct tcp_pcb* pcb);

void tcpCleanup() {
    while (tcp_tw_pcbs != NULL) {
        tcp_abort(tcp_tw_pcbs);
    }
}
#elif defined(ESP32_PLATFORM)
// ESP32 doesn't need TCP cleanup in the same way
void tcpCleanup() {
    // No-op for ESP32
}
#endif

DeviceManager::DeviceManager(EEPROMManager* eeprom, SensorManager* sensor, WiFiManager* wifi) :
    eepromManager(eeprom), sensorManager(sensor), wifiManager(wifi), deviceId(0), operatingMode(0),
    stayAwake(false), timeAtLastSend(0), timeAtLastCheck(0) {
}

DeviceManager::~DeviceManager() {
}

void DeviceManager::init() {
    initPins();
    initDeviceId();
    initSerialNumber();
    operatingMode = eepromManager->getMode();
    Serial.print("Loaded in mode ");
    Serial.println(operatingMode);
}

void DeviceManager::initPins() {
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(SENSE_POWER_PIN, OUTPUT);
    pinMode(AUX_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(SENSE_POWER_PIN, LOW);
}

void DeviceManager::initDeviceId() {
    if (!eepromManager->hasDeviceId()) {
        Serial.println("First start configuration run. Generating ID.");
        int id = random(10000, 99999);
        eepromManager->setDeviceId(id);
    }
    
    deviceId = eepromManager->getDeviceId();
    Serial.print("WIFI Sense Loaded Device ID: ");
    Serial.println(deviceId);
}

void DeviceManager::initSerialNumber() {
    serialNumber = "LT1";
    serialNumber += WiFi.macAddress();
    serialNumber += "";
    serialNumber += deviceId;
    serialNumber.replace(":", "");
    Serial.print("Serial number: ");
    Serial.println(serialNumber);
}

void DeviceManager::handleButtonPress() {
    unsigned long buttonPressedStart = millis();
    bool ledState = false;
    unsigned long lastFlash = 0;
    const unsigned long RESET_THRESHOLD = 10000; // 10 seconds to reset
    const unsigned long MAX_FLASH_INTERVAL = 500; // Slowest flash (ms)
    const unsigned long MIN_FLASH_INTERVAL = 25;  // Fastest flash (ms)

    while (!digitalRead(BUTTON_PIN)) {
        unsigned long buttonPressDuration = millis() - buttonPressedStart;
        
        if (buttonPressDuration > 1000) {
            digitalWrite(GREEN_PIN, HIGH); // Disable sleep
            stayAwake = true;
        }
        
        if (buttonPressDuration > RESET_THRESHOLD) {
            // Hard reset
            Serial.println("Hard reset detected");
            clearConfiguration();
            digitalWrite(RED_PIN, HIGH);
            delay(1000);
            PlatformUtils::restart();
            return;
        }
        
        // Calculate flash interval mathematically based on progress
        // As we approach reset, the interval decreases exponentially
        float progress = (float)buttonPressDuration / RESET_THRESHOLD;
        progress = constrain(progress, 0.0, 1.0);
        
        // Exponential decay: starts slow, accelerates rapidly toward the end
        float flashFactor = 1.0 - pow(progress, 2.5);
        unsigned long flashInterval = MIN_FLASH_INTERVAL +
            (unsigned long)(flashFactor * (MAX_FLASH_INTERVAL - MIN_FLASH_INTERVAL));
        
        // Flash the RED LED to show reset progress
        if (millis() - lastFlash >= flashInterval) {
            ledState = !ledState;
            digitalWrite(RED_PIN, ledState ? HIGH : LOW);
            lastFlash = millis();
        }
        
        delay(10); // Small delay to prevent excessive CPU usage
    }
    
    // Turn off LEDs when button is released
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
}

void DeviceManager::clearConfiguration() {
    eepromManager->clearAll();
}

bool DeviceManager::handleConfigurationMode() {
    bool wifiConfigured = eepromManager->hasWiFiCredentials();
    bool serverUrlConfigured = eepromManager->hasServerUrl();
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    bool inConfigMode = wifiManager->isInConfigMode();
    
    // If WiFi credentials are not configured, must stay in config mode
    if (!wifiConfigured) {
        static unsigned long lastConfigMessage = 0;
        if (millis() - lastConfigMessage > 30000) {
            Serial.println("WiFi credentials not configured - staying in config mode");
            lastConfigMessage = millis();
        }
        
        // Enter config mode if not already in it
        if (!inConfigMode) {
            Serial.println("Entering config mode");
            wifiManager->enableHotspotMode();
            digitalWrite(GREEN_PIN, HIGH);
        }
        
        setStayAwake(true);
        return true; // Stay awake for configuration
    }
    
    // WiFi is configured - check if server URL is also configured
    if (!serverUrlConfigured) {
        static unsigned long lastConfigMessage = 0;
        if (millis() - lastConfigMessage > 30000) {
            Serial.println("Server URL not configured - staying in config mode");
            lastConfigMessage = millis();
        }
        
        // Set config mode but don't enable hotspot - allow normal WiFi connection
        if (!inConfigMode) {
            Serial.println("Entering config mode for server URL configuration");
            wifiManager->setConfigMode(true);  // Set config mode but don't enable AP
            digitalWrite(GREEN_PIN, HIGH);
        }
        
        setStayAwake(true);
        return true; // Stay awake for configuration
    }
    
    // Both WiFi and server URL are configured - exit config mode if connected
    if (inConfigMode && wifiConnected) {
        Serial.println("Configuration complete - exiting config mode");
        wifiManager->setConfigMode(false);
        wifiManager->disableAP();
        digitalWrite(GREEN_PIN, LOW); // Turn off config mode indicator
        return false; // Can proceed with normal operation
    }
    
    // If still in config mode (waiting for WiFi connection), stay awake
    if (inConfigMode) {
        static unsigned long lastConfigMessage = 0;
        if (millis() - lastConfigMessage > 30000) {
            Serial.println("Device in config mode - staying awake");
            lastConfigMessage = millis();
        }
        setStayAwake(true);
        return true; // Stay awake
    }
    
    return false; // Normal operation can proceed
}

void DeviceManager::printDebugInfo() {
#ifdef ESP8266_PLATFORM
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    Serial.printf("CPU Freq MHz: %u", ESP.getCpuFreqMHz());
    Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
    Serial.printf("Flash real size: %u\n\n", realSize);
    Serial.printf("Flash ide  size: %u\n", ideSize);
    Serial.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

    if (ideSize != realSize) {
        Serial.println("Flash Chip configuration wrong!\n");
    } else {
        Serial.println("Flash Chip configuration ok.\n");
    }
#elif defined(ESP32_PLATFORM)
    Serial.printf("CPU Freq MHz: %u\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %u\n", ESP.getFlashChipSize());
    Serial.printf("Flash speed: %u\n", ESP.getFlashChipSpeed());
    Serial.printf("Chip model: %s\n", ESP.getChipModel());
    Serial.printf("Chip revision: %u\n", ESP.getChipRevision());
    Serial.printf("SDK version: %s\n", ESP.getSdkVersion());
#endif
    Serial.println();
}

void DeviceManager::askServerIfShouldStayUp() {
    timeAtLastCheck = millis();
    Serial.println("Asking service if should stay up");
    String url = eepromManager->getServerUrl();

    url += "/should-remain-awake?id=";
    url += serialNumber;
    
    PlatformUtils::beginHTTPClient(httpClient, url);

    int httpCode = httpClient.GET();
    if (httpCode != 200) {
        Serial.println("Failed to get a response");
        stayAwake = false;
        return;
    }

    Serial.print("Got status code: ");
    Serial.println(httpCode);

    String payload = httpClient.getString();
    Serial.println("Got payload: " + payload);
    if (payload == "1") {
        stayAwake = true;
    }
    if (payload == "0") {
        stayAwake = false;
    }

    httpClient.end();
}

void DeviceManager::enterDeepSleep() {
    Serial.print("Been up for ");
    Serial.print(millis());
    Serial.println(" milliseconds");
    Serial.println("Sleeping...");
    PlatformUtils::deepSleep(1 * 60 * 1000000);
}

void DeviceManager::registerWithServer() {
    PlatformUtils::beginHTTPClient(httpClient, eepromManager->getServerUrl() + "/register");
    StaticJsonDocument<200> registrationDoc;
    registrationDoc["id"] = serialNumber;
    registrationDoc["alias"] = eepromManager->getAlias();
    registrationDoc["ipAddress"] = WiFi.localIP().toString();
    registrationDoc["macAddress"] = WiFi.macAddress();
    registrationDoc["mode"] = operatingMode;
    String registrationDocJson = "";
    serializeJson(registrationDoc, registrationDocJson);
    Serial.println("Sending: ");
    Serial.println(registrationDocJson);
    httpClient.addHeader("Content-Type", "application/json");
    int httpCode = httpClient.POST(registrationDocJson);
    if (httpCode > 0) {
        Serial.println("Response: ");
        String payload = httpClient.getString();
        Serial.println(payload);
    }
    httpClient.end();
}

void DeviceManager::sendFailureLogToServer() {
    String failureLog = eepromManager->getWiFiFailureLog();
    
    // Check if there are any failures to report
    if (failureLog.length() == 0 || failureLog == "[]") {
        Serial.println("No WiFi failures to report");
        return;
    }
    
    Serial.println("Sending WiFi failure log to server");
    Serial.println("Failure log: " + failureLog);
    
    PlatformUtils::beginHTTPClient(httpClient, eepromManager->getServerUrl() + "/wifi-failures");
    
    StaticJsonDocument<1024> failureDoc;
    failureDoc["id"] = serialNumber;
    failureDoc["alias"] = eepromManager->getAlias();
    failureDoc["failures"] = failureLog;
    
    String failureDocJson = "";
    serializeJson(failureDoc, failureDocJson);
    
    Serial.println("Sending failure log: ");
    Serial.println(failureDocJson);
    
    httpClient.addHeader("Content-Type", "application/json");
    int httpCode = httpClient.POST(failureDocJson);
    
    if (httpCode > 0) {
        Serial.print("Failure log sent, response code: ");
        Serial.println(httpCode);
        String payload = httpClient.getString();
        Serial.println("Response: " + payload);
        
        // Clear the failure log after successful transmission
        if (httpCode == 200) {
            Serial.println("Clearing failure log after successful transmission");
            eepromManager->clearWiFiFailureLog();
        }
    } else {
        Serial.print("Failed to send failure log, error: ");
        Serial.println(httpCode);
    }
    
    httpClient.end();
}

void DeviceManager::reportNow() {
    Serial.println("Reporting sensor data");
    
    if (operatingMode == MODE_THERMOMETER) {
        Serial.println("Reading temperature");
        float temperature = sensorManager->readTemperature();
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
    }

    Serial.println("Reading analog sensor");
    int soil = sensorManager->readSoilMoisture();
    Serial.print("Analog voltage: ");
    Serial.println(soil);

    timeAtLastSend = millis();
}

void DeviceManager::loop() {
    // Handle configuration mode - returns true if device should stay awake for config
    if (handleConfigurationMode()) {
        return; // Early return - don't do any server communication or sleep logic
    }
    
    // Configuration is complete and device is not in config mode - proceed with normal operation
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (stayAwake) {
        if (millis() - timeAtLastSend > 30 * 1000) {
            reportNow();
        }
        // Only ask server if WiFi is connected
        if (wifiConnected && millis() - timeAtLastCheck > 30 * 1000) {
            askServerIfShouldStayUp();
        }
    }

    // Only check with server if WiFi is connected
    if (!stayAwake && wifiConnected) {
        askServerIfShouldStayUp();
    }

    if (!stayAwake) {
        reportNow();
        enterDeepSleep();
        return;
    }

    unsigned long int timeRunning = millis();
    if (timeRunning > 86400 * 1000) {
        Serial.println("Restarting due to running too long");
        PlatformUtils::restart();
        return;
    }
}

void DeviceManager::setValveState(bool open) {
    if (operatingMode != MODE_LATCHING_VALVE) {
        Serial.println("Error: setValveState called but device not in latching valve mode");
        return;
    }
    
    Serial.print("Setting valve state to: ");
    Serial.println(open ? "OPEN" : "CLOSED");
    
    if (open) {
        // Pulse positive: AUX=HIGH, SENSE_POWER=LOW for H-bridge
        digitalWrite(AUX_PIN, HIGH);
        digitalWrite(SENSE_POWER_PIN, LOW);
        delay(100); // Pulse duration for valve actuation
        digitalWrite(AUX_PIN, LOW); // Return to neutral state
        Serial.println("Valve opened with positive pulse");
    } else {
        // Pulse negative: AUX=LOW, SENSE_POWER=HIGH for H-bridge
        digitalWrite(AUX_PIN, LOW);
        digitalWrite(SENSE_POWER_PIN, HIGH);
        delay(100); // Pulse duration for valve actuation
        digitalWrite(SENSE_POWER_PIN, LOW); // Return to neutral state
        Serial.println("Valve closed with negative pulse");
    }
}

void DeviceManager::openValve() {
    setValveState(true);
}

void DeviceManager::closeValve() {
    setValveState(false);
}