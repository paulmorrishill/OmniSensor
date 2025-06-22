#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <EEPROM.h>
#include <Arduino.h>

class EEPROMManager {
private:
    static const int HAS_SET_ID_EEPROM_POSITION = 100;
    static const int ID_EEPROM_POSITION = 101;
    static const int HAS_SET_SSID_EEPROM_POSITION = 103;
    static const int EEPROM_MODE_POSITION = 200;
    static const int EEPROM_ALIAS_POSITION = EEPROM_MODE_POSITION + 10;
    static const int EEPROM_SERVER_POSITION = EEPROM_ALIAS_POSITION + 255;
    static const int EEPROM_SSID_POSITION = EEPROM_SERVER_POSITION + 255;
    static const int EEPROM_PASSWORD_POSITION = EEPROM_SSID_POSITION + 255;
    static const int EEPROM_FAILURE_LOG_POSITION = EEPROM_PASSWORD_POSITION + 255;
    static const int SSID_SET_VALUE = 233;

public:
    EEPROMManager();
    void init();
    
    // Device ID management
    bool hasDeviceId();
    int getDeviceId();
    void setDeviceId(int id);
    
    // WiFi credentials
    bool hasWiFiCredentials();
    String getSSID();
    String getPassword();
    void saveWiFiCredentials(String ssid, String password);
    
    // Configuration
    int getMode();
    void setMode(int mode);
    String getAlias();
    void setAlias(String alias);
    String getServerUrl();
    void setServerUrl(String server);
    bool hasServerUrl();
    
    // WiFi Failure Log
    void addWiFiFailure(unsigned long timestamp);
    String getWiFiFailureLog();
    void clearWiFiFailureLog();
    
    // Utility
    void clearAll();
    
private:
    void writeString(String input, int startPos);
    String readString(int position);
};

#endif