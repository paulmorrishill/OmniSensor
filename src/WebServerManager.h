#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "platform_config.h"

// Forward declarations
class EEPROMManager;
class WiFiManager;
class SensorManager;
class DeviceManager;

class WebServerManager {
private:
    WebServerType* server;
    HTTPUpdateServerType* httpUpdater;
    EEPROMManager* eepromManager;
    WiFiManager* wifiManager;
    SensorManager* sensorManager;
    DeviceManager* deviceManager;
    
#ifdef ESP32_PLATFORM
    // uSSDP-ESP32 specific objects
    uDevice* ssdpDevice;
    uSSDP* ssdpServer;
#endif
    
    // HTML content constants are now in html_constants.h

public:
    WebServerManager(EEPROMManager* eeprom, WiFiManager* wifi, SensorManager* sensor, DeviceManager* device);
    ~WebServerManager();
    
    void init();
    void handleClient();
    void setupSSDP(String serialNumber, int deviceId);
    
    // Route handlers
    void handleIndex();
    void handleIsUp();
    void handleOutputOn();
    void handleOutputOff();
    void handleControl();
    void handleWifi();
    void handleConfigure();
    void handleReport();
    void handleCurrentConfig();
    void handleGetConfig();
    void handleSetConfig();
    void handleSetMode();
    void handleSSDPSchema();
};

#endif