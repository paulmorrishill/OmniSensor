#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "platform_config.h"

// Forward declaration
class EEPROMManager;

class WiFiManager {
private:
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    bool configMode;
    int deviceId;
    EEPROMManager* eepromManager;

public:
    WiFiManager();
    void init(int id, EEPROMManager* eeprom);
    void enableHotspotMode();
    void disableAP();
    bool connectUsingSavedCredentials(String ssid, String password);
    bool connectToWokwiGuest();
    void scanNetworks(JsonArray& networksArray);
    String getEncryptionName(byte type);
    bool isInConfigMode() const { return configMode; }
    void setConfigMode(bool mode) { configMode = mode; }
};

#endif