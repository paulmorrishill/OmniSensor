#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "platform_config.h"

// Forward declarations
class EEPROMManager;
class SensorManager;


class DeviceManager {
private:
    // Device modes
    static const int MODE_SERVO = 0;
    static const int MODE_INPUT_SWITCH = 1;
    static const int MODE_THERMOMETER = 2;
    static const int MODE_SOIL_SENSOR = 3;
    static const int MODE_RELAY = 4;
    static const int MODE_RGB_LED = 5;
    static const int MODE_LATCHING_VALVE = 6;
    
    // Pin definitions
    static const int BUTTON_PIN = 4;
    static const int RED_PIN = 12;
    static const int GREEN_PIN = 13;
    static const int SENSE_POWER_PIN = 14;
    static const int AUX_PIN = 5;
    
    EEPROMManager* eepromManager;
    SensorManager* sensorManager;
    HTTPClient httpClient;
    
    int deviceId;
    String serialNumber;
    int operatingMode;
    bool stayAwake;
    unsigned long timeAtLastSend;
    unsigned long timeAtLastCheck;

public:
    DeviceManager(EEPROMManager* eeprom, SensorManager* sensor);
    ~DeviceManager();
    
    void init();
    void initPins();
    void initDeviceId();
    void initSerialNumber();
    void handleButtonPress();
    void clearConfiguration();
    void printDebugInfo();
    
    // Power management
    bool shouldStayAwake() const { return stayAwake; }
    void setStayAwake(bool awake) { stayAwake = awake; }
    void askServerIfShouldStayUp();
    void enterDeepSleep();
    
    // Server communication
    void registerWithServer();
    void sendFailureLogToServer();
    void reportNow();
    
    // Getters
    int getDeviceId() const { return deviceId; }
    String getSerialNumber() const { return serialNumber; }
    int getOperatingMode() const { return operatingMode; }
    
    // Latching valve control
    void setValveState(bool open);
    void openValve();
    void closeValve();
    
    // Main loop
    void loop();
};

#endif