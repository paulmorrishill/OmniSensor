#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

// Platform detection and configuration
#ifdef ESP8266_PLATFORM
    // ESP8266 specific includes
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266HTTPUpdateServer.h>
    #include <ESP8266SSDP.h>
    #include <ESP8266HTTPClient.h>
    
    // Type aliases for compatibility
    typedef ESP8266WebServer WebServerType;
    typedef ESP8266HTTPUpdateServer HTTPUpdateServerType;
    
    // Platform-specific constants
    #define PLATFORM_NAME "ESP8266"
    
    // WiFi encryption types
    #define WIFI_ENC_OPEN ENC_TYPE_NONE
    #define WIFI_ENC_WEP ENC_TYPE_WEP
    #define WIFI_ENC_TKIP ENC_TYPE_TKIP
    #define WIFI_ENC_CCMP ENC_TYPE_CCMP
    #define WIFI_ENC_AUTO ENC_TYPE_AUTO
    
#elif defined(ESP32_PLATFORM)
    // ESP32 specific includes
    #include <WiFi.h>
    #include <WebServer.h>
    #include <HTTPUpdateServer.h>
    #include <HTTPClient.h>
    #include <esp_sleep.h>
    #include <ESP32Servo.h>
    #include <uSSDP.h>
    
    // Type aliases for compatibility
    typedef WebServer WebServerType;
    typedef HTTPUpdateServer HTTPUpdateServerType;
    
    // Platform-specific constants
    #define PLATFORM_NAME "ESP32"
    
    // WiFi encryption types (ESP32 uses different constants)
    #define WIFI_ENC_OPEN WIFI_AUTH_OPEN
    #define WIFI_ENC_WEP WIFI_AUTH_WEP
    #define WIFI_ENC_WPA_PSK WIFI_AUTH_WPA_PSK
    #define WIFI_ENC_WPA2_PSK WIFI_AUTH_WPA2_PSK
    #define WIFI_ENC_WPA_WPA2_PSK WIFI_AUTH_WPA_WPA2_PSK
    #define WIFI_ENC_WPA2_ENTERPRISE WIFI_AUTH_WPA2_ENTERPRISE
    #define WIFI_ENC_WPA3_PSK WIFI_AUTH_WPA3_PSK
    #define WIFI_ENC_WPA2_WPA3_PSK WIFI_AUTH_WPA2_WPA3_PSK
    
#else
    #error "Platform not supported. Define ESP8266_PLATFORM or ESP32_PLATFORM"
#endif

// Common includes
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Forward declarations
class EEPROMManager;

// Platform-specific function wrappers
namespace PlatformUtils {
    
    inline void setHostname(const String& hostname) {
        #ifdef ESP8266_PLATFORM
            WiFi.hostname(hostname);
        #elif defined(ESP32_PLATFORM)
            WiFi.setHostname(hostname.c_str());
        #endif
    }
    
    inline void setWiFiPower() {
        #ifdef ESP8266_PLATFORM
            WiFi.setOutputPower(20.5);
            WiFi.setPhyMode(WIFI_PHY_MODE_11N);
        #elif defined(ESP32_PLATFORM)
            WiFi.setTxPower(WIFI_POWER_19_5dBm);
            // ESP32 auto-negotiates PHY mode
        #endif
    }
    
    inline void deepSleep(uint64_t microseconds) {
        #ifdef ESP8266_PLATFORM
            ESP.deepSleep(microseconds);
        #elif defined(ESP32_PLATFORM)
            esp_sleep_enable_timer_wakeup(microseconds);
            esp_deep_sleep_start();
        #endif
    }
    
    inline void restart() {
        ESP.restart();
    }
    
    inline void beginHTTPClient(HTTPClient& client, const String& url) {
        #ifdef ESP8266_PLATFORM
            WiFiClient wifiClient;
            client.begin(wifiClient, url);
        #elif defined(ESP32_PLATFORM)
            client.begin(url);
        #endif
    }
    
    // Wokwi emulator detection
    inline bool isWokwiEmulator() {
        // For now, always return true to use Wokwi-GUEST when in ESP32 environment
        return true;
    }
}

#endif // PLATFORM_CONFIG_H