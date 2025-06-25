#include "WiFiManager.h"
#include "EEPROMManager.h"

WiFiManager::WiFiManager() :
    local_IP(192, 168, 10, 1),
    gateway(192, 168, 10, 1),
    subnet(255, 255, 255, 0),
    configMode(false),
    deviceId(0),
    eepromManager(nullptr) {
}

void WiFiManager::init(int id, EEPROMManager* eeprom) {
    deviceId = id;
    eepromManager = eeprom;
    String hostname = "WiFi_Omni_";
    hostname += id;
    
    PlatformUtils::setHostname(hostname);
}

void WiFiManager::enableHotspotMode() {
    Serial.println("Wifi setup begin");
    WiFi.disconnect();
    disableAP();
    Serial.println("Entering WIFI Config mode");

    WiFi.softAPConfig(local_IP, gateway, subnet);

    String wifiLight = "WiFiSense_";
    String ssid = wifiLight + deviceId;
    WiFi.softAP(ssid.c_str(), "password");

    WiFi.enableAP(true);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    configMode = true;
}

void WiFiManager::disableAP() {
    WiFi.softAPdisconnect(false);
    WiFi.enableAP(false);
}

bool WiFiManager::connectUsingSavedCredentials(String ssid, String password) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    PlatformUtils::setWiFiPower();
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Set timeout to 30 seconds (60 attempts * 500ms)
    int attempts = 0;
    const int maxAttempts = 60;
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connection failed - timeout reached");
        
        // Log the failure with current timestamp (millis since boot)
        if (eepromManager != nullptr) {
            unsigned long timestamp = millis();
            eepromManager->addWiFiFailure(timestamp);
        }
        
        return false;
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool WiFiManager::connectToWokwiGuest() {
    Serial.println("Attempting to connect to Wokwi-GUEST network");
    return connectUsingSavedCredentials("Wokwi-GUEST", "");
}

void WiFiManager::scanNetworks(JsonArray& networksArray) {
    int totalNetworks = WiFi.scanNetworks();
    StaticJsonDocument<1024> thisNetwork;
    
    for (int i = 0; i < totalNetworks; i++) {
        thisNetwork["ssid"] = WiFi.SSID(i);
        thisNetwork["rssi"] = WiFi.RSSI(i);
        thisNetwork["encryption"] = getEncryptionName(WiFi.encryptionType(i));
        networksArray.add(thisNetwork);
    }
}

String WiFiManager::getEncryptionName(byte type) {
    String typeString = "Unknown type: " + String(type);
    
#ifdef ESP8266_PLATFORM
    switch (type) {
        case ENC_TYPE_TKIP:
            typeString = "TKIP (WPA)";
            break;
        case ENC_TYPE_WEP:
            typeString = "WEP";
            break;
        case ENC_TYPE_CCMP:
            typeString = "CCMP (WPA)";
            break;
        case ENC_TYPE_NONE:
            typeString = "None";
            break;
        case ENC_TYPE_AUTO:
            typeString = "Auto";
            break;
    }
#elif defined(ESP32_PLATFORM)
    switch (type) {
        case WIFI_AUTH_OPEN:
            typeString = "None";
            break;
        case WIFI_AUTH_WEP:
            typeString = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            typeString = "WPA PSK";
            break;
        case WIFI_AUTH_WPA2_PSK:
            typeString = "WPA2 PSK";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            typeString = "WPA/WPA2 PSK";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            typeString = "WPA2 Enterprise";
            break;
        case WIFI_AUTH_WPA3_PSK:
            typeString = "WPA3 PSK";
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            typeString = "WPA2/WPA3 PSK";
            break;
    }
#endif
    
    return typeString;
}