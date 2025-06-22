#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Arduino.h>

class SensorManager {
private:
    OneWire* oneWire;
    DallasTemperature* sensors;
    int oneWireBus;
    int sensePowerPin;

public:
    SensorManager(int oneWirePin, int powerPin);
    ~SensorManager();
    void init();
    float readTemperature();
    int readSoilMoisture();
    void powerSensorOn();
    void powerSensorOff();
};

#endif